
/*
  GREEN ION V11 — FIELD-RESILIENT FLIGHT RECORDER
  ESP32 Feather V2 + BMP388 flight logger

  Wi-Fi: GREEN-ION
  Password: GreenIon2026
  Browser: http://192.168.4.1

  NEW:
    Unlimited sequential flight files (up to 9999 per card)
    Flight label and motor designation metadata
    Phone downloads in original RKT4 BIN or analysis-ready CSV
    Postflight summary and data-quality statistics
    Robust stationary calibration with outlier and drift rejection
    Separate raw-log, dashboard, and flight-detection altitude paths
    Dashboard-selectable 10, 25, or 50 sample/second logging
    Safe dashboard upload of GitHub-built OTA firmware from a phone
    Multiple view-only phones with one 30-second renewable controller lease

  True hardware off:
    SPST switch between EN and GND.
    CLOSED = regulator off
    OPEN   = regulator on

  Experimental data logger only.
  Not for ignition/recovery control.
*/

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_BMP3XX.h>
#include <Adafruit_NeoPixel.h>
#include <esp_sleep.h>
#include <esp_system.h>
#include <Update.h>
#include <Preferences.h>
#include <esp_ota_ops.h>
#include <math.h>

#ifndef GREEN_ION_GIT_SHA
#define GREEN_ION_GIT_SHA "v10-standalone"
#endif

const char *FIRMWARE_VERSION = "11.0-rc1";
const char *FIRMWARE_BUILD = GREEN_ION_GIT_SHA;

// ---------- HARDWARE ----------

#ifndef PIN_NEOPIXEL
#define PIN_NEOPIXEL 0
#endif
#ifndef NEOPIXEL_I2C_POWER
#define NEOPIXEL_I2C_POWER 2
#endif

#define SD_CS 33
#define BATTERY_PIN 35
#define RED_LED 13

// ---------- NETWORK ----------

const char *AP_NAME = "GREEN-ION";
const char *AP_PASS = "GreenIon2026";

// ---------- CONFIGURATION ----------

uint16_t sampleRateHz = 25;
uint32_t sampleIntervalUs = 40000;
uint16_t effectiveSampleRateHz = 25;
uint32_t effectiveSampleIntervalUs = 40000;
const uint32_t CONTROL_LEASE_MS = 30000;
const uint32_t FLUSH_MS = 500;
const uint32_t CHECKPOINT_MS = 5000;
const uint32_t CAL_MS = 40;
const uint16_t CAL_WARMUP_SAMPLES = 15;
const uint16_t CAL_TARGET_SAMPLES = 96;
const uint16_t CAL_MIN_VALID_SAMPLES = 80;
const uint16_t CAL_MAX_COLLECTION_ATTEMPTS = 125;
const uint16_t CAL_VERIFY_SAMPLES = 20;
const uint32_t LANDING_MS = 20000;
const size_t BUFFER_BYTES = 1024;
const size_t EMERGENCY_BUFFER_BYTES = 8192;
const uint16_t PREBUFFER_CAPACITY = 150; // 3 seconds at 50 Hz
const uint32_t WIFI_LAUNCH_GRACE_MS = 2000;

uint8_t recordingLimitMin = 10;

// ---------- HARDWARE OBJECTS ----------

Adafruit_BMP3XX bmp;
Adafruit_NeoPixel neo(
  1, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800
);
WebServer web(80);
Preferences nv;
File flightFile;

// ---------- SENSOR ----------

bool sensorOK = false;
bool readingOK = false;
bool calibrated = false;
bool calibrating = false;

float pressurePa = NAN;
float tempC = NAN;
float groundPa = NAN;
float altM = NAN;
float rawAltM = NAN;
float flightAltM = NAN;
float speedMps = 0;
float peakM = 0;
float prevFlightAlt = NAN;
float pressureWindow[3] = {NAN, NAN, NAN};
uint8_t pressureWindowCount = 0;
uint8_t pressureWindowHead = 0;

uint32_t lastReadMs = 0;
uint32_t lastFlightAltMs = 0;
uint32_t goodReads = 0;
uint32_t badReads = 0;
uint32_t unchangedPressureReads = 0;
float lastChangingPressurePa = NAN;
bool sensorFrozen = false;

int calStep = 0;
int calValid = 0;
uint8_t calPhase = 0;
float calSamples[CAL_TARGET_SAMPLES];
uint16_t calCollectionAttempts = 0;
uint16_t calVerifyAttempts = 0;
uint16_t calVerifyValid = 0;
double calVerifySumM = 0;
double calVerifySquareSumM = 0;
float calVerifyMaxAbsM = 0;
float calMedianPa = NAN;
float calNoisePa = NAN;
float calNoiseM = NAN;
float calDriftPaPerSec = NAN;
uint32_t lastCalStepMs = 0;
uint32_t calCount = 0;
uint32_t calTimeMs = 0;
String calStatus = "Not calibrated";

// ---------- POWER ----------

uint16_t batteryMv = 0;
bool batteryPresent = false;
uint32_t lastBatteryMs = 0;
uint16_t flightStartBatteryMv = 0;
uint16_t flightMinimumBatteryMv = 0;
uint16_t flightMaximumDropMv = 0;
uint8_t lowBatterySeconds = 0;
uint8_t criticalBatterySeconds = 0;
bool batteryRateReduced = false;
bool criticalBatteryCheckpointed = false;
uint16_t minimumEffectiveRateHz = 25;
String adaptiveRateStatus = "Normal sample rate";

// ---------- FLIGHT ----------

bool sdOK = false;
bool sdVerified = false;
bool sdFault = false;
bool armed = false;
bool recording = false;
bool launched = false;
bool launchWatch = false;
bool wifiLaunchDisconnectPending = false;
uint32_t wifiLaunchDisconnectAtMs = 0;
bool landingCandidate = false;
bool simulating = false;
bool wifiRunning = false;

float landingReference = 0;
float simAlt = 0;
float simSpeed = 0;

uint32_t landingMs = 0;
uint32_t simStartMs = 0;
uint32_t flightStartMs = 0;
uint32_t finalDurationMs = 0;
uint32_t lastSampleUs = 0;
uint32_t lastFlushMs = 0;
uint32_t lastCheckpointMs = 0;
uint32_t recoveredFlights = 0;
uint32_t lastRecoveredSamples = 0;
String resetReasonText = "unknown";
String previousPowerEvidence = "No previous flight evidence";
String integrityStatus = "Not checked";

uint32_t samples = 0;
uint32_t invalidSamples = 0;
uint32_t missedSamples = 0;
uint32_t writeErrors = 0;
uint32_t lastFlushUs = 0;
uint32_t maximumFlushUs = 0;
uint32_t flushHeartbeat = 0;
uint32_t flightChecksum = 2166136261UL;

String filename = "(none)";
String metadataFilename = "";
String checkpointFilename = "";
String checkpointFilenameB = "";
String healthFilename = "";
String reportFilename = "";
String flightLabel = "Flight 1";
String missionKind = "flight";
uint32_t nextFlightNumber = 1;
uint32_t currentMissionNumber = 0;
uint32_t nextCardSequence = 1;
uint32_t currentCardSequence = 0;
String cardId = "";
String motorLabel = "Unspecified";
String message = "Starting";
String errorText = "";
String sdStatus = "Not checked";
String finishReason = "";

float minimumPressurePa = NAN;
float minimumTemperatureC = NAN;
float maximumTemperatureC = NAN;

// ---------- FLIGHT CONFIDENCE EVENTS ----------

enum FlightEventCode : uint8_t {
  EVENT_LAUNCH = 1,
  EVENT_COAST = 2,
  EVENT_APOGEE = 3,
  EVENT_DESCENT = 4,
  EVENT_PARACHUTE = 5,
  EVENT_LANDING = 6,
  EVENT_PRESSURE_ANOMALY = 7,
  EVENT_RATE_REDUCED = 8,
  EVENT_SENSOR_LOST = 9,
  EVENT_SENSOR_RECOVERED = 10,
  EVENT_SD_FAULT = 11
};

struct FlightEvent {
  uint32_t elapsedMs;
  uint8_t code;
  int16_t value;
};

const uint8_t MAX_FLIGHT_EVENTS = 48;
FlightEvent flightEvents[MAX_FLIGHT_EVENTS];
uint8_t flightEventCount = 0;
uint32_t pressureAnomalyCount = 0;
float lastValidPressurePa = NAN;
uint32_t lastPressureSampleMs = 0;
uint32_t lastPressureAnomalyEventMs = 0;
bool currentPressureAnomaly = false;
float maximumPressureJumpPa = 0;
uint16_t consecutiveInvalidReads = 0;
uint16_t consecutiveRecoveryReads = 0;
bool sensorOutageActive = false;
uint32_t sensorOutageStartedMs = 0;
uint32_t sensorOutageTotalMs = 0;
uint32_t longestSensorOutageMs = 0;
uint16_t sensorOutageCount = 0;
uint16_t sensorRecoveryAttempts = 0;
uint32_t lastSensorRecoveryAttemptMs = 0;
uint8_t pressureRecoverySamples = 0;
uint32_t pressureRecoveryUntilMs = 0;
uint32_t implausibleSamples = 0;

// ---------- PRE-LAUNCH BUFFER AND FLIGHT PHASES ----------

struct PrelaunchRecord {
  uint32_t timestampUs;
  float pressure;
  float temperature;
  uint16_t battery;
  uint16_t valid;
};

PrelaunchRecord prebuffer[PREBUFFER_CAPACITY];
uint16_t prebufferHead = 0;
uint16_t prebufferCount = 0;
uint8_t launchConfirmSamples = 0;
float launchWatchBaseAltM = 0;
float altitudeNoiseM = 0.6f;
float peakAscentSpeedMps = 0;
float fastestDescentMps = 0;
float landingAnchorM = NAN;
uint32_t landingStableSinceMs = 0;
String activePhase = "idle";
uint8_t phaseConfidence = 0;
uint32_t phaseLaunchMs = 0;
uint32_t phaseBoostEndMs = 0;
uint32_t phaseApogeeMs = 0;
uint32_t phaseDescentMs = 0;
uint32_t phaseParachuteMs = 0;
uint32_t phaseLandingMs = 0;

// ---------- PREPARE ALL ----------

bool prepareActive = false;
uint8_t prepareStage = 0;
uint8_t prepareProgress = 0;
String prepareStatus = "Not prepared";
bool prepareReady = false;
String preflightReportFilename = "";
float calibrationTemperatureC = NAN;
uint32_t sdProfileWriteUs = 0;
uint16_t sdSafeRateHz = 10;
String environmentalWarning = "Not checked";
float previousGroundNoiseM = NAN;
uint32_t totalFlights = 0;
uint32_t totalSdTests = 0;
uint32_t totalSensorRecoveries = 0;
String maintenanceReminder = "No maintenance due";

// ---------- PHONE TIME ----------

uint64_t phoneEpochMs = 0;
uint32_t phoneEpochSetAtMs = 0;
int16_t phoneUtcOffsetMin = 0;

// ---------- GROUND TEST ----------

bool groundTestActive = false;
uint32_t groundTestStartMs = 0;
uint32_t groundTestSamples = 0;
uint32_t groundTestInvalid = 0;
uint32_t groundTestExpected = 0;
uint32_t groundTestMissing = 0;
uint32_t groundTestMaximumWriteUs = 0;
uint32_t groundTestMaximumFlushUs = 0;
uint16_t groundTestStartBatteryMv = 0;
uint16_t groundTestMinimumBatteryMv = 0;
float groundTestMinimumAltM = NAN;
float groundTestMaximumAltM = NAN;
String groundTestResult = "Not run";
String groundTestDetail = "Run before flight to verify the complete recorder path";
File groundTestFile;

// ---------- SHUTDOWN ----------

// Shutdown is scheduled AFTER sending HTTP response.
bool shutdownPending = false;
uint32_t shutdownAtMs = 0;

// ---------- FIRMWARE UPDATE ----------

bool updateInProgress = false;
bool updateSucceeded = false;
String updateError = "";
size_t updateBytes = 0;
bool updateHeaderChecked = false;

// ---------- CONTROLLER LEASE ----------

String controllerToken = "";
uint32_t controllerSeenMs = 0;

// ---------- LED ----------

bool manualLed = false;
int ledEffect = 0;
int ledR = 0;
int ledG = 180;
int ledB = 255;
int ledBrightness = 45;

// ---------- LOGGER BUFFER ----------

uint8_t buffer[BUFFER_BYTES];
size_t used = 0;
uint8_t emergencyBuffer[EMERGENCY_BUFFER_BYTES];
size_t emergencyUsed = 0;
uint32_t emergencyRecoveredBytes = 0;
uint32_t lastHealthSnapshotMs = 0;
uint32_t lastLoopHeartbeatMs = 0;

// ---------- JSON ----------

String jb(bool b) {
  return b ? "true" : "false";
}

String jn(float f, int decimals = 2) {
  return isfinite(f) ? String(f, decimals) : "null";
}

String je(String s) {
  s.replace("\\", "\\\\");
  s.replace("\"", "\\\"");
  s.replace("\n", " ");
  s.replace("\r", " ");
  return s;
}

String u64Text(uint64_t value) {
  char text[24];
  snprintf(text, sizeof(text), "%llu", (unsigned long long)value);
  return String(text);
}

String cleanLabel(String s, size_t maximumLength) {
  s.trim();
  String clean;
  clean.reserve(maximumLength);

  for (size_t i = 0;
       i < s.length() && clean.length() < maximumLength;
       i++) {
    char c = s[i];
    if (c >= 32 && c <= 126 &&
        c != ',' && c != '\"' && c != '\\')
      clean += c;
  }

  return clean;
}

void respond(bool ok, String detail);

bool controlLeaseExpired() {
  return controllerToken.length() == 0 ||
    (uint32_t)(millis() - controllerSeenMs) > CONTROL_LEASE_MS;
}

bool controllerMatches(bool refreshLease = true) {
  String token = web.arg("token");
  bool matches =
    !controlLeaseExpired() &&
    token.length() >= 12 &&
    token == controllerToken;

  if (matches && refreshLease)
    controllerSeenMs = millis();

  return matches;
}

bool requireControl() {
  if (controllerMatches()) return true;
  respond(false, "View-only: tap TAKE CONTROL first");
  return false;
}

bool maybeAutoGrantControl() {
  String token = web.arg("token");
  if (WiFi.softAPgetStationNum() != 1 || token.length() < 12 || token.length() > 64)
    return false;
  for (size_t i = 0; i < token.length(); i++)
    if (!isAlphaNumeric(token[i])) return false;
  controllerToken = token;
  controllerSeenMs = millis();
  return true;
}

void respond(bool ok, String detail) {
  String s = "{\"success\":";
  s += jb(ok);
  s += ",\"message\":\"";
  s += je(detail);
  s += "\"}";
  web.sendHeader("Cache-Control", "no-store");
  web.send(ok ? 200 : 409, "application/json", s);
}

// ---------- RKT4 WRITERS ----------

void w16(uint8_t *p, uint16_t n) {
  p[0] = n & 255;
  p[1] = (n >> 8) & 255;
}

void w32(uint8_t *p, uint32_t n) {
  p[0] = n & 255;
  p[1] = (n >> 8) & 255;
  p[2] = (n >> 16) & 255;
  p[3] = (n >> 24) & 255;
}

void wf(uint8_t *p, float f) {
  uint32_t n;
  memcpy(&n, &f, 4);
  w32(p, n);
}

// ---------- BATTERY ----------

void recordFlightEvent(uint8_t code, int16_t value = 0) {
  if (!recording || flightEventCount >= MAX_FLIGHT_EVENTS) return;
  flightEvents[flightEventCount++] = {
    millis() - flightStartMs, code, value
  };
}

String flightEventLabel(uint8_t code) {
  switch (code) {
    case EVENT_LAUNCH: return "launch";
    case EVENT_COAST: return "coast";
    case EVENT_APOGEE: return "apogee";
    case EVENT_DESCENT: return "descent";
    case EVENT_PARACHUTE: return "probable_parachute";
    case EVENT_LANDING: return "landing";
    case EVENT_PRESSURE_ANOMALY: return "pressure_anomaly";
    case EVENT_RATE_REDUCED: return "sample_rate_reduced";
    case EVENT_SENSOR_LOST: return "sensor_lost";
    case EVENT_SENSOR_RECOVERED: return "sensor_recovered";
    case EVENT_SD_FAULT: return "sd_fault";
    default: return "unknown";
  }
}

