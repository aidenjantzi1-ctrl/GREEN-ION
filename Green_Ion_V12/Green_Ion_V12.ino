/*
  GREEN ION V12 development firmware
  Flight-data recorder and recovery aid only.
  It must never control ignition, staging, or recovery deployment.

  This first V12 integration slice is intentionally isolated from V11. It adds
  IMU/GPS/LoRa plumbing and CSV logging with failure isolation. V11 remains the
  functional baseline until its autonomous flight workflow, RKT4 persistence,
  dashboard, recovery, and OTA paths are ported and regression-tested.
*/

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_BMP3XX.h>
#include <Adafruit_LSM6DSO32.h>
#include <Adafruit_GPS.h>
#include <RH_RF95.h>
#include <WiFi.h>
#include <WebServer.h>
#include <GreenIonProtocol.h>
#include <GreenIonRkt4.h>
#include <GreenIonRecorderCore.h>

#include "Config.h"
#include "SensorState.h"
#include "Dashboard.h"

Adafruit_BMP3XX barometer;
Adafruit_LSM6DSO32 imu;
Adafruit_GPS gps(&Wire);
RH_RF95 radio(config::PIN_LORA_CS, config::PIN_LORA_DIO0);
File logFile;
SensorState state;
String logFilename;
WebServer dashboardServer(80);

uint32_t telemetrySequence = 0;
uint32_t lastSensorUs = 0;
uint32_t lastGpsServiceMs = 0;
uint32_t lastTelemetryMs = 0;
uint32_t lastFlushMs = 0;
uint16_t healthBits = 0;
green_ion::FlightState flightState = green_ion::STATE_IDLE;

static uint16_t clampU16(float value) {
  if (!isfinite(value) || value <= 0) return 0;
  if (value >= 65535.0f) return 65535;
  return static_cast<uint16_t>(lroundf(value));
}

static uint32_t clampU32(double value) {
  if (!isfinite(value) || value <= 0) return 0;
  if (value >= 4294967295.0) return UINT32_MAX;
  return static_cast<uint32_t>(llround(value));
}

static int16_t clampI16(float value) {
  if (!isfinite(value)) return 0;
  if (value >= 32767.0f) return INT16_MAX;
  if (value <= -32768.0f) return INT16_MIN;
  return static_cast<int16_t>(lroundf(value));
}

static int32_t clampI32(double value) {
  if (!isfinite(value)) return 0;
  if (value > 2147483647.0) return INT32_MAX;
  if (value < -2147483648.0) return INT32_MIN;
  return static_cast<int32_t>(llround(value));
}

static void selectNoSpiDevice() {
  digitalWrite(config::PIN_SD_CS, HIGH);
  digitalWrite(config::PIN_LORA_CS, HIGH);
}

static void initializeI2c() {
  pinMode(config::PIN_I2C_POWER, OUTPUT);
  digitalWrite(config::PIN_I2C_POWER, HIGH);
  delay(10);
  Wire.begin(config::PIN_I2C_SDA, config::PIN_I2C_SCL);
  Wire.setClock(400000);
}

static void initializeSensors() {
  state.barometerOk = barometer.begin_I2C(0x77) || barometer.begin_I2C(0x76);
  if (state.barometerOk) {
    barometer.setTemperatureOversampling(BMP3_OVERSAMPLING_8X);
    barometer.setPressureOversampling(BMP3_OVERSAMPLING_4X);
    barometer.setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_3);
  } else {
    healthBits |= green_ion::HEALTH_BARO_INIT_ERROR;
  }

  state.imuOk = imu.begin_I2C(0x6A, &Wire) || imu.begin_I2C(0x6B, &Wire);
  if (state.imuOk) {
    imu.setAccelRange(LSM6DSO32_ACCEL_RANGE_32_G);
    imu.setGyroRange(LSM6DS_GYRO_RANGE_2000_DPS);
    imu.setAccelDataRate(LSM6DS_RATE_104_HZ);
    imu.setGyroDataRate(LSM6DS_RATE_104_HZ);
  } else {
    healthBits |= green_ion::HEALTH_IMU_INIT_ERROR;
  }

  state.gpsOk = gps.begin(0x10);
  if (state.gpsOk) {
    gps.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
    gps.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);
  } else {
    healthBits |= green_ion::HEALTH_GPS_INIT_ERROR;
  }
}

