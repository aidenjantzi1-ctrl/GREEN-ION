
/*
  ==================================================
  GREEN ION V8 — ROCKET FLIGHT RECORDER
  ==================================================

  Board: Adafruit ESP32 Feather V2
  Sensor: BMP388 over I2C
  Storage: Optional SPI microSD (CS GPIO33)

  Wi-Fi network: GREEN-ION
  Password: GreenIon2026
  Dashboard: http://192.168.4.1

  Live altitude works without an SD card.
  Calibration zeroes altitude to current height.

  IMPORTANT:
  Experimental flight DATA RECORDER only.
  Not an ignition or recovery controller.
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

// ================================================
// CONFIGURATION
// ================================================

#ifndef PIN_NEOPIXEL
#define PIN_NEOPIXEL 0
#endif

#ifndef NEOPIXEL_I2C_POWER
#define NEOPIXEL_I2C_POWER 2
#endif

const int SD_CS = 33;
const int BATTERY_PIN = 35;
const int RED_LED_PIN = 13;

// Optional USB sense with proper voltage divider.
// Leave disabled unless physically installed.
const int USB_SENSE_PIN = -1;

const char* WIFI_NAME = "GREEN-ION";
const char* WIFI_PASSWORD = "GreenIon2026";

const uint32_t SAMPLE_INTERVAL_US = 40000;
const uint32_t FLUSH_INTERVAL_MS = 500;
const uint32_t MAX_RECORDING_MS = 600000;
const uint32_t LANDING_WAIT_MS = 20000;

const float LAUNCH_ALTITUDE_M = 15.0f;
const float MIN_PEAK_ALTITUDE_M = 20.0f;

const size_t RECORD_BUFFER_SIZE = 1024;

// ================================================
// HARDWARE OBJECTS
// ================================================

Adafruit_BMP3XX bmp;

Adafruit_NeoPixel rgb(
  1,
  PIN_NEOPIXEL,
  NEO_GRB + NEO_KHZ800
);

WebServer server(80);

File flightFile;

// ================================================
// SYSTEM STATE
// ================================================

bool sensorOK = false;
bool sensorReadingOK = false;
bool calibrated = false;

bool sdOK = false;
bool sdVerified = false;
bool sdFault = false;

bool armed = false;
bool recording = false;
bool launched = false;

bool simulation = false;
bool landingCandidate = false;
bool wifiActive = false;

float pressurePa = NAN;
float temperatureC = NAN;
float baselinePa = NAN;

float altitudeM = NAN;
float rawAltitudeM = NAN;
float verticalSpeedMps = 0;
float peakAltitudeM = 0;

float previousAltitudeM = NAN;
float landingReferenceM = 0;

float simulationAltitudeM = 0;
float simulationSpeedMps = 0;

uint16_t batteryMv = 0;
bool batteryLikelyPresent = false;
bool usbPresent = false;

uint32_t lastReadingMs = 0;
uint32_t lastCalibrationMs = 0;
uint32_t calibrationCount = 0;

uint32_t successfulReads = 0;
uint32_t failedReads = 0;

uint32_t lastSampleUs = 0;
uint32_t lastBatteryMs = 0;
uint32_t lastFlushMs = 0;
uint32_t lastAltitudeMs = 0;

uint32_t flightStartMs = 0;
uint32_t finishedDurationMs = 0;
uint32_t landingStartMs = 0;
uint32_t simulationStartMs = 0;

uint32_t recordedSamples = 0;
uint32_t invalidSamples = 0;
uint32_t missedSamples = 0;
uint32_t sdErrors = 0;

String currentFilename = "(none)";
String lastMessage = "Booting";
String lastError = "";
String calibrationStatus = "Not calibrated";
String sdStatus = "Not checked";
String finishReason = "";

uint8_t recordBuffer[RECORD_BUFFER_SIZE];
size_t recordBufferUsed = 0;

// ================================================
// LED STATE
// ================================================

bool ledManual = false;

int ledR = 0;
int ledG = 180;
int ledB = 255;

int ledBrightness = 45;

// 0 solid
// 1 blink
// 2 breathe
// 3 rainbow
// 4 off
int ledEffect = 0;

// ================================================
// BASIC UTILITIES
// ================================================

String jsonBool(bool v) {
  return v ? "true" : "false";
}

String jsonFloat(float v, int places = 2) {
  if (!isfinite(v)) return "null";
  return String(v, places);
}

String jsonEscape(String s) {
  s.replace("\\", "\\\\");
  s.replace("\"", "\\\"");
  s.replace("\n", " ");
  s.replace("\r", " ");
  return s;
}

void reply(bool success, String message) {
  String data = "{\"success\":";
  data += jsonBool(success);
  data += ",\"message\":\"";
  data += jsonEscape(message);
  data += "\"}";

  server.sendHeader("Cache-Control", "no-store");

  server.send(
    success ? 200 : 409,
    "application/json",
    data
  );
}

void putU16(uint8_t* p, uint16_t v) {
  p[0] = v & 255;
  p[1] = (v >> 8) & 255;
}

void putU32(uint8_t* p, uint32_t v) {
  p[0] = v & 255;
  p[1] = (v >> 8) & 255;
  p[2] = (v >> 16) & 255;
  p[3] = (v >> 24) & 255;
}

void putF32(uint8_t* p, float value) {
  uint32_t bits;
  memcpy(&bits, &value, 4);
  putU32(p, bits);
}

// ================================================
// BATTERY MONITOR
// ================================================

void updateBattery() {
  uint32_t reading =
    (uint32_t)analogReadMilliVolts(BATTERY_PIN)
    * 2UL;

  batteryMv = (uint16_t)min(
    reading,
    (uint32_t)65535
  );

  batteryLikelyPresent =
    batteryMv >= 2500 &&
    batteryMv <= 4400;

  if (USB_SENSE_PIN >= 0) {
    usbPresent =
      analogReadMilliVolts(USB_SENSE_PIN) > 1800;
  }
}

bool batteryLow() {
  return batteryLikelyPresent && batteryMv < 3500;
}

bool batteryCritical() {
  return batteryLikelyPresent && batteryMv < 3300;
}

// Approximate resting-voltage lookup.
// Not a true state-of-charge measurement.
int batteryPercent() {
  if (!batteryLikelyPresent) return -1;

  const uint16_t voltages[] = {
    3300, 3500, 3600, 3700, 3750,
    3800, 3850, 3900, 4000, 4100, 4200
  };

  const int percentages[] = {
    0, 4, 8, 17, 27,
    40, 52, 65, 80, 92, 100
  };

  if (batteryMv <= voltages[0]) return 0;

  for (int i = 1; i < 11; i++) {
    if (batteryMv <= voltages[i]) {
      float fraction =
        (batteryMv - voltages[i - 1]) /
        float(voltages[i] - voltages[i - 1]);

      return percentages[i - 1] +
        roundf(
          fraction *
          (percentages[i] - percentages[i - 1])
        );
    }
  }

  return 100;
}

// ================================================
// BMP388
// ================================================

bool initializeSensor() {
  sensorOK = bmp.begin_I2C(0x77, &Wire);

  if (!sensorOK) {
    sensorOK = bmp.begin_I2C(0x76, &Wire);
  }

  if (!sensorOK) {
    lastError =
      "BMP388 not found at 0x76 or 0x77";
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

  lastMessage = "BMP388 detected";
  return true;
}

bool readBMP(float& p, float& t) {
  if (!sensorOK || !bmp.performReading()) {
    return false;
  }

  p = bmp.pressure;
  t = bmp.temperature;

  return isfinite(p) &&
         isfinite(t) &&
         p > 30000 &&
         p < 120000 &&
         t > -50 &&
         t < 100;
}

// ================================================
// CALIBRATION
// ================================================

bool calibrateAltitude() {
  Serial.println("Calibration requested");

  if (armed || recording || simulation) {
    calibrationStatus =
      "Cannot calibrate while armed or recording";
    return false;
  }

  if (!sensorOK) {
    calibrationStatus =
      "Calibration failed: BMP388 not detected";
    return false;
  }

  calibrated = false;
  calibrationStatus =
    "Collecting BMP388 readings";

  double total = 0;
  int valid = 0;

  // Five settling reads + thirty measurements.
  for (int i = 0; i < 35; i++) {
    float p = NAN;
    float t = NAN;

    if (readBMP(p, t)) {
      successfulReads++;

      pressurePa = p;
      temperatureC = t;
      lastReadingMs = millis();
      sensorReadingOK = true;

      if (i >= 5) {
        total += p;
        valid++;
      }
    }
    else {
      failedReads++;
      sensorReadingOK = false;
    }

    delay(45);
    yield();
  }

  if (valid < 25) {
    calibrationStatus =
      "Calibration failed: insufficient valid readings";

    lastError = calibrationStatus;
    return false;
  }

  baselinePa = total / valid;

  altitudeM = 0;
  rawAltitudeM = 0;
  verticalSpeedMps = 0;
  peakAltitudeM = 0;

  previousAltitudeM = NAN;
  lastAltitudeMs = 0;

  launched = false;
  landingCandidate = false;

  calibrated = true;

  calibrationCount++;
  lastCalibrationMs = millis();

  calibrationStatus =
    "Calibration successful - ground altitude zeroed";

  lastMessage = calibrationStatus;
  lastError = "";

  lastSampleUs = micros();

  Serial.print("Calibration successful. Baseline: ");
  Serial.println(baselinePa, 2);

  return true;
}

// ================================================
// SD CARD
// ================================================

bool initializeSD() {
  sdOK = SD.begin(
    SD_CS,
    SPI,
    4000000
  );

  sdVerified = false;

  sdStatus = sdOK
    ? "SD detected - test required"
    : "SD not detected - sensor testing available";

  return sdOK;
}

uint64_t sdFreeBytes() {
  if (!sdOK) return 0;

  uint64_t total = SD.totalBytes();
  uint64_t used = SD.usedBytes();

  return total > used ? total - used : 0;
}

bool testSD() {
  if (armed || recording) {
    sdStatus = "Recorder busy";
    return false;
  }

  if (!sdOK && !initializeSD()) {
    return false;
  }

  const char* filename = "/GITEST.TMP";
  const char* pattern = "GREEN-ION-V8-SD-TEST";

  SD.remove(filename);

  File f = SD.open(filename, FILE_WRITE);

  if (!f) {
    sdVerified = false;
    sdStatus = "SD create test failed";
    return false;
  }

  size_t size = strlen(pattern);

  size_t written = f.write(
    (const uint8_t*)pattern,
    size
  );

  f.flush();
  f.close();

  if (written != size) {
    SD.remove(filename);
    sdVerified = false;
    sdStatus = "SD write test failed";
    return false;
  }

  f = SD.open(filename, FILE_READ);

  if (!f) {
    SD.remove(filename);
    sdVerified = false;
    sdStatus = "SD read test failed";
    return false;
  }

  char readback[32] = {0};

  size_t received = f.readBytes(
    readback,
    size
  );

  f.close();
  SD.remove(filename);

  sdVerified =
    received == size &&
    memcmp(readback, pattern, size) == 0 &&
    sdFreeBytes() > 1048576ULL;

  sdStatus = sdVerified
    ? "SD ready - read/write passed"
    : "SD verification or free-space check failed";

  if (sdVerified) sdFault = false;

  return sdVerified;
}

// ================================================
// BINARY FLIGHT LOGGING — RKT4
// ================================================

bool flushRecords() {
  if (!recording || !flightFile) {
    return false;
  }

  size_t offset = 0;

  while (offset < recordBufferUsed) {
    size_t n = flightFile.write(
      recordBuffer + offset,
      recordBufferUsed - offset
    );

    if (n == 0) {
      sdFault = true;
      sdErrors++;

      lastError = "SD write failure";
      finishReason = "SD write failure";

      finishedDurationMs =
        millis() - flightStartMs;

      recording = false;
      armed = false;

      recordBufferUsed = 0;
      flightFile.close();

      return false;
    }

    offset += n;
  }

  recordBufferUsed = 0;

  flightFile.flush();

  lastFlushMs = millis();

  return true;
}

void addRecord(
  uint32_t timestamp,
  float pressure,
  float temperature,
  bool valid
) {
  if (!recording) return;

  if (recordBufferUsed + 16 >
      RECORD_BUFFER_SIZE) {
    if (!flushRecords()) return;
  }

  uint8_t* p =
    recordBuffer + recordBufferUsed;

  putU32(p, timestamp);
  putF32(p + 4, pressure);
  putF32(p + 8, temperature);
  putU16(p + 12, batteryMv);
  putU16(p + 14, valid ? 1 : 0);

  recordBufferUsed += 16;

  recordedSamples++;

  if (!valid) invalidSamples++;
}

void stopRecording(String reason) {
  if (!recording) return;

  finishedDurationMs =
    millis() - flightStartMs;

  bool saved = flushRecords();

  if (flightFile) {
    flightFile.close();
  }

  recording = false;
  armed = false;
  landingCandidate = false;

  finishReason =
    saved && !sdFault
      ? reason
      : "SD write failure";

  lastMessage =
    saved && !sdFault
      ? "Flight saved: " + currentFilename
      : "Flight saving failed";
}

bool beginRecording() {
  if (!armed ||
      !sensorOK ||
      !sensorReadingOK ||
      !calibrated ||
      !sdOK ||
      !sdVerified ||
      sdFault ||
      simulation ||
      batteryCritical()) {

    lastError =
      "Preflight failed - check sensor, SD, calibration and battery";

    return false;
  }

  if (sdFreeBytes() < 1048576ULL) {
    lastError = "Not enough SD space";
    return false;
  }

  bool found = false;

  for (int i = 1; i <= 999; i++) {
    char filename[24];

    snprintf(
      filename,
      sizeof(filename),
      "/FLIGHT%03d.BIN",
      i
    );

    if (!SD.exists(filename)) {
      currentFilename = filename;
      found = true;
      break;
    }
  }

  if (!found) {
    lastError = "No free flight filename";
    return false;
  }

  flightFile = SD.open(
    currentFilename,
    FILE_WRITE
  );

  if (!flightFile) {
    lastError = "Cannot create flight file";
    return false;
  }

  uint8_t header[32] = {0};

  memcpy(header, "RKT4", 4);

  putU16(header + 4, 1);
  putU16(header + 6, 32);
  putU16(header + 8, 16);
  putU16(header + 10, 25);

  if (flightFile.write(header, 32) != 32) {
    flightFile.close();
    SD.remove(currentFilename);
    lastError = "Header write failed";
    return false;
  }

  flightFile.flush();

  recordBufferUsed = 0;

  recordedSamples = 0;
  invalidSamples = 0;
  missedSamples = 0;
  sdErrors = 0;

  peakAltitudeM = 0;

  launched = false;
  landingCandidate = false;

  flightStartMs = millis();
  finishedDurationMs = 0;

  lastFlushMs = millis();
  lastSampleUs = micros();

  recording = true;
  armed = false;

  ledManual = false;

  finishReason = "";
  lastError = "";
  lastMessage = "Recording started";

  return true;
}

// ================================================
// FLIGHT DETECTION
// ================================================

void checkFlight() {
  if (!recording) return;

  uint32_t elapsed =
    millis() - flightStartMs;

  if (elapsed >= MAX_RECORDING_MS) {
    stopRecording("10-minute timeout");
    return;
  }

  if (!sensorReadingOK ||
      !isfinite(altitudeM)) {
    landingCandidate = false;
    return;
  }

  if (!launched) {
    if (altitudeM >= LAUNCH_ALTITUDE_M) {
      launched = true;
      lastMessage = "Launch detected";
    }

    return;
  }

  bool nearGround =
    fabsf(altitudeM) <= 8.0f;

  bool descended =
    peakAltitudeM >= MIN_PEAK_ALTITUDE_M &&
    peakAltitudeM - altitudeM >= 12.0f;

  bool eligible =
    elapsed > 20000 &&
    nearGround &&
    descended;

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

  if (fabsf(altitudeM - landingReferenceM) > 2.5f) {
    landingReferenceM = altitudeM;
    landingStartMs = millis();
  }

  if (millis() - landingStartMs >=
      LANDING_WAIT_MS) {
    stopRecording("Landing detected");
  }
}

// ================================================
// LIVE SENSOR PROCESSING
// ================================================

void sampleSensor() {
  uint32_t timestamp = micros();

  float p = NAN;
  float t = NAN;

  bool valid = readBMP(p, t);

  sensorReadingOK = valid;

  if (!valid) {
    failedReads++;

    addRecord(timestamp, NAN, NAN, false);
    return;
  }

  successfulReads++;

  lastReadingMs = millis();

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

      float instant =
        (altitudeM - previousAltitudeM) / dt;

      verticalSpeedMps =
        0.8f * verticalSpeedMps +
        0.2f * instant;
    }

    previousAltitudeM = altitudeM;
    lastAltitudeMs = now;
  }

  addRecord(timestamp, p, t, true);

  checkFlight();
}

// ================================================
// SIMULATION
// ================================================

void updateSimulation() {
  if (!simulation) return;

  float seconds =
    (millis() - simulationStartMs) / 1000.0f;

  if (seconds < 10) {
    simulationAltitudeM = 8 * seconds;
    simulationSpeedMps = 8;
  }
  else if (seconds < 20) {
    simulationAltitudeM =
      80 - 8 * (seconds - 10);

    simulationSpeedMps = -8;
  }
  else {
    simulation = false;

    simulationAltitudeM = 0;
    simulationSpeedMps = 0;

    lastMessage = "Simulation completed";
  }
}

// ================================================
// LED EFFECTS
// ================================================

uint32_t rainbowColor(byte value) {
  value = 255 - value;

  if (value < 85) {
    return rgb.Color(
      255 - value * 3,
      0,
      value * 3
    );
  }

  if (value < 170) {
    value -= 85;

    return rgb.Color(
      0,
      value * 3,
      255 - value * 3
    );
  }

  value -= 170;

  return rgb.Color(
    value * 3,
    255 - value * 3,
    0
  );
}

void updateLED() {
  static uint32_t lastUpdate = 0;

  uint32_t now = millis();

  if (now - lastUpdate < 40) return;

  lastUpdate = now;

  bool slowBlink = (now / 450) % 2 == 0;
  bool fastBlink = (now / 150) % 2 == 0;

  uint32_t color = 0;
  int brightness = 45;

  if (!sensorOK || sdFault) {
    color = fastBlink
      ? rgb.Color(255, 0, 0)
      : 0;
  }
  else if (batteryCritical()) {
    color = fastBlink
      ? rgb.Color(255, 0, 0)
      : rgb.Color(255, 120, 0);
  }
  else if (recording) {
    color = slowBlink
      ? rgb.Color(0, 255, 0)
      : 0;
  }
  else if (armed) {
    color = rgb.Color(255, 190, 0);
  }
  else if (ledManual) {
    brightness = ledBrightness;

    switch (ledEffect) {
      case 0:
        color = rgb.Color(ledR, ledG, ledB);
        break;

      case 1:
        color = slowBlink
          ? rgb.Color(ledR, ledG, ledB)
          : 0;
        break;

      case 2: {
        float factor =
          (sinf(now * 0.004f) + 1) / 2;

        color = rgb.Color(
          ledR * factor,
          ledG * factor,
          ledB * factor
        );
        break;
      }

      case 3:
        color = rainbowColor(
          (byte)(now / 12)
        );
        break;

      default:
        color = 0;
        break;
    }
  }
  else if (simulation) {
    color = slowBlink
      ? rgb.Color(170, 0, 255)
      : 0;
  }
  else if (calibrated) {
    color = rgb.Color(0, 180, 45);
  }
  else {
    color = slowBlink
      ? rgb.Color(0, 100, 255)
      : 0;
  }

  rgb.setBrightness(brightness);
  rgb.setPixelColor(0, color);
  rgb.show();

  bool error =
    !sensorOK ||
    sdFault ||
    batteryCritical();

  digitalWrite(
    RED_LED_PIN,
    error ? fastBlink :
    recording ? slowBlink : LOW
  );
}

// ================================================
// STATUS API
// ================================================

void sendStatus() {
  String s;
  s.reserve(2400);

  s = "{";

  s += "\"sensor\":" + jsonBool(sensorOK);
  s += ",\"reading\":" + jsonBool(sensorReadingOK);
  s += ",\"calibrated\":" + jsonBool(calibrated);

  s += ",\"sd\":" + jsonBool(sdOK);
  s += ",\"sdVerified\":" + jsonBool(sdVerified);
  s += ",\"sdFault\":" + jsonBool(sdFault);

  s += ",\"armed\":" + jsonBool(armed);
  s += ",\"recording\":" + jsonBool(recording);
  s += ",\"launched\":" + jsonBool(launched);
  s += ",\"simulation\":" + jsonBool(simulation);

  s += ",\"altitude\":" + jsonFloat(altitudeM);
  s += ",\"speed\":" + jsonFloat(verticalSpeedMps);
  s += ",\"peak\":" + jsonFloat(peakAltitudeM);

  s += ",\"pressure\":" + jsonFloat(pressurePa);
  s += ",\"temperature\":" + jsonFloat(temperatureC);
  s += ",\"baseline\":" + jsonFloat(baselinePa);

  s += ",\"simAltitude\":" +
    jsonFloat(simulationAltitudeM);

  s += ",\"simSpeed\":" +
    jsonFloat(simulationSpeedMps);

  s += ",\"battery\":" +
    jsonFloat(batteryMv / 1000.0f, 3);

  s += ",\"batteryPercent\":" +
    String(batteryPercent());

  s += ",\"batteryPresent\":" +
    jsonBool(batteryLikelyPresent);

  s += ",\"batteryLow\":" +
    jsonBool(batteryLow());

  s += ",\"batteryCritical\":" +
    jsonBool(batteryCritical());

  s += ",\"usbSensor\":" +
    jsonBool(USB_SENSE_PIN >= 0);

  s += ",\"usbPresent\":" +
    jsonBool(usbPresent);

  s += ",\"goodReads\":" +
    String(successfulReads);

  s += ",\"failedReads\":" +
    String(failedReads);

  s += ",\"freshness\":";

  s += lastReadingMs
    ? String(millis() - lastReadingMs)
    : "null";

  s += ",\"calibrationCount\":" +
    String(calibrationCount);

  s += ",\"calibrationAge\":";

  s += lastCalibrationMs
    ? String((millis() - lastCalibrationMs) / 1000)
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
      ? millis() - flightStartMs
      : finishedDurationMs) / 1000.0f;

  s += ",\"duration\":" +
    jsonFloat(duration, 1);

  s += ",\"file\":\"" +
    jsonEscape(currentFilename) + "\"";

  s += ",\"message\":\"" +
    jsonEscape(lastMessage) + "\"";

  s += ",\"error\":\"" +
    jsonEscape(lastError) + "\"";

  s += ",\"calibrationStatus\":\"" +
    jsonEscape(calibrationStatus) + "\"";

  s += ",\"sdStatus\":\"" +
    jsonEscape(sdStatus) + "\"";

  s += ",\"finish\":\"" +
    jsonEscape(finishReason) + "\"";

  s += ",\"ledManual\":" +
    jsonBool(ledManual);

  s += ",\"ledEffect\":" +
    String(ledEffect);

  s += ",\"uptime\":" +
    String(millis() / 1000);

  s += "}";

  server.sendHeader(
    "Cache-Control",
    "no-store"
  );

  server.send(
    200,
    "application/json",
    s
  );
}

// ================================================
// SD FILE MANAGER
// ================================================

bool validFilename(String name) {
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

void listFiles() {
  if (!sdOK || recording) {
    reply(false, "SD unavailable or busy");
    return;
  }

  File root = SD.open("/");

  if (!root) {
    reply(false, "Cannot read SD");
    return;
  }

  String result = "{\"files\":[";

  bool first = true;

  File f = root.openNextFile();

  while (f) {
    String name = f.name();

    int slash = name.lastIndexOf('/');

    if (slash >= 0) {
      name = name.substring(slash + 1);
    }

    if (!f.isDirectory() &&
        validFilename(name)) {

      if (!first) result += ",";
      first = false;

      result += "{\"name\":\"";
      result += name;
      result += "\",\"size\":";
      result += String((uint32_t)f.size());
      result += "}";
    }

    f.close();
    f = root.openNextFile();
  }

  root.close();

  result += "]}";

  server.send(
    200,
    "application/json",
    result
  );
}

void downloadFile() {
  if (!sdOK || recording) {
    server.send(
      409,
      "text/plain",
      "SD busy"
    );
    return;
  }

  String name = server.arg("name");

  if (!validFilename(name)) {
    server.send(
      400,
      "text/plain",
      "Invalid filename"
    );
    return;
  }

  File f = SD.open(
    "/" + name,
    FILE_READ
  );

  if (!f) {
    server.send(
      404,
      "text/plain",
      "File not found"
    );
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

void deleteFile() {
  if (!sdOK || recording || armed) {
    reply(false, "SD busy");
    return;
  }

  String name = server.arg("name");

  if (!validFilename(name)) {
    reply(false, "Invalid filename");
    return;
  }

  bool success = SD.remove("/" + name);

  reply(
    success,
    success ? "File deleted" : "Delete failed"
  );
}

// ================================================
// EMBEDDED RESPONSIVE DASHBOARD
// ================================================

const char WEBPAGE[] PROGMEM = R"GIWEB(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport"
 content="width=device-width, initial-scale=1">

<title>Green Ion V8</title>

<style>
:root{
 --bg:#081320;
 --card:#15283c;
 --line:#34506b;
 --text:#f0f7ff;
 --muted:#a3bbd0;
 --blue:#75d7ff;
 --green:#6ce5a6;
 --red:#ff8999;
 --yellow:#ffd67b
}

*{box-sizing:border-box}

body{
 margin:0;
 background:var(--bg);
 color:var(--text);
 font:15px/1.5 system-ui,Arial,sans-serif
}

main{
 max-width:900px;
 margin:auto;
 padding:20px 15px 60px
}

h1,h2,p{margin-top:0}
h1{font-size:27px;letter-spacing:1px;margin-bottom:3px}
h2{font-size:19px;margin-bottom:15px}

small,.muted{color:var(--muted)}
small{font-size:12px}

header{
 display:flex;
 justify-content:space-between;
 align-items:center;
 gap:20px;
 margin-bottom:21px
}

.connection{
 color:var(--green);
 border:1px solid var(--line);
 border-radius:11px;
 padding:11px 15px;
 white-space:nowrap;
 font-weight:800
}

.units{
 display:flex;
 justify-content:space-between;
 align-items:center;
 gap:15px;
 flex-wrap:wrap;
 padding:16px 18px;
 border:1px solid var(--line);
 background:var(--card);
 border-radius:14px;
 margin-bottom:20px
}

button{
 background:var(--blue);
 color:#081421;
 border:0;
 border-radius:11px;
 font-size:14px;
 font-weight:750;
 padding:13px 16px;
 min-height:46px;
 cursor:pointer
}

button:disabled{
 opacity:.45;
 cursor:not-allowed
}

button.secondary{
 background:#365775;
 color:white
}

button.warning{background:var(--yellow)}
button.danger{background:#934453;color:white}
button.green{background:var(--green)}

.unitButton{
 background:#29445f;
 color:white
}

.unitButton.active{
 background:var(--blue);
 color:#081421
}

.tabs{
 display:flex;
 gap:8px;
 overflow-x:auto;
 margin-bottom:20px
}

.tabs button{
 flex:1;
 min-width:95px;
 background:#263f59;
 color:white
}

.tabs button.active{
 background:var(--blue);
 color:#081421
}

.page{display:none}
.page.active{display:block}

.card,.metric{
 background:var(--card);
 border:1px solid var(--line);
 border-radius:16px;
 padding:20px;
 margin-bottom:17px
}

.metrics{
 display:grid;
 grid-template-columns:repeat(2,minmax(0,1fr));
 gap:14px;
 margin-bottom:17px
}

.metrics .metric{margin:0}

.metricLabel{
 font-size:12px;
 color:var(--muted)
}

.metricValue{
 font-weight:850;
 font-size:clamp(21px,5vw,34px);
 margin-top:8px;
 overflow-wrap:anywhere;
 font-variant-numeric:tabular-nums
}

.metricValue.major{
 font-size:clamp(33px,7vw,58px);
 color:var(--blue)
}

.batteryPercent{
 color:var(--green)
}

.metricNote{
 font-size:12px;
 color:var(--muted);
 margin-top:5px
}

.actions{
 display:grid;
 grid-template-columns:1fr 1fr;
 gap:12px
}

.actions button{width:100%}
.wide{grid-column:1/-1}

.row{
 display:flex;
 align-items:center;
 justify-content:space-between;
 gap:15px;
 padding:13px 0;
 border-bottom:1px solid var(--line)
}

.row:last-child{border-bottom:0}

.row b{
 text-align:right;
 overflow-wrap:anywhere
}

.notice{
 background:#28425d;
 padding:15px;
 border-radius:12px;
 min-height:50px;
 margin:16px 0;
 overflow-wrap:anywhere
}

.checks{
 display:flex;
 flex-wrap:wrap;
 gap:9px;
 margin:14px 0
}

.check{
 font-size:12px;
 padding:9px 12px;
 border-radius:9px;
 background:#263d54
}

.ok{color:var(--green)}
.bad{color:var(--red)}

canvas{
 width:100%;
 height:190px;
 display:block
}

.swatches{
 display:flex;
 gap:11px;
 flex-wrap:wrap;
 margin:15px 0
}

.swatch{
 border:2px solid white;
 width:44px;
 height:44px;
 border-radius:50%;
 padding:0
}

select,input[type=range]{
 display:block;
 width:100%;
 padding:12px;
 margin:12px 0;
 border-radius:10px;
 background:#29435c;
 border:1px solid var(--line);
 color:white
}

.guide{
 display:grid;
 grid-template-columns:18px 1fr;
 align-items:center;
 gap:15px;
 margin-top:18px
}

.ledDot{
 width:16px;
 height:16px;
 border-radius:50%
}

.file{
 padding:15px 0;
 display:flex;
 justify-content:space-between;
 align-items:center;
 gap:12px;
 flex-wrap:wrap;
 border-bottom:1px solid var(--line)
}

.file button{
 padding:10px 12px
}

@media(max-width:460px){
 main{padding:15px 12px 50px}
 .card,.metric{padding:16px}
 h1{font-size:23px}
 header{gap:12px}
}
</style>
</head>

<body>
<main>

<header>
 <div>
  <h1>GREEN ION</h1>
  <small>V8 · Flight Recorder</small>
 </div>
 <div class="connection" id="connection">
  CONNECTING
 </div>
</header>

<section class="units">
 <div>
  <strong>Measurement system</strong>
  <div class="muted">
   <small>Altitude, speed, temperature and pressure</small>
  </div>
 </div>
 <div>
  <button id="metricButton" class="unitButton active"
   onclick="setUnits(false)">Metric</button>
  <button id="imperialButton" class="unitButton"
   onclick="setUnits(true)">Imperial</button>
 </div>
</section>

<nav class="tabs">
 <button class="active" onclick="showTab('overview',this)">
  Overview
 </button>
 <button onclick="showTab('testing',this)">Testing</button>
 <button onclick="showTab('lights',this)">Lights</button>
 <button onclick="showTab('storage',this)">Storage</button>
 <button onclick="showTab('system',this)">System</button>
</nav>

<!-- OVERVIEW -->

<section class="page active" id="overview">

<div class="metrics">

 <div class="metric">
  <div class="metricLabel">LIVE ALTITUDE</div>
  <div class="metricValue major" id="altitude">--</div>
  <div class="metricNote">Relative to calibration</div>
 </div>

 <div class="metric">
  <div class="metricLabel">BATTERY ESTIMATE</div>
  <div class="metricValue major batteryPercent"
   id="batteryPercent">--</div>
  <div class="metricNote" id="batteryVoltage">--</div>
  <div class="metricNote">Voltage-based estimate</div>
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
  <div class="metricLabel">Pressure</div>
  <div class="metricValue" id="pressure">--</div>
  <div class="metricNote" id="pressurePsi">-- PSI</div>
 </div>

</div>

<div class="card">
 <h2>Flight controls</h2>

 <div class="row">
  <span>Status</span>
  <b id="flightState">CONNECTING</b>
 </div>

 <div class="checks" id="checks"></div>

 <div class="notice" id="feedback"
  role="status" aria-live="polite">
  Connecting to device...
 </div>

 <div class="actions">
  <button id="calibrateButton"
   onclick="command('calibrate')">
   1 · CALIBRATE
  </button>

  <button id="armButton" class="warning"
   onclick="command('arm')">
   2 · ARM
  </button>

  <button id="startButton" class="wide green"
   onclick="command('start')">
   3 · START RECORDING
  </button>

  <button id="stopButton" class="danger"
   onclick="confirmStop()">
   STOP & SAVE
  </button>

  <button id="disconnectButton" class="secondary"
   onclick="disconnectWifi()">
   DISCONNECT WI-FI
  </button>
 </div>

 <p class="muted" style="margin-top:17px">
  Live altitude works after calibration, even before
  arming or recording.
 </p>
</div>

<div class="card">
 <h2>Live altitude</h2>
 <canvas id="overviewGraph"></canvas>
</div>

</section>

<!-- TESTING -->

<section class="page" id="testing">

<div class="card">
 <h2>Sensor testing</h2>

 <button onclick="command('calibrate')">
  CALIBRATE ALTITUDE
 </button>

 <div class="notice" id="testingFeedback">
  Awaiting test.
 </div>

 <div class="actions">
  <button onclick="command('simulate')">
   START SIMULATION
  </button>

  <button class="secondary"
   onclick="command('simstop')">
   STOP SIMULATION
  </button>
 </div>

 <p class="muted" style="margin-top:17px">
  Simulation does not write flight data.
 </p>
</div>

<div class="card">
 <h2>Graph controls</h2>

 <div class="actions">
  <button id="pauseButton" class="secondary"
   onclick="toggleGraphPause()">
   Pause graphs
  </button>

  <button class="secondary"
   onclick="clearGraphs()">
   Clear history
  </button>
 </div>

 <select id="graphWindow" onchange="drawGraphs()">
  <option value="30">Last 30 readings</option>
  <option value="60">Last 60 readings</option>
  <option value="120" selected>
   Last 120 readings
  </option>
 </select>
</div>

<div class="card">
 <h2>Altitude graph</h2>
 <canvas id="altitudeGraph"></canvas>
</div>

<div class="card">
 <h2>Pressure graph</h2>
 <canvas id="pressureGraph"></canvas>
</div>

<div class="card">
 <h2>Temperature graph</h2>
 <canvas id="temperatureGraph"></canvas>
</div>

<div class="card">
 <h2>Sensor diagnostics</h2>

 <div class="row">
  <span>Last valid reading</span>
  <b id="freshness">--</b>
 </div>

 <div class="row">
  <span>Successful / failed readings</span>
  <b id="readings">--</b>
 </div>

 <div class="row">
  <span>Calibration count</span>
  <b id="calibrationCount">--</b>
 </div>

 <div class="row">
  <span>Calibration age</span>
  <b id="calibrationAge">--</b>
 </div>

 <div class="row">
  <span>Baseline pressure</span>
  <b id="baseline">--</b>
 </div>
</div>

</section>

<!-- LIGHTS -->

<section class="page" id="lights">

<div class="card">
 <h2>RGB LED controls</h2>

 <div class="row">
  <span>LED mode</span>
  <b id="ledMode">AUTO</b>
 </div>

 <div class="swatches" id="swatches"></div>

 <label>Effect</label>
 <select id="ledEffect" onchange="applyLED()">
  <option value="0">Solid</option>
  <option value="1">Blink</option>
  <option value="2">Breathe</option>
  <option value="3">Rainbow</option>
  <option value="4">Off</option>
 </select>

 <label>
  Brightness
  <b id="brightnessLabel">45%</b>
 </label>

 <input type="range"
  id="brightness"
  min="1"
  max="100"
  value="45"
  oninput="document.getElementById('brightnessLabel').textContent=this.value+'%'"
  onchange="applyLED()">

 <div class="actions">
  <button class="danger" onclick="turnLEDOff()">
   TURN RGB OFF
  </button>

  <button class="secondary" onclick="restoreAuto()">
   AUTO STATUS
  </button>
 </div>

 <div class="notice" id="ledFeedback">
  Automatic LED mode.
 </div>

 <p class="muted">
  Critical warnings and recording override manual
  RGB settings. Separate hardware indicators
  cannot be turned off through this control.
 </p>
</div>

<div class="card">
 <h2>Automatic LED guide</h2>

 <div class="guide">

  <div class="ledDot" style="background:#357bff"></div>
  <div>Blue blinking — awaiting calibration</div>

  <div class="ledDot" style="background:#22c55e"></div>
  <div>Green solid — calibrated</div>

  <div class="ledDot" style="background:#eab308"></div>
  <div>Yellow solid — armed</div>

  <div class="ledDot" style="background:#22c55e"></div>
  <div>Green blinking — recording</div>

  <div class="ledDot" style="background:#a855f7"></div>
  <div>Purple blinking — simulation</div>

  <div class="ledDot" style="background:#ef4444"></div>
  <div>Red blinking — sensor or SD fault</div>

  <div class="ledDot" style="background:#f59e0b"></div>
  <div>Red/orange alternating — critical battery</div>

 </div>
</div>

</section>

<!-- STORAGE -->

<section class="page" id="storage">

<div class="card">
 <h2>microSD card</h2>

 <div class="notice" id="sdStatus">
  Checking SD...
 </div>

 <button onclick="command('sdtest')">
  DETECT & TEST SD CARD
 </button>

 <p class="muted" style="margin-top:16px">
  Calibration and sensor tests work without SD.
  Flight recording requires verified storage.
 </p>
</div>

<div class="card">
 <h2>Flight files</h2>

 <button class="secondary" onclick="loadFiles()">
  REFRESH FILES
 </button>

 <div id="fileList"></div>
</div>

</section>

<!-- SYSTEM -->

<section class="page" id="system">

<div class="card">
 <h2>Power diagnostics</h2>

 <div class="row">
  <span>Estimated battery charge</span>
  <b id="systemPercent">--</b>
 </div>

 <div class="row">
  <span>Measured battery voltage</span>
  <b id="systemBattery">--</b>
 </div>

 <div class="row">
  <span>Battery presence</span>
  <b id="batteryPresent">--</b>
 </div>

 <div class="row">
  <span>Battery warning</span>
  <b id="batteryWarning">--</b>
 </div>

 <div class="row">
  <span>USB power</span>
  <b id="usbStatus">Unknown</b>
 </div>

 <div class="row">
  <span>Charging</span>
  <b>Unknown</b>
 </div>

 <p class="muted" style="margin-top:16px">
  Percentage is estimated from voltage, not measured
  by a fuel gauge. USB and charging state cannot be
  reliably determined without extra hardware.
 </p>
</div>

<div class="card">
 <h2>Recorder diagnostics</h2>

 <div class="row">
  <span>Samples</span>
  <b id="samples">--</b>
 </div>

 <div class="row">
  <span>Invalid / missed samples</span>
  <b id="sampleErrors">--</b>
 </div>

 <div class="row">
  <span>SD errors</span>
  <b id="sdErrors">--</b>
 </div>

 <div class="row">
  <span>Flight duration</span>
  <b id="duration">--</b>
 </div>

 <div class="row">
  <span>Current file</span>
  <b id="currentFile">--</b>
 </div>

 <div class="row">
  <span>Launch detected</span>
  <b id="launchStatus">--</b>
 </div>

 <div class="row">
  <span>Finish reason</span>
  <b id="finishReason">--</b>
 </div>

 <div class="row">
  <span>Uptime</span>
  <b id="uptime">--</b>
 </div>

 <div class="row">
  <span>Last error</span>
  <b id="lastError">--</b>
 </div>

</div>

</section>

</main>

<script>
'use strict';

const $ = id => document.getElementById(id);

let latest = null;
let imperial = false;
let busy = false;
let graphsPaused = false;

let ledColor = [0,180,255];

const history = {
 altitude: [],
 pressure: [],
 temperature: []
};

function fmt(v,places=2){
 if(v===null ||
    v===undefined ||
    !Number.isFinite(Number(v))){
  return '--';
 }
 return Number(v).toFixed(places);
}

function convertAltitude(v){
 return v==null ? null :
  imperial ? v*3.280839895 : v;
}

function convertTemperature(v){
 return v==null ? null :
  imperial ? v*9/5+32 : v;
}

function convertPressure(v){
 return v==null ? null :
  imperial ? v/3386.389 : v/100;
}

function unit(){
 return imperial?'ft':'m';
}

function setUnits(value){
 imperial=value;

 $('metricButton').classList.toggle(
  'active',!value
 );

 $('imperialButton').classList.toggle(
  'active',value
 );

 if(latest)render(latest,false);
 drawGraphs();
}

function showTab(id,button){
 document.querySelectorAll('.page').forEach(
  p=>p.classList.remove('active')
 );

 document.querySelectorAll('.tabs button').forEach(
  b=>b.classList.remove('active')
 );

 $(id).classList.add('active');
 button.classList.add('active');

 if(id==='storage')loadFiles();

 drawGraphs();
}

function feedback(message,success=null){
 const color=
  success===true?'#6ce5a6':
  success===false?'#ff8999':
  '#ffd67b';

 for(const id of ['feedback','testingFeedback']){
  $(id).textContent=message;
  $(id).style.color=color;
 }
}

function checklist(text,good){
 return '<span class="check '+
  (good?'ok':'bad')+'">'+
  (good?'✓ ':'× ')+text+'</span>';
}

function addHistory(name,value){
 if(value==null || !Number.isFinite(value)){
  return;
 }

 history[name].push(value);

 if(history[name].length>120){
  history[name].shift();
 }
}

function render(d,collect=true){
 latest=d;

 $('connection').textContent='● LIVE';
 $('connection').style.color='#6ce5a6';

 const shownAltitude=d.simulation
  ? d.simAltitude
  : d.altitude;

 const shownSpeed=d.simulation
  ? d.simSpeed
  : d.speed;

 $('altitude').textContent=
  (d.calibrated||d.simulation)
   ? fmt(convertAltitude(shownAltitude))+' '+unit()
   : 'CALIBRATE';

 $('batteryPercent').textContent=
  d.batteryPercent<0
   ? 'UNKNOWN'
   : d.batteryPercent+'%';

 $('batteryVoltage').textContent=
  fmt(d.battery,3)+' V'+
  (d.batteryCritical?' · CRITICAL':
   d.batteryLow?' · LOW':'');

 $('batteryPercent').style.color=
  d.batteryCritical?'#ff8999':
  d.batteryLow?'#ffd67b':
  '#6ce5a6';

 $('speed').textContent=
  fmt(convertAltitude(shownSpeed))+
  (imperial?' ft/s':' m/s');

 $('peak').textContent=
  fmt(convertAltitude(d.peak))+' '+unit();

 $('temperature').textContent=
  fmt(convertTemperature(d.temperature),1)+
  (imperial?' °F':' °C');

 $('pressure').textContent=
  fmt(convertPressure(d.pressure),
   imperial?3:1)+
  (imperial?' inHg':' hPa');

 $('pressurePsi').textContent=
  fmt(
   d.pressure==null
    ? null
    : d.pressure/6894.757293,
   3
  )+' PSI absolute';

 $('flightState').textContent=
  d.recording
   ? (d.launched?'IN FLIGHT':'RECORDING')
   : d.armed?'ARMED'
   : d.simulation?'SIMULATION'
   : d.calibrated?'PREFLIGHT LIVE'
   : 'CALIBRATION REQUIRED';

 $('checks').innerHTML=
  checklist('BMP388',d.sensor&&d.reading)+
  checklist('Calibrated',d.calibrated)+
  checklist('SD ready',d.sdVerified)+
  checklist('Battery',!d.batteryCritical);

 $('calibrateButton').disabled=
  busy||d.recording||d.armed||d.simulation;

 $('armButton').disabled=
  busy||d.recording||d.armed||
  d.simulation||!d.calibrated||
  !d.sdVerified||d.sdFault;

 $('startButton').disabled=
  busy||!d.armed||d.recording;

 $('stopButton').disabled=
  busy||!d.recording;

 $('disconnectButton').disabled=
  busy||!d.recording;

 $('freshness').textContent=
  d.freshness==null
   ? 'No readings'
   : d.freshness+' ms ago';

 $('readings').textContent=
  d.goodReads+' / '+d.failedReads;

 $('calibrationCount').textContent=
  d.calibrationCount;

 $('calibrationAge').textContent=
  d.calibrationAge==null
   ? 'Never'
   : d.calibrationAge+' s ago';

 $('baseline').textContent=
  fmt(d.baseline,2)+' Pa';

 $('sdStatus').textContent=d.sdStatus;

 $('ledMode').textContent=
  d.ledManual?'MANUAL':'AUTO';

 $('systemPercent').textContent=
  d.batteryPercent<0
   ? 'Unknown'
   : d.batteryPercent+'% estimated';

 $('systemBattery').textContent=
  fmt(d.battery,3)+' V';

 $('batteryPresent').textContent=
  d.batteryPresent
   ? 'Likely connected'
   : 'Unknown / not detected';

 $('batteryWarning').textContent=
  d.batteryCritical?'CRITICAL':
  d.batteryLow?'LOW':'No warning';

 $('usbStatus').textContent=
  !d.usbSensor
   ? 'Unknown - sensor not installed'
   : d.usbPresent?'Present':'Not present';

 $('samples').textContent=d.samples;

 $('sampleErrors').textContent=
  d.invalidSamples+' / '+d.missedSamples;

 $('sdErrors').textContent=d.sdErrors;

 $('duration').textContent=
  fmt(d.duration,1)+' s';

 $('currentFile').textContent=d.file;

 $('launchStatus').textContent=
  d.launched?'YES':'NO';

 $('finishReason').textContent=
  d.finish||'--';

 $('uptime').textContent=d.uptime+' s';

 $('lastError').textContent=
  d.error||'None';

 if(collect&&!graphsPaused){
  addHistory('altitude',shownAltitude);
  addHistory('pressure',d.pressure);
  addHistory('temperature',d.temperature);

  drawGraphs();
 }
}

async function poll(){
 if(busy)return;

 try{
  const response=await fetch(
   '/api/status',
   {cache:'no-store'}
  );

  if(!response.ok){
   throw Error('Device unavailable');
  }

  render(await response.json());
 }
 catch(error){
  $('connection').textContent='● OFFLINE';
  $('connection').style.color='#ff8999';
 }
}

// All command buttons use POST and JSON responses.

async function command(name){
 if(busy)return;

 busy=true;

 const messages={
  calibrate:
   'Calibration requested - averaging BMP388 readings...',
  arm:
   'Checking preflight and arming...',
  start:
   'Opening flight log...',
  stop:
   'Saving flight data...',
  sdtest:
   'Testing SD read/write...',
  simulate:
   'Starting simulation...',
  simstop:
   'Stopping simulation...'
 };

 feedback(
  messages[name]||'Sending command...',
  null
 );

 if(name==='calibrate'){
  $('calibrateButton').textContent='CALIBRATING...';
 }

 try{
  const response=await fetch(
   '/api/'+name,
   {method:'POST'}
  );

  const result=await response.json();

  feedback(
   result.message||'No message',
   response.ok&&result.success
  );
 }
 catch(error){
  feedback(
   'Request failed: '+error.message,
   false
  );
 }
 finally{
  busy=false;

  $('calibrateButton').textContent=
   '1 · CALIBRATE';

  await poll();
 }
}

function confirmStop(){
 if(confirm('Stop recording and save the flight?')){
  command('stop');
 }
}

async function disconnectWifi(){
 if(!confirm(
  'Disconnect Wi-Fi while recording?'
 ))return;

 feedback('Disconnecting Wi-Fi...',null);

 try{
  await fetch(
   '/api/disconnect',
   {method:'POST'}
  );
 }
 catch(error){}

 $('connection').textContent='● OFFLINE';
}

// ==============================================
// GRAPHS
// ==============================================

function drawGraph(id,key,convert){
 const canvas=$(id);

 const width=Math.max(
  canvas.clientWidth,
  100
 );

 const height=190;
 const ratio=window.devicePixelRatio||1;

 canvas.width=width*ratio;
 canvas.height=height*ratio;

 const ctx=canvas.getContext('2d');
 ctx.scale(ratio,ratio);

 const values=history[key]
  .slice(-Number($('graphWindow').value))
  .map(convert)
  .filter(Number.isFinite);

 let min=values.length
  ?Math.min(...values):0;

 let max=values.length
  ?Math.max(...values):1;

 if(max-min<.01){
  min-=.5;
  max+=.5;
 }

 ctx.strokeStyle='#395672';
 ctx.lineWidth=1;

 for(let i=0;i<4;i++){
  let y=24+i*45;

  ctx.beginPath();
  ctx.moveTo(24,y);
  ctx.lineTo(width-10,y);
  ctx.stroke();
 }

 ctx.fillStyle='#adc3d8';
 ctx.font='11px system-ui';

 ctx.fillText(fmt(max),24,13);
 ctx.fillText(fmt(min),24,185);

 if(values.length<2)return;

 ctx.strokeStyle='#75d7ff';
 ctx.lineWidth=2.5;
 ctx.beginPath();

 values.forEach((v,i)=>{
  const x=24+
   i*(width-36)/(values.length-1);

  const y=165-
   (v-min)/(max-min)*135;

  if(i===0)ctx.moveTo(x,y);
  else ctx.lineTo(x,y);
 });

 ctx.stroke();
}

function drawGraphs(){
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

function toggleGraphPause(){
 graphsPaused=!graphsPaused;

 $('pauseButton').textContent=
  graphsPaused
   ?'Resume graphs'
   :'Pause graphs';
}

function clearGraphs(){
 history.altitude.length=0;
 history.pressure.length=0;
 history.temperature.length=0;

 drawGraphs();
}

// ==============================================
// LED CONTROLS
// ==============================================

async function ledRequest(settings){
 const response=await fetch('/api/led',{
  method:'POST',
  headers:{
   'Content-Type':
    'application/x-www-form-urlencoded'
  },
  body:new URLSearchParams(settings)
 });

 const result=await response.json();

 if(!response.ok||!result.success){
  throw Error(
   result.message||'LED command failed'
  );
 }

 return result;
}

async function applyLED(){
 try{
  await ledRequest({
   manual:'1',
   r:ledColor[0],
   g:ledColor[1],
   b:ledColor[2],
   effect:$('ledEffect').value,
   brightness:$('brightness').value
  });

  $('ledFeedback').textContent=
   'LED settings applied';

  await poll();
 }
 catch(error){
  $('ledFeedback').textContent=error.message;
 }
}

async function turnLEDOff(){
 try{
  await ledRequest({
   manual:'1',
   effect:'4',
   brightness:'1'
  });

  $('ledEffect').value='4';

  $('ledFeedback').textContent=
   'RGB LED off - safety overrides remain active';

  await poll();
 }
 catch(error){
  $('ledFeedback').textContent=error.message;
 }
}

async function restoreAuto(){
 try{
  await ledRequest({
   manual:'0'
  });

  $('ledFeedback').textContent=
   'Automatic LED status restored';

  await poll();
 }
 catch(error){
  $('ledFeedback').textContent=error.message;
 }
}

for(const hex of [
 '#ff4444',
 '#ff9933',
 '#ffe750',
 '#52e798',
 '#32d9ff',
 '#3d80ff',
 '#b768ff',
 '#ffffff'
]){
 const button=document.createElement('button');

 button.className='swatch';
 button.style.background=hex;
 button.title=hex;

 button.onclick=()=>{
  ledColor=[1,3,5].map(
   i=>parseInt(hex.slice(i,i+2),16)
  );

  applyLED();
 };

 $('swatches').appendChild(button);
}

// ==============================================
// FILES
// ==============================================

async function loadFiles(){
 try{
  const response=await fetch('/api/files',{
   cache:'no-store'
  });

  if(!response.ok){
   throw Error('SD unavailable');
  }

  const data=await response.json();
  const container=$('fileList');

  container.replaceChildren();

  if(!data.files.length){
   container.textContent='No flights found';
   return;
  }

  for(const file of data.files){
   const row=document.createElement('div');
   row.className='file';

   const label=document.createElement('span');
   label.textContent=
    file.name+' ('+file.size+' bytes)';

   const controls=document.createElement('div');

   const download=document.createElement('button');
   download.textContent='Download';

   download.onclick=()=>{
    location.href='/api/download?name='+
     encodeURIComponent(file.name);
   };

   const remove=document.createElement('button');
   remove.textContent='Delete';
   remove.className='danger';

   remove.onclick=async()=>{
    if(!confirm('Delete '+file.name+'?'))return;

    await fetch(
     '/api/delete?name='+
      encodeURIComponent(file.name),
     {method:'POST'}
    );

    loadFiles();
   };

   controls.append(download,remove);
   row.append(label,controls);
   container.appendChild(row);
  }
 }
 catch(error){
  $('fileList').textContent=error.message;
 }
}

// ==============================================
// STARTUP
// ==============================================

window.addEventListener('resize',drawGraphs);

setInterval(poll,500);

poll();

</script>
</body>
</html>
)GIWEB";

// ================================================
// WIFI
// ================================================

void startWiFi() {
  if (wifiActive) return;

  WiFi.mode(WIFI_AP);

  wifiActive=WiFi.softAP(
    WIFI_NAME,
    WIFI_PASSWORD
  );

  if (wifiActive) {
    server.begin();

    Serial.println(
      "Dashboard: http://192.168.4.1"
    );
  }
}

void stopWiFi() {
  if (!wifiActive) return;

  server.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);

  wifiActive = false;
}

// ================================================
// API ROUTES
// ================================================

void setupRoutes() {

  server.on("/",HTTP_GET,[](){
    server.send_P(
      200,
      "text/html",
      WEBPAGE
    );
  });

  server.on("/api/status",HTTP_GET,[](){
    sendStatus();
  });

  server.on("/api/calibrate",HTTP_POST,[](){
    bool ok=calibrateAltitude();

    reply(ok,calibrationStatus);
  });

  server.on("/api/arm",HTTP_POST,[](){

    bool ok=
      !armed &&
      !recording &&
      !simulation &&
      sensorOK &&
      sensorReadingOK &&
      calibrated &&
      sdOK &&
      sdVerified &&
      !sdFault &&
      !batteryCritical() &&
      sdFreeBytes() >= 1048576ULL;

    if (ok) {
      armed=true;
      ledManual=false;

      lastMessage="Recorder armed";
      lastError="";
    }
    else {
      lastError=
        "Arm blocked - check preflight requirements";
    }

    reply(
      ok,
      ok ? lastMessage : lastError
    );
  });

  server.on("/api/start",HTTP_POST,[](){
    bool ok=beginRecording();

    reply(
      ok,
      ok ? lastMessage : lastError
    );
  });

  server.on("/api/stop",HTTP_POST,[](){
    if (!recording) {
      reply(false,"Not recording");
      return;
    }

    stopRecording("Manual stop");

    reply(
      !sdFault,
      sdFault
        ? lastError
        : "Flight stopped and saved"
    );
  });

  server.on("/api/disconnect",HTTP_POST,[](){
    if (!recording) {
      reply(false,"Not recording");
      return;
    }

    reply(true,"Wi-Fi disconnecting");

    delay(300);
    stopWiFi();
  });

  server.on("/api/sdtest",HTTP_POST,[](){
    bool ok=testSD();

    reply(ok,sdStatus);
  });

  server.on("/api/simulate",HTTP_POST,[](){
    if (recording||armed) {
      reply(
        false,
        "Cannot simulate while armed or recording"
      );
      return;
    }

    simulation=true;
    simulationStartMs=millis();

    simulationAltitudeM=0;
    simulationSpeedMps=0;

    reply(true,"Simulation started");
  });

  server.on("/api/simstop",HTTP_POST,[](){
    simulation=false;
    simulationAltitudeM=0;
    simulationSpeedMps=0;

    reply(true,"Simulation stopped");
  });

  server.on("/api/led",HTTP_POST,[](){

    if (recording ||
        armed ||
        sdFault ||
        !sensorOK ||
        batteryCritical()) {

      reply(
        false,
        "LED safety override active"
      );
      return;
    }

    ledManual=
      server.arg("manual")=="1";

    if (server.hasArg("r")) {
      ledR=constrain(
        server.arg("r").toInt(),0,255
      );
    }

    if (server.hasArg("g")) {
      ledG=constrain(
        server.arg("g").toInt(),0,255
      );
    }

    if (server.hasArg("b")) {
      ledB=constrain(
        server.arg("b").toInt(),0,255
      );
    }

    if (server.hasArg("effect")) {
      ledEffect=constrain(
        server.arg("effect").toInt(),0,4
      );
    }

    if (server.hasArg("brightness")) {
      ledBrightness=constrain(
        server.arg("brightness").toInt(),1,100
      );
    }

    reply(true,"LED settings applied");
  });

  server.on("/api/files",HTTP_GET,[](){
    listFiles();
  });

  server.on("/api/download",HTTP_GET,[](){
    downloadFile();
  });

  server.on("/api/delete",HTTP_POST,[](){
    deleteFile();
  });

  server.onNotFound([](){
    server.send(
      404,
      "application/json",
      "{\"success\":false,\"message\":\"Unknown route\"}"
    );
  });
}

// ================================================
// SETUP
// ================================================

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

  rgb.begin();
  rgb.clear();
  rgb.show();

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

  initializeSensor();

  SPI.begin();

  if (initializeSD()) {
    testSD();
  }

  setupRoutes();

  startWiFi();

  lastSampleUs=micros();
  lastBatteryMs=millis();
  lastFlushMs=millis();

  Serial.println();
  Serial.println("====================");
  Serial.println("GREEN ION V8 READY");
  Serial.println("====================");
  Serial.println("Wi-Fi: GREEN-ION");
  Serial.println("Password: GreenIon2026");
  Serial.println("URL: http://192.168.4.1");
  Serial.println();
}

// ================================================
// MAIN LOOP
// ================================================

void loop() {
  updateLED();
  updateSimulation();

  uint32_t nowUs=micros();

  if ((uint32_t)(nowUs-lastSampleUs) >=
      SAMPLE_INTERVAL_US) {

    uint32_t slots=
      (uint32_t)(nowUs-lastSampleUs) /
      SAMPLE_INTERVAL_US;

    if (recording && slots>1) {
      missedSamples+=slots-1;
    }

    lastSampleUs+=slots*SAMPLE_INTERVAL_US;

    sampleSensor();
  }

  uint32_t nowMs=millis();

  if (nowMs-lastBatteryMs>=1000) {
    lastBatteryMs=nowMs;
    updateBattery();
  }

  if (recording &&
      nowMs-lastFlushMs>=FLUSH_INTERVAL_MS) {

    flushRecords();
  }

  if (recording &&
      nowMs-flightStartMs>=MAX_RECORDING_MS) {

    stopRecording("Backup timeout");
  }

  if (!recording&&!wifiActive) {
    startWiFi();
  }

  if (wifiActive) {
    server.handleClient();
  }

  yield();
}