void applyEffectiveSampleRate(uint16_t rate, String reason) {
  if (rate != 10 && rate != 25 && rate != 50) return;
  if (recording && rate > effectiveSampleRateHz) return;
  if (rate == effectiveSampleRateHz) return;
  effectiveSampleRateHz = rate;
  effectiveSampleIntervalUs = 1000000UL / effectiveSampleRateHz;
  minimumEffectiveRateHz = min(minimumEffectiveRateHz, effectiveSampleRateHz);
  if (sensorOK) {
    bmp.setOutputDataRate(
      effectiveSampleRateHz >= 50 ? BMP3_ODR_50_HZ : BMP3_ODR_25_HZ
    );
  }
  lastSampleUs = micros();
  adaptiveRateStatus = reason + " · " + String(effectiveSampleRateHz) + " Hz";
  if (recording) recordFlightEvent(EVENT_RATE_REDUCED, effectiveSampleRateHz);
}

void readBattery() {
  uint32_t v =
    (uint32_t)analogReadMilliVolts(BATTERY_PIN) * 2UL;
  batteryMv = (uint16_t)min(v, (uint32_t)65535);
  batteryPresent = batteryMv >= 2500 &&
                   batteryMv <= 4400;

  if (recording && batteryPresent) {
    if (!flightMinimumBatteryMv || batteryMv < flightMinimumBatteryMv)
      flightMinimumBatteryMv = batteryMv;
    if (flightStartBatteryMv > batteryMv)
      flightMaximumDropMv = max(
        flightMaximumDropMv,
        (uint16_t)(flightStartBatteryMv - batteryMv)
      );

    lowBatterySeconds = batteryMv < 3500
      ? min((int)lowBatterySeconds + 1, 255) : 0;
    criticalBatterySeconds = batteryMv < 3300
      ? min((int)criticalBatterySeconds + 1, 255) : 0;

    if (criticalBatterySeconds >= 3) {
      applyEffectiveSampleRate(10, "Critical battery protection");
      if (!criticalBatteryCheckpointed) {
        criticalBatteryCheckpointed = true;
        if (flushLog()) saveCheckpoint();
      }
    } else if (lowBatterySeconds >= 5 && !batteryRateReduced) {
      batteryRateReduced = true;
      uint16_t reducedRate = effectiveSampleRateHz >= 50 ? 25 : 10;
      applyEffectiveSampleRate(reducedRate, "Low battery protection");
    }
  }

  if (groundTestActive && batteryPresent &&
      (!groundTestMinimumBatteryMv || batteryMv < groundTestMinimumBatteryMv))
    groundTestMinimumBatteryMv = batteryMv;
}

bool lowBattery() {
  return batteryPresent && batteryMv < 3500;
}

bool criticalBattery() {
  return batteryPresent && batteryMv < 3300;
}

int batteryPercent() {
  if (!batteryPresent) return -1;

  const int voltage[] = {
    3300, 3500, 3600, 3700, 3750,
    3800, 3850, 3900, 4000, 4100, 4200
  };
  const int percent[] = {
    0, 4, 8, 17, 27,
    40, 52, 65, 80, 92, 100
  };

  if (batteryMv <= voltage[0]) return 0;

  for (int i = 1; i < 11; i++) {
    if (batteryMv <= voltage[i]) {
      float fraction =
        float(batteryMv - voltage[i - 1]) /
        float(voltage[i] - voltage[i - 1]);
      return percent[i - 1] +
        (int)roundf(
          fraction * (percent[i] - percent[i - 1])
        );
    }
  }
  return 100;
}

// ---------- BMP388 ----------

void resetAltitudeFilters() {
  altM = NAN;
  rawAltM = NAN;
  flightAltM = NAN;
  speedMps = 0;
  prevFlightAlt = NAN;
  lastFlightAltMs = 0;
  pressureWindowCount = 0;
  pressureWindowHead = 0;
  for (uint8_t i = 0; i < 3; i++) pressureWindow[i] = NAN;
}

float medianPressure3(float pressure) {
  pressureWindow[pressureWindowHead] = pressure;
  pressureWindowHead = (pressureWindowHead + 1) % 3;
  if (pressureWindowCount < 3) pressureWindowCount++;
  if (pressureWindowCount < 3) return pressure;

  float a = pressureWindow[0];
  float b = pressureWindow[1];
  float c = pressureWindow[2];
  if (a > b) { float t = a; a = b; b = t; }
  if (b > c) { float t = b; b = c; c = t; }
  if (a > b) { float t = a; a = b; b = t; }
  return b;
}

bool setSampleRate(uint16_t requestedRate) {
  if (requestedRate != 10 &&
      requestedRate != 25 &&
      requestedRate != 50)
    return false;

  sampleRateHz = requestedRate;
  sampleIntervalUs = 1000000UL / sampleRateHz;
  effectiveSampleRateHz = sampleRateHz;
  effectiveSampleIntervalUs = sampleIntervalUs;
  nv.putUShort("samplehz",sampleRateHz);

  if (sensorOK) {
    bmp.setOutputDataRate(
      sampleRateHz >= 50
        ? BMP3_ODR_50_HZ
        : BMP3_ODR_25_HZ
    );
  }

  lastSampleUs = micros();
  adaptiveRateStatus = "Normal sample rate · " + String(sampleRateHz) + " Hz";
  resetAltitudeFilters();
  return true;
}

bool initSensor() {
  sensorOK = bmp.begin_I2C(0x77, &Wire);
  if (!sensorOK)
    sensorOK = bmp.begin_I2C(0x76, &Wire);

  if (!sensorOK) {
    errorText = "BMP388 not detected";
    return false;
  }

  bmp.setTemperatureOversampling(
    BMP3_OVERSAMPLING_2X
  );
  bmp.setPressureOversampling(
    BMP3_OVERSAMPLING_4X
  );
  bmp.setIIRFilterCoeff(
    BMP3_IIR_FILTER_COEFF_3
  );
  setSampleRate(sampleRateHz);

  message = "BMP388 detected";
  return true;
}

bool getSensor(float &p, float &t) {
  if (!sensorOK || !bmp.performReading())
    return false;

  p = bmp.pressure;
  t = bmp.temperature;

  return isfinite(p) &&
         isfinite(t) &&
         p > 30000 && p < 120000 &&
         t > -50 && t < 100;
}

// ---------- NON-BLOCKING CALIBRATION ----------

float relativeAltitude(float pressure, float baseline) {
  if (!isfinite(pressure) || !isfinite(baseline) || baseline <= 0) return NAN;
  return 44330.0f * (1 - powf(pressure / baseline, 0.190294957f));
}

void failCalibration(String reason) {
  calibrating = false;
  calibrated = false;
  calPhase = 0;
  calStatus = "Calibration FAILED: " + reason;
  errorText = calStatus;
  message = calStatus;
  Serial.println("[CAL] " + calStatus);
}

void sortCalibrationSamples() {
  for (int i = 1; i < calValid; i++) {
    float value = calSamples[i];
    int j = i - 1;
    while (j >= 0 && calSamples[j] > value) {
      calSamples[j + 1] = calSamples[j];
      j--;
    }
    calSamples[j + 1] = value;
  }
}

bool beginCalibrationVerification() {
  if (calValid < CAL_MIN_VALID_SAMPLES) {
    failCalibration("only " + String(calValid) + "/" +
      String(CAL_TARGET_SAMPLES) + " valid readings");
    return false;
  }

  uint16_t driftWindow = min((uint16_t)12, (uint16_t)(calValid / 4));
  double firstSum = 0;
  double lastSum = 0;
  for (uint16_t i = 0; i < driftWindow; i++) {
    firstSum += calSamples[i];
    lastSum += calSamples[calValid - driftWindow + i];
  }
  float collectionSeconds = max(1.0f,
    calCollectionAttempts * CAL_MS / 1000.0f);
  calDriftPaPerSec = (float)((lastSum - firstSum) / driftWindow) /
    collectionSeconds;

  sortCalibrationSamples();
  calMedianPa = calSamples[calValid / 2];
  uint16_t trim = max((uint16_t)4, (uint16_t)(calValid / 10));
  uint16_t first = trim;
  uint16_t last = calValid - trim;
  double sum = 0;
  for (uint16_t i = first; i < last; i++) sum += calSamples[i];
  groundPa = (float)(sum / (last - first));

  double squareSum = 0;
  for (uint16_t i = first; i < last; i++) {
    double delta = calSamples[i] - groundPa;
    squareSum += delta * delta;
  }
  calNoisePa = sqrt(squareSum / max((uint16_t)1, (uint16_t)(last - first - 1)));
  calNoiseM = fabsf(relativeAltitude(groundPa + calNoisePa, groundPa));

  if (!isfinite(groundPa) || !isfinite(calNoiseM) ||
      calNoiseM > 0.55f) {
    failCalibration("rocket moved or pressure noise was too high (±" +
      String(calNoiseM, 2) + " m)");
    return false;
  }
  if (!isfinite(calDriftPaPerSec) || fabsf(calDriftPaPerSec) > 1.5f) {
    failCalibration("pressure drifted " + String(calDriftPaPerSec, 2) +
      " Pa/s; keep the rocket still and out of direct wind");
    return false;
  }

  calPhase = 2;
  calVerifyAttempts = 0;
  calVerifyValid = 0;
  calVerifySumM = 0;
  calVerifySquareSumM = 0;
  calVerifyMaxAbsM = 0;
  calStatus = "Verifying altitude zero";
  return true;
}

void finishCalibrationVerification() {
  if (calVerifyValid < 16) {
    failCalibration("verification had too few valid readings");
    return;
  }

  float meanM = calVerifySumM / calVerifyValid;
  float variance = calVerifySquareSumM / calVerifyValid - meanM * meanM;
  float deviationM = sqrtf(max(0.0f, variance));
  if (fabsf(meanM) > 0.35f || deviationM > 0.45f ||
      calVerifyMaxAbsM > 1.5f) {
    failCalibration("zero verification was unstable (mean " +
      String(meanM, 2) + " m, noise ±" + String(deviationM, 2) + " m)");
    return;
  }

  calibrating = false;
  calibrated = true;
  calibrationTemperatureC = tempC;
  calPhase = 0;
  calCount++;
  calTimeMs = millis();
  lastSampleUs = micros();
  resetAltitudeFilters();
  altM = rawAltM = flightAltM = 0;
  peakM = 0;
  calStatus = "Calibration PASSED · ±" + String(calNoiseM, 2) +
    " m noise · " + String(calDriftPaPerSec, 2) + " Pa/s drift";
  message = "Ground altitude accurately zeroed";
  errorText = "";
  Serial.printf("[CAL] Baseline %.2f Pa, noise %.3f m, drift %.3f Pa/s\n",
    groundPa, calNoiseM, calDriftPaPerSec);
}

String calibrationFreshnessReason() {
  if (!calibrated || !calTimeMs) return "Not calibrated";
  if (recording || launchWatch) return "Fresh (flight active)";
  if (millis() - calTimeMs > 900000UL) return "Calibration is older than 15 minutes";
  if (isfinite(calibrationTemperatureC) && isfinite(tempC) &&
      fabsf(tempC - calibrationTemperatureC) > 3.0f)
    return "Temperature changed more than 3 C";
  if (isfinite(rawAltM) && fabsf(rawAltM) > 2.0f)
    return "Ground pressure shifted more than 2 m";
  return "Fresh";
}

bool calibrationFresh() {
  String reason = calibrationFreshnessReason();
  return reason == "Fresh" || reason == "Fresh (flight active)";
}

uint8_t calibrationProgress() {
  if (!calibrating) return calibrated ? 100 : 0;
  if (calPhase == 1) {
    if (calStep < CAL_WARMUP_SAMPLES)
      return (uint8_t)(calStep * 10UL / CAL_WARMUP_SAMPLES);
    return (uint8_t)min(80UL,
      10UL + (uint32_t)calValid * 70UL / CAL_TARGET_SAMPLES);
  }
  if (calPhase == 2)
    return (uint8_t)min(100UL,
      80UL + (uint32_t)calVerifyAttempts * 20UL / CAL_VERIFY_SAMPLES);
  return 0;
}

bool startCalibration() {
  if (!sensorOK || armed || recording ||
      simulating || calibrating || groundTestActive || shutdownPending)
    return false;

  calibrated = false;
  calibrating = true;
  calStep = 0;
  calValid = 0;
  calPhase = 1;
  calCollectionAttempts = 0;
  calVerifyAttempts = 0;
  calVerifyValid = 0;
  calMedianPa = NAN;
  calNoisePa = NAN;
  calNoiseM = NAN;
  calDriftPaPerSec = NAN;
  lastCalStepMs = millis() - CAL_MS;
  calStatus = "Warming and collecting stationary pressure";
  message = calStatus;
  errorText = "";

  Serial.println("[CAL] Started");
  return true;
}

void processCalibration() {
  if (!calibrating) return;
  if (millis() - lastCalStepMs < CAL_MS) return;

  lastCalStepMs = millis();

  float p = NAN, t = NAN;
  bool ok = getSensor(p, t);
  readingOK = ok;

  if (ok) {
    goodReads++;
    pressurePa = p;
    tempC = t;
    lastReadMs = millis();

    if (calPhase == 1 && calStep >= CAL_WARMUP_SAMPLES &&
        calValid < CAL_TARGET_SAMPLES) {
      calSamples[calValid++] = p;
    } else if (calPhase == 2) {
      float verifyAltitude = relativeAltitude(p, groundPa);
      if (isfinite(verifyAltitude)) {
        calVerifyValid++;
        calVerifySumM += verifyAltitude;
        calVerifySquareSumM += verifyAltitude * verifyAltitude;
        calVerifyMaxAbsM = max(calVerifyMaxAbsM, fabsf(verifyAltitude));
      }
    }
  } else {
    badReads++;
  }

  calStep++;
  if (calPhase == 1) {
    if (calStep > CAL_WARMUP_SAMPLES) calCollectionAttempts++;
    if (calValid < CAL_TARGET_SAMPLES &&
        calCollectionAttempts < CAL_MAX_COLLECTION_ATTEMPTS) return;
    beginCalibrationVerification();
    return;
  }
  if (calPhase == 2) {
    calVerifyAttempts++;
    if (calVerifyAttempts >= CAL_VERIFY_SAMPLES)
      finishCalibrationVerification();
  }
}

// ---------- SD CARD ----------

bool initSD() {
  sdOK = SD.begin(SD_CS, SPI, 4000000);
  sdVerified = false;
  sdStatus = sdOK ? "SD detected" : "SD not detected";
  return sdOK;
}

uint64_t sdFree() {
  if (!sdOK) return 0;
  uint64_t total = SD.totalBytes();
  uint64_t occupied = SD.usedBytes();
  return total > occupied ? total - occupied : 0;
}

bool verifySD() {
  if (recording || armed || calibrating ||
      shutdownPending) {
    sdStatus = "SD test blocked: device busy";
    return false;
  }

  if (!sdOK && !initSD()) return false;
  sdStatus = "SD test running: writing and reading test file";
  sdVerified = false;

  const char *path = "/GITEST.TMP";
  const char *pattern = "GREEN-ION-V82";

  if (SD.exists(path) && !SD.remove(path)) {
    sdStatus = "SD test failed: cannot clear old test file";
    return false;
  }

  File f = SD.open(path, FILE_WRITE);
  if (!f) {
    sdStatus = "Could not create test file";
    sdVerified = false;
    return false;
  }

  uint32_t profileStartedUs = micros();
  size_t len = strlen(pattern);
  size_t n = f.write((const uint8_t*)pattern, len);
  f.flush();
  sdProfileWriteUs = micros() - profileStartedUs;
  f.close();

  if (n != len) {
    SD.remove(path);
    sdVerified = false;
    sdStatus = "SD write failed";
    return false;
  }

  f = SD.open(path, FILE_READ);
  if (!f) {
    sdVerified = false;
    sdStatus = "SD read failed";
    SD.remove(path);
    return false;
  }

  char result[24] = {0};
  size_t count = f.readBytes(result, len);
  f.close();
  bool cleanupOK = SD.remove(path);

  sdVerified =
    cleanupOK && count == len &&
    memcmp(result, pattern, len) == 0 &&
    sdFree() >= 1048576ULL;
  sdSafeRateHz = sdProfileWriteUs <= 25000UL ? 50 : sdProfileWriteUs <= 75000UL ? 25 : 10;
  totalSdTests++;
  nv.putUInt("sdtests", totalSdTests);

  sdStatus = sdVerified
    ? "SD ready: write, read, cleanup and free space OK"
    : !cleanupOK ? "SD test failed: could not delete test file"
    : count != len || memcmp(result, pattern, len) != 0
      ? "SD test failed: data readback mismatch"
      : "SD test failed: less than 1 MB free";

  if (sdVerified) sdFault = false;
  return sdVerified;
}

