
/*
  =========================================================
  GREEN ION V8.1 — COMPLETE ROCKET FLIGHT RECORDER
  =========================================================

  Hardware:
    Adafruit ESP32 Feather V2
    BMP388 I2C pressure sensor
    Optional SPI microSD card
    Optional 3.7 V LiPo battery

  Wi-Fi:
    Network:  GREEN-ION
    Password: GreenIon2026
    Dashboard: http://192.168.4.1

  Changes:
    - Non-blocking calibration with progress feedback
    - All dashboard commands have matching API handlers
    - Responsive buttons and request error messages
    - Live altitude displays -- until calibrated
    - Battery percentage and voltage on main page
    - Metric / Imperial and absolute PSI
    - Simulation, graphs, LEDs, SD files and diagnostics
    - RKT4 binary logging at nominal 25 Hz

  IMPORTANT:
    This is an experimental data recorder.
    It is NOT an ignition or recovery controller.
*/

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_BMP3XX.h>
#include <Adafruit_NeoPixel.h>
#include <math.h>

// =========================================================
// CONFIGURATION
// =========================================================

#ifndef PIN_NEOPIXEL
#define PIN_NEOPIXEL 0
#endif

#ifndef NEOPIXEL_I2C_POWER
#define NEOPIXEL_I2C_POWER 2
#endif

const int SD_CS = 33;             // Verify your wiring
const int BATTERY_PIN = 35;       // Feather V2 battery monitor
const int RED_LED_PIN = 13;

// Optional USB sensing pin, disabled.
// Requires an external safe resistor divider if enabled.
const int USB_SENSE_PIN = -1;

const char* WIFI_SSID = "GREEN-ION";
const char* WIFI_PASS = "GreenIon2026";

const uint32_t SAMPLE_INTERVAL_US = 40000; // 25 Hz
const uint32_t FLUSH_INTERVAL_MS = 500;
const uint32_t MAX_RECORDING_MS = 600000;

const uint32_t CALIBRATION_INTERVAL_MS = 50;
const int CALIBRATION_TOTAL = 35;
const int CALIBRATION_DISCARD = 5;
const int CALIBRATION_REQUIRED = 25;

const float LAUNCH_THRESHOLD_M = 15.0f;
const float MIN_PEAK_M = 20.0f;
const float DESCENT_THRESHOLD_M = 12.0f;
const float LANDING_NEAR_GROUND_M = 8.0f;
const float LANDING_STABLE_RANGE_M = 2.5f;

const uint32_t LANDING_STABLE_MS = 20000;
const uint64_t MIN_SD_FREE_BYTES = 1048576ULL;

const size_t LOG_BUFFER_SIZE = 1024;

// =========================================================
// HARDWARE
// =========================================================

Adafruit_BMP3XX bmp;

Adafruit_NeoPixel neo(
  1,
  PIN_NEOPIXEL,
  NEO_GRB + NEO_KHZ800
);

WebServer server(80);
File logFile;

// =========================================================
// SENSOR STATE
// =========================================================

bool sensorOK = false;
bool readingOK = false;
bool calibrated = false;
bool calibrating = false;

float pressurePa = NAN;
float temperatureC = NAN;
float baselinePa = NAN;

float rawAltitudeM = NAN;
float altitudeM = NAN;
float speedMps = 0.0f;
float peakAltitudeM = 0.0f;

float previousAltitudeM = NAN;

uint32_t lastSensorReadMs = 0;
uint32_t lastAltitudeMs = 0;

uint32_t goodReadings = 0;
uint32_t failedReadings = 0;

uint32_t calibrationCount = 0;
uint32_t lastCalibrationMs = 0;
uint32_t lastCalibrationStepMs = 0;

int calibrationStep = 0;
int calibrationValid = 0;
double calibrationPressureSum = 0;

String calibrationResult = "Not calibrated";

// =========================================================
// POWER STATE
// =========================================================

uint16_t batteryMv = 0;
bool batteryDetected = false;
bool usbDetected = false;

uint32_t lastBatteryMs = 0;

// =========================================================
// FLIGHT STATE
// =========================================================

bool sdOK = false;
bool sdVerified = false;
bool sdFault = false;

bool armed = false;
bool recording = false;
bool launched = false;

bool landingCandidate = false;
float landingReferenceM = 0;
uint32_t landingStartMs = 0;

bool simulation = false;
float simulationAltitudeM = 0;
float simulationSpeedMps = 0;
uint32_t simulationStartMs = 0;

bool wifiActive = false;

uint32_t recordingStartMs = 0;
uint32_t finishedDurationMs = 0;

uint32_t lastSampleUs = 0;
uint32_t lastFlushMs = 0;

uint32_t recordedSamples = 0;
uint32_t invalidSamples = 0;
uint32_t missedSamples = 0;
uint32_t sdErrors = 0;

String currentFilename = "(none)";
String finishReason = "";

String systemMessage = "Starting";
String lastError = "";
String sdMessage = "Not checked";

// =========================================================
// LOG BUFFER
// =========================================================

uint8_t logBuffer[LOG_BUFFER_SIZE];
size_t logBufferUsed = 0;

// =========================================================
// LED STATE
// =========================================================

bool manualLED = false;

// 0 solid
// 1 blink
// 2 breathe
// 3 rainbow
// 4 off

int ledEffect = 0;

int ledR = 0;
int ledG = 180;
int ledB = 255;
int ledBrightness = 45;

// =========================================================
// JSON HELPERS
// =========================================================

String jsonBool(bool value) {
  return value ? "true" : "false";
}

String jsonNumber(float value, int places = 2) {
  if (!isfinite(value)) return "null";
  return String(value, places);
}

String jsonEscape(String value) {
  value.replace("\\", "\\\\");
  value.replace("\"", "\\\"");
  value.replace("\n", " ");
  value.replace("\r", " ");
  return value;
}

void sendResponse(bool success, String message) {
  String body = "{\"success\":";
  body += jsonBool(success);
  body += ",\"message\":\"";
  body += jsonEscape(message);
  body += "\"}";

  server.sendHeader("Cache-Control", "no-store");
  server.send(
    success ? 200 : 409,
    "application/json",
    body
  );
}

// =========================================================
// BINARY HELPERS
// =========================================================

void writeU16(uint8_t* p, uint16_t value) {
  p[0] = value & 255;
  p[1] = (value >> 8) & 255;
}

void writeU32(uint8_t* p, uint32_t value) {
  p[0] = value & 255;
  p[1] = (value >> 8) & 255;
  p[2] = (value >> 16) & 255;
  p[3] = (value >> 24) & 255;
}

void writeFloat(uint8_t* p, float value) {
  uint32_t bits;
  memcpy(&bits, &value, 4);
  writeU32(p, bits);
}

// =========================================================
// BATTERY
// =========================================================

void updateBattery() {
  uint32_t measured =
    (uint32_t)analogReadMilliVolts(BATTERY_PIN) * 2UL;

  batteryMv = (uint16_t)min(
    measured,
    (uint32_t)65535
  );

  batteryDetected =
    batteryMv >= 2500 &&
    batteryMv <= 4400;

  if (USB_SENSE_PIN >= 0) {
    usbDetected =
      analogReadMilliVolts(USB_SENSE_PIN) > 1800;
  }
}

bool batteryLow() {
  return batteryDetected && batteryMv < 3500;
}

bool batteryCritical() {
  return batteryDetected && batteryMv < 3300;
}

int batteryPercentage() {
  if (!batteryDetected) return -1;

  // Approximate voltage-based LiPo charge.
  // Not a true fuel gauge.

  const uint16_t voltage[] = {
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
        (batteryMv - voltage[i - 1]) /
        float(voltage[i] - voltage[i - 1]);

      return percent[i - 1] +
        (int)roundf(
          fraction *
          (percent[i] - percent[i - 1])
        );
    }
  }

  return 100;
}

// =========================================================
// BMP388 INITIALIZATION
// =========================================================