static void initializeStorage() {
  digitalWrite(config::PIN_LORA_CS, HIGH);
  state.sdOk = SD.begin(config::PIN_SD_CS, SPI);
}

static void initializeRadio() {
  digitalWrite(config::PIN_SD_CS, HIGH);
  pinMode(config::PIN_LORA_RST, OUTPUT);
  digitalWrite(config::PIN_LORA_RST, LOW);
  delay(10);
  digitalWrite(config::PIN_LORA_RST, HIGH);
  delay(10);
  state.radioOk = radio.init() && radio.setFrequency(config::LORA_FREQUENCY_MHZ);
  if (state.radioOk) {
    radio.setTxPower(config::LORA_TX_POWER_DBM, false);
    radio.setSpreadingFactor(config::LORA_SPREADING_FACTOR);
    radio.setSignalBandwidth(config::LORA_SIGNAL_BANDWIDTH_HZ);
    radio.setCodingRate4(config::LORA_CODING_RATE_DENOMINATOR);
  } else {
    healthBits |= green_ion::HEALTH_RADIO_INIT_ERROR;
  }
}

static void readSensors() {
  if (state.barometerOk && barometer.performReading()) {
    state.pressurePa = barometer.pressure;
    state.temperatureC = barometer.temperature;
    state.altitudeM = barometer.readAltitude(1013.25f);
  }

  if (state.imuOk) {
    sensors_event_t accel;
    sensors_event_t gyro;
    sensors_event_t temperature;
    imu.getEvent(&accel, &gyro, &temperature);
    state.accelX = accel.acceleration.x;
    state.accelY = accel.acceleration.y;
    state.accelZ = accel.acceleration.z;
    state.accelMagnitude = sqrtf(state.accelX * state.accelX +
                                 state.accelY * state.accelY +
                                 state.accelZ * state.accelZ);
    state.gyroX = gyro.gyro.x;
    state.gyroY = gyro.gyro.y;
    state.gyroZ = gyro.gyro.z;
  }

  state.batteryMv = static_cast<uint16_t>(analogReadMilliVolts(config::PIN_BATTERY) * 2u);
}

static void serviceGps(uint32_t nowMs) {
  if (!state.gpsOk || nowMs - lastGpsServiceMs < config::GPS_SERVICE_INTERVAL_MS) return;
  lastGpsServiceMs = nowMs;
  gps.read();
  if (!gps.newNMEAreceived()) return;
  char *sentence = gps.lastNMEA();
  if (!sentence || !gps.parse(sentence)) return;
  state.satellites = gps.satellites;
  state.gpsHour = gps.hour;
  state.gpsMinute = gps.minute;
  state.gpsSecond = gps.seconds;
  if (gps.fix) {
    state.gpsHasPosition = true;
    state.lastGpsFixMs = nowMs;
    state.latitude = gps.latitudeDegrees;
    state.longitude = gps.longitudeDegrees;
    state.gpsAltitudeM = gps.altitude;
    state.gpsSpeedMps = gps.speed * 0.514444f;
  }
}

static uint32_t gpsAgeMs(uint32_t nowMs) {
  return state.gpsHasPosition ? nowMs - state.lastGpsFixMs : UINT32_MAX;
}

static bool openRawLog() {
  if (!state.sdOk || logFile) return static_cast<bool>(logFile);
  digitalWrite(config::PIN_LORA_CS, HIGH);
  for (uint16_t sequence = 0; sequence < 1000; ++sequence) {
    char candidate[14];
    snprintf(candidate, sizeof(candidate), "/V12D%03u.BIN", sequence);
    if (!SD.exists(candidate)) {
      logFilename = candidate;
      break;
    }
  }
  if (!logFilename.length()) return false;
  logFile = SD.open(logFilename, FILE_WRITE);
  if (!logFile) return false;

  green_ion::rkt4::Header header = {
    config::SENSOR_RATE_HZ,
    state.pressurePa,
    state.batteryMv,
  };
  uint8_t bytes[green_ion::rkt4::kHeaderBytes];
  if (!green_ion::rkt4::encodeHeader(bytes, header) ||
      logFile.write(bytes, sizeof(bytes)) != sizeof(bytes)) {
    logFile.close();
    SD.remove(logFilename);
    logFilename = "";
    healthBits |= green_ion::HEALTH_SD_WRITE_ERROR;
    state.sdOk = false;
    return false;
  }
  logFile.flush();
  state.recording = true;
  return true;
}