// ---------- RKT4 INTEGRITY AND RECOVERY ----------
// A complete RKT4 file has a 32-byte header followed by 16-byte samples.
// A torn final record is preserved in the original BIN but excluded from CSV.
// This is best-effort power-loss recovery, not guaranteed flash durability.
String journalName(String bin, char slot = 'A') {
  bin.replace(".BIN", slot == 'B' ? ".JRB" : ".JRA");
  return bin;
}

bool loadCardIdentity() {
  if (!sdOK) return false;
  File f = SD.open("/GREENION.CFG", FILE_READ);
  if (f) {
    while (f.available()) {
      String line = f.readStringUntil('\n');
      line.trim();
      if (line.startsWith("card_id=")) cardId = line.substring(8);
      if (line.startsWith("next_sequence=")) nextCardSequence = max(1UL, (uint32_t)line.substring(14).toInt());
    }
    f.close();
  }
  if (!cardId.length()) {
    char id[17];
    uint64_t mac = ESP.getEfuseMac();
    snprintf(id, sizeof(id), "%08lX%08lX", (unsigned long)(mac >> 32),
      (unsigned long)(mac ^ micros()));
    cardId = String(id);
    nextCardSequence = 1;
  }
  SD.remove("/GREENION.CFG");
  f = SD.open("/GREENION.CFG", FILE_WRITE);
  if (!f) return false;
  f.println("GREEN-ION-CARD-1");
  f.println("card_id=" + cardId);
  f.println("next_sequence=" + String(nextCardSequence));
  f.flush();
  f.close();
  return true;
}

bool saveCardSequence() {
  if (!sdOK || !cardId.length()) return false;
  SD.remove("/GREENION.CFG");
  File f = SD.open("/GREENION.CFG", FILE_WRITE);
  if (!f) return false;
  f.println("GREEN-ION-CARD-1");
  f.println("card_id=" + cardId);
  f.println("next_sequence=" + String(nextCardSequence));
  f.flush();
  f.close();
  return true;
}

bool validRKT4Header(File &f) {
  if (!f || f.size() < 32) return false;
  uint8_t h[32];
  if (!f.seek(0) || f.read(h, 32) != 32) return false;
  uint16_t version = (uint16_t)h[4] | ((uint16_t)h[5] << 8);
  uint16_t headerSize = (uint16_t)h[6] | ((uint16_t)h[7] << 8);
  uint16_t recordSize = (uint16_t)h[8] | ((uint16_t)h[9] << 8);
  uint16_t rate = (uint16_t)h[10] | ((uint16_t)h[11] << 8);
  return memcmp(h, "RKT4", 4) == 0 && version == 2 &&
         headerSize == 32 && recordSize == 16 &&
         (rate == 10 || rate == 25 || rate == 50);
}

bool saveCheckpoint() {
  if (!recording || !flightFile || !sdOK || sdFault) return false;
  static bool slotB = false;
  String target = slotB ? checkpointFilenameB : checkpointFilename;
  SD.remove(target);
  File j = SD.open(target, FILE_WRITE);
  if (!j) {
    errorText = "Checkpoint unavailable; flight BIN logging continues";
    return false;
  }
  // Checkpoints are hints only; recovery trusts actual flushed file size.
  j.println("GREEN-ION-CHECKPOINT-1");
  j.println("file=" + filename.substring(1));
  j.println("samples=" + String(samples));
  j.println("committed_bytes=" + String((uint32_t)flightFile.size()));
  j.println("elapsed_ms=" + String(millis() - flightStartMs));
  j.flush();
  j.close();
  slotB = !slotB;
  lastCheckpointMs = millis();
  static uint32_t lastEvidenceMs = 0;
  if (millis() - lastEvidenceMs >= 30000UL) {
    nv.putString("lastfile", filename);
    nv.putString("lastphase", activePhase);
    nv.putUShort("lastbat", batteryMv);
    nv.putUInt("lastsample", samples);
    lastEvidenceMs = millis();
  }
  return true;
}

void recoverInterruptedFlights() {
  if (!sdOK) return;
  File root = SD.open("/");
  if (!root) return;
  File entry = root.openNextFile();
  while (entry) {
    String name = entry.name();
    int slash = name.lastIndexOf('/');
    if (slash >= 0) name = name.substring(slash + 1);
    bool isJournal = !entry.isDirectory() && name.length() == 11 &&
                     name.startsWith("FLT") &&
                     (name.endsWith(".JRA") || name.endsWith(".JRB"));
    for (int i = 3; i < 7 && isJournal; i++)
      if (!isDigit(name[i])) isJournal = false;
    entry.close();
    if (isJournal) {
      String bin = name; bin.replace(name.endsWith(".JRB") ? ".JRB" : ".JRA", ".BIN");
      String meta = name; meta.replace(name.endsWith(".JRB") ? ".JRB" : ".JRA", ".TXT");
      File log = SD.open("/" + bin, FILE_READ);
      bool valid = validRKT4Header(log);
      uint32_t count = valid ? (log.size() - 32) / 16 : 0;
      uint32_t tail = valid ? (log.size() - 32) % 16 : 0;
      log.close();
      // If power failed after metadata was completed but before journal
      // cleanup, the recording is complete and must not be mislabeled.
      bool alreadyComplete = false;
      File previousMeta = SD.open("/" + meta, FILE_READ);
      if (previousMeta) {
        while (previousMeta.available()) {
          String line = previousMeta.readStringUntil('\n');
          line.trim();
          if (line == "status=complete") alreadyComplete = true;
        }
        previousMeta.close();
      }
      if (alreadyComplete) {
        SD.remove("/" + name);
      } else {
      File summary = SD.open("/" + meta, FILE_APPEND);
      if (summary) {
        summary.println("status=recovered_interrupted");
        summary.println("finish_reason=Unexpected reset or power interruption");
        summary.println("recovered_samples=" + String(count));
        summary.println("trailing_partial_bytes=" + String(tail));
        summary.println("rkt4_header_valid=" + String(valid ? "yes" : "no"));
        summary.flush();
        summary.close();
        if (SD.remove("/" + name)) {
          recoveredFlights++;
          lastRecoveredSamples = count;
        }
      } else {
        integrityStatus = "Recovery pending: cannot write metadata";
      }
      }
    }
    entry = root.openNextFile();
  }
  root.close();
  if (recoveredFlights)
    integrityStatus = String(recoveredFlights) + " interrupted flight(s) recovered; verify CSV";
  else if (integrityStatus != "Recovery pending: cannot write metadata")
    integrityStatus = "No interrupted recordings detected";
}

// ---------- FLIGHT LOGGING ----------

bool flushLog() {
  if (!recording || !flightFile) return false;

  uint32_t flushStartedUs = micros();

  if (emergencyUsed) {
    size_t restored = flightFile.write(emergencyBuffer, emergencyUsed);
    if (restored == emergencyUsed) {
      emergencyRecoveredBytes += emergencyUsed;
      emergencyUsed = 0;
      sdFault = false;
      integrityStatus = "SD recovered; emergency RAM samples restored";
    }
  }

  size_t offset = 0;
  while (offset < used) {
    size_t n = flightFile.write(
      buffer + offset, used - offset
    );
    if (!n) {
      if (!sdFault) recordFlightEvent(EVENT_SD_FAULT);
      sdFault = true;
      writeErrors++;
      size_t remaining = used - offset;
      if (emergencyUsed + remaining <= EMERGENCY_BUFFER_BYTES) {
        memcpy(emergencyBuffer + emergencyUsed, buffer + offset, remaining);
        emergencyUsed += remaining;
        used = 0;
        errorText = "SD stalled; samples protected in emergency RAM";
        integrityStatus = "Degraded: emergency RAM spill active";
        return true;
      }
      errorText = "SD and emergency RAM full";
      finishReason = errorText;
      finalDurationMs = millis() - flightStartMs;
      recording = false;
      armed = false;
      used = 0;
      flightFile.close();
      integrityStatus = "Incomplete: SD and emergency RAM exhausted";
      return false;
    }
    offset += n;
  }

  used = 0;
  emergencyUsed = 0;
  emergencyRecoveredBytes = 0;
  lastHealthSnapshotMs = 0;
  implausibleSamples = 0;
  pressureRecoverySamples = 0;
  pressureRecoveryUntilMs = 0;
  flightFile.flush();
  lastFlushUs = micros() - flushStartedUs;
  maximumFlushUs = max(maximumFlushUs, lastFlushUs);
  flushHeartbeat++;
  lastFlushMs = millis();
  return true;
}

void checksumBytes(const uint8_t *data, size_t length) {
  for (size_t i = 0; i < length; i++) {
    flightChecksum ^= data[i];
    flightChecksum *= 16777619UL;
  }
}

void logSample(uint32_t ts, float p,
               float t, bool valid) {
  if (!recording) return;

  if (used + 16 > BUFFER_BYTES) {
    if (!flushLog()) return;
  }

  uint8_t *r = buffer + used;

  w32(r, ts);
  wf(r + 4, p);
  wf(r + 8, t);
  w16(r + 12, batteryMv);
  w16(r + 14, valid ? 1 : 0);

  checksumBytes(r, 16);
  used += 16;
  samples++;
  if (!valid) invalidSamples++;
}

bool writeFlightMetadataStart() {
  File meta = SD.open(metadataFilename, FILE_WRITE);
  if (!meta) return false;

  meta.println("GREEN ION V11 FLIGHT METADATA");
  meta.println("firmware_version=" + String(FIRMWARE_VERSION));
  meta.println("firmware_build=" + String(FIRMWARE_BUILD));
  meta.println("file=" + filename.substring(1));
  meta.println("label=" + flightLabel);
  meta.println("mission_number=" + String(currentMissionNumber));
  meta.println("card_id=" + cardId);
  meta.println("card_sequence=" + String(currentCardSequence));
  meta.println("motor=" + motorLabel);
  meta.println("sample_rate_hz=" + String(sampleRateHz));
  meta.println("initial_effective_sample_rate_hz=" + String(effectiveSampleRateHz));
  meta.println("preflight_report=" + preflightReportFilename);
  meta.println("recording_limit_minutes=" + String(recordingLimitMin));
  if (phoneEpochMs) {
    uint64_t startEpochMs = phoneEpochMs +
      (uint32_t)(flightStartMs - phoneEpochSetAtMs);
    meta.println("phone_start_epoch_ms=" + u64Text(startEpochMs));
    meta.println("phone_utc_offset_minutes=" + String(phoneUtcOffsetMin));
  } else {
    meta.println("phone_start_epoch_ms=unavailable");
  }
  meta.print("baseline_pressure_pa=");
  meta.println(groundPa, 2);
  meta.print("calibration_median_pressure_pa=");
  meta.println(calMedianPa, 2);
  meta.print("calibration_noise_m=");
  meta.println(calNoiseM, 3);
  meta.print("calibration_drift_pa_per_s=");
  meta.println(calDriftPaPerSec, 3);
  meta.print("calibration_temperature_c=");
  meta.println(calibrationTemperatureC, 2);
  meta.println("altitude_filtering=raw_pressure_logged;median3_pressure;separate_display_and_flight_filters");
  meta.print("starting_battery_v=");
  meta.println(batteryMv / 1000.0f, 3);
  meta.println("status=recording");
  meta.println("rkt4_format=2");
  meta.flush();
  meta.close();
  return true;
}

void writeFlightMetadataEnd() {
  File meta = SD.open(metadataFilename, FILE_APPEND);
  if (!meta) {
    writeErrors++;
    return;
  }

  meta.println("status=" + String(sdFault ? "incomplete_sd_error" : "complete"));
  meta.println("integrity=" + integrityStatus);
  meta.println("write_errors=" + String(writeErrors));
  meta.println("finish_reason=" + finishReason);
  meta.print("duration_s=");
  meta.println(finalDurationMs / 1000.0f, 2);
  meta.println("samples=" + String(samples));
  meta.println("invalid_samples=" + String(invalidSamples));
  meta.println("missed_samples=" + String(missedSamples));
  uint32_t totalExpected = samples + missedSamples;
  int confidence = 100;
  if (sdFault || writeErrors) confidence -= 40;
  if (totalExpected) {
    confidence -= min(20, (int)((invalidSamples * 20UL) / totalExpected));
    confidence -= min(20, (int)((missedSamples * 20UL) / totalExpected));
  }
  if (sensorFrozen) confidence -= 20;
  confidence -= min(20, (int)pressureAnomalyCount * 4);
  confidence -= min(30, (int)sensorOutageCount * 10);
  if (maximumFlushUs > effectiveSampleIntervalUs) confidence -= 10;
  confidence = max(0, confidence);
  meta.println("confidence_score=" + String(confidence));
  meta.print("starting_battery_v=");
  meta.println(flightStartBatteryMv / 1000.0f, 3);
  meta.print("minimum_battery_v=");
  meta.println(flightMinimumBatteryMv / 1000.0f, 3);
  meta.print("maximum_voltage_drop_v=");
  meta.println(flightMaximumDropMv / 1000.0f, 3);
  meta.println("maximum_flush_us=" + String(maximumFlushUs));
  meta.println("flush_heartbeat=" + String(flushHeartbeat));
  meta.println("sensor_frozen_detected=" + String(sensorFrozen ? "yes" : "no"));
  meta.println("minimum_effective_sample_rate_hz=" + String(minimumEffectiveRateHz));
  meta.println("adaptive_rate_status=" + adaptiveRateStatus);
  meta.println("pressure_anomaly_count=" + String(pressureAnomalyCount));
  meta.print("maximum_pressure_jump_pa=");
  meta.println(maximumPressureJumpPa, 2);
  meta.println("sensor_outage_count=" + String(sensorOutageCount));
  meta.println("sensor_outage_total_ms=" + String(sensorOutageTotalMs));
  meta.println("sensor_outage_longest_ms=" + String(longestSensorOutageMs));
  meta.println("sensor_recovery_attempts=" + String(sensorRecoveryAttempts));
  meta.println("implausible_samples=" + String(implausibleSamples));
  meta.println("emergency_ram_recovered_bytes=" + String(emergencyRecoveredBytes));
  meta.println("sd_profile_max_write_us=" + String(sdProfileWriteUs));
  meta.println("sd_safe_rate_hz=" + String(sdSafeRateHz));
  meta.println("confidence_timeline_count=" + String(flightEventCount));
  for (uint8_t i = 0; i < flightEventCount; i++) {
    meta.println("confidence_event_" + String(i + 1) + "=" +
      String(flightEvents[i].elapsedMs) + "|" +
      flightEventLabel(flightEvents[i].code) + "|" +
      String(flightEvents[i].value));
  }
  meta.println("phase_detection=estimated_from_bmp388_pressure");
  meta.println("phase_confidence=" + String(phaseConfidence));
  meta.println("phase_launch_ms=" + String(phaseLaunchMs));
  meta.println("phase_boost_end_ms=" + String(phaseBoostEndMs));
  meta.println("phase_apogee_ms=" + String(phaseApogeeMs));
  meta.println("phase_descent_ms=" + String(phaseDescentMs));
  meta.println("phase_probable_parachute_ms=" + String(phaseParachuteMs));
  meta.println("phase_landing_ms=" + String(phaseLandingMs));
  char checksumText[12];
  snprintf(checksumText, sizeof(checksumText), "%08lX", (unsigned long)flightChecksum);
  meta.println("bin_fnv1a32=" + String(checksumText));
  meta.print("peak_altitude_m=");
  meta.println(peakM, 2);
  meta.print("minimum_pressure_pa=");
  meta.println(minimumPressurePa, 2);
  meta.print("temperature_min_c=");
  meta.println(minimumTemperatureC, 2);
  meta.print("temperature_max_c=");
  meta.println(maximumTemperatureC, 2);
  meta.flush();
  meta.close();
}

