
/*
  GREEN ION V8.7 — FLIGHT CONTROL EDITION
  ESP32 Feather V2 + BMP388 flight logger

  Wi-Fi: GREEN-ION
  Password: GreenIon2026
  Browser: http://192.168.4.1

  NEW:
    Unlimited sequential flight files (up to 9999 per card)
    Flight label and motor designation metadata
    Phone downloads in original RKT4 BIN or analysis-ready CSV
    Postflight summary and data-quality statistics
    Adaptive low-jitter altitude filter that responds quickly in flight
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
#include <math.h>

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
const uint32_t CONTROL_LEASE_MS = 30000;
const uint32_t FLUSH_MS = 500;
const uint32_t CHECKPOINT_MS = 5000;
const uint32_t MAX_FLIGHT_MS = 600000;
const uint32_t CAL_MS = 50;
const uint32_t LANDING_MS = 20000;
const size_t BUFFER_BYTES = 1024;

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
float speedMps = 0;
float peakM = 0;
float prevAlt = NAN;

uint32_t lastReadMs = 0;
uint32_t lastAltMs = 0;
uint32_t goodReads = 0;
uint32_t badReads = 0;

int calStep = 0;
int calValid = 0;
double calSum = 0;
uint32_t lastCalStepMs = 0;
uint32_t calCount = 0;
uint32_t calTimeMs = 0;
String calStatus = "Not calibrated";

// ---------- POWER ----------

uint16_t batteryMv = 0;
bool batteryPresent = false;
uint32_t lastBatteryMs = 0;

// ---------- FLIGHT ----------

bool sdOK = false;
bool sdVerified = false;
bool sdFault = false;
bool armed = false;
bool recording = false;
bool launched = false;
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
String integrityStatus = "Not checked";

uint32_t samples = 0;
uint32_t invalidSamples = 0;
uint32_t missedSamples = 0;
uint32_t writeErrors = 0;

String filename = "(none)";
String metadataFilename = "";
String checkpointFilename = "";
String flightLabel = "Flight 1";
String missionKind = "flight";
uint32_t nextFlightNumber = 1;
String motorLabel = "Unspecified";
String message = "Starting";
String errorText = "";
String sdStatus = "Not checked";
String finishReason = "";

float minimumPressurePa = NAN;
float minimumTemperatureC = NAN;
float maximumTemperatureC = NAN;

// ---------- SHUTDOWN ----------

// Shutdown is scheduled AFTER sending HTTP response.
bool shutdownPending = false;
uint32_t shutdownAtMs = 0;

// ---------- FIRMWARE UPDATE ----------

bool updateInProgress = false;
bool updateSucceeded = false;
String updateError = "";
size_t updateBytes = 0;

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

void readBattery() {
  uint32_t v =
    (uint32_t)analogReadMilliVolts(BATTERY_PIN) * 2UL;
  batteryMv = (uint16_t)min(v, (uint32_t)65535);
  batteryPresent = batteryMv >= 2500 &&
                   batteryMv <= 4400;
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

bool setSampleRate(uint16_t requestedRate) {
  if (requestedRate != 10 &&
      requestedRate != 25 &&
      requestedRate != 50)
    return false;

  sampleRateHz = requestedRate;
  sampleIntervalUs = 1000000UL / sampleRateHz;
  nv.putUShort("samplehz",sampleRateHz);

  if (sensorOK) {
    bmp.setOutputDataRate(
      sampleRateHz >= 50
        ? BMP3_ODR_50_HZ
        : BMP3_ODR_25_HZ
    );
  }

  lastSampleUs = micros();
  prevAlt = NAN;
  lastAltMs = 0;
  speedMps = 0;
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

bool startCalibration() {
  if (!sensorOK || armed || recording ||
      simulating || calibrating || shutdownPending)
    return false;

  calibrated = false;
  calibrating = true;
  calStep = 0;
  calValid = 0;
  calSum = 0;
  lastCalStepMs = millis() - CAL_MS;
  calStatus = "Calibration started";
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

    // Discard 5 warm-up readings.
    if (calStep >= 5) {
      calSum += p;
      calValid++;
    }
  } else {
    badReads++;
  }

  calStep++;
  if (calStep < 35) return;

  calibrating = false;

  if (calValid < 25) {
    calibrated = false;
    calStatus = "Calibration FAILED: too few valid reads";
    errorText = calStatus;
    return;
  }

  groundPa = (float)(calSum / calValid);

  altM = 0;
  rawAltM = 0;
  speedMps = 0;
  peakM = 0;
  prevAlt = NAN;
  lastAltMs = 0;

  calibrated = true;
  calCount++;
  calTimeMs = millis();
  lastSampleUs = micros();

  calStatus = "Calibration successful";
  message = "Ground altitude zeroed";
  errorText = "";

  Serial.printf(
    "[CAL] Baseline %.2f Pa\n", groundPa
  );
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

  // A previous successful mount can be stale after a card is removed.
  // Retest only while idle, closing the old mount before attempting a new one.
  if (sdOK) SD.end();
  sdOK = false;
  if (!initSD()) {
    sdStatus = "SD mount failed: check card seating, 3.3 V wiring and FAT/FAT32 format";
    return false;
  }
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

  size_t len = strlen(pattern);
  size_t n = f.write((const uint8_t*)pattern, len);
  f.flush();
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
String journalName(String bin) {
  bin.replace(".BIN", ".JRN");
  return bin;
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
  File j = SD.open(checkpointFilename, FILE_WRITE);
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
  lastCheckpointMs = millis();
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
                     name.startsWith("FLT") && name.endsWith(".JRN");
    for (int i = 3; i < 7 && isJournal; i++)
      if (!isDigit(name[i])) isJournal = false;
    entry.close();
    if (isJournal) {
      String bin = name; bin.replace(".JRN", ".BIN");
      String meta = name; meta.replace(".JRN", ".TXT");
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

  size_t offset = 0;
  while (offset < used) {
    size_t n = flightFile.write(
      buffer + offset, used - offset
    );
    if (!n) {
      sdFault = true;
      writeErrors++;
      errorText = "SD write failure";
      finishReason = "SD write failure";
      finalDurationMs = millis() - flightStartMs;
      recording = false;
      armed = false;
      used = 0;
      flightFile.close();
      integrityStatus = "SD write failed; partial flight may remain";
      return false;
    }
    offset += n;
  }

  used = 0;
  flightFile.flush();
  lastFlushMs = millis();
  return true;
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

  used += 16;
  samples++;
  if (!valid) invalidSamples++;
}

bool writeFlightMetadataStart() {
  File meta = SD.open(metadataFilename, FILE_WRITE);
  if (!meta) return false;

  meta.println("GREEN ION V8.7 FLIGHT METADATA");
  meta.println("file=" + filename.substring(1));
  meta.println("label=" + flightLabel);
  meta.println("motor=" + motorLabel);
  meta.println("sample_rate_hz=" + String(sampleRateHz));
  meta.print("baseline_pressure_pa=");
  meta.println(groundPa, 2);
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

void stopFlight(String reason) {
  if (!recording) return;

  finalDurationMs = millis() - flightStartMs;
  bool saved = flushLog();

  if (flightFile) flightFile.close();

  recording = false;
  armed = false;
  landingCandidate = false;

  finishReason = saved && !sdFault
    ? reason : "SD write failure";

  integrityStatus = saved && !sdFault ? "Saved and flushed" : "Incomplete: SD error";
  writeFlightMetadataEnd();
  if (saved && !sdFault && checkpointFilename.length()) SD.remove(checkpointFilename);

  message = saved && !sdFault
    ? "Saved " + filename
    : "Flight save failed";
}

bool beginFlight() {
  if (!armed || !sensorOK || !readingOK ||
      !calibrated || !sdOK || !sdVerified ||
      sdFault || simulating || calibrating ||
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

  // Keep the displayed Flight/Test number consistent with the actual file.
  flightLabel=String(missionKind=="test"?"Test ":"Flight ")+String(chosenFlightNumber);
  metadataFilename = filename;
  metadataFilename.replace(".BIN", ".TXT");
  checkpointFilename = journalName(filename);

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

  flightFile.flush();

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
  minimumPressurePa = NAN;
  minimumTemperatureC = NAN;
  maximumTemperatureC = NAN;

  flightStartMs = millis();
  lastFlushMs = millis();
  lastCheckpointMs = millis();
  lastSampleUs = micros();
  finalDurationMs = 0;

  peakM = 0;
  launched = false;
  landingCandidate = false;

  nextFlightNumber = chosenFlightNumber + 1;
  nv.putUInt("nextid",nextFlightNumber);
  recording = true;
  armed = false;
  saveCheckpoint();
  manualLed = false;

  finishReason = "";
  errorText = "";
  message = "Recording started";

  return true;
}

// ---------- LANDING DETECTION ----------

void checkLanding() {
  if (!recording) return;

  uint32_t elapsed = millis() - flightStartMs;

  if (elapsed >= MAX_FLIGHT_MS) {
    stopFlight("10-minute timeout");
    return;
  }

  if (!readingOK || !isfinite(altM)) {
    landingCandidate = false;
    return;
  }

  if (!launched) {
    if (altM >= 15) {
      launched = true;
      message = "Launch detected";
    }
    return;
  }

  bool nearGround = fabsf(altM) <= 8;
  bool descending =
    peakM >= 20 && peakM - altM >= 12;

  bool candidate =
    elapsed > 20000 &&
    nearGround &&
    descending;

  if (!candidate) {
    landingCandidate = false;
    return;
  }

  if (!landingCandidate) {
    landingCandidate = true;
    landingReference = altM;
    landingMs = millis();
    return;
  }

  if (fabsf(altM - landingReference) > 2.5f) {
    landingReference = altM;
    landingMs = millis();
  }

  if (millis() - landingMs >= LANDING_MS)
    stopFlight("Landing detected");
}

// ---------- SENSOR SAMPLING ----------

void sampleSensor() {
  float p = NAN, t = NAN;
  uint32_t stamp = micros();

  bool valid = getSensor(p, t);
  readingOK = valid;

  if (!valid) {
    badReads++;
    logSample(stamp, NAN, NAN, false);
    return;
  }

  goodReads++;
  lastReadMs = millis();
  pressurePa = p;
  tempC = t;

  if (!isfinite(minimumPressurePa) || p < minimumPressurePa)
    minimumPressurePa = p;
  if (!isfinite(minimumTemperatureC) || t < minimumTemperatureC)
    minimumTemperatureC = t;
  if (!isfinite(maximumTemperatureC) || t > maximumTemperatureC)
    maximumTemperatureC = t;

  if (calibrated && groundPa > 0) {
    rawAltM =
      44330.0f *
      (1 - powf(p / groundPa, 0.190294957f));

    if (!isfinite(altM)) {
      altM = rawAltM;
    } else {
      float difference = rawAltM - altM;
      float movement = fabsf(difference);

      // Reject tiny stationary noise, but increase response as
      // the rocket moves farther between pressure samples.
      float alpha =
        movement < 0.08f ? 0.0f :
        movement < 0.35f ? 0.08f :
        movement < 1.5f ? 0.24f :
        movement < 5.0f ? 0.48f : 0.72f;

      altM += alpha * difference;
    }

    if (altM > peakM) peakM = altM;

    uint32_t now = millis();

    if (isfinite(prevAlt) &&
        lastAltMs &&
        now > lastAltMs) {
      float instant =
        (altM - prevAlt) * 1000.0f /
        (now - lastAltMs);

      // Ignore impossible single-sample spikes. Fast real motion
      // gets more weight than small stationary variations.
      if (fabsf(instant) <= 500.0f) {
        float velocityAlpha =
          fabsf(instant) > 15.0f ? 0.55f : 0.18f;
        speedMps += velocityAlpha * (instant - speedMps);
      }
    }

    prevAlt = altM;
    lastAltMs = now;
  }

  logSample(stamp, p, t, true);
  checkLanding();
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
  bool thisController = controllerMatches();
  bool controlAvailable = controlLeaseExpired() || thisController;

  String s;
  s.reserve(3200);

  s = "{";

  s += "\"version\":\"8.7\"";
  s += ",\"controller\":" + jb(thisController);
  s += ",\"controlAvailable\":" + jb(controlAvailable);
  s += ",\"controlLeaseSeconds\":";
  s += thisController
    ? String((CONTROL_LEASE_MS -
        min((uint32_t)(millis() - controllerSeenMs), CONTROL_LEASE_MS)) / 1000)
    : "0";
  s += ",\"sensor\":" + jb(sensorOK);
  s += ",\"reading\":" + jb(readingOK);
  s += ",\"calibrated\":" + jb(calibrated);
  s += ",\"calibrating\":" + jb(calibrating);
  s += ",\"calStep\":" + String(calStep);
  s += ",\"calValid\":" + String(calValid);
  s += ",\"calStatus\":\"" + je(calStatus) + "\"";
  s += ",\"calCount\":" + String(calCount);
  s += ",\"calAge\":";
  s += calTimeMs
    ? String((millis() - calTimeMs) / 1000)
    : "null";

  s += ",\"altitude\":" + jn(altM);
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
  s += ",\"integrityStatus\":\"" + je(integrityStatus) + "\"";
  s += ",\"recoveredFlights\":" + String(recoveredFlights);
  s += ",\"recoveredSamples\":" + String(lastRecoveredSamples);
  s += ",\"resetReason\":\"" + je(resetReasonText) + "\"";
  s += ",\"sampleRate\":" + String(sampleRateHz);
  s += ",\"missionKind\":\""+je(missionKind)+"\"";
  s += ",\"nextFlightNumber\":"+String(nextFlightNumber);

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
      output += String((uint32_t)f.size()) + "}";
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

  web.streamFile(f, "application/octet-stream");
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
  bool hasFirstStamp = false;
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

    if (!hasFirstStamp) { firstStamp = stamp; hasFirstStamp = true; }

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
        if (dt >= 0.01f && dt <= 2.0f)
          velocity = (altitude - previousAltitude) / dt;
      }

      previousAltitude = altitude;
      previousStamp = stamp;
    } else {
      // Never estimate derivatives across missing or invalid sensor samples.
      previousAltitude = NAN;
      previousStamp = 0;
      previousVelocity = NAN;
      previousVelocityStamp = 0;
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

  bool ok = SD.remove("/" + n);
  SD.remove("/" + companionFile(n));
  respond(ok, ok ? "Flight deleted" : "Delete failed");
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

void setupRoutes() {
  web.on("/", HTTP_GET, []() {
    web.send_P(200, "text/html", PAGE);
  });

  web.on("/api/status", HTTP_GET, statusAPI);

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
    flightLabel=String(kind=="test"?"Test ":"Flight ")+String(nextFlightNumber);
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
    bool ok = setSampleRate(requested);
    respond(
      ok,
      ok
        ? "Sample rate set to " + String(sampleRateHz) + " Hz"
        : "Choose 10, 25, or 50 Hz"
    );
  });

  web.on("/api/calibrate", HTTP_POST, []() {
    if (!requireControl()) return;
    bool ok = startCalibration();
    respond(ok, ok ? calStatus :
      "Calibration unavailable: sensor missing or device busy");
  });

  web.on("/api/arm", HTTP_POST, []() {
    if (!requireControl()) return;
    bool ready =
      !armed && !recording &&
      !simulating && !calibrating &&
      !shutdownPending &&
      sensorOK && readingOK && calibrated &&
      sdOK && sdVerified && !sdFault &&
      !criticalBattery() &&
      sdFree() >= 1048576ULL;

    if (ready) {
      armed = true;
      manualLed = false;
      message = "Recorder armed";
      errorText = "";
    }

    respond(ready, ready ? message :
      "Cannot arm: check sensor, calibration, SD and battery");
  });

  web.on("/api/start", HTTP_POST, []() {
    if (!requireControl()) return;
    bool ok = beginFlight();
    respond(ok, ok ? message : errorText);
  });

  web.on("/api/stop", HTTP_POST, []() {
    if (!requireControl()) return;
    if (!recording) {
      respond(false, "Not recording");
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
  web.on("/api/csv", HTTP_GET, csvAPI);
  web.on("/api/metadata", HTTP_GET, metadataAPI);
  web.on("/api/flightstats", HTTP_GET, flightStatsAPI);
  web.on("/api/delete", HTTP_POST, deleteAPI);
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
  nextFlightNumber=nv.getUInt("nextid",1);
  if(nextFlightNumber<1)nextFlightNumber=1;
  flightLabel="Flight "+String(nextFlightNumber);

  Wire.begin();
  Wire.setClock(400000);
  initSensor();
  uint16_t rate=nv.getUShort("samplehz",25);
  if(!setSampleRate(rate))setSampleRate(25);

  // Explicit SPI mapping for the wired 3.3 V microSD breakout.
  SPI.begin(5, 21, 19, SD_CS);

  if (initSD()) {
    recoverInterruptedFlights();
    verifySD();
  }

  setupRoutes();
  startWifi();

  lastBatteryMs = millis();
  lastSampleUs = micros();
  lastFlushMs = millis();

  Serial.println();
  Serial.println("GREEN ION V8.7");
  Serial.println("SSID: GREEN-ION");
  Serial.println("PASS: GreenIon2026");
  Serial.println("URL: http://192.168.4.1");
  Serial.println("Deep sleep wake: physical RESET");
}

// ---------- MAIN LOOP ----------

void loop() {
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

    if ((uint32_t)(nowUs - lastSampleUs) >= sampleIntervalUs) {
      uint32_t n =
        (uint32_t)(nowUs - lastSampleUs) / sampleIntervalUs;

      if (recording && n > 1)
        missedSamples += n - 1;

      lastSampleUs += n * sampleIntervalUs;
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

  if (recording &&
      ms - flightStartMs >= MAX_FLIGHT_MS)
    stopFlight("10-minute timeout");

  if (!recording && !wifiRunning &&
      !shutdownPending)
    startWifi();

  yield();
}