bool initializeBMP() {
  sensorOK = bmp.begin_I2C(0x77, &Wire);

  if (!sensorOK) {
    sensorOK = bmp.begin_I2C(0x76, &Wire);
  }

  if (!sensorOK) {
    lastError =
      "BMP388 not detected at 0x76 or 0x77";
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

  bmp.setOutputDataRate(
    BMP3_ODR_25_HZ
  );

  systemMessage = "BMP388 detected";
  return true;
}

bool readBMP(float& pressure, float& temperature) {
  if (!sensorOK) return false;

  if (!bmp.performReading()) return false;

  pressure = bmp.pressure;
  temperature = bmp.temperature;

  return isfinite(pressure) &&
         isfinite(temperature) &&
         pressure > 30000 &&
         pressure < 120000 &&
         temperature > -50 &&
         temperature < 100;
}

// =========================================================
// NON-BLOCKING CALIBRATION
// =========================================================

bool beginCalibration() {
  if (!sensorOK) {
    calibrationResult = "BMP388 not detected";
    return false;
  }

  if (calibrating) {
    calibrationResult = "Calibration already running";
    return false;
  }

  if (armed || recording || simulation) {
    calibrationResult =
      "Stop arming, recording or simulation first";
    return false;
  }

  calibrated = false;
  calibrating = true;

  calibrationStep = 0;
  calibrationValid = 0;
  calibrationPressureSum = 0;

  lastCalibrationStepMs =
    millis() - CALIBRATION_INTERVAL_MS;

  calibrationResult =
    "Calibration started: collecting sensor data";

  systemMessage = calibrationResult;
  lastError = "";

  Serial.println("[CAL] Started");

  return true;
}

void updateCalibration() {
  if (!calibrating) return;

  uint32_t now = millis();

  if (now - lastCalibrationStepMs <
      CALIBRATION_INTERVAL_MS) {
    return;
  }

  lastCalibrationStepMs = now;

  float p = NAN;
  float t = NAN;

  bool valid = readBMP(p, t);

  readingOK = valid;

  if (valid) {
    pressurePa = p;
    temperatureC = t;
    lastSensorReadMs = now;
    goodReadings++;

    if (calibrationStep >= CALIBRATION_DISCARD) {
      calibrationPressureSum += p;
      calibrationValid++;
    }
  }
  else {
    failedReadings++;
  }

  calibrationStep++;

  if (calibrationStep < CALIBRATION_TOTAL) {
    return;
  }

  calibrating = false;

  if (calibrationValid < CALIBRATION_REQUIRED) {
    calibrated = false;

    calibrationResult =
      "Calibration failed: not enough valid readings";

    lastError = calibrationResult;

    Serial.println("[CAL] FAILED");
    return;
  }

  baselinePa =
    (float)(calibrationPressureSum / calibrationValid);

  rawAltitudeM = 0;
  altitudeM = 0;
  speedMps = 0;
  peakAltitudeM = 0;

  previousAltitudeM = NAN;
  lastAltitudeMs = 0;

  launched = false;
  landingCandidate = false;

  calibrated = true;

  calibrationCount++;
  lastCalibrationMs = millis();

  calibrationResult =
    "Calibration successful - altitude zeroed";

  systemMessage = calibrationResult;
  lastError = "";

  lastSampleUs = micros();

  Serial.print("[CAL] Success, baseline: ");
  Serial.println(baselinePa, 2);
}

// =========================================================
// SD CARD
// =========================================================

bool initializeSD() {
  sdOK = SD.begin(
    SD_CS,
    SPI,
    4000000
  );

  sdVerified = false;

  sdMessage = sdOK
    ? "SD detected - verification required"
    : "No SD card detected";

  return sdOK;
}

uint64_t freeSDBytes() {
  if (!sdOK) return 0;

  uint64_t total = SD.totalBytes();
  uint64_t used = SD.usedBytes();

  return total > used ? total - used : 0;
}

bool testSD() {
  if (recording || armed || calibrating) {
    sdMessage =
      "SD test unavailable while recorder is busy";
    return false;
  }

  if (!sdOK && !initializeSD()) {
    return false;
  }

  const char* path = "/GITEST.TMP";
  const char* pattern = "GREEN-ION-TEST-V81";

  SD.remove(path);

  File test = SD.open(path, FILE_WRITE);

  if (!test) {
    sdVerified = false;
    sdMessage = "SD test failed: could not create file";
    return false;
  }

  size_t count = strlen(pattern);

  size_t written = test.write(
    (const uint8_t*)pattern,
    count
  );

  test.flush();
  test.close();

  if (written != count) {
    SD.remove(path);
    sdVerified = false;
    sdMessage = "SD write test failed";
    return false;
  }

  test = SD.open(path, FILE_READ);

  if (!test) {
    SD.remove(path);
    sdVerified = false;
    sdMessage = "SD read test failed";
    return false;
  }

  char buffer[32] = {0};

  size_t received =
    test.readBytes(buffer, count);

  test.close();
  SD.remove(path);

  sdVerified =
    received == count &&
    memcmp(buffer, pattern, count) == 0 &&
    freeSDBytes() >= MIN_SD_FREE_BYTES;

  sdMessage = sdVerified
    ? "SD ready - read/write test passed"
    : "SD verification failed or insufficient space";

  if (sdVerified) sdFault = false;

  return sdVerified;
}

// =========================================================
// RKT4 FLIGHT LOGGING
// =========================================================

bool flushLog() {
  if (!recording || !logFile) return false;

  size_t offset = 0;

  while (offset < logBufferUsed) {
    size_t written = logFile.write(
      logBuffer + offset,
      logBufferUsed - offset
    );

    if (written == 0) {
      sdErrors++;
      sdFault = true;

      lastError = "SD write failure";
      finishReason = "SD write failure";

      finishedDurationMs =
        millis() - recordingStartMs;

      recording = false;
      armed = false;

      logBufferUsed = 0;
      logFile.close();

      return false;
    }

    offset += written;
  }

  logBufferUsed = 0;

  logFile.flush();
  lastFlushMs = millis();

  return true;
}

void addLogRecord(
  uint32_t timestamp,
  float pressure,
  float temperature,
  bool valid
) {
  if (!recording) return;

  if (logBufferUsed + 16 > LOG_BUFFER_SIZE) {
    if (!flushLog()) return;
  }

  uint8_t* p = logBuffer + logBufferUsed;

  writeU32(p, timestamp);
  writeFloat(p + 4, pressure);
  writeFloat(p + 8, temperature);
  writeU16(p + 12, batteryMv);
  writeU16(p + 14, valid ? 1 : 0);

  logBufferUsed += 16;

  recordedSamples++;

  if (!valid) invalidSamples++;
}

void finishFlight(String reason) {
  if (!recording) return;

  finishedDurationMs =
    millis() - recordingStartMs;

  bool saved = flushLog();

  if (logFile) {
    logFile.close();
  }

  recording = false;
  armed = false;
  landingCandidate = false;

  if (saved && !sdFault) {
    finishReason = reason;
    systemMessage =
      "Flight saved: " + currentFilename;
  }
  else {
    finishReason = "SD write failure";
    systemMessage = "Flight saving failed";
  }
}

bool startFlight() {
  if (!armed ||
      !calibrated ||
      !sensorOK ||
      !readingOK ||
      !sdOK ||
      !sdVerified ||
      sdFault ||
      simulation ||
      calibrating ||
      batteryCritical()) {
    lastError =
      "Preflight failed: sensor, calibration, SD or battery";
    return false;
  }

  if (freeSDBytes() < MIN_SD_FREE_BYTES) {
    lastError = "Not enough SD storage";
    return false;
  }

  bool found = false;

  for (int i = 1; i <= 999; i++) {
    char name[24];

    snprintf(
      name,
      sizeof(name),
      "/FLIGHT%03d.BIN",
      i
    );

    if (!SD.exists(name)) {
      currentFilename = name;
      found = true;
      break;
    }
  }

  if (!found) {
    lastError = "No unused flight filename";
    return false;
  }

  logFile = SD.open(
    currentFilename,
    FILE_WRITE
  );

  if (!logFile) {
    lastError = "Could not create flight file";
    return false;
  }

  // RKT4: 32-byte header, 16-byte records.
  uint8_t header[32] = {0};

  memcpy(header, "RKT4", 4);

  writeU16(header + 4, 1);
  writeU16(header + 6, 32);
  writeU16(header + 8, 16);
  writeU16(header + 10, 25);

  if (logFile.write(header, 32) != 32) {
    logFile.close();
    SD.remove(currentFilename);

    lastError = "Flight header write failed";
    return false;
  }

  logFile.flush();

  logBufferUsed = 0;

  recordedSamples = 0;
  invalidSamples = 0;
  missedSamples = 0;
  sdErrors = 0;

  launched = false;
  landingCandidate = false;

  peakAltitudeM =
    isfinite(altitudeM) ? max(0.0f, altitudeM) : 0;

  recordingStartMs = millis();
  finishedDurationMs = 0;

  lastFlushMs = millis();
  lastSampleUs = micros();

  armed = false;
  recording = true;

  manualLED = false;

  finishReason = "";
  lastError = "";
  systemMessage = "Recording started";

  return true;
}

// =========================================================
// FLIGHT DETECTION
// =========================================================

void checkFlight() {
  if (!recording) return;

  uint32_t elapsed =
    millis() - recordingStartMs;

  if (elapsed >= MAX_RECORDING_MS) {
    finishFlight("10-minute timeout");
    return;
  }

  if (!readingOK || !isfinite(altitudeM)) {
    landingCandidate = false;
    return;
  }

  if (!launched) {
    if (altitudeM >= LAUNCH_THRESHOLD_M) {
      launched = true;
      systemMessage = "Launch detected";
    }

    return;
  }

  bool eligible =
    elapsed >= 20000 &&
    peakAltitudeM >= MIN_PEAK_M &&
    (peakAltitudeM - altitudeM) >= DESCENT_THRESHOLD_M &&
    fabsf(altitudeM) <= LANDING_NEAR_GROUND_M;

  if (!eligible) {
    landingCandidate = false;
    return;
  }

  if (!landingCandidate) {
    landingCandidate = true;
    landingStartMs = millis();
    landingReferenceM = altitudeM;
    return;
  }

  if (fabsf(altitudeM - landingReferenceM) >
      LANDING_STABLE_RANGE_M) {
    landingReferenceM = altitudeM;
    landingStartMs = millis();
  }

  if (millis() - landingStartMs >=
      LANDING_STABLE_MS) {
    finishFlight("Landing detected");
  }
}

// =========================================================
// LIVE SENSOR SAMPLING
// =========================================================

void updateSensor() {
  if (calibrating) return;

  uint32_t timestamp = micros();

  float p = NAN;
  float t = NAN;

  bool valid = readBMP(p, t);

  readingOK = valid;

  if (!valid) {
    failedReadings++;

    addLogRecord(timestamp, NAN, NAN, false);
    return;
  }

  goodReadings++;
  lastSensorReadMs = millis();

  pressurePa = p;
  temperatureC = t;

  if (calibrated && baselinePa > 0) {
    rawAltitudeM =
      44330.0f *
      (1.0f - powf(
        pressurePa / baselinePa,
        0.190294957f
      ));

    if (!isfinite(altitudeM)) {
      altitudeM = rawAltitudeM;
    }
    else {
      altitudeM =
        0.8f * altitudeM +
        0.2f * rawAltitudeM;
    }

    if (altitudeM > peakAltitudeM) {
      peakAltitudeM = altitudeM;
    }

    uint32_t now = millis();

    if (isfinite(previousAltitudeM) &&
        lastAltitudeMs != 0 &&
        now > lastAltitudeMs) {

      float dt =
        (now - lastAltitudeMs) / 1000.0f;

      float instantaneous =
        (altitudeM - previousAltitudeM) / dt;

      speedMps =
        0.8f * speedMps +
        0.2f * instantaneous;
    }

    previousAltitudeM = altitudeM;
    lastAltitudeMs = now;
  }

  addLogRecord(timestamp, p, t, true);

  checkFlight();
}

// =========================================================
// SIMULATION
// =========================================================

void updateSimulation() {
  if (!simulation) return;

  float seconds =
    (millis() - simulationStartMs) / 1000.0f;

  if (seconds < 10) {
    simulationAltitudeM = seconds * 8.0f;
    simulationSpeedMps = 8.0f;
  }
  else if (seconds < 20) {
    simulationAltitudeM =
      (20.0f - seconds) * 8.0f;

    simulationSpeedMps = -8.0f;
  }
  else {
    simulation = false;
    simulationAltitudeM = 0;
    simulationSpeedMps = 0;

    systemMessage = "Simulation completed";
  }
}

// =========================================================
// LED CONTROL
// =========================================================

uint32_t rainbowWheel(byte position) {
  position = 255 - position;

  if (position < 85) {
    return neo.Color(
      255 - position * 3,
      0,
      position * 3
    );
  }

  if (position < 170) {
    position -= 85;

    return neo.Color(
      0,
      position * 3,
      255 - position * 3
    );
  }

  position -= 170;

  return neo.Color(
    position * 3,
    255 - position * 3,
    0
  );
}

void updateLED() {
  static uint32_t lastUpdate = 0;

  uint32_t now = millis();

  if (now - lastUpdate < 40) return;

  lastUpdate = now;

  bool slow = (now / 450) % 2 == 0;
  bool fast = (now / 150) % 2 == 0;

  uint32_t color = 0;
  int brightness = 45;

  if (!sensorOK || sdFault) {
    color = fast
      ? neo.Color(255, 0, 0)
      : 0;
  }
  else if (batteryCritical()) {
    color = fast
      ? neo.Color(255, 0, 0)
      : neo.Color(255, 120, 0);
  }
  else if (recording) {
    color = slow
      ? neo.Color(0, 255, 0)
      : 0;
  }
  else if (armed) {
    color = neo.Color(255, 190, 0);
  }
  else if (calibrating) {
    color = slow
      ? neo.Color(0, 120, 255)
      : neo.Color(0, 40, 100);
  }
  else if (manualLED) {
    brightness = ledBrightness;

    switch (ledEffect) {
      case 0:
        color = neo.Color(ledR, ledG, ledB);
        break;

      case 1:
        color = slow
          ? neo.Color(ledR, ledG, ledB)
          : 0;
        break;

      case 2: {
        float factor =
          (sinf(now * 0.004f) + 1.0f) / 2.0f;

        color = neo.Color(
          (int)(ledR * factor),
          (int)(ledG * factor),
          (int)(ledB * factor)
        );
        break;
      }

      case 3:
        color = rainbowWheel(
          (byte)(now / 12)
        );
        break;

      default:
        color = 0;
        break;
    }
  }
  else if (simulation) {
    color = slow
      ? neo.Color(170, 0, 255)
      : 0;
  }
  else if (calibrated) {
    color = neo.Color(0, 180, 45);
  }
  else {
    color = slow
      ? neo.Color(0, 100, 255)
      : 0;
  }

  neo.setBrightness(brightness);
  neo.setPixelColor(0, color);
  neo.show();

  bool warning =
    !sensorOK || sdFault || batteryCritical();

  digitalWrite(
    RED_LED_PIN,
    warning ? fast :
    recording ? slow : LOW
  );
}

// =========================================================
// STATUS API
// =========================================================

void sendStatus() {
  String s;
  s.reserve(3000);

  s = "{";

  s += "\"sensor\":" + jsonBool(sensorOK);
  s += ",\"reading\":" + jsonBool(readingOK);

  s += ",\"calibrated\":" + jsonBool(calibrated);
  s += ",\"calibrating\":" + jsonBool(calibrating);

  s += ",\"calibrationStep\":" +
    String(calibrationStep);

  s += ",\"calibrationTotal\":" +
    String(CALIBRATION_TOTAL);

  s += ",\"calibrationValid\":" +
    String(calibrationValid);

  s += ",\"calibrationResult\":\"" +
    jsonEscape(calibrationResult) + "\"";

  s += ",\"calibrationCount\":" +
    String(calibrationCount);

  s += ",\"calibrationAge\":";

  s += lastCalibrationMs
    ? String((millis() - lastCalibrationMs) / 1000)
    : "null";

  s += ",\"altitude\":" +
    jsonNumber(altitudeM);

  s += ",\"speed\":" +
    jsonNumber(speedMps);

  s += ",\"peak\":" +
    jsonNumber(peakAltitudeM);

  s += ",\"pressure\":" +
    jsonNumber(pressurePa);

  s += ",\"temperature\":" +
    jsonNumber(temperatureC);

  s += ",\"baseline\":" +
    jsonNumber(baselinePa);

  s += ",\"simAltitude\":" +
    jsonNumber(simulationAltitudeM);

  s += ",\"simSpeed\":" +
    jsonNumber(simulationSpeedMps);

  s += ",\"battery\":" +
    jsonNumber(batteryMv / 1000.0f, 3);

  s += ",\"batteryPercent\":" +
    String(batteryPercentage());

  s += ",\"batteryPresent\":" +
    jsonBool(batteryDetected);

  s += ",\"batteryLow\":" +
    jsonBool(batteryLow());

  s += ",\"batteryCritical\":" +
    jsonBool(batteryCritical());

  s += ",\"usbSensor\":" +
    jsonBool(USB_SENSE_PIN >= 0);

  s += ",\"usbPresent\":" +
    jsonBool(usbDetected);

  s += ",\"charging\":\"unknown\"";

  s += ",\"sd\":" + jsonBool(sdOK);
  s += ",\"sdVerified\":" +
    jsonBool(sdVerified);
  s += ",\"sdFault\":" +
    jsonBool(sdFault);

  s += ",\"sdMessage\":\"" +
    jsonEscape(sdMessage) + "\"";

  s += ",\"armed\":" + jsonBool(armed);
  s += ",\"recording\":" + jsonBool(recording);
  s += ",\"launched\":" + jsonBool(launched);
  s += ",\"landing\":" +
    jsonBool(landingCandidate);
  s += ",\"simulation\":" + jsonBool(simulation);

  s += ",\"goodReads\":" +
    String(goodReadings);

  s += ",\"failedReads\":" +
    String(failedReadings);

  s += ",\"freshness\":";

  s += lastSensorReadMs
    ? String(millis() - lastSensorReadMs)
    : "null";

  s += ",\"samples\":" +
    String(recordedSamples);

  s += ",\"invalidSamples\":" +
    String(invalidSamples);

  s += ",\"missedSamples\":" +
    String(missedSamples);

  s += ",\"sdErrors\":" +
    String(sdErrors);

  float duration =
    (recording
      ? millis() - recordingStartMs
      : finishedDurationMs) / 1000.0f;

  s += ",\"duration\":" +
    jsonNumber(duration, 1);

  s += ",\"file\":\"" +
    jsonEscape(currentFilename) + "\"";

  s += ",\"finish\":\"" +
    jsonEscape(finishReason) + "\"";

  s += ",\"message\":\"" +
    jsonEscape(systemMessage) + "\"";

  s += ",\"error\":\"" +
    jsonEscape(lastError) + "\"";

  s += ",\"ledManual\":" +
    jsonBool(manualLED);

  s += ",\"ledEffect\":" +
    String(ledEffect);

  s += ",\"uptime\":" +
    String(millis() / 1000);

  s += "}";

  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", s);
}

// =========================================================
// FLIGHT FILES
// =========================================================

bool validFlightName(String name) {
  if (name.length() != 13 ||
      !name.startsWith("FLIGHT") ||
      !name.endsWith(".BIN")) {
    return false;
  }

  for (int i = 6; i < 9; i++) {
    if (!isDigit(name[i])) return false;
  }

  return true;
}

void listFlightFiles() {
  if (!sdOK || recording) {
    sendResponse(false, "SD unavailable or recording");
    return;
  }

  File root = SD.open("/");

  if (!root) {
    sendResponse(false, "Cannot open SD card");
    return;
  }

  String output = "{\"files\":[";
  bool first = true;

  File f = root.openNextFile();

  while (f) {
    String name = String(f.name());

    int slash = name.lastIndexOf('/');

    if (slash >= 0) {
      name = name.substring(slash + 1);
    }

    if (!f.isDirectory() && validFlightName(name)) {
      if (!first) output += ",";

      first = false;

      output += "{\"name\":\"";
      output += name;
      output += "\",\"size\":";
      output += String((uint32_t)f.size());
      output += "}";
    }

    f.close();
    f = root.openNextFile();
  }

  root.close();

  output += "],\"total\":";
  output += String((unsigned long long)SD.totalBytes());
  output += ",\"used\":";
  output += String((unsigned long long)SD.usedBytes());
  output += "}";

  server.send(200, "application/json", output);
}

void downloadFlight() {
  if (!sdOK || recording) {
    server.send(409, "text/plain", "SD unavailable");
    return;
  }

  String name = server.arg("name");

  if (!validFlightName(name)) {
    server.send(400, "text/plain", "Invalid filename");
    return;
  }

  File f = SD.open("/" + name, FILE_READ);

  if (!f) {
    server.send(404, "text/plain", "File not found");
    return;
  }

  server.sendHeader(
    "Content-Disposition",
    "attachment; filename=\"" + name + "\""
  );

  server.streamFile(
    f,
    "application/octet-stream"
  );

  f.close();
}

void deleteFlight() {
  if (!sdOK || recording || armed) {
    sendResponse(false, "SD busy");
    return;
  }

  String name = server.arg("name");

  if (!validFlightName(name)) {
    sendResponse(false, "Invalid filename");
    return;
  }

  bool success = SD.remove("/" + name);

  sendResponse(
    success,
    success ? "Flight deleted" : "Delete failed"
  );
}

// =========================================================
// EMBEDDED WEB DASHBOARD
// =========================================================

const char PAGE[] PROGMEM = R"GREENION(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport"
content="width=device-width,initial-scale=1">

<title>Green Ion V8.1</title>

<style>
:root{
 --bg:#081420;
 --card:#162a3f;
 --border:#35536f;
 --text:#f2f8ff;
 --muted:#a8bed2;
 --blue:#77d9ff;
 --green:#72e7ac;
 --red:#ff8f9b;
 --yellow:#ffda85;
}

*{box-sizing:border-box}

body{
 margin:0;
 background:var(--bg);
 color:var(--text);
 font:15px/1.5 system-ui,Arial,sans-serif;
}

main{
 max-width:900px;
 margin:auto;
 padding:20px 15px 55px;
}

h1,h2,h3,p{margin-top:0}

h1{
 font-size:27px;
 letter-spacing:1px;
 margin-bottom:2px;
}

h2{
 font-size:19px;
 margin-bottom:16px;
}

small,.muted{color:var(--muted)}

header{
 display:flex;
 align-items:center;
 justify-content:space-between;
 gap:18px;
 margin-bottom:20px;
}

.connection{
 padding:10px 14px;
 border:1px solid var(--border);
 border-radius:11px;
 color:var(--yellow);
 white-space:nowrap;
 font-weight:750;
}

.units{
 display:flex;
 align-items:center;
 justify-content:space-between;
 flex-wrap:wrap;
 gap:16px;
 border:1px solid var(--border);
 background:var(--card);
 padding:17px;
 border-radius:15px;
 margin-bottom:20px;
}

button{
 background:var(--blue);
 color:#071522;
 border:0;
 border-radius:11px;
 min-height:46px;
 padding:13px 15px;
 font:750 14px system-ui,Arial,sans-serif;
 cursor:pointer;
}

button:disabled{
 opacity:.45;
 cursor:not-allowed;
}

button.secondary{
 background:#385a77;
 color:white;
}

button.warning{background:var(--yellow)}
button.green{background:var(--green)}
button.danger{background:#934352;color:white}

button.active{
 background:var(--blue)!important;
 color:#071522!important;
}

.unitButton{
 background:#2a4964;
 color:white;
}

.tabs{
 display:flex;
 gap:9px;
 overflow-x:auto;
 margin-bottom:19px;
}

.tabs button{
 flex:1;
 min-width:96px;
 background:#28425e;
 color:white;
}

.page{display:none}
.page.active{display:block}

.metrics{
 display:grid;
 grid-template-columns:repeat(2,minmax(0,1fr));
 gap:13px;
 margin-bottom:18px;
}

.metric,.card{
 background:var(--card);
 border:1px solid var(--border);
 border-radius:16px;
 padding:19px;
}

.card{margin-bottom:17px}

.metricLabel{
 font-size:12px;
 color:var(--muted);
}

.metricValue{
 font-weight:850;
 font-size:clamp(21px,5vw,34px);
 margin-top:9px;
 overflow-wrap:anywhere;
 font-variant-numeric:tabular-nums;
}

.metricValue.large{
 font-size:clamp(35px,7vw,56px);
 color:var(--blue);
}

.batteryNumber{color:var(--green)!important}

.metricNote{
 font-size:12px;
 color:var(--muted);
 margin-top:6px;
}

.row{
 display:flex;
 align-items:center;
 justify-content:space-between;
 gap:15px;
 padding:13px 0;
 border-bottom:1px solid var(--border);
}

.row:last-child{border-bottom:0}

.row strong{
 text-align:right;
 overflow-wrap:anywhere;
}

.actions{
 display:grid;
 grid-template-columns:1fr 1fr;
 gap:12px;
}

.actions button{width:100%}
.wide{grid-column:1/-1}

.notice{
 background:#29445e;
 padding:15px;
 border-radius:12px;
 min-height:50px;
 margin:16px 0;
 overflow-wrap:anywhere;
}

.checks{
 display:flex;
 gap:9px;
 flex-wrap:wrap;
 margin:15px 0;
}

.check{
 padding:9px 12px;
 background:#29425b;
 border-radius:9px;
 font-size:12px;
}

.ok{color:var(--green)}
.bad{color:var(--red)}

.progress{
 background:#304b66;
 height:10px;
 border-radius:10px;
 overflow:hidden;
 margin:13px 0;
}

.progressFill{
 background:var(--blue);
 height:100%;
 width:0%;
 transition:width .2s;
}

canvas{
 display:block;
 width:100%;
 height:190px;
}

select,input[type=range]{
 display:block;
 width:100%;
 border-radius:10px;
 margin:12px 0;
 padding:12px;
 border:1px solid var(--border);
 background:#2b465f;
 color:white;
}

.swatches{
 display:flex;
 gap:10px;
 flex-wrap:wrap;
 margin:16px 0;
}

.swatch{
 width:44px;
 height:44px;
 min-height:44px;
 border-radius:50%;
 border:2px solid white;
 padding:0;
}

.guide{
 display:grid;
 grid-template-columns:18px 1fr;
 align-items:center;
 gap:15px;
}

.ledDot{
 width:16px;
 height:16px;
 border-radius:50%;
}

.file{
 display:flex;
 justify-content:space-between;
 align-items:center;
 gap:12px;
 flex-wrap:wrap;
 border-bottom:1px solid var(--border);
 padding:14px 0;
}

.fileActions{
 display:flex;
 gap:8px;
}

.fileActions button{
 padding:10px;
 font-size:12px;
}

@media(max-width:470px){
 main{padding:15px 12px 45px}
 .metric,.card{padding:16px}
 h1{font-size:23px}
 header{gap:10px}
}
</style>
</head>

<body>
<main>

<header>
 <div>
  <h1>GREEN ION</h1>
  <small>V8.1 · Flight Recorder</small>
 </div>

 <div class="connection" id="connection">
  CONNECTING
 </div>
</header>

<div class="units">
 <div>
  <strong>Measurement system</strong>
  <div class="muted">
   <small>Changes all main measurement units</small>
  </div>
 </div>

 <div>
  <button id="metricButton" class="unitButton active">
   Metric
  </button>
  <button id="imperialButton" class="unitButton">
   Imperial
  </button>
 </div>
</div>

<nav class="tabs">
 <button class="active" data-tab="overview">Overview</button>
 <button data-tab="testing">Testing</button>
 <button data-tab="lights">Lights</button>
 <button data-tab="storage">Storage</button>
 <button data-tab="system">System</button>
</nav>

<!-- OVERVIEW -->

<section class="page active" id="overview">

<div class="metrics">

 <div class="metric">
  <div class="metricLabel">LIVE ALTITUDE</div>
  <div class="metricValue large" id="altitude">--</div>
  <div class="metricNote" id="altitudeNote">
   Awaiting calibration
  </div>
 </div>

 <div class="metric">
  <div class="metricLabel">BATTERY ESTIMATE</div>
  <div class="metricValue large batteryNumber"
       id="batteryPercent">--</div>
  <div class="metricNote" id="batteryVoltage">--</div>
  <div class="metricNote">
   Estimated from measured voltage
  </div>
 </div>

 <div class="metric">
  <div class="metricLabel">Vertical speed</div>
  <div class="metricValue" id="speed">--</div>
 </div>

 <div class="metric">
  <div class="metricLabel">Peak altitude</div>
  <div class="metricValue" id="peak">--</div>
 </div>

 <div class="metric">
  <div class="metricLabel">Temperature</div>
  <div class="metricValue" id="temperature">--</div>
 </div>

 <div class="metric">
  <div class="metricLabel">Atmospheric pressure</div>
  <div class="metricValue" id="pressure">--</div>
  <div class="metricNote" id="psi">-- PSI</div>
 </div>

</div>

<div class="card">
 <h2>Flight controls</h2>

 <div class="row">
  <span>Flight status</span>
  <strong id="flightState">CONNECTING</strong>
 </div>

 <div class="checks" id="checks"></div>

 <div class="notice" id="commandMessage"
      role="status" aria-live="polite">
  Waiting for connection...
 </div>

 <div id="calibrationProgressArea" style="display:none">
  <div class="progress">
   <div class="progressFill"
        id="calibrationProgress"></div>
  </div>
  <small id="calibrationProgressText">
   Collecting readings
  </small>
 </div>

 <div class="actions">
  <button id="calibrateButton">
   1 · CALIBRATE
  </button>

  <button id="armButton" class="warning">
   2 · ARM
  </button>

  <button id="startButton" class="wide green">
   3 · START RECORDING
  </button>

  <button id="stopButton" class="danger">
   STOP & SAVE
  </button>

  <button id="disconnectButton" class="secondary">
   DISCONNECT WI-FI
  </button>
 </div>

 <p class="muted" style="margin-top:17px">
  Calibrate at ground level. Live altitude then
  works even before arming or recording.
 </p>
</div>

<div class="card">
 <h2>Live altitude graph</h2>
 <canvas id="overviewGraph"></canvas>
</div>

</section>

<!-- TESTING -->

<section class="page" id="testing">

<div class="card">
 <h2>Sensor testing</h2>

 <button id="testCalibrateButton">
  CALIBRATE ALTITUDE
 </button>

 <div class="notice" id="testingMessage">
  Connect to the device to begin testing.
 </div>

 <div class="actions">
  <button id="simulateButton">
   START SIMULATION
  </button>

  <button id="stopSimButton" class="secondary">
   STOP SIMULATION
  </button>
 </div>

 <p class="muted" style="margin-top:16px">
  Simulated altitude is shown on the dashboard
  but is not written into flight logs.
 </p>
</div>

<div class="card">
 <h2>Graph controls</h2>

 <div class="actions">
  <button class="secondary" id="pauseGraphsButton">
   Pause graphs
  </button>

  <button class="secondary" id="clearGraphsButton">
   Clear history
  </button>
 </div>

 <select id="graphWindow">
  <option value="30">Last 30 readings</option>
  <option value="60">Last 60 readings</option>
  <option value="120" selected>Last 120 readings</option>
 </select>
</div>

<div class="card">
 <h2>Altitude</h2>
 <canvas id="altitudeGraph"></canvas>
</div>

<div class="card">
 <h2>Pressure</h2>
 <canvas id="pressureGraph"></canvas>
</div>

<div class="card">
 <h2>Temperature</h2>
 <canvas id="temperatureGraph"></canvas>
</div>

<div class="card">
 <h2>Sensor diagnostics</h2>

 <div class="row">
  <span>Reading freshness</span>
  <strong id="freshness">--</strong>
 </div>

 <div class="row">
  <span>Valid / failed readings</span>
  <strong id="readCounts">--</strong>
 </div>

 <div class="row">
  <span>Calibration count</span>
  <strong id="calibrationCount">--</strong>
 </div>

 <div class="row">
  <span>Calibration age</span>
  <strong id="calibrationAge">--</strong>
 </div>

 <div class="row">
  <span>Ground pressure reference</span>
  <strong id="baseline">--</strong>
 </div>

 <div class="row">
  <span>Last calibration result</span>
  <strong id="calibrationResult">--</strong>
 </div>
</div>

</section>

<!-- LIGHTS -->

<section class="page" id="lights">

<div class="card">
 <h2>RGB LED controls</h2>

 <div class="row">
  <span>Mode</span>
  <strong id="ledMode">AUTO</strong>
 </div>

 <h3 style="margin-top:17px">Color</h3>
 <div class="swatches" id="swatches"></div>

 <label for="ledEffect">Effect</label>
 <select id="ledEffect">
  <option value="0">Solid</option>
  <option value="1">Blink</option>
  <option value="2">Breathe</option>
  <option value="3">Rainbow</option>
  <option value="4">Off</option>
 </select>

 <label for="brightness">
  Brightness
  <strong id="brightnessValue">45%</strong>
 </label>

 <input id="brightness" type="range"
        min="1" max="100" value="45">

 <div class="actions">
  <button id="ledOffButton" class="danger">
   TURN RGB OFF
  </button>

  <button id="ledAutoButton" class="secondary">
   AUTO STATUS
  </button>
 </div>

 <div class="notice" id="ledMessage">
  Automatic status mode.
 </div>

 <p class="muted">
  Warning lights and recording override manual RGB
  settings. Separate hardware LEDs remain independent.
 </p>
</div>

<div class="card">
 <h2>Automatic LED guide</h2>

 <div class="guide">
  <div class="ledDot" style="background:#357bff"></div>
  <div>Blue blinking — not calibrated</div>

  <div class="ledDot" style="background:#22c55e"></div>
  <div>Green solid — calibrated and ready for checks</div>

  <div class="ledDot" style="background:#eab308"></div>
  <div>Yellow solid — armed</div>

  <div class="ledDot" style="background:#22c55e"></div>
  <div>Green blinking — recording</div>

  <div class="ledDot" style="background:#a855f7"></div>
  <div>Purple blinking — simulation</div>

  <div class="ledDot" style="background:#ef4444"></div>
  <div>Red blinking — sensor or SD fault</div>

  <div class="ledDot" style="background:#f59e0b"></div>
  <div>Alternating red/orange — critical battery</div>
 </div>
</div>

</section>

<!-- STORAGE -->

<section class="page" id="storage">

<div class="card">
 <h2>microSD diagnostics</h2>

 <div class="notice" id="sdMessage">
  SD status not loaded.
 </div>

 <button id="testSDButton">
  DETECT & TEST SD CARD
 </button>

 <p class="muted" style="margin-top:15px">
  Calibration, simulation, graphs and live altitude
  work without a microSD card.
 </p>
</div>

<div class="card">
 <h2>Flight files</h2>

 <div class="row">
  <span>Storage used</span>
  <strong id="storageUsed">--</strong>
 </div>

 <button class="secondary" id="refreshFilesButton">
  REFRESH FILES
 </button>

 <div id="fileList"></div>
</div>

</section>

<!-- SYSTEM -->

<section class="page" id="system">

<div class="card">
 <h2>Battery diagnostics</h2>

 <div class="row">
  <span>Battery estimate</span>
  <strong id="sysPercent">--</strong>
 </div>

 <div class="row">
  <span>Measured voltage</span>
  <strong id="sysBattery">--</strong>
 </div>

 <div class="row">
  <span>Battery detection</span>
  <strong id="sysBatteryDetected">--</strong>
 </div>

 <div class="row">
  <span>Battery warning</span>
  <strong id="sysBatteryWarning">--</strong>
 </div>

 <div class="row">
  <span>USB power</span>
  <strong id="sysUSB">Unknown</strong>
 </div>

 <div class="row">
  <span>Charging</span>
  <strong>Unknown</strong>
 </div>

 <p class="muted" style="margin-top:16px">
  Battery percentage is an approximate estimate from
  voltage, not a measured state of charge. Charging
  cannot be confirmed without additional hardware.
 </p>
</div>

<div class="card">
 <h2>Recorder diagnostics</h2>

 <div class="row">
  <span>Samples recorded</span>
  <strong id="sampleCount">--</strong>
 </div>

 <div class="row">
  <span>Invalid / missed samples</span>
  <strong id="sampleErrors">--</strong>
 </div>

 <div class="row">
  <span>SD write errors</span>
  <strong id="sdErrors">--</strong>
 </div>

 <div class="row">
  <span>Recording duration</span>
  <strong id="duration">--</strong>
 </div>

 <div class="row">
  <span>Current file</span>
  <strong id="currentFile">--</strong>
 </div>

 <div class="row">
  <span>Launch detected</span>
  <strong id="launchStatus">--</strong>
 </div>

 <div class="row">
  <span>Finish reason</span>
  <strong id="finishReason">--</strong>
 </div>

 <div class="row">
  <span>Uptime</span>
  <strong id="uptime">--</strong>
 </div>

 <div class="row">
  <span>Last error</span>
  <strong id="lastError">--</strong>
 </div>
</div>

</section>

</main>

<script>
"use strict";

// ======================================================
// DOM AND STATE
// ======================================================

const byId = id => document.getElementById(id);

let latest = null;
let imperial = false;
let commandBusy = false;
let graphPaused = false;
let calibrationWasRunning = false;
let receivedFirstStatus = false;

let ledColor = [0,180,255];

let graphs = {
 altitude: [],
 pressure: [],
 temperature: []
};

// ======================================================
// ERROR DISPLAY
// ======================================================

function showMessage(text,success=null) {
 const color =
  success === true ? '#72e7ac' :
  success === false ? '#ff8f9b' :
  '#ffda85';

 for (const id of ['commandMessage','testingMessage']) {
  const el = byId(id);

  if (el) {
   el.textContent = text;
   el.style.color = color;
  }
 }
}

window.addEventListener('error',event => {
 showMessage(
  'Dashboard error: ' + event.message,
  false
 );
});

// ======================================================
// UNITS
// ======================================================

function fmt(v,places=2) {
 if (v === null ||
     v === undefined ||
     !Number.isFinite(Number(v))) {
  return '--';
 }

 return Number(v).toFixed(places);
}

function altitudeUnit() {
 return imperial ? 'ft' : 'm';
}

function convertAltitude(v) {
 if (v == null) return null;
 return imperial ? v * 3.280839895 : v;
}

function convertTemperature(v) {
 if (v == null) return null;
 return imperial ? v * 9/5 + 32 : v;
}

function convertPressure(v) {
 if (v == null) return null;
 return imperial ? v / 3386.389 : v / 100;
}

function setUnits(value) {
 imperial = value;

 byId('metricButton').classList.toggle(
  'active',!value
 );

 byId('imperialButton').classList.toggle(
  'active',value
 );

 if (latest) renderStatus(latest,false);

 drawAllGraphs();
}

// ======================================================
// TABS
// ======================================================

function openTab(name,button) {
 document.querySelectorAll('.page').forEach(el => {
  el.classList.remove('active');
 });

 document.querySelectorAll('.tabs button').forEach(el => {
  el.classList.remove('active');
 });

 byId(name).classList.add('active');
 button.classList.add('active');

 if (name === 'storage') loadFiles();

 drawAllGraphs();
}

// ======================================================
// CHECKLIST
// ======================================================

function checklist(label,passed) {
 const item = document.createElement('span');

 item.className =
  'check ' + (passed ? 'ok' : 'bad');

 item.textContent =
  (passed ? '✓ ' : '× ') + label;

 return item;
}

// ======================================================
// STATUS RENDERING
// ======================================================

function renderStatus(d,collect=true) {
 latest = d;

 byId('connection').textContent = '● LIVE';
 byId('connection').style.color = '#72e7ac';

 if (!receivedFirstStatus) {
  receivedFirstStatus = true;

  showMessage(
   'ESP32 connected. ' +
   (d.calibrated
    ? 'Altitude is calibrated.'
    : 'Press CALIBRATE to begin live altitude testing.'),
   true
  );
 }

 const shownAltitude = d.simulation
  ? d.simAltitude
  : d.altitude;

 const shownSpeed = d.simulation
  ? d.simSpeed
  : d.speed;

 // IMPORTANT FIX:
 // Do NOT display CALIBRATE in the large altitude field.
 byId('altitude').textContent =
  (d.calibrated || d.simulation)
   ? fmt(convertAltitude(shownAltitude)) +
     ' ' + altitudeUnit()
   : '--';

 byId('altitudeNote').textContent =
  d.calibrating
   ? 'Calibrating...'
   : d.simulation
    ? 'Simulated altitude'
    : d.calibrated
     ? 'Live altitude relative to ground'
     : 'Awaiting calibration';

 byId('batteryPercent').textContent =
  d.batteryPercent < 0
   ? '--'
   : d.batteryPercent + '%';

 byId('batteryVoltage').textContent =
  fmt(d.battery,3) + ' V' +
  (d.batteryCritical
   ? ' · CRITICAL'
   : d.batteryLow
    ? ' · LOW'
    : '');

 byId('batteryPercent').style.color =
  d.batteryCritical ? '#ff8f9b' :
  d.batteryLow ? '#ffda85' :
  '#72e7ac';

 byId('speed').textContent =
  fmt(convertAltitude(shownSpeed)) +
  (imperial ? ' ft/s' : ' m/s');

 byId('peak').textContent =
  fmt(convertAltitude(d.peak)) +
  ' ' + altitudeUnit();

 byId('temperature').textContent =
  fmt(convertTemperature(d.temperature),1) +
  (imperial ? ' °F' : ' °C');

 byId('pressure').textContent =
  fmt(convertPressure(d.pressure),
   imperial ? 3 : 1) +
  (imperial ? ' inHg' : ' hPa');

 byId('psi').textContent =
  fmt(
   d.pressure == null
    ? null
    : d.pressure / 6894.757293,
   3
  ) + ' PSI absolute';

 byId('flightState').textContent =
  d.calibrating ? 'CALIBRATING' :
  d.recording
   ? (d.launched ? 'IN FLIGHT' : 'RECORDING')
   : d.armed ? 'ARMED'
   : d.simulation ? 'SIMULATING'
   : d.calibrated ? 'PREFLIGHT LIVE'
   : 'NOT CALIBRATED';

 const checks = byId('checks');
 checks.replaceChildren(
  checklist('BMP388',d.sensor && d.reading),
  checklist('Calibrated',d.calibrated),
  checklist('SD ready',d.sdVerified),
  checklist('Battery',!d.batteryCritical)
 );

 const locked =
  commandBusy || d.calibrating;

 byId('calibrateButton').disabled =
  locked || d.armed || d.recording || d.simulation;

 byId('testCalibrateButton').disabled =
  locked || d.armed || d.recording || d.simulation;

 byId('armButton').disabled =
  locked || d.armed || d.recording ||
  d.simulation || !d.sensor || !d.reading ||
  !d.calibrated || !d.sdVerified ||
  d.sdFault || d.batteryCritical;

 byId('startButton').disabled =
  locked || !d.armed || d.recording;

 byId('stopButton').disabled =
  locked || !d.recording;

 byId('disconnectButton').disabled =
  locked || !d.recording;

 byId('simulateButton').disabled =
  locked || d.armed || d.recording ||
  d.simulation;

 byId('stopSimButton').disabled =
  commandBusy || !d.simulation;

 byId('testSDButton').disabled =
  locked || d.armed || d.recording;

 // Calibration progress
 const progressArea =
  byId('calibrationProgressArea');

 progressArea.style.display =
  d.calibrating ? 'block' : 'none';

 const percent = d.calibrationTotal > 0
  ? Math.min(
      100,
      d.calibrationStep /
      d.calibrationTotal * 100
    )
  : 0;

 byId('calibrationProgress').style.width =
  percent + '%';

 byId('calibrationProgressText').textContent =
  'Readings ' +
  d.calibrationStep + '/' +
  d.calibrationTotal +
  ' · Valid: ' + d.calibrationValid;

 if (d.calibrating) {
  showMessage(
   'Calibrating BMP388... ' +
   d.calibrationStep + '/' +
   d.calibrationTotal +
   ' readings collected',
   null
  );
 }

 if (calibrationWasRunning && !d.calibrating) {
  showMessage(
   d.calibrationResult,
   d.calibrated
  );

  if (d.calibrated) {
   clearGraphs();
  }
 }

 calibrationWasRunning = d.calibrating;

 // Sensor diagnostics
 byId('freshness').textContent =
  d.freshness == null
   ? 'No valid readings'
   : d.freshness + ' ms ago';

 byId('readCounts').textContent =
  d.goodReads + ' / ' + d.failedReads;

 byId('calibrationCount').textContent =
  d.calibrationCount;

 byId('calibrationAge').textContent =
  d.calibrationAge == null
   ? 'Never'
   : d.calibrationAge + ' s ago';

 byId('baseline').textContent =
  fmt(d.baseline,2) + ' Pa';

 byId('calibrationResult').textContent =
  d.calibrationResult;

 // Storage
 byId('sdMessage').textContent =
  d.sdMessage;

 // LEDs
 byId('ledMode').textContent =
  d.ledManual ? 'MANUAL' : 'AUTO';

 // Battery
 byId('sysPercent').textContent =
  d.batteryPercent < 0
   ? 'Unknown'
   : d.batteryPercent + '% estimated';

 byId('sysBattery').textContent =
  fmt(d.battery,3) + ' V';

 byId('sysBatteryDetected').textContent =
  d.batteryPresent
   ? 'Likely connected'
   : 'Unknown / not detected';

 byId('sysBatteryWarning').textContent =
  d.batteryCritical ? 'CRITICAL' :
  d.batteryLow ? 'LOW' : 'No warning';

 byId('sysUSB').textContent =
  !d.usbSensor
   ? 'Unknown - no USB sensor'
   : d.usbPresent
    ? 'Detected'
    : 'Not detected';

 // Recording
 byId('sampleCount').textContent =
  d.samples;

 byId('sampleErrors').textContent =
  d.invalidSamples + ' / ' + d.missedSamples;

 byId('sdErrors').textContent =
  d.sdErrors;

 byId('duration').textContent =
  fmt(d.duration,1) + ' s';

 byId('currentFile').textContent =
  d.file;

 byId('launchStatus').textContent =
  d.launched ? 'YES' : 'NO';

 byId('finishReason').textContent =
  d.finish || '--';

 byId('uptime').textContent =
  d.uptime + ' s';

 byId('lastError').textContent =
  d.error || 'None';

 if (collect && !graphPaused) {
  appendGraph('altitude',shownAltitude);
  appendGraph('pressure',d.pressure);
  appendGraph('temperature',d.temperature);

  drawAllGraphs();
 }
}

// ======================================================
// NETWORK REQUESTS
// ======================================================

async function requestJSON(url,method='GET',body=null) {
 const controller = new AbortController();

 const timeout = setTimeout(
  () => controller.abort(),
  8000
 );

 try {
  const options = {
   method:method,
   cache:'no-store',
   signal:controller.signal
  };

  if (body !== null) {
   options.headers = {
    'Content-Type':
     'application/x-www-form-urlencoded'
   };

   options.body = new URLSearchParams(body).toString();
  }

  const response = await fetch(url,options);

  const text = await response.text();

  let data;

  try {
   data = JSON.parse(text);
  }
  catch(e) {
   throw Error(
    'Device returned an invalid response'
   );
  }

  if (!response.ok || data.success === false) {
   throw Error(
    data.message || 'HTTP ' + response.status
   );
  }

  return data;
 }
 finally {
  clearTimeout(timeout);
 }
}

// ======================================================
// STATUS POLLING
// ======================================================

let polling = false;

async function poll() {
 if (polling) return;

 polling = true;

 try {
  const d = await requestJSON('/api/status');
  renderStatus(d);
 }
 catch(error) {
  byId('connection').textContent = '● OFFLINE';
  byId('connection').style.color = '#ff8f9b';

  if (!receivedFirstStatus) {
   showMessage(
    'Connection failed: ' + error.message,
    false
   );
  }
 }
 finally {
  polling = false;
 }
}

// ======================================================
// COMMAND BUTTONS
// ======================================================

async function sendCommand(name) {
 if (commandBusy) return;

 commandBusy = true;

 const messages = {
  calibrate:
   'Calibration request sent to ESP32...',
  arm:
   'Checking flight preflight...',
  start:
   'Opening flight log...',
  stop:
   'Stopping and saving flight...',
  sdtest:
   'Testing SD card read/write...',
  simulate:
   'Starting simulated flight...',
  simstop:
   'Stopping simulation...'
 };

 showMessage(
  messages[name] || 'Sending command...',
  null
 );

 try {
  const result =
   await requestJSON('/api/' + name,'POST');

  showMessage(
   result.message || 'Command accepted',
   true
  );
 }
 catch(error) {
  showMessage(
   name.toUpperCase() + ': ' + error.message,
   false
  );
 }
 finally {
  commandBusy = false;
  await poll();
 }
}

function stopFlightCommand() {
 if (confirm('Stop and save the flight?')) {
  sendCommand('stop');
 }
}

async function disconnectWifi() {
 if (!confirm(
  'Disconnect Wi-Fi during recording?'
 )) return;

 showMessage(
  'Wi-Fi disconnect requested...',
  null
 );

 try {
  await requestJSON(
   '/api/disconnect',
   'POST'
  );
 }
 catch(error) {
  showMessage(error.message,false);
 }

 byId('connection').textContent = '● OFFLINE';
}

// ======================================================
// GRAPHS
// ======================================================

function appendGraph(name,value) {
 if (value == null || !Number.isFinite(value)) {
  return;
 }

 graphs[name].push(value);

 if (graphs[name].length > 120) {
  graphs[name].shift();
 }
}

function drawGraph(id,name,conversion) {
 const canvas = byId(id);

 const width = Math.max(
  canvas.clientWidth,
  100
 );

 const height = 190;
 const ratio = window.devicePixelRatio || 1;

 canvas.width = width * ratio;
 canvas.height = height * ratio;

 const ctx = canvas.getContext('2d');

 ctx.scale(ratio,ratio);

 const count = Number(byId('graphWindow').value);

 const values = graphs[name]
  .slice(-count)
  .map(conversion)
  .filter(Number.isFinite);

 let min = values.length
  ? Math.min(...values)
  : 0;

 let max = values.length
  ? Math.max(...values)
  : 1;

 if (max-min < .01) {
  min -= .5;
  max += .5;
 }

 ctx.strokeStyle = '#395773';
 ctx.lineWidth = 1;

 for (let i=0;i<4;i++) {
  const y = 25+i*45;

  ctx.beginPath();
  ctx.moveTo(25,y);
  ctx.lineTo(width-10,y);
  ctx.stroke();
 }

 ctx.fillStyle = '#a8bed2';
 ctx.font = '11px system-ui';

 ctx.fillText(fmt(max),25,14);
 ctx.fillText(fmt(min),25,185);

 if (values.length < 2) return;

 ctx.strokeStyle = '#77d9ff';
 ctx.lineWidth = 2.5;
 ctx.beginPath();

 values.forEach((v,i) => {
  const x =
   25+i*(width-38)/(values.length-1);

  const y =
   165-(v-min)/(max-min)*135;

  if (i===0) ctx.moveTo(x,y);
  else ctx.lineTo(x,y);
 });

 ctx.stroke();
}

function drawAllGraphs() {
 drawGraph(
  'overviewGraph',
  'altitude',
  convertAltitude
 );

 drawGraph(
  'altitudeGraph',
  'altitude',
  convertAltitude
 );

 drawGraph(
  'pressureGraph',
  'pressure',
  convertPressure
 );

 drawGraph(
  'temperatureGraph',
  'temperature',
  convertTemperature
 );
}

function clearGraphs() {
 graphs.altitude.length=0;
 graphs.pressure.length=0;
 graphs.temperature.length=0;

 drawAllGraphs();
}

function toggleGraphPause() {
 graphPaused = !graphPaused;

 byId('pauseGraphsButton').textContent =
  graphPaused ? 'Resume graphs' : 'Pause graphs';
}

// ======================================================
// LED CONTROLS
// ======================================================

async function sendLED(settings) {
 try {
  const result = await requestJSON(
   '/api/led',
   'POST',
   settings
  );

  byId('ledMessage').textContent =
   result.message;

  await poll();
 }
 catch(error) {
  byId('ledMessage').textContent =
   error.message;
 }
}

function applyLED() {
 sendLED({
  manual:'1',
  r:ledColor[0],
  g:ledColor[1],
  b:ledColor[2],
  effect:byId('ledEffect').value,
  brightness:byId('brightness').value
 });
}

function turnLEDOff() {
 byId('ledEffect').value='4';

 sendLED({
  manual:'1',
  effect:'4',
  brightness:'1'
 });
}

function restoreAuto() {
 sendLED({
  manual:'0'
 });
}

function createSwatches() {
 const colors = [
  '#ff4444',
  '#ff9933',
  '#ffe750',
  '#52e798',
  '#32d9ff',
  '#3d80ff',
  '#b768ff',
  '#ffffff'
 ];

 for (const hex of colors) {
  const button = document.createElement('button');

  button.className = 'swatch';
  button.style.background = hex;
  button.title = hex;

  button.addEventListener('click',() => {
   ledColor = [1,3,5].map(
    i => parseInt(hex.slice(i,i+2),16)
   );

   applyLED();
  });

  byId('swatches').appendChild(button);
 }
}

// ======================================================
// FILE MANAGER
// ======================================================

function formatBytes(value) {
 if (value >= 1048576) {
  return (value/1048576).toFixed(2)+' MB';
 }

 if (value >= 1024) {
  return (value/1024).toFixed(1)+' KB';
 }

 return value+' bytes';
}

async function loadFiles() {
 try {
  const data = await requestJSON('/api/files');

  byId('storageUsed').textContent =
   formatBytes(data.used) +
   ' / ' +
   formatBytes(data.total);

  const container = byId('fileList');

  container.replaceChildren();

  if (!data.files.length) {
   container.textContent =
    'No flight files found.';
   return;
  }

  for (const file of data.files) {
   const row = document.createElement('div');
   row.className = 'file';

   const label = document.createElement('span');
   label.textContent =
    file.name+' ('+formatBytes(file.size)+')';

   const actions = document.createElement('div');
   actions.className = 'fileActions';

   const download = document.createElement('button');
   download.textContent = 'Download';

   download.addEventListener('click',() => {
    location.href =
     '/api/download?name=' +
     encodeURIComponent(file.name);
   });

   const remove = document.createElement('button');
   remove.textContent='Delete';
   remove.className='danger';

   remove.addEventListener('click',async () => {
    if (!confirm('Delete '+file.name+'?')) return;

    try {
     const result = await requestJSON(
      '/api/delete?name=' +
      encodeURIComponent(file.name),
      'POST'
     );

     alert(result.message);
     await loadFiles();
    }
    catch(error) {
     alert(error.message);
    }
   });

   actions.append(download,remove);
   row.append(label,actions);
   container.appendChild(row);
  }
 }
 catch(error) {
  byId('fileList').textContent =
   'Storage error: '+error.message;
 }
}

// ======================================================
// ATTACH ALL BUTTON HANDLERS
// ======================================================

function initializeUI() {
 byId('metricButton').addEventListener(
  'click',() => setUnits(false)
 );

 byId('imperialButton').addEventListener(
  'click',() => setUnits(true)
 );

 document.querySelectorAll('[data-tab]').forEach(
  button => {
   button.addEventListener('click',() => {
    openTab(button.dataset.tab,button);
   });
  }
 );

 byId('calibrateButton').addEventListener(
  'click',() => sendCommand('calibrate')
 );

 byId('testCalibrateButton').addEventListener(
  'click',() => sendCommand('calibrate')
 );

 byId('armButton').addEventListener(
  'click',() => sendCommand('arm')
 );

 byId('startButton').addEventListener(
  'click',() => sendCommand('start')
 );

 byId('stopButton').addEventListener(
  'click',stopFlightCommand
 );

 byId('disconnectButton').addEventListener(
  'click',disconnectWifi
 );

 byId('simulateButton').addEventListener(
  'click',() => sendCommand('simulate')
 );

 byId('stopSimButton').addEventListener(
  'click',() => sendCommand('simstop')
 );

 byId('testSDButton').addEventListener(
  'click',() => sendCommand('sdtest')
 );

 byId('refreshFilesButton').addEventListener(
  'click',loadFiles
 );

 byId('pauseGraphsButton').addEventListener(
  'click',toggleGraphPause
 );

 byId('clearGraphsButton').addEventListener(
  'click',clearGraphs
 );

 byId('graphWindow').addEventListener(
  'change',drawAllGraphs
 );

 byId('ledEffect').addEventListener(
  'change',applyLED
 );

 byId('brightness').addEventListener(
  'input',() => {
   byId('brightnessValue').textContent =
    byId('brightness').value+'%';
  }
 );

 byId('brightness').addEventListener(
  'change',applyLED
 );

 byId('ledOffButton').addEventListener(
  'click',turnLEDOff
 );

 byId('ledAutoButton').addEventListener(
  'click',restoreAuto
 );

 createSwatches();

 window.addEventListener(
  'resize',drawAllGraphs
 );

 setInterval(poll,500);

 showMessage('Dashboard ready. Checking ESP32...',null);

 poll();
}

// Script runs at bottom of body, after all controls.
initializeUI();

</script>
</body>
</html>
)GREENION";

// =========================================================
// WI-FI
// =========================================================

void startWifi() {
  if (wifiActive) return;

  WiFi.mode(WIFI_AP);

  wifiActive = WiFi.softAP(
    WIFI_SSID,
    WIFI_PASS
  );

  if (wifiActive) {
    server.begin();

    Serial.println("Wi-Fi network: GREEN-ION");
    Serial.println("URL: http://192.168.4.1");
  }
}

void stopWifi() {
  if (!wifiActive) return;

  server.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);

  wifiActive = false;
}