void writeHealthSnapshot() {
  if (!recording || !sdOK || millis() - lastHealthSnapshotMs < 5000UL) return;
  File h = SD.open(healthFilename, FILE_APPEND);
  if (!h) return;
  h.println(String(millis() - flightStartMs) + "," + String(batteryMv) + "," +
    String(used) + "," + String(emergencyUsed) + "," + String(missedSamples) + "," +
    String(maximumFlushUs) + "," + String(readingOK ? 1 : 0) + "," +
    String(effectiveSampleRateHz));
  h.close();
  lastHealthSnapshotMs = millis();
}

void writePostflightReport() {
  if (!sdOK || !reportFilename.length()) return;
  SD.remove(reportFilename);
  File r = SD.open(reportFilename, FILE_WRITE);
  if (!r) return;
  r.println("GREEN ION V11 POST-FLIGHT REPORT");
  r.println("flight=" + flightLabel);
  r.println("file=" + filename.substring(1));
  r.println("finish=" + finishReason);
  r.println("duration_s=" + String(finalDurationMs / 1000.0f, 2));
  r.println("peak_altitude_m=" + String(peakM, 2));
  r.println("peak_ascent_mps=" + String(peakAscentSpeedMps, 2));
  r.println("fastest_descent_mps=" + String(fastestDescentMps, 2));
  r.println("battery_drop_mv=" + String(flightMaximumDropMv));
  r.println("sensor_outages=" + String(sensorOutageCount));
  r.println("pressure_anomalies=" + String(pressureAnomalyCount));
  r.println("emergency_ram_recovered_bytes=" + String(emergencyRecoveredBytes));
  r.println("phase_confidence=" + String(phaseConfidence));
  r.println("interpretation=All phases and motion values are pressure-derived estimates");
  r.close();
}

void rebuildFlightManifest() {
  if (!sdOK || recording) return;
  SD.remove("/MANIFEST.CSV");
  File out = SD.open("/MANIFEST.CSV", FILE_WRITE);
  File root = SD.open("/");
  if (!out || !root) { if (out) out.close(); if (root) root.close(); return; }
  out.println("file,size_bytes,complete_records,integrity");
  File f = root.openNextFile();
  while (f) {
    String n = f.name(); int slash = n.lastIndexOf('/'); if (slash >= 0) n = n.substring(slash + 1);
    if (!f.isDirectory() && validFile(n)) {
      uint32_t size = f.size();
      String integrity = size >= 32 && (size - 32) % 16 == 0 ? "structurally_valid" : "review";
      out.println(n + "," + String(size) + "," + String(size >= 32 ? (size - 32) / 16 : 0) + "," + integrity);
    }
    f.close(); f = root.openNextFile();
  }
  root.close(); out.close();
}

void stopFlight(String reason) {
  if (!recording) return;

  if (sensorOutageActive) {
    uint32_t duration = millis() - sensorOutageStartedMs;
    sensorOutageTotalMs += duration;
    longestSensorOutageMs = max(longestSensorOutageMs, duration);
    sensorOutageActive = false;
  }

  finalDurationMs = millis() - flightStartMs;
  bool saved = flushLog();

  if (flightFile) flightFile.close();

  recording = false;
  armed = false;
  launchWatch = false;
  landingCandidate = false;

  finishReason = saved && !sdFault
    ? reason : "SD write failure";

  integrityStatus = saved && !sdFault ? "Saved and flushed" : "Incomplete: SD error";
  writeFlightMetadataEnd();
  writePostflightReport();
  if (saved && !sdFault && checkpointFilename.length()) {
    SD.remove(checkpointFilename);
    SD.remove(checkpointFilenameB);
  }

  message = saved && !sdFault
    ? "Saved " + filename
    : "Flight save failed";
  activePhase = saved && !sdFault ? "saved" : "error";
  wifiLaunchDisconnectPending = false;
  totalFlights++;
  nv.putUInt("totalflights", totalFlights);
  rebuildFlightManifest();
}

void resetPrebuffer() {
  prebufferHead = 0;
  prebufferCount = 0;
  launchConfirmSamples = 0;
}

void capturePrelaunch(uint32_t stamp, float p, float t, bool valid) {
  PrelaunchRecord &r = prebuffer[prebufferHead];
  r.timestampUs = stamp;
  r.pressure = p;
  r.temperature = t;
  r.battery = batteryMv;
  r.valid = valid ? 1 : 0;
  prebufferHead = (prebufferHead + 1) % PREBUFFER_CAPACITY;
  uint16_t target = min(PREBUFFER_CAPACITY, (uint16_t)(sampleRateHz * 3));
  if (prebufferCount < target) prebufferCount++;
}

void writePrebuffer() {
  uint16_t start = (prebufferHead + PREBUFFER_CAPACITY - prebufferCount) % PREBUFFER_CAPACITY;
  for (uint16_t i = 0; i < prebufferCount && recording; i++) {
    PrelaunchRecord &r = prebuffer[(start + i) % PREBUFFER_CAPACITY];
    uint16_t savedBattery = batteryMv;
    batteryMv = r.battery;
    logSample(r.timestampUs, r.pressure, r.temperature, r.valid != 0);
    batteryMv = savedBattery;
  }
}

bool startLaunchWatch() {
  if (!armed || recording || launchWatch || !calibrated || !sdVerified || sdFault) {
    errorText = "Arm after successful preparation before launch watch";
    return false;
  }
  resetPrebuffer();
  launchWatchBaseAltM = isfinite(flightAltM) ? flightAltM : 0;
  launchWatch = true;
  activePhase = "launch watch";
  message = "Watching for launch with 3-second prebuffer";
  errorText = "";
  return true;
}

void cancelLaunchWatch() {
  launchWatch = false;
  armed = false;
  resetPrebuffer();
  activePhase = "idle";
  message = "Launch watch cancelled";
}

bool beginFlight() {
  if (!armed || !sensorOK || !readingOK ||
      !calibrated || !sdOK || !sdVerified ||
      sdFault || simulating || calibrating || groundTestActive || !launchWatch ||
      shutdownPending || criticalBattery()) {
    errorText = "Preflight incomplete";
    return false;
  }

  if (sdFree() < 1048576ULL) {
    errorText = "Not enough SD space";
    return false;
  }

  bool found = false;
  uint32_t chosenFlightNumber = 0;

  for (int i = (int)nextFlightNumber; i <= 9999; i++) {
    char path[24];
    snprintf(
      path, sizeof(path), "/FLT%04d.BIN", i
    );

    if (!SD.exists(path)) {
      filename = path;
      chosenFlightNumber = (uint32_t)i;
      found = true;
      break;
    }
  }

  if (!found) {
    errorText = "No free flight filename (FLT0001-FLT9999)";
    return false;
  }

  currentMissionNumber = chosenFlightNumber;
  currentCardSequence = nextCardSequence;
  flightLabel = "Mission " + String(currentMissionNumber) +
    " · Card Flight " + String(currentCardSequence);
  metadataFilename = filename;
  metadataFilename.replace(".BIN", ".TXT");
  checkpointFilename = journalName(filename, 'A');
  checkpointFilenameB = journalName(filename, 'B');
  healthFilename = filename; healthFilename.replace(".BIN", ".HLT");
  reportFilename = filename; reportFilename.replace(".BIN", ".RPT");

  flightFile = SD.open(filename, FILE_WRITE);

  if (!flightFile) {
    errorText = "Could not create flight file";
    return false;
  }

  uint8_t hdr[32] = {0};
  memcpy(hdr, "RKT4", 4);
  w16(hdr + 4, 2);
  w16(hdr + 6, 32);
  w16(hdr + 8, 16);
  w16(hdr + 10, sampleRateHz);
  wf(hdr + 12, groundPa);
  w16(hdr + 16, batteryMv);

  if (flightFile.write(hdr, 32) != 32) {
    flightFile.close();
    SD.remove(filename);
    errorText = "Header write failed";
    return false;
  }

  flightChecksum = 2166136261UL;
  checksumBytes(hdr, 32);

  flightFile.flush();

  flightStartMs = millis();
  SD.remove(metadataFilename);
  if (!writeFlightMetadataStart()) {
    flightFile.close();
    SD.remove(filename);
    errorText = "Could not create flight metadata";
    return false;
  }

  used = 0;
  integrityStatus = "Recording; buffered samples flush every 500 ms";
  samples = 0;
  invalidSamples = 0;
  missedSamples = 0;
  writeErrors = 0;
  lastFlushUs = 0;
  maximumFlushUs = 0;
  flushHeartbeat = 0;
  flightStartBatteryMv = batteryMv;
  flightMinimumBatteryMv = batteryMv;
  flightMaximumDropMv = 0;
  effectiveSampleRateHz = sampleRateHz;
  effectiveSampleIntervalUs = sampleIntervalUs;
  minimumEffectiveRateHz = sampleRateHz;
  adaptiveRateStatus = "Normal sample rate · " + String(sampleRateHz) + " Hz";
  lowBatterySeconds = criticalBatterySeconds = 0;
  batteryRateReduced = false;
  criticalBatteryCheckpointed = false;
  flightEventCount = 0;
  pressureAnomalyCount = 0;
  maximumPressureJumpPa = 0;
  lastValidPressurePa = NAN;
  lastPressureSampleMs = 0;
  lastPressureAnomalyEventMs = 0;
  currentPressureAnomaly = false;
  consecutiveInvalidReads = consecutiveRecoveryReads = 0;
  sensorOutageActive = false;
  sensorOutageStartedMs = 0;
  sensorOutageTotalMs = longestSensorOutageMs = 0;
  sensorOutageCount = sensorRecoveryAttempts = 0;
  lastSensorRecoveryAttemptMs = 0;
  minimumPressurePa = NAN;
  minimumTemperatureC = NAN;
  maximumTemperatureC = NAN;

  lastFlushMs = millis();
  lastCheckpointMs = millis();
  lastSampleUs = micros();
  finalDurationMs = 0;

  peakM = 0;
  launched = true;
  landingCandidate = false;

  nextFlightNumber = chosenFlightNumber + 1;
  nv.putUInt("nextid",nextFlightNumber);
  nextCardSequence = currentCardSequence + 1;
  saveCardSequence();
  recording = true;
  armed = false;
  launchWatch = false;
  activePhase = "powered ascent";
  recordFlightEvent(EVENT_LAUNCH);
  phaseConfidence = 70;
  phaseLaunchMs = 0;
  phaseBoostEndMs = phaseApogeeMs = phaseDescentMs = phaseParachuteMs = phaseLandingMs = 0;
  peakAscentSpeedMps = max(0.0f, speedMps);
  fastestDescentMps = 0;
  landingStableSinceMs = 0;
  landingAnchorM = NAN;
  writePrebuffer();
  if (flushLog() && saveCheckpoint()) {
    wifiLaunchDisconnectPending = true;
    wifiLaunchDisconnectAtMs = millis() + WIFI_LAUNCH_GRACE_MS;
  }
  manualLed = false;

  finishReason = "";
  errorText = "";
  message = "Launch detected; prebuffer saved";

  return true;
}

// ---------- LANDING DETECTION ----------

void checkLanding() {
  if (!recording) return;

  uint32_t elapsed = millis() - flightStartMs;

  if (elapsed >= (uint32_t)recordingLimitMin * 60000UL) {
    stopFlight(String(recordingLimitMin) + "-minute timeout");
    return;
  }

  if (!readingOK || !isfinite(flightAltM)) {
    landingCandidate = false;
    return;
  }

  // Pressure-only phase estimates. These are recorded as estimates, never as
  // recovery-control signals.
  if (activePhase == "powered ascent")
    peakAscentSpeedMps = max(peakAscentSpeedMps, speedMps);
  if (activePhase == "powered ascent" && elapsed > 500 &&
      peakAscentSpeedMps > 8 && speedMps < peakAscentSpeedMps - 5) {
    activePhase = "coast";
    phaseBoostEndMs = elapsed;
    phaseConfidence = max(phaseConfidence, (uint8_t)60);
    recordFlightEvent(EVENT_COAST);
  }
  if ((activePhase == "powered ascent" || activePhase == "coast") &&
      elapsed > 1500 && speedMps <= 0.5f && peakM > 8) {
    activePhase = "apogee";
    phaseApogeeMs = elapsed;
    phaseConfidence = max(phaseConfidence, (uint8_t)75);
    recordFlightEvent(EVENT_APOGEE);
  }
  if ((activePhase == "apogee" || activePhase == "coast") && speedMps < -1.5f) {
    activePhase = "descent";
    phaseDescentMs = elapsed;
    recordFlightEvent(EVENT_DESCENT);
  }
  if (activePhase == "descent")
    fastestDescentMps = min(fastestDescentMps, speedMps);
  if (activePhase == "descent" && fastestDescentMps < -8 &&
      speedMps > fastestDescentMps + 4 && speedMps < -0.5f) {
    activePhase = "probable parachute";
    phaseParachuteMs = elapsed;
    phaseConfidence = max(phaseConfidence, (uint8_t)65);
    recordFlightEvent(EVENT_PARACHUTE);
  }
  bool descended = peakM >= 12 && peakM - flightAltM >= 8;
  float landingSpeedLimit = constrain(0.7f + altitudeNoiseM * 1.5f, 0.9f, 2.0f);
  uint32_t adaptiveLandingMs = (uint32_t)constrain(12000.0f + altitudeNoiseM * 8000.0f,
    14000.0f, 26000.0f);
  bool stable = elapsed > 15000 && descended && fabsf(speedMps) <= landingSpeedLimit;
  if (!stable) {
    landingStableSinceMs = 0;
    landingAnchorM = flightAltM;
    landingCandidate = false;
    return;
  }
  if (!landingStableSinceMs || !isfinite(landingAnchorM) ||
      fabsf(flightAltM - landingAnchorM) > 2.0f) {
    landingStableSinceMs = millis();
    landingAnchorM = flightAltM;
    landingCandidate = true;
    return;
  }
  if (millis() - landingStableSinceMs >= adaptiveLandingMs) {
    activePhase = "landing";
    phaseLandingMs = elapsed;
    recordFlightEvent(EVENT_LANDING);
    stopFlight("Automatic landing save");
  }
}

// ---------- SENSOR SAMPLING ----------

void groundTestSample(uint32_t stamp, float p, float t, bool valid) {
  if (!groundTestActive || !groundTestFile) return;

  uint8_t record[16];
  w32(record, stamp);
  wf(record + 4, p);
  wf(record + 8, t);
  w16(record + 12, batteryMv);
  w16(record + 14, valid ? 1 : 0);

  uint32_t startedUs = micros();
  size_t written = groundTestFile.write(record, sizeof(record));
  groundTestMaximumWriteUs = max(
    groundTestMaximumWriteUs,
    micros() - startedUs
  );
  groundTestSamples++;
  if (written != sizeof(record) || !valid) groundTestInvalid++;

  if (valid && calibrated && groundPa > 0) {
    float testAltitude = 44330.0f *
      (1 - powf(p / groundPa, 0.190294957f));
    if (!isfinite(groundTestMinimumAltM) || testAltitude < groundTestMinimumAltM)
      groundTestMinimumAltM = testAltitude;
    if (!isfinite(groundTestMaximumAltM) || testAltitude > groundTestMaximumAltM)
      groundTestMaximumAltM = testAltitude;
  }
}