static void appendLog(uint32_t nowUs) {
  if (!state.sdOk || (!logFile && !openRawLog())) return;
  const bool valid = state.barometerOk && isfinite(state.pressurePa) &&
                     isfinite(state.temperatureC);
  green_ion::rkt4::Record record = {
    nowUs,
    state.pressurePa,
    state.temperatureC,
    state.batteryMv,
    valid,
  };
  uint8_t bytes[green_ion::rkt4::kRecordBytes];
  green_ion::rkt4::encodeRecord(bytes, record);
  digitalWrite(config::PIN_LORA_CS, HIGH);
  if (logFile.write(bytes, sizeof(bytes)) != sizeof(bytes)) {
    healthBits |= green_ion::HEALTH_SD_WRITE_ERROR;
    state.sdOk = false;
    state.recording = false;
  }
}

static uint16_t telemetryFlags(uint32_t ageMs) {
  uint16_t flags = 0;
  if (state.gpsHasPosition) flags |= green_ion::FLAG_GPS_HAS_POSITION;
  if (state.gpsHasPosition && ageMs <= config::GPS_CURRENT_MAX_AGE_MS)
    flags |= green_ion::FLAG_GPS_CURRENT;
  if (state.sdOk) flags |= green_ion::FLAG_SD_OK;
  if (state.imuOk) flags |= green_ion::FLAG_IMU_OK;
  if (state.barometerOk) flags |= green_ion::FLAG_BARO_OK;
  if (state.radioOk) flags |= green_ion::FLAG_RADIO_OK;
  if (state.recording) flags |= green_ion::FLAG_RECORDING;
  if (state.buzzerActive) flags |= green_ion::FLAG_BUZZER_ACTIVE;
  return flags;
}

static void sendDashboardStatus() {
  const uint32_t age = gpsAgeMs(millis());
  const char *gpsStatus = !state.gpsHasPosition ? "missing" :
                          (age <= config::GPS_CURRENT_MAX_AGE_MS ? "live" : "stale");
  const float radiansToDegrees = 180.0f / PI;
  const auto numberOrNull = [](float value, uint8_t decimals) {
    return isfinite(value) ? String(value, static_cast<unsigned int>(decimals)) : String("null");
  };
  String body;
  body.reserve(512);
  body += "{\"recording\":";
  body += state.recording ? "true" : "false";
  body += ",\"log_file\":\"" + logFilename + "\",\"pressure_pa\":";
  body += numberOrNull(state.pressurePa, 0);
  body += ",\"altitude_m\":" + numberOrNull(state.altitudeM, 2);
  body += ",\"acceleration_g\":" + numberOrNull(state.accelMagnitude / 9.80665f, 3);
  body += ",\"rotation_x_dps\":" + numberOrNull(state.gyroX * radiansToDegrees, 1);
  body += ",\"rotation_y_dps\":" + numberOrNull(state.gyroY * radiansToDegrees, 1);
  body += ",\"rotation_z_dps\":" + numberOrNull(state.gyroZ * radiansToDegrees, 1);
  body += ",\"battery_v\":" + String(state.batteryMv / 1000.0f, 3);
  body += ",\"gps_status\":\"" + String(gpsStatus) + "\",\"satellites\":";
  body += String(state.satellites);
  body += ",\"health\":" + String(healthBits) + "}";
  dashboardServer.send(200, "application/json", body);
}