// =========================================================
// HTTP API
// =========================================================

void setupRoutes() {

  server.on("/",HTTP_GET,[]() {
    server.send_P(
      200,
      "text/html",
      PAGE
    );
  });

  server.on("/api/status",HTTP_GET,[]() {
    sendStatus();
  });

  // Calibration immediately acknowledges the request.
  // The main loop finishes the actual calibration.
  server.on("/api/calibrate",HTTP_POST,[]() {
    bool success = beginCalibration();

    sendResponse(
      success,
      calibrationResult
    );
  });

  server.on("/api/arm",HTTP_POST,[]() {
    bool ready =
      !armed &&
      !recording &&
      !simulation &&
      !calibrating &&
      sensorOK &&
      readingOK &&
      calibrated &&
      sdOK &&
      sdVerified &&
      !sdFault &&
      !batteryCritical() &&
      freeSDBytes() >= MIN_SD_FREE_BYTES;

    if (ready) {
      armed = true;
      manualLED = false;

      lastError = "";
      systemMessage = "Recorder armed successfully";
    }
    else {
      lastError =
        "Arm blocked: check calibration, sensor, SD and battery";
    }

    sendResponse(
      ready,
      ready ? systemMessage : lastError
    );
  });

  server.on("/api/start",HTTP_POST,[]() {
    bool success = startFlight();

    sendResponse(
      success,
      success ? systemMessage : lastError
    );
  });

  server.on("/api/stop",HTTP_POST,[]() {
    if (!recording) {
      sendResponse(false,"Not recording");
      return;
    }

    finishFlight("Manual stop");

    sendResponse(
      !sdFault,
      sdFault
        ? lastError
        : "Flight stopped and saved"
    );
  });

  server.on("/api/disconnect",HTTP_POST,[]() {
    if (!recording) {
      sendResponse(false,"Record first");
      return;
    }

    sendResponse(true,"Wi-Fi disconnecting");

    delay(250);
    stopWifi();
  });

  server.on("/api/sdtest",HTTP_POST,[]() {
    bool success = testSD();

    sendResponse(
      success,
      sdMessage
    );
  });

  server.on("/api/simulate",HTTP_POST,[]() {
    if (armed || recording || calibrating) {
      sendResponse(
        false,
        "Cannot simulate while recorder is busy"
      );
      return;
    }

    simulation = true;

    simulationStartMs = millis();
    simulationAltitudeM = 0;
    simulationSpeedMps = 0;

    systemMessage = "Simulation started";

    sendResponse(true,systemMessage);
  });

  server.on("/api/simstop",HTTP_POST,[]() {
    simulation = false;

    simulationAltitudeM = 0;
    simulationSpeedMps = 0;

    systemMessage = "Simulation stopped";

    sendResponse(true,systemMessage);
  });

  server.on("/api/led",HTTP_POST,[]() {
    if (armed ||
        recording ||
        sdFault ||
        !sensorOK ||
        batteryCritical()) {

      sendResponse(
        false,
        "Safety status overrides LED controls"
      );
      return;
    }

    manualLED =
      server.arg("manual") == "1";

    if (server.hasArg("r")) {
      ledR = constrain(
        server.arg("r").toInt(),0,255
      );
    }

    if (server.hasArg("g")) {
      ledG = constrain(
        server.arg("g").toInt(),0,255
      );
    }

    if (server.hasArg("b")) {
      ledB = constrain(
        server.arg("b").toInt(),0,255
      );
    }

    if (server.hasArg("effect")) {
      ledEffect = constrain(
        server.arg("effect").toInt(),0,4
      );
    }

    if (server.hasArg("brightness")) {
      ledBrightness = constrain(
        server.arg("brightness").toInt(),1,100
      );
    }

    sendResponse(true,"LED settings applied");
  });

  server.on("/api/files",HTTP_GET,[]() {
    listFlightFiles();
  });

  server.on("/api/download",HTTP_GET,[]() {
    downloadFlight();
  });

  server.on("/api/delete",HTTP_POST,[]() {
    deleteFlight();
  });

  server.onNotFound([]() {
    server.send(
      404,
      "application/json",
      "{\"success\":false,\"message\":\"Unknown endpoint\"}"
    );
  });
}