bool startGroundTest() {
  if (groundTestActive || recording || armed || calibrating || simulating ||
      !sensorOK || !readingOK || !calibrated || !sdOK || !sdVerified) {
    errorText = "Calibrate and verify sensor/SD before ground test";
    return false;
  }

  SD.remove("/GNDTEST.TMP");
  groundTestFile = SD.open("/GNDTEST.TMP", FILE_WRITE);
  if (!groundTestFile) {
    errorText = "Could not create ground-test file";
    return false;
  }

  groundTestActive = true;
  groundTestStartMs = millis();
  groundTestSamples = 0;
  groundTestInvalid = 0;
  groundTestExpected = 0;
  groundTestMissing = 0;
  groundTestMaximumWriteUs = 0;
  groundTestMaximumFlushUs = 0;
  groundTestStartBatteryMv = batteryMv;
  groundTestMinimumBatteryMv = batteryMv;
  groundTestMinimumAltM = NAN;
  groundTestMaximumAltM = NAN;
  groundTestResult = "RUNNING";
  groundTestDetail = "Sampling sensor, battery, timing, writes, and flushes for 10 seconds";
  message = "Ground test running";
  errorText = "";
  return true;
}

void finishGroundTest() {
  if (!groundTestActive) return;

  uint32_t durationMs = millis() - groundTestStartMs;
  uint32_t flushStartedUs = micros();
  groundTestFile.flush();
  groundTestMaximumFlushUs = micros() - flushStartedUs;
  size_t bytes = groundTestFile.size();
  groundTestFile.close();
  SD.remove("/GNDTEST.TMP");
  groundTestActive = false;

  groundTestExpected = (durationMs * sampleRateHz) / 1000UL;
  groundTestMissing = groundTestExpected > groundTestSamples
    ? groundTestExpected - groundTestSamples : 0;
  float noise = isfinite(groundTestMinimumAltM) && isfinite(groundTestMaximumAltM)
    ? groundTestMaximumAltM - groundTestMinimumAltM : NAN;
  if (isfinite(noise)) altitudeNoiseM = max(0.15f, noise);
  uint16_t batteryDrop = groundTestStartBatteryMv > groundTestMinimumBatteryMv
    ? groundTestStartBatteryMv - groundTestMinimumBatteryMv : 0;

  bool complete = bytes == groundTestSamples * 16UL;
  bool timingGood = groundTestMissing <= max(2UL, groundTestExpected / 50UL);
  bool sensorGood = groundTestInvalid == 0 && !sensorFrozen;
  bool batteryGood = batteryDrop <= 150;
  bool storageGood = groundTestMaximumFlushUs <= 250000UL;
  bool noiseGood = isfinite(noise) && noise <= 1.5f;

  groundTestResult = complete && timingGood && sensorGood && batteryGood &&
    storageGood && noiseGood ? "PASS" : "REVIEW";
  String noiseRating = !isfinite(noise) ? "unavailable" :
    noise <= 0.6f ? "Excellent" : noise <= 1.5f ? "Acceptable" : "Too noisy";
  groundTestDetail = String(groundTestSamples) + "/" + String(groundTestExpected) +
    " samples, " + String(groundTestMissing) + " missing, altitude noise " +
    (isfinite(noise) ? String(noise, 2) + " m (" + noiseRating + ")" : noiseRating) +
    ", battery drop " + String(batteryDrop) + " mV, max write " +
    String(groundTestMaximumWriteUs) + " us, flush " +
    String(groundTestMaximumFlushUs) + " us";
  previousGroundNoiseM = nv.getFloat("gndnoise", NAN);
  if (isfinite(noise)) nv.putFloat("gndnoise", noise);
  String comparison = isfinite(previousGroundNoiseM) && isfinite(noise) ?
    " · previous noise " + String(previousGroundNoiseM, 2) + " m" : " · first saved ground test";
  groundTestDetail += comparison;
  environmentalWarning = !noiseGood ? "Wind, vibration, or pressure instability detected" :
    fabsf(calDriftPaPerSec) > 1.0f ? "Weather pressure is changing; review calibration" :
    fabsf(tempC - calibrationTemperatureC) > 2.0f ? "Temperature is changing" : "Environment stable";
  if (sdSafeRateHz < sampleRateHz) {
    setSampleRate(sdSafeRateHz);
    groundTestDetail += " · sample rate reduced to SD-safe " + String(sdSafeRateHz) + " Hz";
  }
  message = "Ground test " + groundTestResult;
}

bool startPrepareAll() {
  if (prepareActive || armed || recording || launchWatch || calibrating ||
      groundTestActive || simulating || shutdownPending) return false;
  prepareActive = true;
  prepareReady = false;
  prepareStage = 1;
  prepareProgress = 5;
  prepareStatus = "Checking sensor, battery, and microSD";
  errorText = "";
  return true;
}

bool writePreflightReport() {
  if (!sdOK || sdFault) return false;
  char path[24];
  snprintf(path, sizeof(path), "/PRE%04lu.TXT", (unsigned long)nextFlightNumber);
  preflightReportFilename = String(path).substring(1);
  SD.remove(path);
  File report = SD.open(path, FILE_WRITE);
  if (!report) return false;
  report.println("GREEN ION V11 PREFLIGHT REPORT");
  report.println("firmware_version=" + String(FIRMWARE_VERSION));
  report.println("firmware_build=" + String(FIRMWARE_BUILD));
  report.println("mission_number=" + String(nextFlightNumber));
  report.println("card_sequence=" + String(nextCardSequence));
  report.println("sensor=" + String(sensorOK && readingOK ? "PASS" : "REVIEW"));
  report.println("sd=" + String(sdOK && sdVerified && !sdFault ? "PASS" : "REVIEW"));
  report.println("battery_mv=" + String(batteryMv));
  report.println("battery=" + String(criticalBattery() ? "REVIEW" : "PASS"));
  report.print("baseline_pressure_pa="); report.println(groundPa, 2);
  report.print("calibration_noise_m="); report.println(calNoiseM, 3);
  report.print("calibration_drift_pa_per_s="); report.println(calDriftPaPerSec, 3);
  report.print("calibration_temperature_c="); report.println(calibrationTemperatureC, 2);
  report.println("calibration_age_seconds=" + String(calTimeMs ? (millis() - calTimeMs) / 1000UL : 0));
  report.println("calibration_freshness=" + calibrationFreshnessReason());
  report.println("sd_total_bytes=" + u64Text(SD.totalBytes()));
  report.println("sd_free_bytes=" + u64Text(sdFree()));
  uint64_t estimatedBytes = 32ULL + (uint64_t)sampleRateHz * recordingLimitMin * 60ULL * 16ULL;
  report.println("estimated_flight_bytes=" + u64Text(estimatedBytes));
  report.println("estimated_flights_remaining=" + String(estimatedBytes ? sdFree() / estimatedBytes : 0));
  report.println("sd_profile_write_us=" + String(sdProfileWriteUs));
  report.println("sd_safe_rate_hz=" + String(sdSafeRateHz));
  report.println("environment=" + environmentalWarning);
  report.println("ground_test_previous_noise_m=" + String(previousGroundNoiseM, 3));
  report.println("configured_sample_rate_hz=" + String(sampleRateHz));
  report.println("recording_limit_minutes=" + String(recordingLimitMin));
  report.println("ground_test=" + groundTestResult);
  report.println("ground_test_detail=" + groundTestDetail);
  report.println("final_result=" + String(prepareReady ? "READY" : "REVIEW"));
  report.flush();
  report.close();
  return true;
}

void finishPrepareReview(String reason) {
  prepareActive = false;
  prepareReady = false;
  prepareProgress = 100;
  prepareStatus = "REVIEW: " + reason;
  errorText = prepareStatus;
}

void processPrepareAll() {
  if (!prepareActive) return;
  if (prepareStage == 1) {
    if (!sensorOK || !readingOK) { finishPrepareReview("BMP388 is not producing valid readings"); return; }
    if (criticalBattery()) { finishPrepareReview("battery is critical"); return; }
    if (!verifySD()) { finishPrepareReview(sdStatus); return; }
    prepareStage = 2;
    prepareProgress = 20;
    prepareStatus = "Calibrating altitude";
    if (!startCalibration()) finishPrepareReview("calibration could not start");
    return;
  }
  if (prepareStage == 2) {
    prepareProgress = 20 + (uint32_t)calibrationProgress() * 40UL / 100UL;
    if (calibrating) return;
    if (!calibrated) { finishPrepareReview(calStatus); return; }
    prepareStage = 3;
    prepareProgress = 60;
    prepareStatus = "Running complete 10-second ground test";
    if (!startGroundTest()) finishPrepareReview(errorText);
    return;
  }
  if (prepareStage == 3) {
    if (groundTestActive) {
      prepareProgress = 60 + min(35UL, (millis() - groundTestStartMs) * 35UL / 10000UL);
      return;
    }
    prepareActive = false;
    prepareProgress = 100;
    prepareReady = groundTestResult == "PASS";
    prepareStatus = prepareReady ? "READY: calibration and ground test passed" :
      "REVIEW: " + groundTestDetail;
    if (!prepareReady) errorText = prepareStatus;
    if (!writePreflightReport()) {
      prepareReady = false;
      prepareStatus = "REVIEW: could not save preflight report";
      errorText = prepareStatus;
    }
  }
}

bool attemptSensorRecovery() {
  sensorRecoveryAttempts++;
  totalSensorRecoveries++;
  nv.putUInt("sensorrec", totalSensorRecoveries);
  lastSensorRecoveryAttemptMs = millis();
  bool found = bmp.begin_I2C(0x77, &Wire);
  if (!found) found = bmp.begin_I2C(0x76, &Wire);
  sensorOK = found;
  if (!found) return false;
  bmp.setTemperatureOversampling(BMP3_OVERSAMPLING_2X);
  bmp.setPressureOversampling(BMP3_OVERSAMPLING_4X);
  bmp.setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_3);
  bmp.setOutputDataRate(effectiveSampleRateHz >= 50 ? BMP3_ODR_50_HZ : BMP3_ODR_25_HZ);
  return true;
}

void noteInvalidSensorRead() {
  consecutiveInvalidReads++;
  consecutiveRecoveryReads = 0;
  if (recording && consecutiveInvalidReads >= 3 && !sensorOutageActive) {
    sensorOutageActive = true;
    sensorOutageStartedMs = millis();
    sensorOutageCount++;
    recordFlightEvent(EVENT_SENSOR_LOST, consecutiveInvalidReads);
  }
  if (consecutiveInvalidReads >= 10 && millis() - lastSensorRecoveryAttemptMs >= 2000UL)
    attemptSensorRecovery();
}

void noteValidSensorRead() {
  consecutiveInvalidReads = 0;
  if (!sensorOutageActive) return;
  if (++consecutiveRecoveryReads < 3) return;
  uint32_t duration = millis() - sensorOutageStartedMs;
  sensorOutageTotalMs += duration;
  longestSensorOutageMs = max(longestSensorOutageMs, duration);
  sensorOutageActive = false;
  consecutiveRecoveryReads = 0;
  lastValidPressurePa = NAN;
  resetAltitudeFilters();
  recordFlightEvent(EVENT_SENSOR_RECOVERED, min(32767UL, duration));
}

void detectPressureAnomaly(float p) {
  currentPressureAnomaly = false;
  uint32_t now = millis();
  if (recording && isfinite(lastValidPressurePa) && lastPressureSampleMs &&
      now > lastPressureSampleMs && now - flightStartMs > 1000UL) {
    float jump = p - lastValidPressurePa;
    float rate = jump * 1000.0f / (now - lastPressureSampleMs);
    bool anomaly = (jump > 40.0f && rate > 1200.0f) || fabsf(jump) > 250.0f;
    if (anomaly) {
      currentPressureAnomaly = true;
      pressureRecoverySamples = 0;
      pressureRecoveryUntilMs = now + 750UL;
      maximumPressureJumpPa = max(maximumPressureJumpPa, fabsf(jump));
      if (now - lastPressureAnomalyEventMs >= 500UL) {
        pressureAnomalyCount++;
        lastPressureAnomalyEventMs = now;
        phaseConfidence = phaseConfidence > 5 ? phaseConfidence - 5 : 0;
        recordFlightEvent(EVENT_PRESSURE_ANOMALY,
          constrain((int)lroundf(jump), -32768, 32767));
      }
    }
  }
  lastValidPressurePa = p;
  lastPressureSampleMs = now;
}

void sampleSensor() {
  float p = NAN, t = NAN;
  uint32_t stamp = micros();

  bool valid = getSensor(p, t);
  readingOK = valid;

  if (!valid) {
    badReads++;
    noteInvalidSensorRead();
    if (launchWatch) capturePrelaunch(stamp, NAN, NAN, false);
    else logSample(stamp, NAN, NAN, false);
    groundTestSample(stamp, NAN, NAN, false);
    return;
  }

  goodReads++;
  noteValidSensorRead();
  detectPressureAnomaly(p);
  lastReadMs = millis();
  pressurePa = p;
  tempC = t;

  if (isfinite(lastChangingPressurePa) && fabsf(p - lastChangingPressurePa) < 0.01f) {
    unchangedPressureReads++;
  } else {
    lastChangingPressurePa = p;
    unchangedPressureReads = 0;
    sensorFrozen = false;
  }
  if (unchangedPressureReads > effectiveSampleRateHz * 2UL)
    sensorFrozen = true;

  if (!isfinite(minimumPressurePa) || p < minimumPressurePa)
    minimumPressurePa = p;
  if (!isfinite(minimumTemperatureC) || t < minimumTemperatureC)
    minimumTemperatureC = t;
  if (!isfinite(maximumTemperatureC) || t > maximumTemperatureC)
    maximumTemperatureC = t;

  if (calibrated && groundPa > 0) {
    // Preserve the original pressure in RKT4. A three-point pressure median
    // rejects single spikes before conversion, then independent filters serve
    // the dashboard and time-sensitive flight-state detector.
    rawAltM = relativeAltitude(p, groundPa);
    float cleanPressure = medianPressure3(p);
    float measuredAltitude = relativeAltitude(cleanPressure, groundPa);
    bool plausible = isfinite(measuredAltitude) &&
      (!isfinite(flightAltM) || fabsf(measuredAltitude - flightAltM) <= 120.0f ||
       (lastFlightAltMs && (millis() - lastFlightAltMs) > 500UL));
    if (!plausible) {
      implausibleSamples++;
      currentPressureAnomaly = true;
    }
    if (millis() < pressureRecoveryUntilMs || pressureRecoverySamples < 4) {
      if (!currentPressureAnomaly) pressureRecoverySamples++;
      if (pressureRecoverySamples < 4) plausible = false;
    }

    if (!plausible) {
      logSample(stamp, p, t, true);
      groundTestSample(stamp, p, t, true);
      return;
    }
    bool descentFilter = recording &&
      (activePhase == "descent" || activePhase == "probable parachute");

    if (!isfinite(altM)) altM = measuredAltitude;
    else {
      float difference = measuredAltitude - altM;
      float movement = fabsf(difference);
      float displayAlpha = descentFilter ? 0.18f : movement < 0.25f ? 0.10f :
        movement < 1.5f ? 0.24f : movement < 5.0f ? 0.48f : 0.70f;
      altM += displayAlpha * difference;
    }

    if (!isfinite(flightAltM)) flightAltM = measuredAltitude;
    else {
      float difference = measuredAltitude - flightAltM;
      float movement = fabsf(difference);
      float flightAlpha = descentFilter ? 0.34f : movement < 0.25f ? 0.28f :
        movement < 1.5f ? 0.48f : movement < 5.0f ? 0.68f : 0.84f;
      flightAltM += flightAlpha * difference;
    }

    if (flightAltM > peakM) peakM = flightAltM;

    uint32_t now = millis();

    if (isfinite(prevFlightAlt) &&
        lastFlightAltMs &&
        now > lastFlightAltMs) {
      float instant =
        (flightAltM - prevFlightAlt) * 1000.0f /
        (now - lastFlightAltMs);

      // Ignore impossible single-sample spikes. Fast real motion
      // gets more weight than small stationary variations.
      if (fabsf(instant) <= 500.0f) {
        float velocityAlpha =
          fabsf(instant) > 15.0f ? 0.55f : 0.18f;
        speedMps += velocityAlpha * (instant - speedMps);
      }
    }

    prevFlightAlt = flightAltM;
    lastFlightAltMs = now;
  }

  if (launchWatch) {
    capturePrelaunch(stamp, p, t, true);
    float gain = isfinite(flightAltM) ? flightAltM - launchWatchBaseAltM : 0;
    float requiredGain = max(1.5f, altitudeNoiseM * 5.0f);
    bool rising = gain >= requiredGain && speedMps >= 4.0f && p < groundPa;
    launchConfirmSamples = rising ? min(launchConfirmSamples + 1, 255) : 0;
    uint8_t requiredSamples = max(2, (int)(effectiveSampleRateHz / 10));
    if (launchConfirmSamples >= requiredSamples) {
      if (!beginFlight()) {
        launchWatch = false;
        armed = false;
        activePhase = "error";
      }
    }
  } else {
    logSample(stamp, p, t, true);
  }
  groundTestSample(stamp, p, t, true);
  if (!currentPressureAnomaly) checkLanding();
}

