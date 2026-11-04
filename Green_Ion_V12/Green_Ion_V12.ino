/*
  GREEN ION V12 development firmware
  Flight-data recorder and recovery aid only.
  It must never control ignition, staging, or recovery deployment.

  V12 remains intentionally isolated from V11. This development build combines
  RKT4 recording, a bounded prebuffer, stationary pressure calibration, and a
  recorder-only launch watcher. Recovery and OTA paths are still incomplete.
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
#include <GreenIonTelemetryQueue.h>
#include <GreenIonRadioRecovery.h>
#include <GreenIonRkt4.h>
#include <GreenIonRecorderCore.h>
#include <GreenIonFlightWatcher.h>
#include <GreenIonCalibration.h>
#include <GreenIonCheckpoint.h>
#include <GreenIonSampleClock.h>
#include <GreenIonMetadata.h>
#include <GreenIonLandingWatcher.h>
#include <GreenIonSensorLog.h>
#include <GreenIonSensorLogCore.h>

#include "Config.h"
#include "SensorState.h"
#include "Dashboard.h"

Adafruit_BMP3XX barometer;
Adafruit_LSM6DSO32 imu;
Adafruit_GPS gps(&Wire);
RH_RF95 radio(config::PIN_LORA_CS, config::PIN_LORA_DIO0);
File logFile;
File sensorLogFile;
SensorState state;
String logFilename;
String sensorLogFilename;
String checkpointFilenameA;
String checkpointFilenameB;
String metadataFilename;
WebServer dashboardServer(80);

class SdFileSink : public green_ion::recorder::ByteSink {
 public:
  SdFileSink() : file_(nullptr) {}

  void attach(File &file) { file_ = &file; }

  bool write(const uint8_t *data, size_t length) override {
    if (!file_ || !*file_) return false;
    digitalWrite(config::PIN_LORA_CS, HIGH);
    return file_->write(data, length) == length;
  }

  bool flush() override {
    if (!file_ || !*file_) return false;
    digitalWrite(config::PIN_LORA_CS, HIGH);
    file_->flush();
    return static_cast<bool>(*file_);
  }

 private:
  File *file_;
};

SdFileSink logSink;
SdFileSink sensorLogSink;
green_ion::recorder::RecorderCore<config::PREBUFFER_RECORDS> recorderCore;
green_ion::sensor_log::SensorLogCore<config::PREBUFFER_RECORDS,
                                     config::SENSOR_LOG_QUEUE_RECORDS>
    sensorLogCore;
green_ion::recorder::SampleClock sampleClock(config::SENSOR_INTERVAL_US);
green_ion::flight::FlightWatcher flightWatcher;
green_ion::flight::LandingWatcher landingWatcher;
green_ion::TelemetryQueue<config::TELEMETRY_QUEUE_CAPACITY> telemetryQueue;
green_ion::RadioRecovery radioRecovery(config::RADIO_REINITIALIZE_DELAY_MS,
                                       config::RADIO_SEND_FAILURE_THRESHOLD);

enum RecorderMode : uint8_t {
  RECORDER_CALIBRATING = 0,
  RECORDER_WATCHING,
  RECORDER_RECORDING,
  RECORDER_COMPLETE,
  RECORDER_FAULT,
};

RecorderMode recorderMode = RECORDER_CALIBRATING;
float calibrationSamples[config::CALIBRATION_TARGET_SAMPLES];
uint16_t calibrationWarmup = 0;
uint16_t calibrationAttempts = 0;
uint16_t calibrationValid = 0;
uint32_t lastCalibrationMs = 0;
uint32_t calibrationRetryAtMs = 0;
float calibrationBaselinePa = NAN;
float calibrationNoiseM = NAN;
float watcherAltitudeM = NAN;
float watcherPreviousAltitudeM = NAN;
float watcherSpeedMps = NAN;
uint32_t watcherPreviousMs = 0;

uint32_t telemetrySequence = 0;
uint32_t radioSendFailures = 0;
uint32_t lastRadioAttemptMs = 0;
uint16_t radioBenchPacketsSent = 0;
uint8_t radioBenchPacketsRemaining = 0;
bool radioManualRetryRequested = false;
uint32_t lastGpsServiceMs = 0;
uint32_t lastBenchOutputMs = 0;
uint32_t lastTelemetryMs = 0;
uint32_t lastFlushMs = 0;
uint32_t lastSensorLogFlushMs = 0;
uint32_t lastCheckpointMs = 0;
uint32_t checkpointGeneration = 0;
uint32_t checkpointFailures = 0;
uint32_t metadataFailures = 0;
uint32_t finalizationFailures = 0;
uint32_t sensorLogSequence = 0;
uint32_t sensorLogFailures = 0;
uint32_t recordingStartedMs = 0;
bool checkpointSlotB = false;
uint32_t recoveredLogs = 0;
uint32_t recoveredRecords = 0;
uint32_t recoveredTrailingBytes = 0;
uint32_t recoveryPending = 0;
String recoveryStatus = "Not scanned";
uint16_t healthBits = 0;
green_ion::FlightState flightState = green_ion::STATE_IDLE;

enum BenchMode : uint8_t {
  BENCH_OFF = 0,
  BENCH_IMU,
  BENCH_GPS,
  BENCH_ALL,
};

BenchMode benchMode = BENCH_ALL;
char serialCommand[32] = {};
uint8_t serialCommandLength = 0;

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

static bool isV12JournalName(const String &name, const char *suffix) {
  if (name.length() != 11 || !name.startsWith("V12D") || !name.endsWith(suffix))
    return false;
  for (uint8_t i = 4; i < 7; ++i)
    if (name[i] < '0' || name[i] > '9') return false;
  return true;
}

static size_t readCheckpointFile(const String &path,
                                 uint8_t out[green_ion::recovery::kCheckpointBytes]) {
  File file = SD.open(path, FILE_READ);
  if (!file || file.size() != green_ion::recovery::kCheckpointBytes) {
    if (file) file.close();
    return 0;
  }
  const size_t bytes = file.read(out, green_ion::recovery::kCheckpointBytes);
  file.close();
  return bytes;
}

class SdRecoveryCommitter : public green_ion::recovery::RecoveryCommitter {
 public:
  SdRecoveryCommitter(const String &reportPath, const String &report,
                      const String &slotAPath, const String &slotBPath)
      : reportPath_(reportPath), report_(report),
        slotAPath_(slotAPath), slotBPath_(slotBPath) {}

  bool saveReport() override {
    if (SD.exists(reportPath_) && !SD.remove(reportPath_)) return false;
    File recovery = SD.open(reportPath_, FILE_WRITE);
    const bool saved = recovery && recovery.print(report_) == report_.length();
    if (recovery) {
      recovery.flush();
      recovery.close();
    }
    return saved;
  }

  bool removeSlotA() override {
    return !SD.exists(slotAPath_) || SD.remove(slotAPath_);
  }

  bool removeSlotB() override {
    return !SD.exists(slotBPath_) || SD.remove(slotBPath_);
  }

 private:
  const String &reportPath_;
  const String &report_;
  const String &slotAPath_;
  const String &slotBPath_;
};

static bool recoverInterruptedLog(const String &journalLeaf) {
  String binLeaf = journalLeaf;
  binLeaf.replace(journalLeaf.endsWith(".JRB") ? ".JRB" : ".JRA", ".BIN");
  const String binPath = "/" + binLeaf;
  String slotAPath = "/" + binLeaf;
  slotAPath.replace(".BIN", ".JRA");
  String slotBPath = "/" + binLeaf;
  slotBPath.replace(".BIN", ".JRB");

  File log = SD.open(binPath, FILE_READ);
  if (!log || static_cast<uint64_t>(log.size()) > UINT32_MAX) {
    if (log) log.close();
    ++recoveryPending;
    return false;
  }
  const uint32_t fileBytes = static_cast<uint32_t>(log.size());
  uint8_t header[green_ion::rkt4::kHeaderBytes];
  const size_t headerBytes = log.read(header, sizeof(header));
  log.close();
  const auto inspection = green_ion::recovery::inspectRkt4(
      header, headerBytes, fileBytes);

  uint8_t slotA[green_ion::recovery::kCheckpointBytes] = {};
  uint8_t slotB[green_ion::recovery::kCheckpointBytes] = {};
  const size_t slotABytes = readCheckpointFile(slotAPath, slotA);
  const size_t slotBBytes = readCheckpointFile(slotBPath, slotB);
  green_ion::recovery::Checkpoint checkpoint = {};
  const bool checkpointValid = green_ion::recovery::selectNewestCheckpoint(
      slotA, slotABytes, slotB, slotBBytes, fileBytes, checkpoint);
  if (inspection.headerResult != green_ion::rkt4::RKT4_VALID || !checkpointValid) {
    ++recoveryPending;
    return false;
  }

  String reportPath = "/" + binLeaf;
  reportPath.replace(".BIN", ".RCV");
  String report;
  report.reserve(256);
  report += "GREEN-ION-RECOVERY-1\nfile=" + binLeaf;
  report += "\nstatus=interrupted";
  report += "\ncheckpoint_generation=" + String(checkpoint.generation);
  report += "\ncheckpoint_committed_bytes=" + String(checkpoint.committedBytes);
  report += "\nactual_bytes=" + String(fileBytes);
  report += "\ncomplete_records=" + String(inspection.completeRecords);
  report += "\ntrailing_partial_bytes=" + String(inspection.trailingBytes);
  report += "\nrkt4_header_valid=yes\n";
  const bool haveSlotA = SD.exists(slotAPath);
  const bool haveSlotB = SD.exists(slotBPath);
  SdRecoveryCommitter committer(reportPath, report, slotAPath, slotBPath);
  const auto commitResult = green_ion::recovery::commitRecovery(
      committer, haveSlotA, haveSlotB);
  if (commitResult == green_ion::recovery::RECOVERY_NO_JOURNAL ||
      commitResult == green_ion::recovery::RECOVERY_REPORT_WRITE_FAILED) {
    ++recoveryPending;
    return false;
  }
  ++recoveredLogs;
  recoveredRecords += inspection.completeRecords;
  recoveredTrailingBytes += inspection.trailingBytes;
  if (commitResult != green_ion::recovery::RECOVERY_COMMITTED) ++recoveryPending;
  return commitResult == green_ion::recovery::RECOVERY_COMMITTED;
}

static int journalSequence(const String &name) {
  if (!isV12JournalName(name, ".JRA") && !isV12JournalName(name, ".JRB"))
    return -1;
  return (name[4] - '0') * 100 + (name[5] - '0') * 10 + (name[6] - '0');
}

static void recoverInterruptedLogs() {
  if (!state.sdOk) {
    recoveryStatus = "SD unavailable; recovery not scanned";
    return;
  }
  // Two bits per possible V12D000..V12D999 log. Discovery completes before
  // any journal is removed, avoiding mutation while the FAT directory is open.
  uint8_t journalSlots[250] = {};
  File root = SD.open("/");
  if (!root) {
    ++recoveryPending;
    recoveryStatus = "Recovery directory unavailable";
    return;
  }
  File entry = root.openNextFile();
  while (entry) {
    String leaf = entry.name();
    const bool directory = entry.isDirectory();
    entry.close();
    const int slash = leaf.lastIndexOf('/');
    if (slash >= 0) leaf = leaf.substring(slash + 1);
    const int sequence = directory ? -1 : journalSequence(leaf);
    if (sequence >= 0) {
      const uint16_t bit = static_cast<uint16_t>(sequence) * 2u +
                           (leaf.endsWith(".JRB") ? 1u : 0u);
      journalSlots[bit / 8] |= static_cast<uint8_t>(1u << (bit % 8));
    }
    entry = root.openNextFile();
  }
  root.close();

  for (uint16_t sequence = 0; sequence < 1000; ++sequence) {
    const uint16_t bitA = sequence * 2u;
    const bool haveA = journalSlots[bitA / 8] & (1u << (bitA % 8));
    const uint16_t bitB = bitA + 1u;
    const bool haveB = journalSlots[bitB / 8] & (1u << (bitB % 8));
    if (!haveA && !haveB) continue;
    char leaf[12];
    snprintf(leaf, sizeof(leaf), "V12D%03u.%s", sequence, haveA ? "JRA" : "JRB");
    recoverInterruptedLog(String(leaf));
  }
  if (recoveryPending)
    recoveryStatus = String(recoveryPending) + " recovery item(s) pending inspection";
  else if (recoveredLogs)
    recoveryStatus = String(recoveredLogs) + " interrupted log(s) reported";
  else
    recoveryStatus = "No interrupted logs detected";
}

static bool configureRadioHardware() {
  digitalWrite(config::PIN_SD_CS, HIGH);
  pinMode(config::PIN_LORA_RST, OUTPUT);
  digitalWrite(config::PIN_LORA_RST, LOW);
  delay(10);
  digitalWrite(config::PIN_LORA_RST, HIGH);
  delay(10);
  state.radioOk = radio.init() && radio.setFrequency(config::LORA_FREQUENCY_MHZ);
  if (state.radioOk) {
    radio.setCADTimeout(0);
    radio.setTxPower(config::LORA_TX_POWER_DBM, false);
    radio.setSpreadingFactor(config::LORA_SPREADING_FACTOR);
    radio.setSignalBandwidth(config::LORA_SIGNAL_BANDWIDTH_HZ);
    radio.setCodingRate4(config::LORA_CODING_RATE_DENOMINATOR);
  } else {
    healthBits |= green_ion::HEALTH_RADIO_INIT_ERROR;
  }
  return state.radioOk;
}

static void initializeRadio() {
  radioRecovery.begin(millis(), configureRadioHardware());
}

static bool readSensors() {
  bool barometerReadingValid = false;
  if (state.barometerOk && barometer.performReading()) {
    state.pressurePa = barometer.pressure;
    state.temperatureC = barometer.temperature;
    state.altitudeM = barometer.readAltitude(1013.25f);
    barometerReadingValid = isfinite(state.pressurePa) && isfinite(state.temperatureC) &&
                            state.pressurePa > 30000.0f && state.pressurePa < 120000.0f;
  }

  if (state.imuOk) {
    sensors_event_t accel;
    sensors_event_t gyro;
    sensors_event_t temperature;
    state.imuDataValid = imu.getEvent(&accel, &gyro, &temperature) &&
                         isfinite(accel.acceleration.x) &&
                         isfinite(accel.acceleration.y) &&
                         isfinite(accel.acceleration.z) &&
                         isfinite(gyro.gyro.x) && isfinite(gyro.gyro.y) &&
                         isfinite(gyro.gyro.z);
    if (state.imuDataValid) {
      state.accelX = accel.acceleration.x;
      state.accelY = accel.acceleration.y;
      state.accelZ = accel.acceleration.z;
      state.accelMagnitude = sqrtf(state.accelX * state.accelX +
                                   state.accelY * state.accelY +
                                   state.accelZ * state.accelZ);
      state.gyroX = gyro.gyro.x;
      state.gyroY = gyro.gyro.y;
      state.gyroZ = gyro.gyro.z;
    } else {
      ++state.imuReadFailures;
      healthBits |= green_ion::HEALTH_IMU_READ_ERROR;
      state.accelX = state.accelY = state.accelZ = state.accelMagnitude = NAN;
      state.gyroX = state.gyroY = state.gyroZ = NAN;
    }
  }

  state.batteryMv = static_cast<uint16_t>(analogReadMilliVolts(config::PIN_BATTERY) * 2u);
  return barometerReadingValid;
}

static void serviceGps(uint32_t nowMs) {
  if (!state.gpsOk || nowMs - lastGpsServiceMs < config::GPS_SERVICE_INTERVAL_MS) return;
  lastGpsServiceMs = nowMs;
  gps.read();
  if (!gps.newNMEAreceived()) return;
  char *sentence = gps.lastNMEA();
  if (!sentence || !gps.parse(sentence)) {
    ++state.gpsParseFailures;
    return;
  }
  state.lastGpsSentenceMs = nowMs;
  state.satellites = gps.satellites;
  state.gpsHour = gps.hour;
  state.gpsMinute = gps.minute;
  state.gpsSecond = gps.seconds;
  state.gpsFixCurrent = gps.fix;
  if (state.gpsFixCurrent) {
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

static bool gpsPositionCurrent(uint32_t nowMs) {
  return state.gpsHasPosition && state.gpsFixCurrent &&
         gpsAgeMs(nowMs) <= config::GPS_CURRENT_MAX_AGE_MS;
}

static void printImuBenchStatus() {
  if (!state.imuOk) {
    Serial.println("BENCH IMU status=INIT_FAILED expected_i2c=0x6A_or_0x6B");
    return;
  }
  if (!state.imuDataValid) {
    Serial.printf("BENCH IMU status=READ_FAILED failures=%lu\n",
                  static_cast<unsigned long>(state.imuReadFailures));
    return;
  }
  const float toG = 1.0f / 9.80665f;
  const float toDegrees = 180.0f / PI;
  Serial.printf(
      "BENCH IMU status=OK range=+-32g rate_hz=104 "
      "accel_mps2=%.3f,%.3f,%.3f accel_g=%.3f,%.3f,%.3f "
      "gyro_rad_s=%.3f,%.3f,%.3f gyro_dps=%.2f,%.2f,%.2f\n",
      state.accelX, state.accelY, state.accelZ,
      state.accelX * toG, state.accelY * toG, state.accelZ * toG,
      state.gyroX, state.gyroY, state.gyroZ,
      state.gyroX * toDegrees, state.gyroY * toDegrees,
      state.gyroZ * toDegrees);
}

static void printGpsBenchStatus(uint32_t nowMs) {
  if (!state.gpsOk) {
    Serial.println("BENCH GPS status=INIT_FAILED expected_i2c=0x10");
    return;
  }
  if (!state.gpsHasPosition) {
    Serial.printf("BENCH GPS status=NO_FIX satellites=%u parse_failures=%lu\n",
                  state.satellites,
                  static_cast<unsigned long>(state.gpsParseFailures));
    return;
  }
  const uint32_t age = gpsAgeMs(nowMs);
  Serial.printf(
      "BENCH GPS status=%s fix=%d satellites=%u age_ms=%lu "
      "last_lat=%.7f last_lon=%.7f last_alt_m=%.2f utc=%02u:%02u:%02u\n",
      gpsPositionCurrent(nowMs) ? "LIVE" : "STALE",
      state.gpsFixCurrent ? 1 : 0, state.satellites,
      static_cast<unsigned long>(age), state.latitude, state.longitude,
      state.gpsAltitudeM, state.gpsHour, state.gpsMinute, state.gpsSecond);
}

static void printBenchStatus(uint32_t nowMs) {
  if (benchMode == BENCH_IMU || benchMode == BENCH_ALL) printImuBenchStatus();
  if (benchMode == BENCH_GPS || benchMode == BENCH_ALL) printGpsBenchStatus(nowMs);
}

static void applySerialCommand(uint32_t nowMs) {
  if (!strcmp(serialCommand, "bench imu")) benchMode = BENCH_IMU;
  else if (!strcmp(serialCommand, "bench gps")) benchMode = BENCH_GPS;
  else if (!strcmp(serialCommand, "bench all")) benchMode = BENCH_ALL;
  else if (!strcmp(serialCommand, "bench off")) benchMode = BENCH_OFF;
  else if (!strcmp(serialCommand, "bench status")) printBenchStatus(nowMs);
  else if (!strcmp(serialCommand, "radio test")) {
    if (!state.radioOk) {
      Serial.println("BENCH RADIO status=INIT_FAILED; test not started");
      return;
    }
    radioBenchPacketsRemaining = 5;
    lastTelemetryMs = nowMs - config::TELEMETRY_INTERVAL_MS;
    Serial.println("BENCH RADIO status=START packets=5 antenna_required=1");
    return;
  }
  else if (!strcmp(serialCommand, "radio retry")) {
    if (recorderMode == RECORDER_RECORDING) {
      Serial.println("BENCH RADIO status=RETRY_DEFERRED reason=recording_active");
      return;
    }
    radioManualRetryRequested = true;
    Serial.println("BENCH RADIO status=RETRY_REQUESTED calibration_will_restart=1");
    return;
  }
  else if (!strcmp(serialCommand, "radio status")) {
    Serial.printf(
        "BENCH RADIO status=%s sent=%u remaining=%u queued=%u dropped=%lu send_failures=%lu consecutive_failures=%u init_failures=%lu recoveries=%lu "
        "frequency_mhz=%.1f bandwidth_hz=%ld sf=%u coding_rate=4/%u power_dbm=%d\n",
        state.radioOk ? "READY" : "FAILED", radioBenchPacketsSent,
        radioBenchPacketsRemaining, static_cast<unsigned int>(telemetryQueue.size()),
        static_cast<unsigned long>(telemetryQueue.dropped()),
        static_cast<unsigned long>(radioSendFailures),
        radioRecovery.consecutiveSendFailures(),
        static_cast<unsigned long>(radioRecovery.initFailures()),
        static_cast<unsigned long>(radioRecovery.recoveries()),
        config::LORA_FREQUENCY_MHZ, config::LORA_SIGNAL_BANDWIDTH_HZ,
        config::LORA_SPREADING_FACTOR, config::LORA_CODING_RATE_DENOMINATOR,
        config::LORA_TX_POWER_DBM);
    return;
  }
  else {
    Serial.println("Commands: bench imu | bench gps | bench all | bench status | bench off | radio test | radio retry | radio status");
    return;
  }
  Serial.printf("BENCH mode=%u\n", static_cast<unsigned int>(benchMode));
}

static void serviceSerialBench(uint32_t nowMs) {
  while (Serial.available()) {
    const char value = static_cast<char>(Serial.read());
    if (value == '\r') continue;
    if (value == '\n') {
      serialCommand[serialCommandLength] = '\0';
      if (serialCommandLength) applySerialCommand(nowMs);
      serialCommandLength = 0;
    } else if (serialCommandLength + 1 < sizeof(serialCommand)) {
      serialCommand[serialCommandLength++] = value;
    } else {
      serialCommandLength = 0;
      Serial.println("BENCH command too long; input cleared");
    }
  }
  if (benchMode != BENCH_OFF &&
      nowMs - lastBenchOutputMs >= config::BENCH_OUTPUT_INTERVAL_MS) {
    lastBenchOutputMs = nowMs;
    printBenchStatus(nowMs);
  }
}

static void resetCalibration(uint32_t nowMs) {
  flightWatcher.cancel();
  recorderCore.resetBuffer();
  calibrationWarmup = 0;
  calibrationAttempts = 0;
  calibrationValid = 0;
  lastCalibrationMs = nowMs - config::CALIBRATION_INTERVAL_MS;
  calibrationRetryAtMs = 0;
  calibrationBaselinePa = NAN;
  calibrationNoiseM = NAN;
  recorderMode = RECORDER_CALIBRATING;
}

static void serviceRadioRecovery(uint32_t nowMs) {
  const bool recorderCritical = recorderMode != RECORDER_COMPLETE &&
                                recorderMode != RECORDER_FAULT;
  const bool manual = radioManualRetryRequested;
  if (!manual && !radioRecovery.shouldAttempt(nowMs, recorderCritical)) return;
  if (manual && recorderMode == RECORDER_RECORDING) return;
  radioManualRetryRequested = false;
  const bool restartCalibration = manual &&
      (recorderMode == RECORDER_CALIBRATING || recorderMode == RECORDER_WATCHING);
  const bool initialized = configureRadioHardware();
  radioRecovery.noteInitialize(nowMs, initialized);
  Serial.printf("BENCH RADIO status=%s recoveries=%lu init_failures=%lu\n",
                initialized ? "RECOVERED" : "RETRY_FAILED",
                static_cast<unsigned long>(radioRecovery.recoveries()),
                static_cast<unsigned long>(radioRecovery.initFailures()));
  if (restartCalibration) resetCalibration(millis());
}

static void processCalibration(uint32_t nowMs, bool valid) {
  if (recorderMode != RECORDER_CALIBRATING ||
      nowMs - lastCalibrationMs < config::CALIBRATION_INTERVAL_MS) return;
  lastCalibrationMs = nowMs;
  if (calibrationWarmup < config::CALIBRATION_WARMUP_SAMPLES) {
    ++calibrationWarmup;
    return;
  }
  ++calibrationAttempts;
  if (valid && calibrationValid < config::CALIBRATION_TARGET_SAMPLES)
    calibrationSamples[calibrationValid++] = state.pressurePa;
  if (calibrationValid < config::CALIBRATION_TARGET_SAMPLES &&
      calibrationAttempts < config::CALIBRATION_MAX_ATTEMPTS) return;

  const float collectionSeconds = fmaxf(1.0f,
      calibrationAttempts * config::CALIBRATION_INTERVAL_MS / 1000.0f);
  const auto result = green_ion::calibration::analyze<config::CALIBRATION_TARGET_SAMPLES>(
      calibrationSamples, calibrationValid, collectionSeconds);
  if (result.code != green_ion::calibration::CALIBRATION_VALID) {
    recorderMode = RECORDER_FAULT;
    calibrationRetryAtMs = nowMs + config::CALIBRATION_RETRY_MS;
    return;
  }

  calibrationBaselinePa = result.baselinePressurePa;
  calibrationNoiseM = result.noiseAltitudeM;
  recorderCore.resetBuffer();
  watcherAltitudeM = watcherPreviousAltitudeM = NAN;
  watcherSpeedMps = NAN;
  watcherPreviousMs = 0;
  const bool watching = flightWatcher.begin({
      config::SENSOR_RATE_HZ, 0.0f, calibrationBaselinePa, calibrationNoiseM});
  recorderMode = watching ? RECORDER_WATCHING : RECORDER_FAULT;
  if (!watching) calibrationRetryAtMs = nowMs + config::CALIBRATION_RETRY_MS;
}

static void updateFlightEstimate(uint32_t nowMs, bool valid) {
  if (recorderMode != RECORDER_WATCHING && recorderMode != RECORDER_RECORDING)
    return;
  const float relativeM = valid ? green_ion::calibration::relativeAltitudeM(
      state.pressurePa, calibrationBaselinePa) : NAN;
  if (isfinite(relativeM)) {
    if (!isfinite(watcherAltitudeM)) watcherAltitudeM = relativeM;
    const float difference = fabsf(relativeM - watcherAltitudeM);
    const float alpha = difference < 0.25f ? 0.28f :
                        difference < 1.5f ? 0.48f :
                        difference < 5.0f ? 0.68f : 0.84f;
    watcherAltitudeM += alpha * (relativeM - watcherAltitudeM);
    if (isfinite(watcherPreviousAltitudeM) && watcherPreviousMs != nowMs) {
      const uint32_t elapsedMs = nowMs - watcherPreviousMs;
      watcherSpeedMps =
          (watcherAltitudeM - watcherPreviousAltitudeM) * 1000.0f / elapsedMs;
    } else {
      watcherSpeedMps = 0.0f;
    }
    watcherPreviousAltitudeM = watcherAltitudeM;
    watcherPreviousMs = nowMs;
  }
}

static bool updateLaunchWatcher(uint32_t nowMs, bool valid) {
  if (recorderMode != RECORDER_WATCHING) return false;
  return flightWatcher.update({nowMs, watcherAltitudeM, watcherSpeedMps,
                               state.pressurePa, valid});
}

static bool saveCheckpoint(uint32_t nowMs) {
  if (!state.sdOk || !recorderCore.recording() || !logFile ||
      !checkpointFilenameA.length() || !checkpointFilenameB.length()) return false;
  // Throttle both successful and failed journal attempts so primary BIN writes
  // always keep priority over recovery metadata.
  lastCheckpointMs = nowMs;
  const uint64_t fileSize = static_cast<uint64_t>(logFile.size());
  if (fileSize > UINT32_MAX) {
    ++checkpointFailures;
    return false;
  }
  const uint32_t committedBytes = static_cast<uint32_t>(fileSize);
  green_ion::recovery::Checkpoint checkpoint = {
    checkpointGeneration + 1,
    committedBytes,
    static_cast<uint32_t>(green_ion::rkt4::completeRecordCount(committedBytes)),
    nowMs - recordingStartedMs,
  };
  uint8_t encoded[green_ion::recovery::kCheckpointBytes];
  if (!green_ion::recovery::encodeCheckpoint(encoded, checkpoint)) {
    ++checkpointFailures;
    return false;
  }

  const String &target = checkpointSlotB ? checkpointFilenameB : checkpointFilenameA;
  digitalWrite(config::PIN_LORA_CS, HIGH);
  if (SD.exists(target) && !SD.remove(target)) {
    ++checkpointFailures;
    return false;
  }
  File journal = SD.open(target, FILE_WRITE);
  const bool saved = journal && journal.write(encoded, sizeof(encoded)) == sizeof(encoded);
  if (journal) {
    journal.flush();
    journal.close();
  }
  if (!saved) {
    ++checkpointFailures;
    return false;
  }
  checkpointGeneration = checkpoint.generation;
  checkpointSlotB = !checkpointSlotB;
  return true;
}

static bool writeStartMetadata() {
  char encoded[320];
  size_t encodedBytes = 0;
  const green_ion::metadata::FlightStart metadata = {
    logFilename.c_str(),
    config::SENSOR_RATE_HZ,
    calibrationBaselinePa,
    calibrationNoiseM,
    state.batteryMv,
    config::PREBUFFER_RECORDS,
  };
  if (green_ion::metadata::encodeFlightStart(
          encoded, sizeof(encoded), metadata, encodedBytes) !=
      green_ion::metadata::METADATA_VALID) {
    ++metadataFailures;
    return false;
  }
  digitalWrite(config::PIN_LORA_CS, HIGH);
  if (SD.exists(metadataFilename) && !SD.remove(metadataFilename)) {
    ++metadataFailures;
    return false;
  }
  File metadataFile = SD.open(metadataFilename, FILE_WRITE);
  const bool saved = metadataFile &&
                     metadataFile.write(reinterpret_cast<const uint8_t *>(encoded),
                                        encodedBytes) == encodedBytes;
  if (metadataFile) {
    metadataFile.flush();
    metadataFile.close();
  }
  if (!saved) ++metadataFailures;
  return saved;
}

static bool writeCompletionMetadata(uint32_t nowMs, uint32_t fileBytes) {
  char encoded[320];
  size_t encodedBytes = 0;
  const green_ion::metadata::FlightComplete metadata = {
    logFilename.c_str(),
    nowMs - recordingStartedMs,
    fileBytes,
    static_cast<uint32_t>(green_ion::rkt4::completeRecordCount(fileBytes)),
    sampleClock.missedPeriods(),
    state.batteryMv,
    landingWatcher.peakAltitudeM(),
  };
  if (green_ion::metadata::encodeFlightComplete(
          encoded, sizeof(encoded), metadata, encodedBytes) !=
      green_ion::metadata::METADATA_VALID) {
    ++metadataFailures;
    return false;
  }
  digitalWrite(config::PIN_LORA_CS, HIGH);
  File metadataFile = SD.open(metadataFilename, FILE_APPEND);
  const bool saved = metadataFile &&
                     metadataFile.write(reinterpret_cast<const uint8_t *>(encoded),
                                        encodedBytes) == encodedBytes;
  if (metadataFile) {
    metadataFile.flush();
    metadataFile.close();
  }
  if (!saved) ++metadataFailures;
  return saved;
}

static bool finalizeRecording(uint32_t nowMs) {
  if (recorderMode != RECORDER_RECORDING || !recorderCore.recording() || !logFile)
    return false;
  if (!recorderCore.stop()) {
    ++finalizationFailures;
    healthBits |= green_ion::HEALTH_SD_WRITE_ERROR;
    state.sdOk = false;
    state.recording = false;
    recorderMode = RECORDER_FAULT;
    if (logFile) logFile.close();
    if (sensorLogFile) sensorLogFile.close();
    return false;
  }
  const uint64_t size = static_cast<uint64_t>(logFile.size());
  logFile.close();
  if (sensorLogCore.active() && !sensorLogCore.stop()) ++sensorLogFailures;
  if (sensorLogFile) sensorLogFile.close();
  state.recording = false;
  if (size > UINT32_MAX ||
      !writeCompletionMetadata(nowMs, static_cast<uint32_t>(size))) {
    ++finalizationFailures;
    recorderMode = RECORDER_FAULT;
    return false;  // Journals remain as recovery evidence.
  }
  if (SD.exists(checkpointFilenameA) && !SD.remove(checkpointFilenameA)) {
    ++finalizationFailures;
    recorderMode = RECORDER_FAULT;
    return false;  // Keep slot B after slot A cleanup fails.
  }
  if (SD.exists(checkpointFilenameB) && !SD.remove(checkpointFilenameB)) {
    ++finalizationFailures;
    recorderMode = RECORDER_FAULT;
    return false;
  }
  landingWatcher.cancel();
  recorderMode = RECORDER_COMPLETE;
  flightState = green_ion::STATE_LANDED;
  return true;
}

static bool openRawLog(uint32_t nowMs, uint32_t nowUs) {
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
  checkpointFilenameA = logFilename;
  checkpointFilenameA.replace(".BIN", ".JRA");
  checkpointFilenameB = logFilename;
  checkpointFilenameB.replace(".BIN", ".JRB");
  metadataFilename = logFilename;
  metadataFilename.replace(".BIN", ".TXT");
  sensorLogFilename = logFilename;
  sensorLogFilename.replace(".BIN", ".SEN");
  checkpointGeneration = 0;
  checkpointSlotB = false;
  recordingStartedMs = nowMs;
  logFile = SD.open(logFilename, FILE_WRITE);
  if (!logFile) {
    logFilename = "";
    checkpointFilenameA = "";
    checkpointFilenameB = "";
    metadataFilename = "";
    sensorLogFilename = "";
    return false;
  }

  green_ion::rkt4::Header header = {
    config::SENSOR_RATE_HZ,
    calibrationBaselinePa,
    state.batteryMv,
  };
  logSink.attach(logFile);
  if (!recorderCore.start(logSink, header)) {
    logFile.close();
    SD.remove(logFilename);
    logFilename = "";
    checkpointFilenameA = "";
    checkpointFilenameB = "";
    metadataFilename = "";
    sensorLogFilename = "";
    healthBits |= green_ion::HEALTH_SD_WRITE_ERROR;
    state.sdOk = false;
    return false;
  }
  state.recording = true;
  lastCheckpointMs = nowMs;
  saveCheckpoint(nowMs);  // RecorderCore::start already flushed header and prebuffer.
  writeStartMetadata();    // Metadata failure never stops the primary BIN recorder.

  // The full-resolution companion is deliberately secondary: the primary BIN
  // is already flushed, checkpointed, and described before this file is opened.
  uint8_t rkt4HeaderBytes[green_ion::rkt4::kHeaderBytes];
  const uint16_t flightSequence =
      static_cast<uint16_t>(logFilename.substring(5, 8).toInt());
  const green_ion::sensor_log::Header sensorHeader = {
    config::SENSOR_RATE_HZ,
    flightSequence,
    nowUs,
    green_ion::rkt4::encodeHeader(rkt4HeaderBytes, header)
        ? green_ion::sensor_log::crc32(rkt4HeaderBytes, sizeof(rkt4HeaderBytes))
        : 0,
  };
  digitalWrite(config::PIN_LORA_CS, HIGH);
  sensorLogFile = SD.open(sensorLogFilename, FILE_WRITE);
  if (!sensorLogFile) {
    ++sensorLogFailures;
  } else {
    sensorLogSink.attach(sensorLogFile);
    if (!sensorLogCore.start(sensorLogSink, sensorHeader)) {
      ++sensorLogFailures;
      sensorLogFile.close();
    } else {
      lastSensorLogFlushMs = nowMs;
    }
  }
  return true;
}

static green_ion::sensor_log::Record makeSensorLogRecord(uint32_t nowUs,
                                                          uint32_t nowMs) {
  uint16_t flags = 0;
  if (state.imuDataValid) flags |= green_ion::sensor_log::FLAG_IMU_VALID;
  const bool positionValid = state.gpsHasPosition &&
      isfinite(state.latitude) && isfinite(state.longitude) &&
      state.latitude >= -90.0 && state.latitude <= 90.0 &&
      state.longitude >= -180.0 && state.longitude <= 180.0 &&
      isfinite(state.gpsAltitudeM) && isfinite(state.gpsSpeedMps);
  if (positionValid) flags |= green_ion::sensor_log::FLAG_GPS_HAS_POSITION;
  if (positionValid && gpsPositionCurrent(nowMs))
    flags |= green_ion::sensor_log::FLAG_GPS_CURRENT;
  return {
    nowUs,
    sensorLogSequence++,
    state.accelX,
    state.accelY,
    state.accelZ,
    state.gyroX,
    state.gyroY,
    state.gyroZ,
    positionValid ? clampI32(state.latitude * 1e7) : 0,
    positionValid ? clampI32(state.longitude * 1e7) : 0,
    positionValid ? state.gpsAltitudeM : 0.0f,
    positionValid ? state.gpsSpeedMps : 0.0f,
    positionValid ? gpsAgeMs(nowMs) : UINT32_MAX,
    positionValid ? state.lastGpsFixMs : 0,
    flags,
    state.satellites <= 64 ? state.satellites : 64,
    state.gpsHour,
    state.gpsMinute,
    state.gpsSecond,
  };
}

static void serviceSensorLog(uint32_t nowMs, uint32_t nowUs) {
  if (!sensorLogCore.active() || !sensorLogFile) return;
  const uint32_t marginUs = sampleClock.nextDeadlineUs() - nowUs;
  if (marginUs >= 0x80000000u ||
      marginUs < config::SENSOR_LOG_SERVICE_GUARD_US)
    return;
  if (!sensorLogCore.serviceOne()) {
    ++sensorLogFailures;
    sensorLogFile.close();
    return;
  }
  if (sensorLogCore.queuedRecords() == 0 &&
      nowMs - lastSensorLogFlushMs >= config::LOG_FLUSH_INTERVAL_MS) {
    lastSensorLogFlushMs = nowMs;
    if (!sensorLogCore.flushIfDrained()) {
      ++sensorLogFailures;
      sensorLogFile.close();
    }
  }
}

static void appendLog(uint32_t nowUs, uint32_t nowMs, bool valid) {
  if (recorderMode == RECORDER_COMPLETE) return;
  green_ion::rkt4::Record record = {
    nowUs,
    state.pressurePa,
    state.temperatureC,
    state.batteryMv,
    valid,
  };
  recorderCore.observe(record);
  sensorLogCore.observe(makeSensorLogRecord(nowUs, nowMs));
  processCalibration(nowMs, valid);
  if (recorderMode == RECORDER_FAULT && calibrationRetryAtMs &&
      static_cast<int32_t>(nowMs - calibrationRetryAtMs) >= 0)
    resetCalibration(nowMs);
  updateFlightEstimate(nowMs, valid);
  if (updateLaunchWatcher(nowMs, valid)) {
    if (state.sdOk && openRawLog(nowMs, nowUs)) {
      recorderMode = RECORDER_RECORDING;
      flightState = green_ion::STATE_ASCENT;
      if (!landingWatcher.begin(nowMs, {
            config::SENSOR_RATE_HZ, 0.0f, calibrationNoiseM}))
        ++finalizationFailures;
    } else {
      recorderMode = RECORDER_FAULT;
    }
  }
  if (recorderMode == RECORDER_RECORDING &&
      landingWatcher.update({nowMs, watcherAltitudeM, watcherSpeedMps, valid}))
    finalizeRecording(nowMs);
  state.recording = recorderCore.recording();
  if (recorderCore.status() == green_ion::recorder::CORE_RECORD_WRITE_FAILED) {
    healthBits |= green_ion::HEALTH_SD_WRITE_ERROR;
    state.sdOk = false;
    state.recording = false;
    recorderMode = RECORDER_FAULT;
    if (logFile) logFile.close();
    if (sensorLogFile) sensorLogFile.close();
  }
}

static uint16_t telemetryFlags(uint32_t ageMs) {
  uint16_t flags = 0;
  if (state.gpsHasPosition) flags |= green_ion::FLAG_GPS_HAS_POSITION;
  if (state.gpsFixCurrent && state.gpsHasPosition &&
      ageMs <= config::GPS_CURRENT_MAX_AGE_MS)
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
                          (gpsPositionCurrent(millis()) ? "live" : "stale");
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
  body += ",\"health\":" + String(healthBits);
  body += ",\"prebuffer_records\":" + String(recorderCore.bufferedRecords());
  body += ",\"recorder_status\":" + String(static_cast<uint8_t>(recorderCore.status()));
  body += ",\"recorder_mode\":" + String(static_cast<uint8_t>(recorderMode));
  body += ",\"calibration_baseline_pa\":" + numberOrNull(calibrationBaselinePa, 1);
  body += ",\"calibration_noise_m\":" + numberOrNull(calibrationNoiseM, 3);
  body += ",\"relative_altitude_m\":" + numberOrNull(watcherAltitudeM, 2);
  body += ",\"checkpoint_generation\":" + String(checkpointGeneration);
  body += ",\"checkpoint_failures\":" + String(checkpointFailures);
  body += ",\"metadata_file\":\"" + metadataFilename + "\"";
  body += ",\"metadata_failures\":" + String(metadataFailures);
  body += ",\"sensor_log_file\":\"" + sensorLogFilename + "\"";
  body += ",\"sensor_log_failures\":" + String(sensorLogFailures);
  body += ",\"sensor_log_pending\":" + String(sensorLogCore.queuedRecords());
  body += ",\"sensor_log_dropped\":" + String(sensorLogCore.droppedRecords());
  body += ",\"sensor_log_status\":" + String(static_cast<uint8_t>(sensorLogCore.status()));
  body += ",\"finalization_failures\":" + String(finalizationFailures);
  body += ",\"landing_peak_m\":" + numberOrNull(landingWatcher.peakAltitudeM(), 2);
  body += ",\"missed_sample_periods\":" + String(sampleClock.missedPeriods());
  body += ",\"recovered_logs\":" + String(recoveredLogs);
  body += ",\"recovered_records\":" + String(recoveredRecords);
  body += ",\"recovered_trailing_bytes\":" + String(recoveredTrailingBytes);
  body += ",\"recovery_pending\":" + String(recoveryPending);
  body += ",\"recovery_status\":\"" + recoveryStatus + "\"}";
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

static void queueTelemetry(uint32_t nowMs) {
  if (nowMs - lastTelemetryMs < config::TELEMETRY_INTERVAL_MS) return;
  lastTelemetryMs = nowMs;
  const uint32_t age = gpsAgeMs(nowMs);
  green_ion::TelemetryPacket packet;
  green_ion::initialize(packet);
  const bool benchPacket = radioBenchPacketsRemaining > 0;
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
  if (benchPacket) packet.reserved = green_ion::kBenchRadioMarker;
  green_ion::seal(packet);

  telemetryQueue.push(packet);
  if (benchPacket) --radioBenchPacketsRemaining;
}

static void serviceTelemetry(uint32_t nowMs, uint32_t nowUs) {
  if (nowMs - lastRadioAttemptMs < config::RADIO_RETRY_INTERVAL_MS) return;
  const bool radioBusy = state.radioOk && radio.mode() == RHGenericDriver::RHModeTx;
  if (!green_ion::radioDispatchAllowed(
          state.radioOk, radioBusy, false, telemetryQueue.size() > 0,
          nowUs, sampleClock.nextDeadlineUs(), config::RADIO_SERVICE_GUARD_US))
    return;

  const green_ion::TelemetryPacket *queued = telemetryQueue.front();
  if (!queued) return;
  const green_ion::TelemetryPacket packet = *queued;
  lastRadioAttemptMs = nowMs;

  digitalWrite(config::PIN_SD_CS, HIGH);
  if (!radio.send(reinterpret_cast<const uint8_t *>(&packet), sizeof(packet))) {
    ++radioSendFailures;
    healthBits |= green_ion::HEALTH_RADIO_SEND_ERROR;
    if (radioRecovery.noteSendResult(nowMs, false)) state.radioOk = false;
    Serial.printf("BENCH RADIO status=SEND_FAILED sequence=%lu failures=%lu\n",
                  static_cast<unsigned long>(packet.sequence),
                  static_cast<unsigned long>(radioSendFailures));
  } else {
    radioRecovery.noteSendResult(nowMs, true);
    telemetryQueue.pop();
    if (!green_ion::isBenchRadioPacket(packet)) return;
    ++radioBenchPacketsSent;
    Serial.printf("BENCH RADIO status=TX sequence=%lu remaining=%u\n",
                  static_cast<unsigned long>(packet.sequence),
                  radioBenchPacketsRemaining);
  }
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
  recoverInterruptedLogs();
  resetCalibration(millis());
  initializeRadio();
  initializeDashboard();
  Serial.printf("GREEN ION V12 dev: baro=%d imu=%d gps=%d sd=%d radio=%d\n",
                state.barometerOk, state.imuOk, state.gpsOk, state.sdOk, state.radioOk);
  Serial.println("Serial bench output is ON. Commands: bench imu | bench gps | bench all | bench status | bench off | radio test | radio retry | radio status");
  Serial.println("RADIO SAFETY: attach a suitable antenna before entering 'radio test'.");
  sampleClock.start(micros());
}

void loop() {
  const uint32_t nowMs = millis();
  const uint32_t nowUs = micros();

  if (sampleClock.due(nowUs)) {
    const bool barometerReadingValid = readSensors();
    appendLog(nowUs, nowMs, barometerReadingValid);
  }
  serviceGps(nowMs);
  serviceSerialBench(nowMs);

  if (state.sdOk && recorderCore.recording() && nowMs - lastFlushMs >= config::LOG_FLUSH_INTERVAL_MS) {
    lastFlushMs = nowMs;
    if (!recorderCore.flush()) {
      healthBits |= green_ion::HEALTH_SD_WRITE_ERROR;
      state.sdOk = false;
      state.recording = false;
      recorderMode = RECORDER_FAULT;
      if (logFile) logFile.close();
      if (sensorLogFile) sensorLogFile.close();
    } else if (nowMs - lastCheckpointMs >= config::CHECKPOINT_INTERVAL_MS) {
      saveCheckpoint(nowMs);
    }
  }

  // Only spend remaining sample-cycle time on the companion after the primary
  // BIN flush/checkpoint path has completed.
  serviceSensorLog(nowMs, micros());

  serviceRadioRecovery(nowMs);
  queueTelemetry(nowMs);
  serviceTelemetry(nowMs, micros());
  dashboardServer.handleClient();
  yield();
}