static void initializeDashboard() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(config::DASHBOARD_SSID, config::DASHBOARD_PASSWORD);
  dashboardServer.on("/", []() {
    dashboardServer.send_P(200, "text/html", DASHBOARD_HTML);
  });
  dashboardServer.on("/api/status", sendDashboardStatus);
  dashboardServer.begin();
}

static void transmitTelemetry(uint32_t nowMs) {
  if (!state.radioOk || nowMs - lastTelemetryMs < config::TELEMETRY_INTERVAL_MS) return;
  lastTelemetryMs = nowMs;
  const uint32_t age = gpsAgeMs(nowMs);
  green_ion::TelemetryPacket packet;
  green_ion::initialize(packet);
  packet.sequence = telemetrySequence++;
  packet.uptimeMs = nowMs;
  packet.latitudeE7 = state.gpsHasPosition ? clampI32(state.latitude * 1e7) : 0;
  packet.longitudeE7 = state.gpsHasPosition ? clampI32(state.longitude * 1e7) : 0;
  packet.altitudeCm = clampI32(state.altitudeM * 100.0);
  packet.pressurePa = clampU32(state.pressurePa);
  packet.gpsAltitudeCm = state.gpsHasPosition ? clampI32(state.gpsAltitudeM * 100.0) : 0;
  packet.speedCms = clampU16(state.gpsSpeedMps * 100.0f);
  packet.accelerationMg = clampU16(state.accelMagnitude * 1000.0f / 9.80665f);
  packet.accelerationXMg = clampI16(state.accelX * 1000.0f / 9.80665f);
  packet.accelerationYMg = clampI16(state.accelY * 1000.0f / 9.80665f);
  packet.accelerationZMg = clampI16(state.accelZ * 1000.0f / 9.80665f);
  const float radiansToDegrees10 = 10.0f * 180.0f / PI;
  packet.rotationXDps10 = clampI16(state.gyroX * radiansToDegrees10);
  packet.rotationYDps10 = clampI16(state.gyroY * radiansToDegrees10);
  packet.rotationZDps10 = clampI16(state.gyroZ * radiansToDegrees10);
  packet.batteryMv = state.batteryMv;
  packet.gpsFixAgeMs = age;
  packet.satellites = state.satellites;
  packet.flightState = flightState;
  packet.flags = telemetryFlags(age);
  packet.health = healthBits;
  green_ion::seal(packet);

  digitalWrite(config::PIN_SD_CS, HIGH);
  if (!radio.send(reinterpret_cast<uint8_t *>(&packet), sizeof(packet))) {
    state.radioOk = false;
  }
  // Do not block waiting for packet completion; logging owns the critical path.
}

void setup() {
  Serial.begin(config::SERIAL_BAUD);
  pinMode(config::PIN_SD_CS, OUTPUT);
  pinMode(config::PIN_LORA_CS, OUTPUT);
  pinMode(config::PIN_BUZZER, OUTPUT);
  digitalWrite(config::PIN_BUZZER, LOW);
  selectNoSpiDevice();
  initializeI2c();
  SPI.begin(config::PIN_SPI_SCK, config::PIN_SPI_MISO, config::PIN_SPI_MOSI);
  initializeSensors();
  initializeStorage();
  initializeRadio();
  initializeDashboard();
  Serial.printf("GREEN ION V12 dev: baro=%d imu=%d gps=%d sd=%d radio=%d\n",
                state.barometerOk, state.imuOk, state.gpsOk, state.sdOk, state.radioOk);
}

void loop() {
  const uint32_t nowMs = millis();
  const uint32_t nowUs = micros();
  serviceGps(nowMs);

  if (static_cast<uint32_t>(nowUs - lastSensorUs) >= config::SENSOR_INTERVAL_US) {
    lastSensorUs = nowUs;
    readSensors();
    appendLog(nowUs);
  }

  if (state.sdOk && logFile && nowMs - lastFlushMs >= config::LOG_FLUSH_INTERVAL_MS) {
    lastFlushMs = nowMs;
    digitalWrite(config::PIN_LORA_CS, HIGH);
    logFile.flush();
  }

  transmitTelemetry(nowMs);
  dashboardServer.handleClient();
  yield();
}

