/*
  GREEN ION Feather V16.2 firmware
  Flight-data recorder and recovery aid only.
  It must never control ignition, staging, or recovery deployment.

  V16.2 retains the validated recorder and RKT4/GIS1 file compatibility. This generation combines
  RKT4 recording, a bounded prebuffer, stationary pressure calibration, and a
  recorder-only launch watcher, recovery, and bench-only OTA maintenance.
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
#include <esp_ota_ops.h>
#include <esp_system.h>
#include <Preferences.h>
#include <GreenIonPreflight.h>
#include <GreenIonFileNaming.h>
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
#include <GreenIonSensorRecovery.h>
#include <GreenIonSensorHealth.h>
#include <GreenIonBenchTests.h>

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
// User-selectable per-rocket metadata is written only to .TXT, never RKT4.
String selectedRocket = ""; // Never assume which rocket is installed.
String selectedMotor = "";
uint32_t calibratedAtMs = 0;
bool orphanHomeFile = false;
WebServer dashboardServer(80);

static const char FIRMWARE_NAME[] = "GREEN ION Feather V16.2";
static const char FIRMWARE_VERSION[] = "16.2.0";
static uint32_t bootSessionId = 0;
static esp_reset_reason_t bootResetReason = ESP_RST_UNKNOWN;
static uint32_t lastSdRecoveryAttemptMs = 0;
static uint32_t lastImuRecoveryAttemptMs = 0;
static uint32_t lastGpsRecoveryAttemptMs = 0;

static const char *resetReasonName(esp_reset_reason_t reason) {
  switch (reason) {
    case ESP_RST_POWERON: return "power_on";
    case ESP_RST_EXT: return "external_reset";
    case ESP_RST_SW: return "software_reset";
    case ESP_RST_PANIC: return "panic";
    case ESP_RST_INT_WDT: return "interrupt_watchdog";
    case ESP_RST_TASK_WDT: return "task_watchdog";
    case ESP_RST_WDT: return "watchdog";
    case ESP_RST_DEEPSLEEP: return "deep_sleep_wake";
    case ESP_RST_BROWNOUT: return "brownout";
    case ESP_RST_SDIO: return "sdio_reset";
    default: return "unknown";
  }
}

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
green_ion::SensorHealth barometerHealth(
    config::BAROMETER_REINITIALIZE_DELAY_MS,
    config::BAROMETER_READ_FAILURE_THRESHOLD);
green_ion::bench::ImuTest imuHardwareTest(config::IMU_TEST_DURATION_MS,
                                          config::IMU_TEST_MINIMUM_SAMPLES);
green_ion::bench::GpsTest gpsHardwareTest(config::GPS_TEST_DURATION_MS);

enum HardwareTestRequest : uint8_t {
  HARDWARE_TEST_NONE = 0,
  HARDWARE_TEST_IMU,
  HARDWARE_TEST_GPS,
  HARDWARE_TEST_ALL,
};

HardwareTestRequest hardwareTestRequest = HARDWARE_TEST_NONE;
bool imuHardwareTestReported = true;
bool gpsHardwareTestReported = true;
bool imuRange32gVerified = false;

enum RecorderMode : uint8_t {
  RECORDER_CALIBRATING = 0,
  RECORDER_WATCHING,
  RECORDER_RECORDING,
  RECORDER_COMPLETE,
  RECORDER_FAULT,
  RECORDER_MAINTENANCE,
  RECORDER_HOME_TEST,
};

RecorderMode recorderMode = RECORDER_CALIBRATING;
// In maintenance, the flight watcher is disabled; never update on a launch pad.
static bool otaUploadInProgress = false;
static bool otaUploadSucceeded = false;
static bool otaHeaderChecked = false;
static size_t otaUploadedBytes = 0;
static String otaUploadError;
static String otaMaintenanceToken;
static uint32_t consoleLastStatusMs = 0;
static uint8_t consoleLastMode = 0xff;
// Dedicated reserved-file home verification, isolated from actual flights.
static File homeFile;
static bool homeLogActive = false, homeWifiSeen = false, homeWifiAbsent = false;
static bool homeCleanupOk = true;
static uint8_t homeResult = 0;  // idle=0, running=1, pass=2, fail=3
static uint16_t homeSamples = 0, homeValidSamples = 0;
static uint32_t homeStartedMs = 0, homeMissedStart = 0, homeMissed = 0;
static uint32_t homeRadioStartSent = 0, homeRadioStartFailures = 0;
static uint8_t homeStepStatuses[9] = {}; // Persistent evidence codes (not tests in progress).
static bool homeInterrupted = false;
static bool homeSdVerified = false;
// Prevent stale IMU/GPS results from prior sessions being certified as new.
static bool homeImuTestStarted = false;
static bool homeGpsTestStarted = false;
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
uint32_t sensorLogCommittedBytes = 0;
uint32_t sensorLogCommittedRecords = 0;
bool sensorLogCheckpointable = false;
uint32_t recordingStartedMs = 0;
bool checkpointSlotB = false;
uint32_t recoveredLogs = 0;
uint32_t recoveredRecords = 0;
uint32_t recoveredTrailingBytes = 0;
uint32_t recoveredSensorRecords = 0;
uint32_t sensorRecoveryIssues = 0;
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

BenchMode benchMode = BENCH_OFF;
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

static bool configureBarometerHardware() {
  const bool initialized = barometer.begin_I2C(0x77) || barometer.begin_I2C(0x76);
  if (initialized) {
    barometer.setTemperatureOversampling(BMP3_OVERSAMPLING_8X);
    barometer.setPressureOversampling(BMP3_OVERSAMPLING_4X);
    barometer.setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_3);
  }
  return initialized;
}

static bool configureImuHardware() {
  const bool initialized = imu.begin_I2C(0x6A, &Wire) ||
                           imu.begin_I2C(0x6B, &Wire);
  if (initialized) {
    imu.setAccelRange(LSM6DSO32_ACCEL_RANGE_32_G);
    imu.setGyroRange(LSM6DS_GYRO_RANGE_2000_DPS);
    imu.setAccelDataRate(LSM6DS_RATE_104_HZ);
    imu.setGyroDataRate(LSM6DS_RATE_104_HZ);
    imuRange32gVerified =
        imu.getAccelRange() == LSM6DSO32_ACCEL_RANGE_32_G;
  } else {
    imuRange32gVerified = false;
  }
  return initialized;
}

static bool configureGpsHardware() {
  const bool initialized = gps.begin(0x10);
  if (initialized) {
    gps.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
    gps.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);
  }
  return initialized;
}

static void initializeSensors() {
  state.barometerOk = configureBarometerHardware();
  barometerHealth.begin(millis(), state.barometerOk);
  if (!state.barometerOk) healthBits |= green_ion::HEALTH_BARO_INIT_ERROR;

  state.imuOk = configureImuHardware();
  if (!state.imuOk) healthBits |= green_ion::HEALTH_IMU_INIT_ERROR;

  state.gpsOk = configureGpsHardware();
  if (!state.gpsOk) healthBits |= green_ion::HEALTH_GPS_INIT_ERROR;
}

static void initializeStorage() {
  digitalWrite(config::PIN_LORA_CS, HIGH);
  ++state.sdMountAttempts;
  state.sdOk = SD.begin(config::PIN_SD_CS, SPI);
  if (!state.sdOk) healthBits |= green_ion::HEALTH_SD_INIT_ERROR;
  orphanHomeFile = state.sdOk && SD.exists("/GIHOME.BIN");
  if (orphanHomeFile)
    Serial.println("[HOME] Interrupted temporary file found. Run home verification cleanup; do not consider preflight complete.");
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
  if (!file) return 0;
  const size_t fileBytes = static_cast<size_t>(file.size());
  if (fileBytes != green_ion::recovery::kCheckpointV1Bytes &&
      fileBytes != green_ion::recovery::kCheckpointBytes) {
    if (file) file.close();
    return 0;
  }
  const int readResult = file.read(out, fileBytes);
  const size_t bytes = readResult > 0 ? static_cast<size_t>(readResult) : 0;
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

static void appendSensorRecoveryEvidence(
    const String &binLeaf,
    const uint8_t rkt4Header[green_ion::rkt4::kHeaderBytes], String &report,
    const green_ion::recovery::Checkpoint &checkpoint,
    uint32_t &validSensorRecords, bool &sensorIssue) {
  validSensorRecords = 0;
  sensorIssue = false;
  String sensorLeaf = binLeaf;
  sensorLeaf.replace(".BIN", ".SEN");
  const String sensorPath = "/" + sensorLeaf;
  report += "\nsensor_file=" + sensorLeaf;
  if (!SD.exists(sensorPath)) {
    report += "\nsensor_log_present=no\nsensor_log_status=missing\n";
    sensorIssue = checkpoint.sensorPresent;
    return;
  }

  report += "\nsensor_log_present=yes";
  File sensor = SD.open(sensorPath, FILE_READ);
  if (!sensor || static_cast<uint64_t>(sensor.size()) > UINT32_MAX) {
    if (sensor) sensor.close();
    report += "\nsensor_log_status=unreadable\n";
    sensorIssue = true;
    return;
  }

  const uint32_t sensorBytes = static_cast<uint32_t>(sensor.size());
  const bool checkpointWithinFile =
      green_ion::recovery::sensorCheckpointForFile(checkpoint, sensorBytes);
  uint8_t sensorHeaderBytes[green_ion::sensor_log::kHeaderBytes] = {};
  const int sensorHeaderResult = sensor.read(
      sensorHeaderBytes, sizeof(sensorHeaderBytes));
  const size_t sensorHeaderRead = sensorHeaderResult > 0
      ? static_cast<size_t>(sensorHeaderResult) : 0;
  green_ion::sensor_log::Header sensorHeader = {};
  auto inspection = green_ion::sensor_recovery::beginInspection(
      sensorHeaderBytes, sensorHeaderRead, sensorBytes, sensorHeader);
  const uint16_t expectedSequence =
      static_cast<uint16_t>(binLeaf.substring(4, 7).toInt());
  const bool bindingValid =
      inspection.headerResult == green_ion::sensor_log::SENSOR_LOG_VALID &&
      green_ion::sensor_recovery::headerMatchesFlight(
          sensorHeader, expectedSequence, rkt4Header,
          green_ion::rkt4::kHeaderBytes);

  uint8_t sensorRecord[green_ion::sensor_log::kRecordBytes];
  while (inspection.headerResult == green_ion::sensor_log::SENSOR_LOG_VALID &&
         inspection.recordsInspected < inspection.completeRecords) {
    const int recordRead = sensor.read(sensorRecord, sizeof(sensorRecord));
    const size_t bytes = recordRead > 0 ? static_cast<size_t>(recordRead) : 0;
    if (!green_ion::sensor_recovery::inspectRecord(
            inspection, sensorRecord, bytes))
      break;
  }
  sensor.close();
  green_ion::sensor_recovery::finishInspection(inspection);
  const bool recordsValid =
      green_ion::sensor_recovery::completeRecordsValid(inspection);
  const bool healthy = bindingValid && checkpointWithinFile && recordsValid &&
                       !inspection.trailingBytes;
  validSensorRecords = inspection.validPrefixRecords;
  sensorIssue = !healthy;

  report += "\nsensor_actual_bytes=" + String(sensorBytes);
  report += "\nsensor_checkpoint_present=" +
            String(checkpoint.sensorPresent ? "yes" : "no");
  report += "\nsensor_checkpoint_committed_bytes=" +
            String(checkpoint.sensorCommittedBytes);
  report += "\nsensor_checkpoint_record_count=" +
            String(checkpoint.sensorRecordCount);
  report += "\nsensor_checkpoint_within_file=" +
            String(checkpointWithinFile ? "yes" : "no");
  report += "\nsensor_header_result=" +
            String(static_cast<uint8_t>(inspection.headerResult));
  report += "\nsensor_binding_valid=" + String(bindingValid ? "yes" : "no");
  report += "\nsensor_complete_records=" + String(inspection.completeRecords);
  report += "\nsensor_valid_prefix_records=" +
            String(inspection.validPrefixRecords);
  report += "\nsensor_trailing_partial_bytes=" +
            String(inspection.trailingBytes);
  if (inspection.firstInvalidRecord == UINT32_MAX) {
    report += "\nsensor_first_invalid_record=none";
  } else {
    report += "\nsensor_first_invalid_record=" +
              String(inspection.firstInvalidRecord);
  }
  report += "\nsensor_record_result=" +
            String(static_cast<uint8_t>(inspection.firstInvalidResult));
  report += "\nsensor_log_status=";
  report += healthy ? "valid" :
            (bindingValid && recordsValid ? "torn_tail" : "invalid");
  report += "\n";
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
  String report;
  report.reserve(640);
  report += "GREEN-ION-RECOVERY-1\nfile=" + binLeaf;
  report += "\nstatus=interrupted";
  report += "\ncheckpoint_generation=" + String(checkpoint.generation);
  report += "\ncheckpoint_committed_bytes=" + String(checkpoint.committedBytes);
  report += "\ncheckpoint_format=GIC" + String(checkpoint.formatVersion);
  report += "\nactual_bytes=" + String(fileBytes);
  report += "\ncomplete_records=" + String(inspection.completeRecords);
  report += "\ntrailing_partial_bytes=" + String(inspection.trailingBytes);
  report += "\nrkt4_header_valid=yes";
  uint32_t validSensorRecords = 0;
  bool sensorIssue = false;
  appendSensorRecoveryEvidence(
      binLeaf, header, report, checkpoint, validSensorRecords, sensorIssue);
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
  recoveredSensorRecords += validSensorRecords;
  if (sensorIssue) ++sensorRecoveryIssues;
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

static bool readSensors(uint32_t nowMs) {
  bool barometerReadingValid = false;
  if (state.barometerOk && barometer.performReading()) {
    state.pressurePa = barometer.pressure;
    state.temperatureC = barometer.temperature;
    state.altitudeM = barometer.readAltitude(1013.25f);
    state.lastBarometerReadMs = nowMs;
    barometerReadingValid = isfinite(state.pressurePa) && isfinite(state.temperatureC) &&
                            state.pressurePa > 30000.0f && state.pressurePa < 120000.0f;
  }
  if (state.barometerOk && !barometerReadingValid) {
    healthBits |= green_ion::HEALTH_BARO_READ_ERROR;
    state.pressurePa = state.temperatureC = state.altitudeM = NAN;
  }
  if (state.barometerOk && barometerHealth.noteRead(nowMs, barometerReadingValid)) {
    state.barometerOk = false;
    Serial.printf("BENCH BARO status=RUNTIME_LOST losses=%lu retry_deferred_while_active=1\n",
                  static_cast<unsigned long>(barometerHealth.runtimeLosses()));
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
      state.imuTemperatureC = temperature.temperature;
      state.lastImuReadMs = nowMs;
      state.imuConsecutiveReadFailures = 0;
    } else {
      ++state.imuReadFailures;
      if (state.imuConsecutiveReadFailures != UINT8_MAX)
        ++state.imuConsecutiveReadFailures;
      healthBits |= green_ion::HEALTH_IMU_READ_ERROR;
      state.accelX = state.accelY = state.accelZ = state.accelMagnitude = NAN;
      state.gyroX = state.gyroY = state.gyroZ = NAN;
      state.imuTemperatureC = NAN;
      if (state.imuConsecutiveReadFailures >= 3) {
        state.imuOk = false;
        healthBits |= green_ion::HEALTH_IMU_RUNTIME_LOST;
      }
    }
  }

  const float toG = 1.0f / 9.80665f;
  const float toDegrees = 180.0f / PI;
  imuHardwareTest.observe(nowMs, state.imuDataValid,
                          state.accelMagnitude * toG,
                          state.gyroX * toDegrees,
                          state.gyroY * toDegrees,
                          state.gyroZ * toDegrees,
                          state.imuTemperatureC);

  state.batteryMv = static_cast<uint16_t>(analogReadMilliVolts(config::PIN_BATTERY) * 2u);
  state.lastBatteryReadMs = nowMs;
  return barometerReadingValid;
}

static void serviceGps(uint32_t nowMs) {
  if (!state.gpsOk || nowMs - lastGpsServiceMs < config::GPS_SERVICE_INTERVAL_MS) return;
  lastGpsServiceMs = nowMs;
  gps.read();
  if (!gps.newNMEAreceived()) return;
  state.lastGpsNmeaMs = nowMs;
  if (state.gpsNmeaSentences != UINT32_MAX) ++state.gpsNmeaSentences;
  char *sentence = gps.lastNMEA();
  if (!sentence || !gps.parse(sentence)) {
    ++state.gpsParseFailures;
    return;
  }
  if (state.gpsParsedSentences != UINT32_MAX) ++state.gpsParsedSentences;
  state.lastGpsSentenceMs = nowMs;
  state.satellites = gps.satellites;
  state.gpsHour = gps.hour;
  state.gpsMinute = gps.minute;
  state.gpsSecond = gps.seconds;
  state.gpsFixCurrent = gps.fix;
  state.gpsFixQuality = gps.fixquality;
  if (state.gpsFixCurrent) {
    state.gpsHasPosition = true;
    state.lastGpsFixMs = nowMs;
    state.latitude = gps.latitudeDegrees;
    state.longitude = gps.longitudeDegrees;
    state.gpsAltitudeM = gps.altitude;
    state.gpsSpeedMps = gps.speed * 0.514444f;
    state.gpsCourseDeg = isfinite(gps.angle) && state.gpsSpeedMps >= 0.75f
                             ? gps.angle : NAN;  // Unreliable when stationary
    state.gpsHdop = isfinite(gps.HDOP) && gps.HDOP > 0.0f ? gps.HDOP : NAN;
    state.gpsGeoidSeparationM = isfinite(gps.geoidheight) ? gps.geoidheight : NAN;
  }
}

static uint32_t gpsAgeMs(uint32_t nowMs) {
  return state.gpsHasPosition ? nowMs - state.lastGpsFixMs : UINT32_MAX;
}

static bool gpsPositionCurrent(uint32_t nowMs) {
  return state.gpsHasPosition && state.gpsFixCurrent &&
         gpsAgeMs(nowMs) <= config::GPS_CURRENT_MAX_AGE_MS;
}

static void resetCalibration(uint32_t nowMs);

static const char *hardwareTestResultName(green_ion::bench::Result result) {
  switch (result) {
    case green_ion::bench::RESULT_RUNNING: return "RUNNING";
    case green_ion::bench::RESULT_PASS: return "PASS";
    case green_ion::bench::RESULT_FAIL: return "FAIL";
    case green_ion::bench::RESULT_INCONCLUSIVE: return "INCONCLUSIVE";
    case green_ion::bench::RESULT_DEFERRED: return "DEFERRED";
    default: return "IDLE";
  }
}

static const char *imuHardwareTestMessage() {
  const auto result = imuHardwareTest.result();
  if (result == green_ion::bench::RESULT_RUNNING)
    return "Keep the Feather still while samples are collected";
  if (result == green_ion::bench::RESULT_DEFERRED)
    return "Test stopped because flight recording became active";
  if (result == green_ion::bench::RESULT_INCONCLUSIVE)
    return "Measurements are valid but the Feather moved; repeat while stationary";
  if (result == green_ion::bench::RESULT_FAIL) {
    if (!state.imuOk) return "LSM6DSO32 not detected at 0x6A or 0x6B";
    if (!imuRange32gVerified) return "LSM6DSO32 range did not read back as +/-32 g";
    return "Too few valid IMU samples; check the STEMMA QT cable";
  }
  if (result == green_ion::bench::RESULT_PASS)
    return "Stationary acceleration and gyro bias are within test limits";
  return "Not run";
}

static const char *gpsHardwareTestMessage() {
  const auto result = gpsHardwareTest.result();
  if (result == green_ion::bench::RESULT_RUNNING)
    return "Listening for PA1010D NMEA sentences";
  if (result == green_ion::bench::RESULT_DEFERRED)
    return "Test stopped because flight recording became active";
  if (result == green_ion::bench::RESULT_INCONCLUSIVE)
    return "NMEA communication passed; move outdoors for a current fix";
  if (result == green_ion::bench::RESULT_FAIL)
    return state.gpsOk ? "No valid NMEA received; check cable and GPS power" :
                         "PA1010D not detected at I2C address 0x10";
  if (result == green_ion::bench::RESULT_PASS)
    return "NMEA communication and current GPS fix passed";
  return "Not run";
}

static bool hardwareTestsAllowed() {
  return recorderMode != RECORDER_RECORDING &&
         recorderMode != RECORDER_MAINTENANCE &&
         !otaUploadInProgress && !recorderCore.recording();
}

static bool sensorReinitializationAllowed() {
  return recorderMode != RECORDER_WATCHING && hardwareTestsAllowed();
}

static bool storageRecoveryAllowed() {
  return !state.recording && !recorderCore.recording() && !logFile && !sensorLogFile &&
         recorderMode != RECORDER_RECORDING && recorderMode != RECORDER_HOME_TEST &&
         recorderMode != RECORDER_MAINTENANCE && !otaUploadInProgress;
}

static void serviceStorageRecovery(uint32_t nowMs) {
  if (state.sdOk || !storageRecoveryAllowed() ||
      nowMs - lastSdRecoveryAttemptMs < config::SD_REINITIALIZE_DELAY_MS) return;
  lastSdRecoveryAttemptMs = nowMs;
  digitalWrite(config::PIN_LORA_CS, HIGH);
  ++state.sdMountAttempts;
  if (SD.begin(config::PIN_SD_CS, SPI)) {
    state.sdOk = true;
    ++state.sdRecoveries;
    Serial.printf("[SD] Card remounted while recorder idle; recoveries=%lu. Existing recorder fault is not auto-cleared.\n",
                  static_cast<unsigned long>(state.sdRecoveries));
  }
}

static void servicePeripheralRecovery(uint32_t nowMs) {
  if (!sensorReinitializationAllowed()) return;
  if (!state.imuOk && nowMs - lastImuRecoveryAttemptMs >= config::IMU_REINITIALIZE_DELAY_MS) {
    lastImuRecoveryAttemptMs = nowMs;
    if (configureImuHardware()) {
      state.imuOk = true;
      state.imuConsecutiveReadFailures = 0;
      state.imuDataValid = false;
      ++state.imuRecoveries;
      Serial.printf("[IMU] Reinitialized while idle; recoveries=%lu\n",
                    static_cast<unsigned long>(state.imuRecoveries));
    }
  }
  const bool gpsCommunicationLost =
      !state.gpsOk || (state.lastGpsNmeaMs &&
      nowMs - state.lastGpsNmeaMs > config::GPS_COMMUNICATION_STALE_MS * 2UL);
  if (gpsCommunicationLost &&
      nowMs - lastGpsRecoveryAttemptMs >= config::GPS_REINITIALIZE_DELAY_MS) {
    lastGpsRecoveryAttemptMs = nowMs;
    if (configureGpsHardware()) {
      state.gpsOk = true;
      ++state.gpsRecoveries;
      Serial.printf("[GPS] Reinitialized while idle; recoveries=%lu\n",
                    static_cast<unsigned long>(state.gpsRecoveries));
    } else {
      healthBits |= green_ion::HEALTH_GPS_RUNTIME_ERROR;
    }
  }
}

static void requestHardwareTest(HardwareTestRequest request) {
  if (!hardwareTestsAllowed()) {
    if (request == HARDWARE_TEST_IMU || request == HARDWARE_TEST_ALL)
      imuHardwareTest.defer();
    if (request == HARDWARE_TEST_GPS || request == HARDWARE_TEST_ALL)
      gpsHardwareTest.defer();
    return;
  }
  hardwareTestRequest = request;
}

static void startRequestedHardwareTests(uint32_t nowMs) {
  if (hardwareTestRequest == HARDWARE_TEST_NONE) return;
  const HardwareTestRequest request = hardwareTestRequest;
  hardwareTestRequest = HARDWARE_TEST_NONE;
  if (!hardwareTestsAllowed()) {
    requestHardwareTest(request);
    return;
  }

  if (request == HARDWARE_TEST_IMU || request == HARDWARE_TEST_ALL) {
    if (!state.imuOk && recorderMode == RECORDER_WATCHING)
      resetCalibration(nowMs);
    if (!state.imuOk && sensorReinitializationAllowed()) {
      state.imuOk = configureImuHardware();
      state.imuConsecutiveReadFailures = 0;
      state.imuDataValid = false;
      if (!state.imuOk) healthBits |= green_ion::HEALTH_IMU_INIT_ERROR;
    }
    imuHardwareTest.begin(nowMs, state.imuOk, imuRange32gVerified);
    if (recorderMode == RECORDER_HOME_TEST) homeImuTestStarted = true;
    imuHardwareTestReported = false;
  }

  if (request == HARDWARE_TEST_GPS || request == HARDWARE_TEST_ALL) {
    const bool communicationStale = state.lastGpsNmeaMs == 0 ||
        nowMs - state.lastGpsNmeaMs > config::GPS_COMMUNICATION_STALE_MS;
    if ((!state.gpsOk || communicationStale) &&
        recorderMode == RECORDER_WATCHING)
      resetCalibration(nowMs);
    if ((!state.gpsOk || communicationStale) && sensorReinitializationAllowed()) {
      state.gpsOk = configureGpsHardware();
      if (!state.gpsOk) healthBits |= green_ion::HEALTH_GPS_INIT_ERROR;
    }
    gpsHardwareTest.begin(nowMs, state.gpsOk, state.gpsNmeaSentences,
                          state.gpsParsedSentences);
    if (recorderMode == RECORDER_HOME_TEST) homeGpsTestStarted = true;
    gpsHardwareTestReported = false;
  }
}

static void printHardwareTestStatus(uint32_t nowMs) {
  Serial.printf(
      "TEST IMU result=%s detected=%d range_32g=%d samples=%u invalid=%u "
      "mean_g=%.3f gyro_bias_dps=%.3f,%.3f,%.3f temperature_c=%.2f message=%s\n",
      hardwareTestResultName(imuHardwareTest.result()), state.imuOk ? 1 : 0,
      imuRange32gVerified ? 1 : 0, imuHardwareTest.validSamples(),
      imuHardwareTest.invalidSamples(), imuHardwareTest.meanAccelerationG(),
      imuHardwareTest.gyroBiasXDps(), imuHardwareTest.gyroBiasYDps(),
      imuHardwareTest.gyroBiasZDps(), imuHardwareTest.meanTemperatureC(),
      imuHardwareTestMessage());
  Serial.printf(
      "TEST GPS result=%s detected=%d communication=%s nmea=%lu parsed=%lu "
      "fix=%d satellites=%u age_ms=%lu lat=%.7f lon=%.7f alt_m=%.2f "
      "speed_mps=%.2f utc=%02u:%02u:%02u message=%s\n",
      hardwareTestResultName(gpsHardwareTest.result()), state.gpsOk ? 1 : 0,
      gpsHardwareTest.communicationOk() ? "PASS" :
        (gpsHardwareTest.running() ? "TESTING" : "FAIL"),
      static_cast<unsigned long>(gpsHardwareTest.nmeaReceived()),
      static_cast<unsigned long>(gpsHardwareTest.parsedSentences()),
      gpsPositionCurrent(nowMs) ? 1 : 0, state.satellites,
      static_cast<unsigned long>(gpsAgeMs(nowMs)), state.latitude,
      state.longitude, state.gpsAltitudeM, state.gpsSpeedMps,
      state.gpsHour, state.gpsMinute, state.gpsSecond,
      gpsHardwareTestMessage());
}

static void serviceHardwareTests(uint32_t nowMs) {
  if (!hardwareTestsAllowed()) {
    if (imuHardwareTest.running()) imuHardwareTest.defer();
    if (gpsHardwareTest.running()) gpsHardwareTest.defer();
    return;
  }
  startRequestedHardwareTests(nowMs);
  gpsHardwareTest.update(nowMs, state.gpsNmeaSentences,
                         state.gpsParsedSentences, gpsPositionCurrent(nowMs),
                         state.satellites, gpsAgeMs(nowMs));
  if (!imuHardwareTest.running() && !imuHardwareTestReported) {
    imuHardwareTestReported = true;
    printHardwareTestStatus(nowMs);
  }
  if (!gpsHardwareTest.running() && !gpsHardwareTestReported) {
    gpsHardwareTestReported = true;
    printHardwareTestStatus(nowMs);
  }
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
      "total_g=%.3f gyro_rad_s=%.3f,%.3f,%.3f "
      "gyro_dps=%.2f,%.2f,%.2f temperature_c=%.2f range_verified=%d\n",
      state.accelX, state.accelY, state.accelZ,
      state.accelX * toG, state.accelY * toG, state.accelZ * toG,
      state.accelMagnitude * toG,
      state.gyroX, state.gyroY, state.gyroZ,
      state.gyroX * toDegrees, state.gyroY * toDegrees,
      state.gyroZ * toDegrees, state.imuTemperatureC,
      imuRange32gVerified ? 1 : 0);
}

static void printGpsBenchStatus(uint32_t nowMs) {
  if (!state.gpsOk) {
    Serial.println("BENCH GPS status=INIT_FAILED expected_i2c=0x10");
    return;
  }
  if (!state.gpsHasPosition) {
    Serial.printf("BENCH GPS status=NO_FIX communication=%s satellites=%u nmea=%lu parsed=%lu parse_failures=%lu\n",
                  state.lastGpsNmeaMs ? "OK" : "WAITING",
                  state.satellites,
                  static_cast<unsigned long>(state.gpsNmeaSentences),
                  static_cast<unsigned long>(state.gpsParsedSentences),
                  static_cast<unsigned long>(state.gpsParseFailures));
    return;
  }
  const uint32_t age = gpsAgeMs(nowMs);
  Serial.printf(
      "BENCH GPS status=%s fix=%d satellites=%u age_ms=%lu "
      "last_lat=%.7f last_lon=%.7f last_alt_m=%.2f speed_mps=%.2f "
      "nmea=%lu parsed=%lu utc=%02u:%02u:%02u\n",
      gpsPositionCurrent(nowMs) ? "LIVE" : "STALE",
      state.gpsFixCurrent ? 1 : 0, state.satellites,
      static_cast<unsigned long>(age), state.latitude, state.longitude,
      state.gpsAltitudeM, state.gpsSpeedMps,
      static_cast<unsigned long>(state.gpsNmeaSentences),
      static_cast<unsigned long>(state.gpsParsedSentences),
      state.gpsHour, state.gpsMinute, state.gpsSecond);
}

static void printBenchStatus(uint32_t nowMs) {
  if (benchMode == BENCH_IMU || benchMode == BENCH_ALL) printImuBenchStatus();
  if (benchMode == BENCH_GPS || benchMode == BENCH_ALL) printGpsBenchStatus(nowMs);
}

static const char *recorderModeLabel() {
  switch (recorderMode) {
    case RECORDER_CALIBRATING: return "CALIBRATING";
    case RECORDER_WATCHING: return "READY - WATCHING FOR LAUNCH";
    case RECORDER_RECORDING: return "RECORDING FLIGHT";
    case RECORDER_COMPLETE: return "FLIGHT SAVED";
    case RECORDER_FAULT: return "FAULT - CHECK HARDWARE";
    case RECORDER_MAINTENANCE: return "MAINTENANCE - LAUNCH WATCH OFF";
    case RECORDER_HOME_TEST: return "HOME VERIFICATION - LAUNCH WATCH OFF";
    default: return "UNKNOWN";
  }
}

static void printConsoleSummary(uint32_t nowMs, bool requested = false) {
  const uint8_t mode = static_cast<uint8_t>(recorderMode);
  if (mode != consoleLastMode) {
    Serial.printf("[FLIGHT] %s\n", recorderModeLabel());
    consoleLastMode = mode;
  }
  // Do not print periodic summaries while logging 50 Hz data to microSD.
  if (!requested && (recorderMode == RECORDER_RECORDING ||
                     static_cast<uint32_t>(nowMs - consoleLastStatusMs) < 5000))
    return;
  consoleLastStatusMs = nowMs;
  Serial.printf("[STATUS] BMP388:%s  IMU:%s  GPS:%s  SD:%s  LoRa:%s  Battery:%.2f V\n",
                state.barometerOk ? "OK" : "FAIL",
                state.imuOk ? "OK" : "MISSING",
                gpsPositionCurrent(nowMs) ? "FIX" :
                  (state.gpsOk ? "NO FIX" : "MISSING"),
                state.sdOk ? "OK" : "FAIL",
                state.radioOk ? "OK" : "MISSING",
                state.batteryMv / 1000.0f);
  Serial.printf("[FLIGHT] %s | Relative altitude: %.1f m | Missed periods: %lu\n",
                recorderModeLabel(),
                isfinite(watcherAltitudeM) ? watcherAltitudeM : 0.0f,
                static_cast<unsigned long>(sampleClock.missedPeriods()));
}

static void applySerialCommand(uint32_t nowMs) {
  if (!strcmp(serialCommand, "status")) { printConsoleSummary(nowMs, true); return; }
  if (!strcmp(serialCommand, "help")) {
    Serial.println("[HELP] status | test imu | test gps | test all | test status");
    Serial.println("[HELP] bench imu | bench gps | bench all | bench status | bench off");
    Serial.println("[HELP] radio status | radio test | radio retry");
    Serial.println("[HELP] Firmware updates and rollback: use System tab on the bench.");
    return;
  }
  if (!strcmp(serialCommand, "bench imu")) benchMode = BENCH_IMU;
  else if (!strcmp(serialCommand, "bench gps")) benchMode = BENCH_GPS;
  else if (!strcmp(serialCommand, "bench all")) benchMode = BENCH_ALL;
  else if (!strcmp(serialCommand, "bench off")) benchMode = BENCH_OFF;
  else if (!strcmp(serialCommand, "bench status")) printBenchStatus(nowMs);
  else if (!strcmp(serialCommand, "test imu")) {
    requestHardwareTest(HARDWARE_TEST_IMU);
    Serial.println(hardwareTestsAllowed() ? "TEST IMU requested" :
                   "TEST IMU DEFERRED recorder_critical=1");
    return;
  }
  else if (!strcmp(serialCommand, "test gps")) {
    requestHardwareTest(HARDWARE_TEST_GPS);
    Serial.println(hardwareTestsAllowed() ? "TEST GPS requested" :
                   "TEST GPS DEFERRED recorder_critical=1");
    return;
  }
  else if (!strcmp(serialCommand, "test all")) {
    requestHardwareTest(HARDWARE_TEST_ALL);
    Serial.println(hardwareTestsAllowed() ? "TEST ALL requested" :
                   "TEST ALL DEFERRED recorder_critical=1");
    return;
  }
  else if (!strcmp(serialCommand, "test status")) {
    printHardwareTestStatus(nowMs);
    return;
  }
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
    Serial.println("Commands: test imu | test gps | test all | test status | bench imu | bench gps | bench all | bench status | bench off | radio test | radio retry | radio status");
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
  if (benchMode != BENCH_OFF && recorderMode != RECORDER_RECORDING &&
      recorderMode != RECORDER_MAINTENANCE &&
      nowMs - lastBenchOutputMs >= config::BENCH_OUTPUT_INTERVAL_MS) {
    lastBenchOutputMs = nowMs;
    printBenchStatus(nowMs);
  }
}

static void resetCalibration(uint32_t nowMs) {
  if (otaUploadInProgress) return;
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

static void serviceBarometerRecovery(uint32_t nowMs) {
  const bool recorderCritical = recorderMode == RECORDER_WATCHING ||
                                recorderMode == RECORDER_RECORDING ||
                                recorderMode == RECORDER_HOME_TEST;
  if (!barometerHealth.shouldAttempt(nowMs, recorderCritical)) return;
  const bool initialized = configureBarometerHardware();
  barometerHealth.noteInitialize(nowMs, initialized);
  state.barometerOk = initialized;
  if (!initialized) healthBits |= green_ion::HEALTH_BARO_INIT_ERROR;
  Serial.printf("BENCH BARO status=%s recoveries=%lu init_failures=%lu\n",
                initialized ? "RECOVERED" : "RETRY_FAILED",
                static_cast<unsigned long>(barometerHealth.recoveries()),
                static_cast<unsigned long>(barometerHealth.initFailures()));
  if (initialized) resetCalibration(millis());
}

static void serviceRadioRecovery(uint32_t nowMs) {
  if (recorderMode == RECORDER_MAINTENANCE) return;
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
  if (watching) calibratedAtMs = nowMs;
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
    sensorLogCommittedBytes,
    sensorLogCommittedRecords,
    sensorLogCheckpointable,
    0,
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
  bool saved = metadataFile &&
                     metadataFile.write(reinterpret_cast<const uint8_t *>(encoded),
                                        encodedBytes) == encodedBytes;
  // Metadata extension; source RKT4/SEN bytes remain exactly unchanged.
  if (saved) {
    saved = metadataFile.print("rocket=" + selectedRocket + "\n") > 0 &&
            metadataFile.print("motor=" + selectedMotor + "\n") > 0 &&
            metadataFile.print("firmware_name=" + String(FIRMWARE_NAME) + "\n") > 0 &&
            metadataFile.print("firmware_version=" + String(FIRMWARE_VERSION) + "\n") > 0 &&
            metadataFile.print("boot_session_id=" + String(bootSessionId) + "\n") > 0 &&
            metadataFile.print("reset_reason=" + String(resetReasonName(bootResetReason)) + "\n") > 0;
  }
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
    const bool occupied = green_ion::storage::flightSequenceOccupied(
      sequence, [](const char *path) { return SD.exists(path); });
    if (!occupied) {
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
  sensorLogCommittedBytes = 0;
  sensorLogCommittedRecords = 0;
  sensorLogCheckpointable = false;
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
      sensorLogCommittedBytes = static_cast<uint32_t>(sensorLogFile.size());
      sensorLogCommittedRecords = 0;
      sensorLogCheckpointable = true;
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
    } else {
      sensorLogCommittedBytes = static_cast<uint32_t>(sensorLogFile.size());
      sensorLogCommittedRecords = static_cast<uint32_t>(
          green_ion::sensor_log::completeRecordCount(sensorLogCommittedBytes));
    }
  }
}

static void appendLog(uint32_t nowUs, uint32_t nowMs, bool valid) {
  if (recorderMode == RECORDER_COMPLETE ||
      recorderMode == RECORDER_MAINTENANCE ||
      recorderMode == RECORDER_HOME_TEST) return;
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

static bool validFlightProfileValue(const String &value, bool optional = false) {
  if (value.length() > 24 || (!optional && value.isEmpty())) return false;
  for (unsigned int i = 0; i < value.length(); ++i) {
    const char ch = value[i];
    const bool allowed = (ch >= 'A' && ch <= 'Z') ||
        (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') ||
        ch == ' ' || ch == '-' || ch == '_' || ch == '+' || ch == '.' || ch == '/';
    if (!allowed) return false;
  }
  return true;
}

static void loadFlightProfile() {
  Preferences preferences;
  if (preferences.begin("gioflight", true)) {
    const String rocket = preferences.getString("rocket", "");
    const String motor = preferences.getString("motor", "");
    if (validFlightProfileValue(rocket)) selectedRocket = rocket;
    if (validFlightProfileValue(motor, true)) selectedMotor = motor;
    preferences.end();
  }
}

static void saveFlightProfile() {
  if (state.recording || recorderCore.recording() || logFile || sensorLogFile ||
      recorderMode == RECORDER_RECORDING || recorderMode == RECORDER_MAINTENANCE ||
      recorderMode == RECORDER_HOME_TEST || otaUploadInProgress) {
    dashboardServer.send(409, "application/json",
      "{\"success\":false,\"message\":\"Cannot edit a profile during recording, a home test or firmware maintenance\"}");
    return;
  }
  const String rocket = dashboardServer.arg("rocket");
  const String motor = dashboardServer.arg("motor");
  if (!validFlightProfileValue(rocket) || !validFlightProfileValue(motor, true)) {
    dashboardServer.send(400, "application/json",
      "{\"success\":false,\"message\":\"Use 1-24 safe characters for rocket and 0-24 for motor\"}");
    return;
  }
  // Cancel watcher before flash/NVS work. Resume only after stationary recalibration.
  flightWatcher.cancel();
  recorderMode = RECORDER_CALIBRATING;
  Preferences preferences;
  if (!preferences.begin("gioflight", false)) {
    resetCalibration(millis());
    dashboardServer.send(500, "application/json",
      "{\"success\":false,\"message\":\"Could not open persistent flight profile storage\"}");
    return;
  }
  const bool storedRocket = preferences.putString("rocket", rocket) == rocket.length();
  const bool storedMotor = preferences.putString("motor", motor) == motor.length();
  const bool stored = storedRocket && storedMotor;
  preferences.end();
  if (stored) { selectedRocket = rocket; selectedMotor = motor; }
  resetCalibration(millis());
  dashboardServer.send(stored ? 200 : 500, "application/json",
    stored ? "{\"success\":true,\"message\":\"Profile saved. Keep the Feather stationary during recalibration\"}" :
             "{\"success\":false,\"message\":\"Profile storage failed; recalibration restarted\"}");
}

static green_ion::preflight::Result currentPreflight() {
  const bool calibrated = isfinite(calibrationBaselinePa) && calibrationBaselinePa > 30000.0f &&
                          isfinite(calibrationNoiseM) && calibrationNoiseM >= 0.0f;
  const bool monitoring = recorderMode == RECORDER_WATCHING;
  const bool disabled = recorderMode == RECORDER_MAINTENANCE ||
                        recorderMode == RECORDER_HOME_TEST;
  const green_ion::preflight::Input input = {
    state.sdOk, state.barometerOk, calibrated, monitoring, disabled,
    recoveryPending > 0, orphanHomeFile || homeInterrupted || !homeCleanupOk,
    state.imuOk && imuRange32gVerified, gpsPositionCurrent(millis()),
    state.radioOk, sampleClock.missedPeriods() != 0,
    sensorLogFailures || checkpointFailures || metadataFailures || finalizationFailures,
    calibratedAtMs == 0 || millis() - calibratedAtMs > 1800000UL,
    validFlightProfileValue(selectedRocket), state.batteryMv
  };
  return green_ion::preflight::evaluate(input);
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
  body.reserve(3400);
  body += "{\"version\":16,\"firmware_name\":\"" + String(FIRMWARE_NAME) + "\"";
  body += ",\"firmware_version\":\"" + String(FIRMWARE_VERSION) + "\"";
  body += ",\"radio_protocol\":" + String(green_ion::kProtocolVersion);
  body += ",\"boot_session_id\":" + String(bootSessionId);
  body += ",\"reset_reason\":\"" + String(resetReasonName(bootResetReason)) + "\"";
  body += ",\"reset_reason_code\":" + String(static_cast<uint8_t>(bootResetReason));
  body += ",\"recording\":";
  body += state.recording ? "true" : "false";
  body += ",\"uptime_s\":" + String(millis() / 1000UL);
  body += ",\"flight_state\":" + String(static_cast<uint8_t>(flightState));
  body += ",\"sd_ok\":";
  body += state.sdOk ? "true" : "false";
  body += ",\"imu_ok\":";
  body += state.imuOk ? "true" : "false";
  body += ",\"gps_ok\":";
  body += state.gpsOk ? "true" : "false";
  body += ",\"radio_ok\":";
  body += state.radioOk ? "true" : "false";
  body += ",\"log_file\":\"" + logFilename + "\",\"pressure_pa\":";
  body += numberOrNull(state.pressurePa, 0);
  body += ",\"altitude_m\":" + numberOrNull(state.altitudeM, 2);
  body += ",\"temperature_c\":" + numberOrNull(state.temperatureC, 2);
  body += ",\"acceleration_g\":" + numberOrNull(state.accelMagnitude / 9.80665f, 3);
  body += ",\"acceleration_x_g\":" + numberOrNull(state.accelX / 9.80665f, 3);
  body += ",\"acceleration_y_g\":" + numberOrNull(state.accelY / 9.80665f, 3);
  body += ",\"acceleration_z_g\":" + numberOrNull(state.accelZ / 9.80665f, 3);
  body += ",\"rotation_x_dps\":" + numberOrNull(state.gyroX * radiansToDegrees, 1);
  body += ",\"rotation_y_dps\":" + numberOrNull(state.gyroY * radiansToDegrees, 1);
  body += ",\"rotation_z_dps\":" + numberOrNull(state.gyroZ * radiansToDegrees, 1);
  body += ",\"imu_temperature_c\":" + numberOrNull(state.imuTemperatureC, 2);
  body += ",\"imu_data_valid\":";
  body += state.imuDataValid ? "true" : "false";
  body += ",\"imu_read_failures\":" + String(state.imuReadFailures);
  body += ",\"imu_consecutive_read_failures\":" + String(state.imuConsecutiveReadFailures);
  body += ",\"battery_v\":" + String(state.batteryMv / 1000.0f, 3);
  body += ",\"battery_mv\":" + String(state.batteryMv);
  body += ",\"gps_status\":\"" + String(gpsStatus) + "\",\"satellites\":";
  body += String(state.satellites);
  body += ",\"gps_communication\":\"";
  body += state.lastGpsNmeaMs ? "ok" : "waiting";
  body += "\",\"gps_nmea_sentences\":" + String(state.gpsNmeaSentences);
  body += ",\"gps_parsed_sentences\":" + String(state.gpsParsedSentences);
  body += ",\"gps_parse_failures\":" + String(state.gpsParseFailures);
  body += ",\"gps_nmea_age_ms\":";
  body += state.lastGpsNmeaMs ? String(millis() - state.lastGpsNmeaMs) : "null";
  body += ",\"gps_fix_age_ms\":" + String(age);
  body += ",\"latitude\":";
  body += state.gpsHasPosition ? String(state.latitude, 7) : String("null");
  body += ",\"longitude\":";
  body += state.gpsHasPosition ? String(state.longitude, 7) : String("null");
  body += ",\"gps_altitude_m\":" + numberOrNull(state.gpsAltitudeM, 2);
  body += ",\"gps_speed_mps\":" + numberOrNull(state.gpsSpeedMps, 2);
  body += ",\"gps_course_deg\":" + numberOrNull(state.gpsCourseDeg, 1);
  body += ",\"gps_hdop\":" + numberOrNull(state.gpsHdop, 2);
  body += ",\"gps_geoid_separation_m\":" + numberOrNull(state.gpsGeoidSeparationM, 2);
  body += ",\"gps_fix_quality\":" + String(state.gpsFixQuality);
  body += ",\"gps_utc_hour\":" + String(state.gpsHour);
  body += ",\"gps_utc_minute\":" + String(state.gpsMinute);
  body += ",\"gps_utc_second\":" + String(state.gpsSecond);
  body += ",\"hardware_tests_allowed\":";
  body += hardwareTestsAllowed() ? "true" : "false";
  body += ",\"imu_test_result\":\"" + String(hardwareTestResultName(imuHardwareTest.result())) + "\"";
  body += ",\"imu_test_message\":\"" + String(imuHardwareTestMessage()) + "\"";
  body += ",\"imu_range_32g\":";
  body += imuRange32gVerified ? "true" : "false";
  body += ",\"imu_test_samples\":" + String(imuHardwareTest.validSamples());
  body += ",\"imu_test_invalid\":" + String(imuHardwareTest.invalidSamples());
  body += ",\"imu_test_mean_g\":" + numberOrNull(imuHardwareTest.meanAccelerationG(), 3);
  body += ",\"imu_test_bias_x_dps\":" + numberOrNull(imuHardwareTest.gyroBiasXDps(), 3);
  body += ",\"imu_test_bias_y_dps\":" + numberOrNull(imuHardwareTest.gyroBiasYDps(), 3);
  body += ",\"imu_test_bias_z_dps\":" + numberOrNull(imuHardwareTest.gyroBiasZDps(), 3);
  body += ",\"imu_test_temperature_c\":" + numberOrNull(imuHardwareTest.meanTemperatureC(), 2);
  body += ",\"gps_test_result\":\"" + String(hardwareTestResultName(gpsHardwareTest.result())) + "\"";
  body += ",\"gps_test_message\":\"" + String(gpsHardwareTestMessage()) + "\"";
  body += ",\"gps_test_communication\":";
  body += gpsHardwareTest.communicationOk() ? "true" : "false";
  body += ",\"gps_test_nmea\":" + String(gpsHardwareTest.nmeaReceived());
  body += ",\"gps_test_parsed\":" + String(gpsHardwareTest.parsedSentences());
  body += ",\"health\":" + String(healthBits);
  body += ",\"barometer_ready\":";
  body += state.barometerOk ? "true" : "false";
  body += ",\"barometer_runtime_losses\":" + String(barometerHealth.runtimeLosses());
  body += ",\"barometer_recoveries\":" + String(barometerHealth.recoveries());
  body += ",\"barometer_init_failures\":" + String(barometerHealth.initFailures());
  body += ",\"prebuffer_records\":" + String(recorderCore.bufferedRecords());
  body += ",\"recorder_status\":" + String(static_cast<uint8_t>(recorderCore.status()));
  body += ",\"recorder_mode\":" + String(static_cast<uint8_t>(recorderMode));
  body += ",\"calibration_baseline_pa\":" + numberOrNull(calibrationBaselinePa, 1);
  body += ",\"calibration_noise_m\":" + numberOrNull(calibrationNoiseM, 3);
  body += ",\"relative_altitude_m\":" + numberOrNull(watcherAltitudeM, 2);
  body += ",\"vertical_speed_mps\":" + numberOrNull(watcherSpeedMps, 2);
  body += ",\"checkpoint_generation\":" + String(checkpointGeneration);
  body += ",\"checkpoint_failures\":" + String(checkpointFailures);
  body += ",\"metadata_file\":\"" + metadataFilename + "\"";
  body += ",\"metadata_failures\":" + String(metadataFailures);
  body += ",\"sensor_log_file\":\"" + sensorLogFilename + "\"";
  body += ",\"sensor_log_failures\":" + String(sensorLogFailures);
  body += ",\"sensor_log_pending\":" + String(sensorLogCore.queuedRecords());
  body += ",\"sensor_log_dropped\":" + String(sensorLogCore.droppedRecords());
  body += ",\"sensor_log_status\":" + String(static_cast<uint8_t>(sensorLogCore.status()));
  body += ",\"telemetry_queue_depth\":" + String(telemetryQueue.size());
  body += ",\"telemetry_queue_dropped\":" + String(telemetryQueue.dropped());
  body += ",\"telemetry_sequence\":" + String(telemetrySequence);
  body += ",\"sd_mount_attempts\":" + String(state.sdMountAttempts);
  body += ",\"sd_recoveries\":" + String(state.sdRecoveries);
  body += ",\"imu_recoveries\":" + String(state.imuRecoveries);
  body += ",\"gps_recoveries\":" + String(state.gpsRecoveries);
  body += ",\"barometer_read_age_ms\":";
  body += state.lastBarometerReadMs ? String(millis() - state.lastBarometerReadMs) : "null";
  body += ",\"imu_read_age_ms\":";
  body += state.lastImuReadMs ? String(millis() - state.lastImuReadMs) : "null";
  body += ",\"battery_read_age_ms\":";
  body += state.lastBatteryReadMs ? String(millis() - state.lastBatteryReadMs) : "null";
  body += ",\"finalization_failures\":" + String(finalizationFailures);
  body += ",\"landing_peak_m\":" + numberOrNull(landingWatcher.peakAltitudeM(), 2);
  body += ",\"missed_sample_periods\":" + String(sampleClock.missedPeriods());
  body += ",\"recovered_logs\":" + String(recoveredLogs);
  body += ",\"recovered_records\":" + String(recoveredRecords);
  body += ",\"recovered_trailing_bytes\":" + String(recoveredTrailingBytes);
  body += ",\"recovered_sensor_records\":" + String(recoveredSensorRecords);
  body += ",\"sensor_recovery_issues\":" + String(sensorRecoveryIssues);
  body += ",\"recovery_pending\":" + String(recoveryPending);
  body += ",\"recovery_status\":\"" + recoveryStatus + "\"";
  const auto preflight = currentPreflight();
  body += ",\"preflight_state\":\"" + String(preflight.label) + "\"";
  body += ",\"preflight_flags\":" + String(preflight.flags);
  body += ",\"selected_rocket\":\"" + selectedRocket + "\"";
  body += ",\"selected_motor\":\"" + selectedMotor + "\"";
  body += ",\"calibration_age_s\":" +
      String(calibratedAtMs ? (millis() - calibratedAtMs) / 1000UL : 0UL);
  body += ",\"orphan_home_file\":";
  body += orphanHomeFile ? "true" : "false";
  body += ",\"sample_rate_hz\":" + String(config::SENSOR_RATE_HZ);
  body += ",\"firmware_build\":\"" + String(FIRMWARE_VERSION) + " " + String(__DATE__) + " " + String(__TIME__) + "\"";
  body += ",\"maintenance_mode\":";
  body += recorderMode == RECORDER_MAINTENANCE ? "true" : "false";
  body += ",\"update_in_progress\":";
  body += otaUploadInProgress ? "true" : "false";
  body += ",\"ota_available\":";
  body += esp_ota_get_next_update_partition(esp_ota_get_running_partition()) ? "true" : "false";
  body += ",\"home_interrupted\":";
  body += homeInterrupted ? "true" : "false";
  body += ",\"home_steps\":\"";
  for (uint8_t i = 0; i < 9; ++i) body += String(homeStepStatuses[i]);
  body += "\"";
  body += ",\"home_test_active\":";
  body += recorderMode == RECORDER_HOME_TEST ? "true" : "false";
  body += ",\"home_record_result\":\"" +
    String(homeResult == 1 ? "RUNNING" : homeResult == 2 ? "PASS" :
           homeResult == 3 ? "FAIL" : "IDLE") + "\"";
  body += ",\"home_record_samples\":" + String(homeSamples);
  body += ",\"home_record_valid_samples\":" + String(homeValidSamples);
  body += ",\"home_record_missed\":" + String(homeMissed);
  body += ",\"home_record_elapsed_s\":" +
          String(homeLogActive ? (millis() - homeStartedMs) / 1000UL : 0);
  body += ",\"home_record_wifi_absent\":";
  body += homeWifiAbsent ? "true" : "false";
  body += ",\"home_record_cleanup_ok\":";
  body += homeCleanupOk ? "true" : "false";
  body += ",\"home_radio_sent\":" + String(radioBenchPacketsSent - homeRadioStartSent);
  body += ",\"home_radio_remaining\":" + String(radioBenchPacketsRemaining);
  body += ",\"home_radio_failures\":" + String(radioSendFailures - homeRadioStartFailures);
  body += ",\"radio_send_failures\":" + String(radioSendFailures);
  body += ",\"radio_bench_packets_sent\":" + String(radioBenchPacketsSent);
  body += "}";
  dashboardServer.send(200, "application/json", body);
}

static bool dashboardFlightFilename(const String &name, const char *suffix) {
  if (name.length() != 11 || !name.startsWith("V12D") || !name.endsWith(suffix))
    return false;
  for (uint8_t index = 4; index < 7; ++index)
    if (!isDigit(name[index])) return false;
  return true;
}

static bool dashboardSdDeleteBlocked() {
  return state.recording || recorderCore.recording() || logFile || sensorLogFile ||
         recorderMode == RECORDER_RECORDING ||
         recorderMode == RECORDER_HOME_TEST ||
         recorderMode == RECORDER_MAINTENANCE ||
         homeLogActive || otaUploadInProgress;
}

static void deleteDashboardFlight() {
  if (dashboardSdDeleteBlocked()) {
    dashboardServer.send(409, "application/json",
      "{\"success\":false,\"message\":\"SD deletion is blocked during recording, home testing or firmware maintenance\"}");
    return;
  }
  if (!state.sdOk) {
    dashboardServer.send(503, "application/json",
      "{\"success\":false,\"message\":\"SD card is unavailable\"}");
    return;
  }

  const String name = dashboardServer.arg("name");
  const String confirmation = dashboardServer.arg("confirm");
  if (!dashboardFlightFilename(name, ".BIN")) {
    dashboardServer.send(400, "application/json",
      "{\"success\":false,\"message\":\"Only V12D###.BIN flight packages can be deleted\"}");
    return;
  }
  if (confirmation != name) {
    dashboardServer.send(400, "application/json",
      "{\"success\":false,\"message\":\"Deletion confirmation did not match the flight filename\"}");
    return;
  }

  const String binPath = "/" + name;
  digitalWrite(config::PIN_LORA_CS, HIGH);
  if (!SD.exists(binPath)) {
    dashboardServer.send(404, "application/json",
      "{\"success\":false,\"message\":\"Flight file not found on the SD card\"}");
    return;
  }

  // Never mutate the SD while launch watching is active. A deletion request
  // pauses the watcher and restarts stationary calibration after the package
  // operation completes. This keeps primary recording higher priority.
  const bool restartCalibration =
      recorderMode == RECORDER_WATCHING || recorderMode == RECORDER_CALIBRATING;
  if (restartCalibration) flightWatcher.cancel();

  const String base = name.substring(0, 7);
  static const char *companions[] = {".SEN", ".TXT", ".JRA", ".JRB", ".RCV"};
  uint8_t removed = 0;
  String failed = "";

  // Delete companions/recovery artifacts first and the immutable primary BIN
  // last. If any companion cannot be removed, retain the BIN for recovery.
  for (const char *extension : companions) {
    const String path = String("/") + base + extension;
    if (!SD.exists(path)) continue;
    if (!SD.remove(path)) {
      failed = path;
      break;
    }
    ++removed;
  }

  if (failed.isEmpty()) {
    if (!SD.remove(binPath)) failed = binPath;
    else ++removed;
  }

  bool verified = failed.isEmpty();
  if (verified) {
    verified = !SD.exists(binPath);
    for (const char *extension : companions) {
      if (SD.exists(String("/") + base + extension)) verified = false;
    }
  }

  if (restartCalibration) resetCalibration(millis());

  if (!failed.isEmpty() || !verified) {
    String body = "{\"success\":false,\"message\":\"Delete did not fully complete; primary flight data was retained when possible\",\"failed_path\":\"";
    body += failed;
    body += "\",\"verified\":false}";
    dashboardServer.send(500, "application/json", body);
    return;
  }

  String body = "{\"success\":true,\"message\":\"Deleted flight package from Feather SD only";
  if (restartCalibration) body += "; stationary calibration restarted";
  body += "\",\"name\":\"" + name + "\",\"deleted_files\":" + String(removed);
  body += ",\"verified\":true,\"used\":" + String(SD.usedBytes());
  body += ",\"total\":" + String(SD.totalBytes()) + "}";
  dashboardServer.send(200, "application/json", body);
}

static void sendDashboardFiles() {
  if (dashboardSdDeleteBlocked()) {
    dashboardServer.send(409, "application/json",
                         "{\"success\":false,\"message\":\"SD browsing is deferred during recording, home testing or firmware maintenance\"}");
    return;
  }
  if (!state.sdOk) {
    dashboardServer.send(503, "application/json",
                         "{\"success\":false,\"message\":\"SD card is unavailable\"}");
    return;
  }
  String body = "{\"success\":true,\"used\":" + String(SD.usedBytes()) +
                ",\"total\":" + String(SD.totalBytes()) + ",\"files\":[";
  bool first = true;
  digitalWrite(config::PIN_LORA_CS, HIGH);
  File root = SD.open("/");
  if (root) {
    File entry = root.openNextFile();
    while (entry) {
      String name = entry.name();
      if (name.startsWith("/")) name.remove(0, 1);
      if (!entry.isDirectory() && dashboardFlightFilename(name, ".BIN")) {
        if (!first) body += ',';
        first = false;
        body += "{\"name\":\"" + name + "\",\"size\":" + String(entry.size()) + "}";
      }
      entry.close();
      entry = root.openNextFile();
    }
    root.close();
  }
  body += "]}";
  dashboardServer.send(200, "application/json", body);
}

static void sendDashboardDownload(bool metadata, bool sensors = false) {
  if (dashboardSdDeleteBlocked()) {
    dashboardServer.send(409, "application/json",
                         "{\"success\":false,\"message\":\"Downloads are deferred during recording, home testing or firmware maintenance\"}");
    return;
  }
  String name = dashboardServer.arg("name");
  if (metadata && dashboardFlightFilename(name, ".BIN"))
    name.replace(".BIN", ".TXT");
  if (sensors && dashboardFlightFilename(name, ".BIN"))
    name.replace(".BIN", ".SEN");
  const char *suffix = metadata ? ".TXT" : (sensors ? ".SEN" : ".BIN");
  if (!dashboardFlightFilename(name, suffix)) {
    dashboardServer.send(400, "application/json",
                         "{\"success\":false,\"message\":\"Invalid V12 flight filename\"}");
    return;
  }
  const String path = "/" + name;
  digitalWrite(config::PIN_LORA_CS, HIGH);
  File file = SD.open(path, FILE_READ);
  if (!file) {
    dashboardServer.send(404, "application/json",
                         "{\"success\":false,\"message\":\"Flight file not found\"}");
    return;
  }
  dashboardServer.sendHeader("Content-Disposition", "attachment; filename=\"" + name + "\"");
  dashboardServer.streamFile(file, metadata ? "text/plain" : "application/octet-stream");
  file.close();
}

static void runDashboardSdTest() {
  if (state.recording || recorderMode == RECORDER_RECORDING ||
      recorderMode == RECORDER_MAINTENANCE || homeLogActive) {
    dashboardServer.send(409, "application/json",
                         "{\"success\":false,\"message\":\"SD test is disabled while recording\"}");
    return;
  }
  if (!state.sdOk) {
    dashboardServer.send(503, "application/json",
                         "{\"success\":false,\"message\":\"SD card is unavailable\"}");
    return;
  }
  static const char path[] = "/V12SD.TMP";
  static const uint8_t pattern[] = {0x47, 0x49, 0x4f, 0x4e, 0x12, 0xa5};
  digitalWrite(config::PIN_LORA_CS, HIGH);
  SD.remove(path);
  File file = SD.open(path, FILE_WRITE);
  bool ok = file && file.write(pattern, sizeof(pattern)) == sizeof(pattern);
  if (file) { file.flush(); file.close(); }
  uint8_t readback[sizeof(pattern)] = {};
  file = SD.open(path, FILE_READ);
  ok = ok && file && file.read(readback, sizeof(readback)) == sizeof(readback) &&
       memcmp(pattern, readback, sizeof(pattern)) == 0;
  if (file) file.close();
  ok = ok && SD.remove(path);
  if (recorderMode == RECORDER_HOME_TEST) homeSdVerified = ok;
  dashboardServer.send(ok ? 200 : 500, "application/json",
      ok ? "{\"success\":true,\"message\":\"SD write, read, verify and cleanup passed\"}"
         : "{\"success\":false,\"message\":\"SD write/read verification failed\"}");
}

#include "HomeVerification.h"
#include "FirmwareMaintenance.h"

static void initializeDashboard() {
  // Only the production GREEN ION dashboard may read/control the Feather from
  // another web origin. Never enable wildcard CORS: telemetry contains GPS,
  // and some API endpoints can change recorder state.
  // This is browser-origin defense, NOT authentication on the Feather AP.
  const char *requestHeaders[] = {"Origin", "Access-Control-Request-Private-Network"};
  dashboardServer.collectHeaders(requestHeaders, 2);
  dashboardServer.addMiddleware([](WebServer &server, Middleware::Callback next) -> bool {
    const String origin = server.header("Origin");
    // Preserve same-origin Feather dashboard, as well as native bridge calls.
    if (origin.isEmpty() || origin == "http://192.168.4.1") return next();
    if (origin != "https://green-ion-beta.vercel.app") {
      server.send(403, "application/json",
                  "{\"success\":false,\"message\":\"Untrusted website origin\"}");
      return true;
    }
    server.sendHeader("Access-Control-Allow-Origin", "https://green-ion-beta.vercel.app");
    server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
    server.sendHeader("Access-Control-Allow-Private-Network", "true");
    server.sendHeader("Vary", "Origin");
    if (server.method() == HTTP_OPTIONS) {
      server.send(204, "text/plain", "");
      return true;
    }
    return next();
  });
  WiFi.mode(WIFI_AP);
  WiFi.softAP(config::DASHBOARD_SSID, config::DASHBOARD_PASSWORD);
  dashboardServer.on("/", []() {
    dashboardServer.send_P(200, "text/html", DASHBOARD_HTML);
  });
  dashboardServer.on("/manifest.webmanifest", []() {
    dashboardServer.send(200, "application/manifest+json",
        "{\"name\":\"GREEN ION V14\",\"short_name\":\"GREEN ION\","
        "\"start_url\":\"/\",\"display\":\"standalone\","
        "\"background_color\":\"#081420\",\"theme_color\":\"#081420\"}");
  });
  dashboardServer.on("/service-worker.js", []() {
    dashboardServer.sendHeader("Service-Worker-Allowed", "/");
    dashboardServer.send(200, "application/javascript",
        "const C='green-ion-v13-dashboard-v1';"
        "self.addEventListener('install',e=>e.waitUntil(caches.open(C).then(c=>c.add('/'))));"
        "self.addEventListener('activate',e=>e.waitUntil(self.clients.claim()));"
        "self.addEventListener('fetch',e=>{if(e.request.method!=='GET'||new URL(e.request.url).pathname.startsWith('/api/'))return;"
        "e.respondWith(fetch(e.request).then(r=>{const x=r.clone();caches.open(C).then(c=>c.put(e.request,x));return r;})"
        ".catch(()=>caches.match(e.request).then(r=>r||caches.match('/'))));});");
  });
  dashboardServer.on("/api/status", sendDashboardStatus);
  dashboardServer.on("/api/profile", HTTP_POST, saveFlightProfile);
  dashboardServer.on("/api/maintenance", HTTP_POST, startFirmwareMaintenance);
  dashboardServer.on("/api/maintenance/exit", HTTP_POST, exitFirmwareMaintenance);
  dashboardServer.on("/api/update", HTTP_POST, finishFirmwareUpload, handleFirmwareUpload);
  dashboardServer.on("/api/rollback", HTTP_POST, rollbackFirmware);
  dashboardServer.on("/api/files", sendDashboardFiles);
  dashboardServer.on("/api/delete", HTTP_POST, deleteDashboardFlight);
  dashboardServer.on("/api/download", []() { sendDashboardDownload(false); });
  dashboardServer.on("/api/metadata", []() { sendDashboardDownload(true); });
  dashboardServer.on("/api/flightstats", []() { sendDashboardDownload(true); });
  dashboardServer.on("/api/sensors", []() { sendDashboardDownload(false, true); });
  dashboardServer.on("/api/sdtest", HTTP_POST, runDashboardSdTest);
  dashboardServer.on("/api/home/start", HTTP_POST, homeStart);
  dashboardServer.on("/api/home/record", HTTP_POST, homeStartLog);
  dashboardServer.on("/api/home/radio", HTTP_POST, homeRadio);
  dashboardServer.on("/api/home/finish", HTTP_POST, homeFinish);
  dashboardServer.on("/api/home/step", HTTP_POST, homeStoreStepResult);
  dashboardServer.on("/api/calibrate", HTTP_POST, []() {
    if (state.recording || recorderMode == RECORDER_RECORDING ||
        recorderMode == RECORDER_MAINTENANCE ||
        recorderMode == RECORDER_HOME_TEST || otaUploadInProgress) {
      dashboardServer.send(409, "application/json",
                           "{\"success\":false,\"message\":\"Calibration blocked during flight or firmware maintenance\"}");
      return;
    }
    resetCalibration(millis());
    dashboardServer.send(202, "application/json",
                         "{\"success\":true,\"message\":\"Stationary pressure calibration restarted\"}");
  });
  dashboardServer.on("/api/stop", HTTP_POST, []() {
    if (!state.recording || recorderMode != RECORDER_RECORDING) {
      dashboardServer.send(409, "application/json",
                           "{\"success\":false,\"message\":\"No active recording\"}");
      return;
    }
    const bool saved = finalizeRecording(millis());
    dashboardServer.send(saved ? 200 : 500, "application/json",
        saved ? "{\"success\":true,\"message\":\"Recording stopped, flushed and finalized\"}"
              : "{\"success\":false,\"message\":\"Recorder finalization failed; recovery evidence was retained\"}");
  });
  dashboardServer.on("/api/hardware-test", HTTP_POST, []() {
    if (!dashboardServer.hasArg("sensor")) {
      dashboardServer.send(400, "application/json",
                           "{\"accepted\":false,\"error\":\"missing sensor\"}");
      return;
    }
    HardwareTestRequest request = HARDWARE_TEST_NONE;
    const String sensor = dashboardServer.arg("sensor");
    if (sensor == "imu") request = HARDWARE_TEST_IMU;
    else if (sensor == "gps") request = HARDWARE_TEST_GPS;
    else if (sensor == "all") request = HARDWARE_TEST_ALL;
    else {
      dashboardServer.send(400, "application/json",
                           "{\"accepted\":false,\"error\":\"unknown sensor\"}");
      return;
    }
    if (!hardwareTestsAllowed()) {
      requestHardwareTest(request);
      dashboardServer.send(409, "application/json",
                           "{\"accepted\":false,\"error\":\"flight recorder active; test deferred\"}");
      return;
    }
    requestHardwareTest(request);
    dashboardServer.send(202, "application/json", "{\"accepted\":true}");
  });
  dashboardServer.begin();
}

static void queueTelemetry(uint32_t nowMs) {
  if (recorderMode == RECORDER_MAINTENANCE) return;
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
  if (recorderMode == RECORDER_MAINTENANCE) return;
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
  bootSessionId = esp_random();
  bootResetReason = esp_reset_reason();
  pinMode(config::PIN_SD_CS, OUTPUT);
  pinMode(config::PIN_LORA_CS, OUTPUT);
  pinMode(config::PIN_BUZZER, OUTPUT);
  digitalWrite(config::PIN_BUZZER, LOW);
  selectNoSpiDevice();
  initializeI2c();
  SPI.begin(config::PIN_SPI_SCK, config::PIN_SPI_MISO, config::PIN_SPI_MOSI);
  initializeSensors();
  loadFlightProfile();
  initializeStorage();
  homeLoadProgress();
  recoverInterruptedLogs();
  resetCalibration(millis());
  initializeRadio();
  initializeDashboard();
  Serial.println();
  Serial.println("==================================================");
  Serial.println(" GREEN ION Feather V16.2 | rocket flight recorder");
  Serial.println("==================================================");
  Serial.printf(" Firmware: %s | session=%lu | reset=%s\n", FIRMWARE_VERSION,
                static_cast<unsigned long>(bootSessionId), resetReasonName(bootResetReason));
  Serial.printf(" Wi-Fi: %s | http://192.168.4.1\n", config::DASHBOARD_SSID);
  Serial.println(" Mode: automatic calibration and launch watching");
  Serial.println(" Type 'help' for commands or 'status' for health.");
  Serial.println(" LoRa requires a suitable antenna before transmitting.");
  Serial.println(" OTA / rollback: use System tab, on the bench only.");
  printConsoleSummary(millis(), true);
  sampleClock.start(micros());
}

void loop() {
  const uint32_t nowMs = millis();
  const uint32_t nowUs = micros();

  if (sampleClock.due(nowUs)) {
    const bool barometerReadingValid = readSensors(nowMs);
    appendLog(nowUs, nowMs, barometerReadingValid);
    homeCapture(nowUs, barometerReadingValid);
  }
  serviceGps(nowMs);
  serviceHardwareTests(nowMs);
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
  if (sensorLogCore.droppedRecords()) healthBits |= green_ion::HEALTH_SENSOR_LOG_DROPPED;

  serviceStorageRecovery(nowMs);
  servicePeripheralRecovery(nowMs);
  serviceBarometerRecovery(nowMs);
  serviceRadioRecovery(nowMs);
  queueTelemetry(nowMs);
  serviceTelemetry(nowMs, micros());
  printConsoleSummary(nowMs);
  dashboardServer.handleClient();
  yield();
}