// ---------- SIMULATION ----------

void updateSimulation() {
  if (!simulating) return;

  float s =
    (millis() - simStartMs) / 1000.0f;

  if (s < 10) {
    simAlt = s * 8;
    simSpeed = 8;
  } else if (s < 20) {
    simAlt = (20 - s) * 8;
    simSpeed = -8;
  } else {
    simulating = false;
    simAlt = 0;
    simSpeed = 0;
    message = "Simulation completed";
  }
}

// ---------- LED EFFECTS ----------

uint32_t wheel(byte pos) {
  pos = 255 - pos;

  if (pos < 85)
    return neo.Color(255 - pos * 3, 0, pos * 3);

  if (pos < 170) {
    pos -= 85;
    return neo.Color(0, pos * 3, 255 - pos * 3);
  }

  pos -= 170;
  return neo.Color(pos * 3, 255 - pos * 3, 0);
}

void updateLights() {
  static uint32_t last = 0;

  if (millis() - last < 40) return;
  last = millis();

  uint32_t now = millis();
  bool blink = (now / 450) % 2 == 0;
  bool fast = (now / 150) % 2 == 0;

  uint32_t c = 0;
  int brightness = 45;

  if (!sensorOK || sdFault) {
    c = fast ? neo.Color(255, 0, 0) : 0;
  }
  else if (criticalBattery()) {
    c = fast
      ? neo.Color(255, 0, 0)
      : neo.Color(255, 115, 0);
  }
  else if (updateInProgress) {
    c = fast ? neo.Color(0, 190, 255) : 0;
  }
  else if (recording) {
    c = blink ? neo.Color(0, 255, 0) : 0;
  }
  else if (armed) {
    c = neo.Color(255, 190, 0);
  }
  else if (calibrating) {
    c = blink
      ? neo.Color(0, 110, 255)
      : neo.Color(0, 20, 55);
  }
  else if (manualLed) {
    brightness = ledBrightness;

    switch (ledEffect) {
      case 0:
        c = neo.Color(ledR, ledG, ledB);
        break;
      case 1:
        c = blink ? neo.Color(ledR, ledG, ledB) : 0;
        break;
      case 2: {
        float f = (sinf(now * 0.004f) + 1) / 2;
        c = neo.Color(
          ledR * f, ledG * f, ledB * f
        );
        break;
      }
      case 3:
        c = wheel((byte)(now / 12));
        break;
      default:
        c = 0;
    }
  }
  else if (simulating) {
    c = blink ? neo.Color(170, 0, 255) : 0;
  }
  else if (calibrated) {
    c = neo.Color(0, 180, 45);
  }
  else {
    c = blink ? neo.Color(0, 110, 255) : 0;
  }

  neo.setBrightness(brightness);
  neo.setPixelColor(0, c);
  neo.show();

  bool fault =
    !sensorOK || sdFault || criticalBattery();

  digitalWrite(
    RED_LED,
    fault ? fast : recording ? blink : LOW
  );
}

// ---------- SAFE DEEP SLEEP ----------

void enterDeepSleep() {
  Serial.println("[POWER] Entering deep sleep");

  // A requested shutdown should not lose the log.
  if (recording)
    stopFlight("Shutdown requested");

  armed = false;
  simulating = false;
  calibrating = false;

  // Clear both software-controlled LEDs.
  neo.setBrightness(0);
  neo.setPixelColor(0, 0);
  neo.show();

  digitalWrite(RED_LED, LOW);

  // Disconnect network first.
  web.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  wifiRunning = false;

  // Shut down STEMMA QT and NeoPixel regulator.
  // BMP388 on QT will lose power.
  pinMode(NEOPIXEL_I2C_POWER, OUTPUT);
  digitalWrite(NEOPIXEL_I2C_POWER, LOW);

  delay(80);
  Serial.flush();

  // No timed or GPIO wake source configured.
  // Press the Feather's RESET button to reboot.
  esp_sleep_disable_wakeup_source(
    ESP_SLEEP_WAKEUP_ALL
  );
  esp_deep_sleep_start();

  // Never reached under normal circumstances.
}

// ---------- PHONE FIRMWARE UPDATE ----------

void handleFirmwareUpload() {
  HTTPUpload &upload = web.upload();

  if (upload.status == UPLOAD_FILE_START) {
    updateSucceeded = false;
    updateError = "";
    updateBytes = 0;
    updateHeaderChecked = false;

    String uploadName = upload.filename;
    uploadName.toLowerCase();

    if (!controllerMatches()) {
      updateError = "View-only: take control before updating";
      return;
    }

    if (armed || recording || calibrating ||
        simulating || shutdownPending || criticalBattery()) {
      updateError =
        "Update blocked: disarm, stop recording and check battery";
      return;
    }

    if (!uploadName.endsWith(".bin")) {
      updateError = "Choose the GitHub-built .ino.bin file";
      return;
    }

    if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) {
      updateError =
        "Update could not start (error " +
        String(Update.getError()) + ")";
      return;
    }

    updateInProgress = true;
    manualLed = false;
    message = "Receiving firmware update";
  }
  else if (upload.status == UPLOAD_FILE_WRITE) {
    if (!updateInProgress) return;

    if (!updateHeaderChecked) {
      updateHeaderChecked = true;
      if (!upload.currentSize || upload.buf[0] != 0xE9) {
        updateError = "Invalid ESP32 firmware image header";
        Update.abort(); updateInProgress = false; return;
      }
    }

    size_t written = Update.write(
      upload.buf, upload.currentSize
    );

    if (written != upload.currentSize) {
      updateError =
        "Firmware write failed (error " +
        String(Update.getError()) + ")";
      Update.abort();
      updateInProgress = false;
      return;
    }

    updateBytes += written;
  }
  else if (upload.status == UPLOAD_FILE_END) {
    if (!updateInProgress) return;

    if (updateBytes < 200000) {
      updateError = "Firmware image is unexpectedly small";
      Update.abort(); updateInProgress = false; return;
    }

    if (Update.end(true)) {
      updateSucceeded = true;
      message = "Firmware verified; restarting";
    } else {
      updateError =
        "Firmware verification failed (error " +
        String(Update.getError()) + ")";
      updateInProgress = false;
    }
  }
  else if (upload.status == UPLOAD_FILE_ABORTED) {
    if (updateInProgress) Update.abort();
    updateInProgress = false;
    updateError = "Firmware upload was cancelled";
  }
}

void finishFirmwareUpload() {
  if (!updateSucceeded) {
    updateInProgress = false;
    respond(
      false,
      updateError.length()
        ? updateError : "Firmware upload failed"
    );
    return;
  }

  String result =
    "{\"success\":true,\"message\":\"Firmware installed; "
    "restarting\",\"bytes\":" + String(updateBytes) + "}";

  web.sendHeader("Cache-Control", "no-store");
  web.send(200, "application/json", result);
  delay(750);
  ESP.restart();
}

// ---------- STATUS JSON ----------

void statusAPI() {
  bool autoGranted = maybeAutoGrantControl();
  bool thisController = controllerMatches();
  bool controlAvailable = controlLeaseExpired() || thisController;

  String s;
  s.reserve(4300);

  s = "{";

  s += "\"version\":\"" + String(FIRMWARE_VERSION) + "\"";
  s += ",\"build\":\"" + je(String(FIRMWARE_BUILD)) + "\"";
  s += ",\"controller\":" + jb(thisController);
  s += ",\"autoControl\":" + jb(autoGranted || (thisController && WiFi.softAPgetStationNum() == 1));
  s += ",\"connectedDevices\":" + String(WiFi.softAPgetStationNum());
  s += ",\"controlAvailable\":" + jb(controlAvailable);
  s += ",\"controlLeaseSeconds\":";
  s += thisController
    ? String((CONTROL_LEASE_MS -
        min((uint32_t)(millis() - controllerSeenMs), CONTROL_LEASE_MS)) / 1000)
    : "0";
  s += ",\"sensor\":" + jb(sensorOK);
  s += ",\"reading\":" + jb(readingOK);
  s += ",\"sensorFrozen\":" + jb(sensorFrozen);
  s += ",\"calibrated\":" + jb(calibrated);
  s += ",\"calibrating\":" + jb(calibrating);
  s += ",\"calStep\":" + String(calStep);
  s += ",\"calValid\":" + String(calValid);
  s += ",\"calTarget\":" + String(CAL_TARGET_SAMPLES);
  s += ",\"calProgress\":" + String(calibrationProgress());
  s += ",\"calPhase\":" + String(calPhase);
  s += ",\"calNoiseM\":" + jn(calNoiseM, 3);
  s += ",\"calDriftPaPerSec\":" + jn(calDriftPaPerSec, 3);
  s += ",\"calStatus\":\"" + je(calStatus) + "\"";
  s += ",\"calCount\":" + String(calCount);
  s += ",\"calAge\":";
  s += calTimeMs
    ? String((millis() - calTimeMs) / 1000)
    : "null";

  s += ",\"altitude\":" + jn(altM);
  s += ",\"rawAltitude\":" + jn(rawAltM);
  s += ",\"flightAltitude\":" + jn(flightAltM);
  s += ",\"speed\":" + jn(speedMps);
  s += ",\"peak\":" + jn(peakM);
  s += ",\"pressure\":" + jn(pressurePa);
  s += ",\"temperature\":" + jn(tempC);
  s += ",\"baseline\":" + jn(groundPa);
  s += ",\"simAltitude\":" + jn(simAlt);
  s += ",\"simSpeed\":" + jn(simSpeed);

  s += ",\"battery\":" + jn(batteryMv / 1000.0f, 3);
  s += ",\"batteryPercent\":" +
    String(batteryPercent());
  s += ",\"batteryPresent\":" + jb(batteryPresent);
  s += ",\"batteryLow\":" + jb(lowBattery());
  s += ",\"batteryCritical\":" + jb(criticalBattery());

  s += ",\"sd\":" + jb(sdOK);
  s += ",\"sdVerified\":" + jb(sdVerified);
  s += ",\"sdFault\":" + jb(sdFault);
  s += ",\"sdStatus\":\"" + je(sdStatus) + "\"";

  s += ",\"armed\":" + jb(armed);
  s += ",\"recording\":" + jb(recording);
  s += ",\"launchWatch\":" + jb(launchWatch);
  s += ",\"wifiLaunchPending\":" + jb(wifiLaunchDisconnectPending);
  s += ",\"launched\":" + jb(launched);
  s += ",\"simulation\":" + jb(simulating);
  s += ",\"shutdownPending\":" + jb(shutdownPending);
  s += ",\"updateInProgress\":" + jb(updateInProgress);

  s += ",\"good\":" + String(goodReads);
  s += ",\"bad\":" + String(badReads);
  s += ",\"freshness\":";
  s += lastReadMs
    ? String(millis() - lastReadMs) : "null";

  s += ",\"samples\":" + String(samples);
  s += ",\"invalid\":" + String(invalidSamples);
  s += ",\"missed\":" + String(missedSamples);
  s += ",\"sdErrors\":" + String(writeErrors);
  s += ",\"lastFlushUs\":" + String(lastFlushUs);
  s += ",\"maximumFlushUs\":" + String(maximumFlushUs);
  s += ",\"flushHeartbeat\":" + String(flushHeartbeat);
  s += ",\"integrityStatus\":\"" + je(integrityStatus) + "\"";
  s += ",\"recoveredFlights\":" + String(recoveredFlights);
  s += ",\"recoveredSamples\":" + String(lastRecoveredSamples);
  s += ",\"resetReason\":\"" + je(resetReasonText) + "\"";
  s += ",\"previousPowerEvidence\":\"" + je(previousPowerEvidence) + "\"";
  s += ",\"sampleRate\":" + String(sampleRateHz);
  s += ",\"effectiveSampleRate\":" + String(effectiveSampleRateHz);
  s += ",\"adaptiveRateStatus\":\"" + je(adaptiveRateStatus) + "\"";
  s += ",\"recordingLimitMinutes\":" + String(recordingLimitMin);
  s += ",\"groundTestActive\":" + jb(groundTestActive);
  s += ",\"groundTestResult\":\"" + je(groundTestResult) + "\"";
  s += ",\"groundTestDetail\":\"" + je(groundTestDetail) + "\"";
  s += ",\"prepareActive\":" + jb(prepareActive);
  s += ",\"prepareReady\":" + jb(prepareReady);
  s += ",\"prepareProgress\":" + String(prepareProgress);
  s += ",\"prepareStatus\":\"" + je(prepareStatus) + "\"";
  s += ",\"calibrationFresh\":" + jb(calibrationFresh());
  s += ",\"calibrationFreshness\":\"" + je(calibrationFreshnessReason()) + "\"";
  s += ",\"preflightReport\":\"" + je(preflightReportFilename) + "\"";
  uint64_t estimatedBytes = 32ULL + (uint64_t)sampleRateHz * recordingLimitMin * 60ULL * 16ULL;
  s += ",\"estimatedFlightBytes\":" + u64Text(estimatedBytes);
  s += ",\"estimatedFlightsRemaining\":" + String(estimatedBytes ? sdFree() / estimatedBytes : 0);
  s += ",\"sdProfileWriteUs\":" + String(sdProfileWriteUs);
  s += ",\"sdSafeRate\":" + String(sdSafeRateHz);
  s += ",\"environmentalWarning\":\"" + je(environmentalWarning) + "\"";
  s += ",\"maintenanceReminder\":\"" + je(maintenanceReminder) + "\"";
  s += ",\"totalFlights\":" + String(totalFlights);
  s += ",\"totalSdTests\":" + String(totalSdTests);
  s += ",\"emergencyBytes\":" + String(emergencyUsed);
  s += ",\"pressureAnomalies\":" + String(pressureAnomalyCount);
  s += ",\"sensorOutage\":" + jb(sensorOutageActive);
  s += ",\"sensorOutageCount\":" + String(sensorOutageCount);
  s += ",\"missionKind\":\""+je(missionKind)+"\"";
  s += ",\"nextFlightNumber\":"+String(nextFlightNumber);
  s += ",\"missionNumber\":" + String(currentMissionNumber ? currentMissionNumber : nextFlightNumber);
  s += ",\"cardSequence\":" + String(currentCardSequence ? currentCardSequence : nextCardSequence);
  s += ",\"cardId\":\"" + je(cardId) + "\"";
  s += ",\"prebufferSamples\":" + String(prebufferCount);
  s += ",\"prebufferCapacity\":" + String(min(PREBUFFER_CAPACITY, (uint16_t)(sampleRateHz * 3)));
  s += ",\"activePhase\":\"" + je(activePhase) + "\"";
  s += ",\"phaseConfidence\":" + String(phaseConfidence);

  float duration =
    (recording
      ? millis() - flightStartMs
      : finalDurationMs) / 1000.0f;

  s += ",\"duration\":" + jn(duration, 1);
  s += ",\"file\":\"" + je(filename) + "\"";
  s += ",\"flightLabel\":\"" + je(flightLabel) + "\"";
  s += ",\"motorLabel\":\"" + je(motorLabel) + "\"";
  s += ",\"minimumPressure\":" + jn(minimumPressurePa);
  s += ",\"temperatureMin\":" + jn(minimumTemperatureC);
  s += ",\"temperatureMax\":" + jn(maximumTemperatureC);
  s += ",\"finish\":\"" + je(finishReason) + "\"";

  s += ",\"message\":\"" + je(message) + "\"";
  s += ",\"error\":\"" + je(errorText) + "\"";
  s += ",\"ledManual\":" + jb(manualLed);
  s += ",\"uptime\":" + String(millis() / 1000);
  s += "}";

  web.sendHeader("Cache-Control", "no-store");
  web.send(200, "application/json", s);
}