// =========================================================
// SETUP
// =========================================================

void setup() {
  Serial.begin(115200);
  delay(400);

  pinMode(RED_LED_PIN,OUTPUT);
  digitalWrite(RED_LED_PIN,LOW);

  pinMode(
    NEOPIXEL_I2C_POWER,
    OUTPUT
  );

  digitalWrite(
    NEOPIXEL_I2C_POWER,
    HIGH
  );

  delay(100);

  neo.begin();
  neo.clear();
  neo.show();

  analogReadResolution(12);

  analogSetPinAttenuation(
    BATTERY_PIN,
    ADC_11db
  );

  if (USB_SENSE_PIN >= 0) {
    pinMode(USB_SENSE_PIN,INPUT);

    analogSetPinAttenuation(
      USB_SENSE_PIN,
      ADC_11db
    );
  }

  updateBattery();

  Wire.begin();
  Wire.setClock(400000);

  initializeBMP();

  SPI.begin();

  if (initializeSD()) {
    testSD();
  }

  setupRoutes();
  startWifi();

  lastSampleUs = micros();
  lastBatteryMs = millis();
  lastFlushMs = millis();

  Serial.println();
  Serial.println("==========================");
  Serial.println("GREEN ION V8.1 READY");
  Serial.println("==========================");
  Serial.println("SSID: GREEN-ION");
  Serial.println("PASS: GreenIon2026");
  Serial.println("URL: http://192.168.4.1");
  Serial.println();

  if (sensorOK) {
    Serial.println("BMP388: DETECTED");
  }
  else {
    Serial.println("BMP388: NOT DETECTED");
  }

  Serial.println(
    sdOK ? "SD: DETECTED" : "SD: NOT DETECTED"
  );
}

