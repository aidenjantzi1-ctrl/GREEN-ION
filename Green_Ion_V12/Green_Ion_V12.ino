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
#include <GreenIonRkt4.h>
#include <GreenIonRecorderCore.h>
#include <GreenIonFlightWatcher.h>
#include <GreenIonCalibration.h>
#include <GreenIonCheckpoint.h>

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
String checkpointFilenameA;
String checkpointFilenameB;
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
green_ion::recorder::RecorderCore<config::PREBUFFER_RECORDS> recorderCore;
green_ion::flight::FlightWatcher flightWatcher;

enum RecorderMode : uint8_t {
  RECORDER_CALIBRATING = 0,
  RECORDER_WATCHING,
  RECORDER_RECORDING,
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
uint32_t watcherPreviousMs = 0;

uint32_t telemetrySequence = 0;
uint32_t lastSensorUs = 0;
uint32_t lastGpsServiceMs = 0;
uint32_t lastTelemetryMs = 0;
uint32_t lastFlushMs = 0;
uint32_t lastCheckpointMs = 0;
uint32_t checkpointGeneration = 0;
uint32_t checkpointFailures = 0;
uint32_t recordingStartedMs = 0;
bool checkpointSlotB = false;
uint32_t recoveredLogs = 0;
uint32_t recoveredRecords = 0;
uint32_t recoveredTrailingBytes = 0;
uint32_t recoveryPending = 0;
String recoveryStatus = "Not scanned";
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
  if (SD.exists(reportPath) && !SD.remove(reportPath)) {
    ++recoveryPending;
    return false;
  }
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
  File recovery = SD.open(reportPath, FILE_WRITE);
  const bool reportSaved = recovery && recovery.print(report) == report.length();
  if (recovery) {
    recovery.flush();
    recovery.close();
  }
  if (!reportSaved) {
    ++recoveryPending;
    return false;
  }

  bool journalsRemoved = true;
  if (SD.exists(slotAPath)) journalsRemoved = SD.remove(slotAPath) && journalsRemoved;
  if (SD.exists(slotBPath)) journalsRemoved = SD.remove(slotBPath) && journalsRemoved;
  ++recoveredLogs;
  recoveredRecords += inspection.completeRecords;
  recoveredTrailingBytes += inspection.trailingBytes;
  if (!journalsRemoved) ++recoveryPending;
  return journalsRemoved;
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
  return barometerReadingValid;
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
  watcherPreviousMs = 0;
  const bool watching = flightWatcher.begin({
      config::SENSOR_RATE_HZ, 0.0f, calibrationBaselinePa, calibrationNoiseM});
  recorderMode = watching ? RECORDER_WATCHING : RECORDER_FAULT;
  if (!watching) calibrationRetryAtMs = nowMs + config::CALIBRATION_RETRY_MS;
}

static bool updateLaunchWatcher(uint32_t nowMs, bool valid) {
  if (recorderMode != RECORDER_WATCHING) return false;
  const float relativeM = valid ? green_ion::calibration::relativeAltitudeM(
      state.pressurePa, calibrationBaselinePa) : NAN;
  float speedMps = NAN;
  if (isfinite(relativeM)) {
    if (!isfinite(watcherAltitudeM)) watcherAltitudeM = relativeM;
    const float difference = fabsf(relativeM - watcherAltitudeM);
    const float alpha = difference < 0.25f ? 0.28f :
                        difference < 1.5f ? 0.48f :
                        difference < 5.0f ? 0.68f : 0.84f;
    watcherAltitudeM += alpha * (relativeM - watcherAltitudeM);
    if (isfinite(watcherPreviousAltitudeM) && watcherPreviousMs != nowMs) {
      const uint32_t elapsedMs = nowMs - watcherPreviousMs;
      speedMps = (watcherAltitudeM - watcherPreviousAltitudeM) * 1000.0f / elapsedMs;
    } else {
      speedMps = 0.0f;
    }
    watcherPreviousAltitudeM = watcherAltitudeM;
    watcherPreviousMs = nowMs;
  }
  return flightWatcher.update({nowMs, watcherAltitudeM, speedMps,
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

static bool openRawLog(uint32_t nowMs) {
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
  checkpointGeneration = 0;
  checkpointSlotB = false;
  recordingStartedMs = nowMs;
  logFile = SD.open(logFilename, FILE_WRITE);
  if (!logFile) return false;

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
    healthBits |= green_ion::HEALTH_SD_WRITE_ERROR;
    state.sdOk = false;
    return false;
  }
  state.recording = true;
  lastCheckpointMs = nowMs;
  saveCheckpoint(nowMs);  // RecorderCore::start already flushed header and prebuffer.
  return true;
}

static void appendLog(uint32_t nowUs, uint32_t nowMs, bool valid) {
  green_ion::rkt4::Record record = {
    nowUs,
    state.pressurePa,
    state.temperatureC,
    state.batteryMv,
    valid,
  };
  recorderCore.observe(record);
  processCalibration(nowMs, valid);
  if (recorderMode == RECORDER_FAULT && calibrationRetryAtMs &&
      static_cast<int32_t>(nowMs - calibrationRetryAtMs) >= 0)
    resetCalibration(nowMs);
  if (updateLaunchWatcher(nowMs, valid)) {
    if (state.sdOk && openRawLog(nowMs)) {
      recorderMode = RECORDER_RECORDING;
      flightState = green_ion::STATE_ASCENT;
    } else {
      recorderMode = RECORDER_FAULT;
    }
  }
  state.recording = recorderCore.recording();
  if (recorderCore.status() == green_ion::recorder::CORE_RECORD_WRITE_FAILED) {
    healthBits |= green_ion::HEALTH_SD_WRITE_ERROR;
    state.sdOk = false;
    state.recording = false;
    recorderMode = RECORDER_FAULT;
    if (logFile) logFile.close();
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
  body += ",\"health\":" + String(healthBits);
  body += ",\"prebuffer_records\":" + String(recorderCore.bufferedRecords());
  body += ",\"recorder_status\":" + String(static_cast<uint8_t>(recorderCore.status()));
  body += ",\"recorder_mode\":" + String(static_cast<uint8_t>(recorderMode));
  body += ",\"calibration_baseline_pa\":" + numberOrNull(calibrationBaselinePa, 1);
  body += ",\"calibration_noise_m\":" + numberOrNull(calibrationNoiseM, 3);
  body += ",\"relative_altitude_m\":" + numberOrNull(watcherAltitudeM, 2);
  body += ",\"checkpoint_generation\":" + String(checkpointGeneration);
  body += ",\"checkpoint_failures\":" + String(checkpointFailures);
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
  recoverInterruptedLogs();
  resetCalibration(millis());
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
    const bool barometerReadingValid = readSensors();
    appendLog(nowUs, nowMs, barometerReadingValid);
  }

  if (state.sdOk && recorderCore.recording() && nowMs - lastFlushMs >= config::LOG_FLUSH_INTERVAL_MS) {
    lastFlushMs = nowMs;
    if (!recorderCore.flush()) {
      healthBits |= green_ion::HEALTH_SD_WRITE_ERROR;
      state.sdOk = false;
      state.recording = false;
      recorderMode = RECORDER_FAULT;
      if (logFile) logFile.close();
    } else if (nowMs - lastCheckpointMs >= config::CHECKPOINT_INTERVAL_MS) {
      saveCheckpoint(nowMs);
    }
  }

  transmitTelemetry(nowMs);
  dashboardServer.handleClient();
  yield();
}