// ---------- SD FILE OPERATIONS ----------

bool validFile(String n) {
  if (n.length() != 11 ||
      !n.startsWith("FLT") ||
      !n.endsWith(".BIN"))
    return false;

  for (int i = 3; i < 7; i++)
    if (!isDigit(n[i])) return false;

  return true;
}

String companionFile(String n) {
  n.replace(".BIN", ".TXT");
  return n;
}

String protectionFile(String n) {
  n.replace(".BIN", ".LCK");
  return n;
}

uint16_t read16(const uint8_t *p) {
  return (uint16_t)p[0] |
    ((uint16_t)p[1] << 8);
}

uint32_t read32(const uint8_t *p) {
  return (uint32_t)p[0] |
    ((uint32_t)p[1] << 8) |
    ((uint32_t)p[2] << 16) |
    ((uint32_t)p[3] << 24);
}

float readFloat(const uint8_t *p) {
  uint32_t bits = read32(p);
  float value;
  memcpy(&value, &bits, sizeof(value));
  return value;
}

void filesAPI() {
  if (!sdOK || recording) {
    respond(false, "SD busy or unavailable");
    return;
  }

  File root = SD.open("/");
  if (!root) {
    respond(false, "Cannot open SD root");
    return;
  }

  String output = "{\"files\":[";
  bool first = true;

  File f = root.openNextFile();

  while (f) {
    String n = f.name();
    int slash = n.lastIndexOf('/');
    if (slash >= 0) n = n.substring(slash + 1);

    if (!f.isDirectory() && validFile(n)) {
      if (!first) output += ",";
      first = false;
      output += "{\"name\":\"" + n + "\",\"size\":";
      output += String((uint32_t)f.size());
      output += ",\"protected\":" + jb(SD.exists("/" + protectionFile(n))) + "}";
    }

    f.close();
    f = root.openNextFile();
  }

  root.close();

  output += "],\"total\":";
  output += String((uint32_t)SD.totalBytes());
  output += ",\"used\":";
  output += String((uint32_t)SD.usedBytes());
  output += "}";

  web.send(200, "application/json", output);
}

void downloadAPI() {
  if (!sdOK || recording) {
    web.send(409, "text/plain", "SD busy");
    return;
  }

  String n = web.arg("name");

  if (!validFile(n)) {
    web.send(400, "text/plain", "Invalid filename");
    return;
  }

  File f = SD.open("/" + n, FILE_READ);
  if (!f) {
    web.send(404, "text/plain", "File not found");
    return;
  }

  web.sendHeader(
    "Content-Disposition",
    "attachment; filename=\"" + n + "\""
  );
  web.sendHeader("X-Green-Ion-Bytes", String((uint32_t)f.size()));
  web.sendHeader("X-Green-Ion-RKT4-Version", "2");

  web.streamFile(f, "application/octet-stream");
  f.close();
}

void preflightAPI() {
  if (!sdOK || recording || !preflightReportFilename.length()) {
    web.send(404, "text/plain", "No preflight report available");
    return;
  }
  String path = "/" + preflightReportFilename;
  File f = SD.open(path, FILE_READ);
  if (!f) { web.send(404, "text/plain", "Preflight report not found"); return; }
  web.sendHeader("Content-Disposition",
    "attachment; filename=\"" + preflightReportFilename + "\"");
  web.streamFile(f, "text/plain");
  f.close();
}

void csvAPI() {
  if (!sdOK || recording) {
    web.send(409, "text/plain", "SD busy");
    return;
  }

  String n = web.arg("name");
  if (!validFile(n)) {
    web.send(400, "text/plain", "Invalid filename");
    return;
  }

  File f = SD.open("/" + n, FILE_READ);
  if (!f) {
    web.send(404, "text/plain", "File not found");
    return;
  }

  uint8_t header[32];
  if (f.read(header, sizeof(header)) != sizeof(header) ||
      memcmp(header, "RKT4", 4) != 0 ||
      read16(header + 4) != 2 ||
      read16(header + 6) != 32 ||
      read16(header + 8) != 16 ||
      (read16(header + 10) != 10 && read16(header + 10) != 25 && read16(header + 10) != 50)) {
    f.close();
    web.send(422, "text/plain", "Invalid RKT4 file");
    return;
  }

  float baseline = readFloat(header + 12);
  if (!isfinite(baseline) || baseline < 30000 || baseline > 120000)
    baseline = NAN;

  String csvName = n;
  csvName.replace(".BIN", ".CSV");
  web.sendHeader(
    "Content-Disposition",
    "attachment; filename=\"" + csvName + "\""
  );
  web.setContentLength(CONTENT_LENGTH_UNKNOWN);
  web.send(200, "text/csv", "");
  web.sendContent(
    "time_s,pressure_pa,temperature_c,battery_v,valid,"
    "altitude_m,vertical_speed_mps,estimated_acceleration_mps2,estimated_vertical_apparent_g\r\n"
  );

  uint8_t record[16];
  uint32_t firstStamp = 0;
  uint32_t previousStamp = 0;
  float previousAltitude = NAN;
  float previousVelocity = NAN;
  uint32_t previousVelocityStamp = 0;

  while (f.available() >= 16) {
    if (f.read(record, sizeof(record)) != sizeof(record)) break;

    uint32_t stamp = read32(record);
    float pressure = readFloat(record + 4);
    float temperature = readFloat(record + 8);
    uint16_t millivolts = read16(record + 12);
    bool valid = (read16(record + 14) & 1) != 0;

    if (!firstStamp) firstStamp = stamp;

    float relativeTime =
      (uint32_t)(stamp - firstStamp) / 1000000.0f;
    float altitude = NAN;
    float velocity = NAN;

    if (valid && isfinite(baseline) &&
        isfinite(pressure) && pressure > 0) {
      altitude = 44330.0f *
        (1 - powf(pressure / baseline, 0.190294957f));

      if (isfinite(previousAltitude) && previousStamp != stamp) {
        float dt =
          (uint32_t)(stamp - previousStamp) / 1000000.0f;
        if (dt > 0) velocity = (altitude - previousAltitude) / dt;
      }

      previousAltitude = altitude;
      previousStamp = stamp;
    }

    float acceleration = NAN;
    float apparentG = NAN;
    if (isfinite(velocity) && isfinite(previousVelocity) && stamp != previousVelocityStamp) {
      float dt = (uint32_t)(stamp - previousVelocityStamp) / 1000000.0f;
      if (dt > 0.01f && dt < 2.0f) {
        acceleration = (velocity - previousVelocity) / dt;
        apparentG = 1.0f + acceleration / 9.80665f;
      }
    }
    if (isfinite(velocity)) { previousVelocity = velocity; previousVelocityStamp = stamp; }

    String row;
    row.reserve(190);
    row += String(relativeTime, 4) + ",";
    row += jn(pressure, 2) + ",";
    row += jn(temperature, 2) + ",";
    row += String(millivolts / 1000.0f, 3) + ",";
    row += valid ? "1," : "0,";
    row += jn(altitude, 3) + ",";
    row += jn(velocity, 3) + ",";
    row += jn(acceleration, 3) + ",";
    row += jn(apparentG, 3) + "\r\n";
    web.sendContent(row);
    yield();
  }

  f.close();
  web.sendContent("");
}

void metadataAPI() {
  if (!sdOK || recording) {
    web.send(409, "text/plain", "SD busy");
    return;
  }

  String n = web.arg("name");
  if (!validFile(n)) {
    web.send(400, "text/plain", "Invalid filename");
    return;
  }

  String metaName = companionFile(n);
  File f = SD.open("/" + metaName, FILE_READ);
  if (!f) {
    web.send(404, "text/plain", "Summary not found");
    return;
  }

  web.sendHeader(
    "Content-Disposition",
    "attachment; filename=\"" + metaName + "\""
  );
  web.streamFile(f, "text/plain");
  f.close();
}

// Serve saved flight statistics inline to the dashboard, without downloading.
void flightStatsAPI() {
  if (!sdOK || recording) {
    web.send(409, "text/plain", "SD busy or unavailable");
    return;
  }
  String n = web.arg("name");
  if (!validFile(n)) {
    web.send(400, "text/plain", "Invalid flight filename");
    return;
  }
  File f = SD.open("/" + companionFile(n), FILE_READ);
  if (!f) {
    web.send(404, "text/plain", "Flight summary unavailable");
    return;
  }
  web.sendHeader("Cache-Control", "no-store");
  web.streamFile(f, "text/plain");
  f.close();
}

void deleteAPI() {
  if (!requireControl()) return;

  if (!sdOK || recording || armed) {
    respond(false, "SD busy");
    return;
  }

  String n = web.arg("name");
  if (!validFile(n)) {
    respond(false, "Invalid filename");
    return;
  }

  if (SD.exists("/" + protectionFile(n))) {
    respond(false, "Flight is protected; unprotect it first");
    return;
  }

  bool ok = SD.remove("/" + n);
  SD.remove("/" + companionFile(n));
  respond(ok, ok ? "Flight deleted" : "Delete failed");
}

void protectAPI() {
  if (!requireControl()) return;
  if (!sdOK || recording || armed) { respond(false, "SD busy"); return; }
  String n = web.arg("name");
  if (!validFile(n) || !SD.exists("/" + n)) { respond(false, "Invalid flight"); return; }
  String lock = "/" + protectionFile(n);
  bool protect = web.arg("value") != "0";
  bool ok = false;
  if (protect) {
    File f = SD.open(lock, FILE_WRITE);
    if (f) { f.println("Protected by GREEN ION V11"); f.close(); ok = true; }
  } else {
    ok = !SD.exists(lock) || SD.remove(lock);
  }
  respond(ok, ok ? (protect ? "Flight protected" : "Flight unprotected") : "Protection change failed");
}

void noteAPI() {
  if (!requireControl()) return;
  if (!sdOK || recording || armed) { respond(false, "SD busy"); return; }
  String n = web.arg("name");
  String note = cleanLabel(web.arg("note"), 120);
  if (!validFile(n) || !SD.exists("/" + n) || !note.length()) {
    respond(false, "Choose a flight and enter a note"); return;
  }
  File f = SD.open("/" + companionFile(n), FILE_APPEND);
  if (!f) { respond(false, "Could not open flight summary"); return; }
  f.println("user_note=" + note);
  f.flush();
  f.close();
  respond(true, "Note saved with flight");
}

void diagnosticsAPI() {
  String report;
  report.reserve(1600);
  report += "GREEN ION DIAGNOSTIC REPORT\r\n";
  report += "firmware=" + String(FIRMWARE_VERSION) + "\r\n";
  report += "build=" + String(FIRMWARE_BUILD) + "\r\n";
  report += "reset_reason=" + resetReasonText + "\r\n";
  report += "uptime_s=" + String(millis() / 1000UL) + "\r\n";
  report += "sensor_detected=" + String(sensorOK ? "yes" : "no") + "\r\n";
  report += "sensor_reading=" + String(readingOK ? "yes" : "no") + "\r\n";
  report += "sensor_frozen=" + String(sensorFrozen ? "yes" : "no") + "\r\n";
  report += "good_reads=" + String(goodReads) + "\r\n";
  report += "bad_reads=" + String(badReads) + "\r\n";
  report += "battery_v=" + String(batteryMv / 1000.0f, 3) + "\r\n";
  report += "sd_detected=" + String(sdOK ? "yes" : "no") + "\r\n";
  report += "sd_verified=" + String(sdVerified ? "yes" : "no") + "\r\n";
  report += "sd_status=" + sdStatus + "\r\n";
  report += "sd_free_bytes=" + u64Text(sdFree()) + "\r\n";
  report += "sample_rate_hz=" + String(sampleRateHz) + "\r\n";
  report += "recording_limit_minutes=" + String(recordingLimitMin) + "\r\n";
  report += "max_flush_us=" + String(maximumFlushUs) + "\r\n";
  report += "flush_heartbeat=" + String(flushHeartbeat) + "\r\n";
  report += "ground_test=" + groundTestResult + "\r\n";
  report += "ground_test_detail=" + groundTestDetail + "\r\n";
  report += "integrity=" + integrityStatus + "\r\n";
  report += "last_error=" + errorText + "\r\n";
  web.sendHeader("Content-Disposition", "attachment; filename=GREEN-ION-DIAGNOSTICS.txt");
  web.send(200, "text/plain", report);
}

// =========================================================
// EMBEDDED PHONE DASHBOARD
// =========================================================

#include "Dashboard.h"

// ---------- WI-FI ----------

void startWifi() {
  if (wifiRunning) return;

  WiFi.mode(WIFI_AP);
  wifiRunning = WiFi.softAP(AP_NAME, AP_PASS);

  if (wifiRunning) {
    web.begin();
    Serial.println("http://192.168.4.1");
  }
}

void stopWifi() {
  if (!wifiRunning) return;

  web.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  wifiRunning = false;
}

// ---------- API ROUTES ----------

void configurationBackupAPI() {
  String json = "{\"format\":1,\"firmware\":\"" + String(FIRMWARE_VERSION) +
    "\",\"sampleRate\":" + String(sampleRateHz) +
    ",\"recordingLimit\":" + String(recordingLimitMin) +
    ",\"motor\":\"" + je(motorLabel) + "\",\"nextMission\":" +
    String(nextFlightNumber) + ",\"ledBrightness\":" + String(ledBrightness) + "}";
  web.sendHeader("Content-Disposition", "attachment; filename=\"GREENION-CONFIG.json\"");
  web.send(200, "application/json", json);
}

void configurationRestoreAPI() {
  if (!requireControl()) return;
  if (armed || recording || calibrating) { respond(false, "Recorder busy"); return; }
  uint16_t rate = web.arg("sampleRate").toInt();
  uint8_t limit = web.arg("recordingLimit").toInt();
  if (!setSampleRate(rate) || !(limit == 2 || limit == 5 || limit == 10 || limit == 20)) {
    respond(false, "Invalid configuration values"); return;
  }
  recordingLimitMin = limit; nv.putUChar("limitmin", limit);
  String motor = cleanLabel(web.arg("motor"), 24); if (motor.length()) motorLabel = motor;
  ledBrightness = constrain(web.arg("ledBrightness").toInt(), 1, 255);
  respond(true, "Configuration restored; pressure calibration was intentionally not restored");
}

void integrityScanAPI() {
  if (!sdOK || recording) { respond(false, "SD unavailable or busy"); return; }
  String n = web.arg("name");
  if (!validFile(n)) { respond(false, "Invalid flight filename"); return; }
  File f = SD.open("/" + n, FILE_READ);
  if (!f) { respond(false, "Flight not found"); return; }
  bool header = validRKT4Header(f); uint32_t size = f.size();
  uint32_t complete = size >= 32 ? (size - 32) / 16 : 0;
  uint32_t tail = size >= 32 ? (size - 32) % 16 : size;
  uint32_t implausible = 0; uint8_t rec[16];
  if (header) while (f.available() >= 16) {
    if (f.read(rec, 16) != 16) break;
    float p = readFloat(rec + 4), t = readFloat(rec + 8);
    if (!isfinite(p) || p < 30000 || p > 120000 || !isfinite(t) || t < -50 || t > 100) implausible++;
  }
  f.close();
  String json = "{\"success\":true,\"header\":" + jb(header) +
    ",\"completeRecords\":" + String(complete) + ",\"trailingBytes\":" +
    String(tail) + ",\"implausibleRecords\":" + String(implausible) +
    ",\"result\":\"" + String(header && !tail ? "PASS" : "REVIEW") + "\"}";
  web.send(200, "application/json", json);
}

void rollbackAPI() {
  if (!requireControl()) return;
  if (armed || recording || calibrating || criticalBattery()) {
    respond(false, "Rollback blocked while busy or battery critical"); return;
  }
  const esp_partition_t *running = esp_ota_get_running_partition();
  const esp_partition_t *other = esp_ota_get_next_update_partition(running);
  esp_app_desc_t description;
  if (!other || esp_ota_get_partition_description(other, &description) != ESP_OK) {
    respond(false, "No valid previous OTA image is available"); return;
  }
  if (esp_ota_set_boot_partition(other) != ESP_OK) {
    respond(false, "Could not select previous firmware"); return;
  }
  respond(true, "Previous firmware selected; restarting"); delay(500); ESP.restart();
}