// =========================================================
// MAIN LOOP
// =========================================================

void loop() {
  updateLED();
  updateSimulation();

  // Web requests remain responsive during calibration.
  if (wifiActive) {
    server.handleClient();
  }

  if (calibrating) {
    updateCalibration();
  }
  else {
    uint32_t nowUs = micros();

    if ((uint32_t)(nowUs-lastSampleUs) >=
        SAMPLE_INTERVAL_US) {

      uint32_t slots =
        (uint32_t)(nowUs-lastSampleUs) /
        SAMPLE_INTERVAL_US;

      if (recording && slots > 1) {
        missedSamples += slots - 1;
      }

      lastSampleUs +=
        slots * SAMPLE_INTERVAL_US;

      updateSensor();
    }
  }

  uint32_t nowMs = millis();

  if (nowMs-lastBatteryMs >= 1000) {
    lastBatteryMs = nowMs;
    updateBattery();
  }

  if (recording &&
      nowMs-lastFlushMs >= FLUSH_INTERVAL_MS) {
    flushLog();
  }

  if (recording &&
      nowMs-recordingStartMs >=
      MAX_RECORDING_MS) {
    finishFlight("Backup timeout");
  }

  // Reconnect the dashboard when recording finishes.
  if (!recording && !wifiActive) {
    startWifi();
  }

  yield();
}