bool armRecorder() {
  bool ready = !armed && !recording && !simulating && !calibrating &&
    !groundTestActive && !shutdownPending && sensorOK && readingOK &&
    calibrated && calibrationFresh() && prepareReady && sdOK && sdVerified &&
    !sdFault && !criticalBattery() && sdFree() >= 1048576ULL;
  if (!ready) {
    errorText = calibrationFresh() ?
      "Run PREPARE FOR LAUNCH successfully first" : calibrationFreshnessReason();
    return false;
  }
  armed = true;
  resetPrebuffer();
  activePhase = "armed";
  manualLed = false;
  message = "Recorder armed";
  errorText = "";
  return true;
}

void setupRoutes() {
  web.on("/", HTTP_GET, []() {
    web.send_P(200, "text/html", PAGE);
  });

  web.on("/api/status", HTTP_GET, statusAPI);
  web.on("/api/config", HTTP_GET, configurationBackupAPI);
  web.on("/api/config", HTTP_POST, configurationRestoreAPI);
  web.on("/api/integrity", HTTP_GET, integrityScanAPI);
  web.on("/api/rollback", HTTP_POST, rollbackAPI);
  web.on("/api/manifest", HTTP_GET, []() {
    if (!sdOK || recording) { web.send(409, "text/plain", "SD busy"); return; }
    rebuildFlightManifest(); File f = SD.open("/MANIFEST.CSV", FILE_READ);
    if (!f) { web.send(404, "text/plain", "Manifest unavailable"); return; }
    web.sendHeader("Content-Disposition", "attachment; filename=\"GREEN-ION-MANIFEST.csv\"");
    web.streamFile(f, "text/csv"); f.close();
  });

  web.on("/api/control", HTTP_POST, []() {
    String token = web.arg("token");
    bool valid = token.length() >= 12 && token.length() <= 64;

    for (size_t i = 0; i < token.length() && valid; i++)
      valid = isAlphaNumeric(token[i]);

    if (!valid) {
      respond(false, "Invalid controller token");
      return;
    }

    if (!controlLeaseExpired() && token != controllerToken) {
      respond(false, "Another phone currently has control");
      return;
    }

    controllerToken = token;
    controllerSeenMs = millis();
    respond(true, "This phone now controls Green Ion");
  });

  web.on("/api/release", HTTP_POST, []() {
    if (!controllerMatches(false)) {
      respond(false, "This phone does not hold control");
      return;
    }
    controllerToken = "";
    controllerSeenMs = 0;
    respond(true, "Control released");
  });

  web.on("/api/mission", HTTP_POST, []() {
    if (!requireControl()) return;
    if (armed || recording || calibrating || shutdownPending) {
      respond(false, "Flight details locked while device is busy");
      return;
    }

    String kind=web.arg("kind");
    if(kind!="flight" && kind!="test"){respond(false,"Select Flight or Test");return;}
    missionKind=kind;
    flightLabel = "Mission " + String(nextFlightNumber) +
      " · Card Flight " + String(nextCardSequence);
    String newMotor=cleanLabel(web.arg("motor"),24);
    motorLabel=newMotor.length()?newMotor:"Unspecified";

    respond(true, "Flight details saved");
  });

  web.on("/api/settings", HTTP_POST, []() {
    if (!requireControl()) return;
    if (armed || recording || calibrating || shutdownPending) {
      respond(false, "Sample rate locked while device is busy");
      return;
    }

    uint16_t requested = (uint16_t)web.arg("rate").toInt();
    uint8_t requestedLimit = (uint8_t)web.arg("limit").toInt();
    bool validLimit = requestedLimit == 2 || requestedLimit == 5 ||
      requestedLimit == 10 || requestedLimit == 20;
    bool ok = setSampleRate(requested) && validLimit;
    if (ok) {
      recordingLimitMin = requestedLimit;
      nv.putUChar("limitmin", recordingLimitMin);
    }
    respond(
      ok,
      ok
        ? "Saved " + String(sampleRateHz) + " Hz and " + String(recordingLimitMin) + " minute limit"
        : "Choose 10/25/50 Hz and a 2/5/10/20 minute limit"
    );
  });

  web.on("/api/time", HTTP_POST, []() {
    String epoch = web.arg("epoch");
    uint64_t value = strtoull(epoch.c_str(), nullptr, 10);
    if (value < 1600000000000ULL) { respond(false, "Invalid phone time"); return; }
    phoneEpochMs = value;
    phoneEpochSetAtMs = millis();
    phoneUtcOffsetMin = (int16_t)web.arg("offset").toInt();
    respond(true, "Phone date and time synchronized");
  });

  web.on("/api/calibrate", HTTP_POST, []() {
    if (!requireControl()) return;
    bool ok = startCalibration();
    respond(ok, ok ? calStatus :
      "Calibration unavailable: sensor missing or device busy");
  });

  web.on("/api/prepare", HTTP_POST, []() {
    if (!requireControl()) return;
    if (web.hasArg("kind")) {
      String kind = web.arg("kind");
      if (kind == "flight" || kind == "test") missionKind = kind;
    }
    if (web.hasArg("motor")) {
      String cleaned = cleanLabel(web.arg("motor"), 24);
      motorLabel = cleaned.length() ? cleaned : "Unspecified";
    }
    if (web.hasArg("rate")) {
      if (!setSampleRate((uint16_t)web.arg("rate").toInt())) {
        respond(false, "Choose 10, 25, or 50 Hz"); return;
      }
    }
    if (web.hasArg("limit")) {
      uint8_t limit = (uint8_t)web.arg("limit").toInt();
      if (limit != 2 && limit != 5 && limit != 10 && limit != 20) {
        respond(false, "Choose a 2, 5, 10, or 20 minute limit"); return;
      }
      recordingLimitMin = limit;
      nv.putUChar("limitmin", recordingLimitMin);
    }
    if (web.hasArg("epoch")) {
      uint64_t value = strtoull(web.arg("epoch").c_str(), nullptr, 10);
      if (value >= 1600000000000ULL) {
        phoneEpochMs = value;
        phoneEpochSetAtMs = millis();
        phoneUtcOffsetMin = (int16_t)web.arg("offset").toInt();
      }
    }
    flightLabel = "Mission " + String(nextFlightNumber) +
      " · Card Flight " + String(nextCardSequence);
    bool ok = startPrepareAll();
    respond(ok, ok ? "Preparation started" : "Preparation unavailable while device is busy");
  });

  web.on("/api/arm", HTTP_POST, []() {
    if (!requireControl()) return;
    bool ready = armRecorder();
    respond(ready, ready ? message : errorText);
  });

  web.on("/api/start", HTTP_POST, []() {
    if (!requireControl()) return;
    bool ok = startLaunchWatch();
    respond(ok, ok ? message : errorText);
  });

  web.on("/api/launchwatch", HTTP_POST, []() {
    if (!requireControl()) return;
    bool ok = startLaunchWatch();
    respond(ok, ok ? message : errorText);
  });

  web.on("/api/armwatch", HTTP_POST, []() {
    if (!requireControl()) return;
    bool wasArmed = armed;
    bool ok = wasArmed || armRecorder();
    if (ok) ok = startLaunchWatch();
    if (!ok && !wasArmed) armed = false;
    respond(ok, ok ? message : errorText);
  });

  web.on("/api/cancel", HTTP_POST, []() {
    if (!requireControl()) return;
    if (!launchWatch && !armed) { respond(false, "Nothing to cancel"); return; }
    cancelLaunchWatch();
    respond(true, message);
  });

  web.on("/api/stop", HTTP_POST, []() {
    if (!requireControl()) return;
    if (launchWatch && !recording) {
      cancelLaunchWatch();
      respond(true, message);
      return;
    }
    if (!recording) {
      respond(false, "Not recording or watching for launch");
      return;
    }

    stopFlight("Manual stop");
    respond(!sdFault, message);
  });

  web.on("/api/disconnect", HTTP_POST, []() {
    if (!requireControl()) return;
    if (!recording) {
      respond(false, "Record first");
      return;
    }

    respond(true, "Wi-Fi disconnecting");
    delay(200);
    stopWifi();
  });

  web.on("/api/sdtest", HTTP_POST, []() {
    if (!requireControl()) return;
    bool ok = verifySD();
    respond(ok, sdStatus);
  });

  web.on("/api/groundtest", HTTP_POST, []() {
    if (!requireControl()) return;
    bool ok = startGroundTest();
    respond(ok, ok ? "10-second ground test started" : errorText);
  });

  web.on("/api/simulate", HTTP_POST, []() {
    if (!requireControl()) return;
    if (armed || recording || calibrating ||
        shutdownPending) {
      respond(false, "Recorder busy");
      return;
    }

    simulating = true;
    simStartMs = millis();
    simAlt = 0;
    simSpeed = 0;
    respond(true, "Simulation started");
  });

  web.on("/api/simstop", HTTP_POST, []() {
    if (!requireControl()) return;
    simulating = false;
    simAlt = 0;
    simSpeed = 0;
    respond(true, "Simulation stopped");
  });

  web.on("/api/led", HTTP_POST, []() {
    if (!requireControl()) return;
    if (!sensorOK || sdFault || armed ||
        recording || shutdownPending ||
        criticalBattery()) {
      respond(false, "LED warning override active");
      return;
    }

    manualLed = web.arg("manual") == "1";

    if (web.hasArg("effect"))
      ledEffect = constrain(
        web.arg("effect").toInt(), 0, 4
      );
    if (web.hasArg("brightness"))
      ledBrightness = constrain(
        web.arg("brightness").toInt(), 1, 100
      );
    if (web.hasArg("r"))
      ledR = constrain(web.arg("r").toInt(), 0, 255);
    if (web.hasArg("g"))
      ledG = constrain(web.arg("g").toInt(), 0, 255);
    if (web.hasArg("b"))
      ledB = constrain(web.arg("b").toInt(), 0, 255);

    respond(true, "RGB LED settings applied");
  });

  web.on("/api/files", HTTP_GET, filesAPI);
  web.on("/api/download", HTTP_GET, downloadAPI);
  web.on("/api/preflight", HTTP_GET, preflightAPI);
  web.on("/api/csv", HTTP_GET, csvAPI);
  web.on("/api/metadata", HTTP_GET, metadataAPI);
  web.on("/api/flightstats", HTTP_GET, flightStatsAPI);
  web.on("/api/delete", HTTP_POST, deleteAPI);
  web.on("/api/protect", HTTP_POST, protectAPI);
  web.on("/api/note", HTTP_POST, noteAPI);
  web.on("/api/diagnostics", HTTP_GET, diagnosticsAPI);
  web.on(
    "/api/update",
    HTTP_POST,
    finishFirmwareUpload,
    handleFirmwareUpload
  );

  // NEW — software power shutdown.
  web.on("/api/shutdown", HTTP_POST, []() {
    if (!requireControl()) return;
    if (shutdownPending) {
      respond(false, "Shutdown already requested");
      return;
    }

    shutdownPending = true;
    shutdownAtMs = millis() + 1000;

    message = "Deep sleep scheduled; press RESET to wake";

    // Response is sent before deep sleep starts.
    respond(true, message);
  });

  web.onNotFound([]() {
    web.send(
      404, "application/json",
      "{\"success\":false,\"message\":\"Unknown route\"}"
    );
  });
}

// ---------- SETUP ----------

void setup() {
  Serial.begin(115200);
  delay(350);
  resetReasonText = String((int)esp_reset_reason());

  pinMode(RED_LED, OUTPUT);
  digitalWrite(RED_LED, LOW);

  // Enable the Feather's QT and NeoPixel regulator.
  pinMode(NEOPIXEL_I2C_POWER, OUTPUT);
  digitalWrite(NEOPIXEL_I2C_POWER, HIGH);
  delay(100);

  neo.begin();
  neo.clear();
  neo.show();

  analogReadResolution(12);
  analogSetPinAttenuation(
    BATTERY_PIN, ADC_11db
  );
  readBattery();
  nv.begin("greenion",false);
  enableLoopWDT();
  totalFlights = nv.getUInt("totalflights", 0);
  totalSdTests = nv.getUInt("sdtests", 0);
  totalSensorRecoveries = nv.getUInt("sensorrec", 0);
  previousPowerEvidence = "file " + nv.getString("lastfile", "none") +
    " · phase " + nv.getString("lastphase", "none") +
    " · battery " + String(nv.getUShort("lastbat", 0)) + " mV · samples " +
    String(nv.getUInt("lastsample", 0));
  maintenanceReminder = totalSensorRecoveries >= 5 ? "Inspect BMP388 wiring and connector" :
    totalFlights && totalFlights % 20 == 0 ? "Inspect mounts, battery, SD card, and wiring" :
    totalSdTests >= 50 ? "Consider replacing or reformatting the SD card after backup" : "No maintenance due";
  nextFlightNumber=nv.getUInt("nextid",1);
  if(nextFlightNumber<1)nextFlightNumber=1;
  flightLabel="Flight "+String(nextFlightNumber);

  Wire.begin();
  Wire.setClock(400000);
  initSensor();
  uint16_t rate=nv.getUShort("samplehz",25);
  if(!setSampleRate(rate))setSampleRate(25);
  uint8_t savedLimit = nv.getUChar("limitmin", 10);
  recordingLimitMin = (savedLimit == 2 || savedLimit == 5 ||
    savedLimit == 10 || savedLimit == 20) ? savedLimit : 10;

  // Explicit SPI mapping for the wired 3.3 V microSD breakout.
  SPI.begin(5, 21, 19, SD_CS);

  if (initSD()) {
    loadCardIdentity();
    recoverInterruptedFlights();
    verifySD();
  }

  setupRoutes();
  startWifi();

  lastBatteryMs = millis();
  lastSampleUs = micros();
  lastFlushMs = millis();

  Serial.println();
  Serial.println("GREEN ION V11");
  Serial.println(String("BUILD: ") + FIRMWARE_BUILD);
  Serial.println("SSID: GREEN-ION");
  Serial.println("PASS: GreenIon2026");
  Serial.println("URL: http://192.168.4.1");
  Serial.println("Deep sleep wake: physical RESET");
}

// ---------- MAIN LOOP ----------

void loop() {
  feedLoopWDT();
  lastLoopHeartbeatMs = millis();
  if (shutdownPending &&
      (int32_t)(millis() - shutdownAtMs) >= 0) {
    enterDeepSleep();
  }

  updateLights();
  updateSimulation();

  // Keeps controls working during calibration.
  if (wifiRunning)
    web.handleClient();

  if (calibrating) {
    processCalibration();
  } else {
    uint32_t nowUs = micros();

    if ((uint32_t)(nowUs - lastSampleUs) >= effectiveSampleIntervalUs) {
      uint32_t n =
        (uint32_t)(nowUs - lastSampleUs) / effectiveSampleIntervalUs;

      if (recording && n > 1)
        missedSamples += n - 1;

      lastSampleUs += n * effectiveSampleIntervalUs;
      sampleSensor();
    }
  }

  uint32_t ms = millis();

  if (ms - lastBatteryMs >= 1000) {
    lastBatteryMs = ms;
    readBattery();
  }

  if (recording && ms - lastFlushMs >= FLUSH_MS)
    flushLog();

  if (recording && ms - lastCheckpointMs >= CHECKPOINT_MS) {
    // Only journal bytes that have been flushed to the card.
    if (flushLog()) saveCheckpoint();
  }
  writeHealthSnapshot();

  if (groundTestActive && ms - groundTestStartMs >= 10000UL)
    finishGroundTest();

  processPrepareAll();

  if (wifiLaunchDisconnectPending && recording && wifiRunning &&
      (int32_t)(ms - wifiLaunchDisconnectAtMs) >= 0) {
    wifiLaunchDisconnectPending = false;
    stopWifi();
  }

  if (recording && ms - flightStartMs >= (uint32_t)recordingLimitMin * 60000UL)
    stopFlight(String(recordingLimitMin) + "-minute timeout");

  if (!recording && !wifiRunning &&
      !shutdownPending)
    startWifi();

  yield();
}

