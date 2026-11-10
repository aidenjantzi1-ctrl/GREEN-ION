#pragma once
// Guided home bench test. Never touches V12D### flight recordings.
static const char HOME_TEST_PATH[] = "/GIHOME.BIN";
static const uint32_t HOME_TEST_DURATION_MS = 30000UL;

static void homeReply(int code, bool ok, const char *message) {
  dashboardServer.send(code, "application/json",
    String("{\"success\":") + (ok ? "true" : "false") +
    ",\"message\":\"" + message + "\"}");
}

static bool homeIdle() {
  return recorderMode == RECORDER_HOME_TEST &&
    !state.recording && !recorderCore.recording() &&
    !logFile && !sensorLogFile && !otaUploadInProgress;
}

static void homeCloseFile() {
  if (homeFile) { homeFile.flush(); homeFile.close(); }
}

static void homeVerifyFile() {
  if (!homeLogActive) return;
  homeLogActive = false;
  homeCloseFile();
  homeMissed = sampleClock.missedPeriods() - homeMissedStart;
  bool good = false;
  digitalWrite(config::PIN_LORA_CS, HIGH);
  File f = SD.open(HOME_TEST_PATH, FILE_READ);
  if (f) {
    uint8_t raw[green_ion::rkt4::kHeaderBytes] = {};
    green_ion::rkt4::Header decoded = {};
    const size_t expected = green_ion::rkt4::kHeaderBytes +
      static_cast<size_t>(homeSamples) * green_ion::rkt4::kRecordBytes;
    good = f.size() == expected &&
      f.read(raw, sizeof(raw)) == sizeof(raw) &&
      green_ion::rkt4::decodeHeader(raw, sizeof(raw), decoded) &&
      decoded.sampleRateHz == config::SENSOR_RATE_HZ;
    uint32_t previous = 0;
    for (uint16_t i = 0; good && i < homeSamples; ++i) {
      uint8_t recordBytes[green_ion::rkt4::kRecordBytes] = {};
      green_ion::rkt4::Record record = {};
      if (f.read(recordBytes, sizeof(recordBytes)) != sizeof(recordBytes)) {
        good = false; break;
      }
      green_ion::rkt4::decodeRecord(recordBytes, record);
      if (i) {
        const uint32_t delta = green_ion::rkt4::timestampDeltaUs(record.timestampUs, previous);
        if (delta == 0 || delta > 100000UL) good = false;
      }
      previous = record.timestampUs;
    }
    f.close();
  }
  homeCleanupOk = SD.remove(HOME_TEST_PATH);
  good = good && homeCleanupOk && homeSamples >= 1400 &&
      homeValidSamples >= (static_cast<uint32_t>(homeSamples) * 9UL) / 10UL &&
      homeMissed <= 30;
  homeResult = good ? 2 : 3;
  Serial.printf("[HOME] Temporary 50 Hz RKT4 %s | samples=%u valid=%u missed=%lu | Wi-Fi disconnected=%s | temp deleted=%s\n",
    good ? "PASS" : "FAIL", homeSamples, homeValidSamples,
    static_cast<unsigned long>(homeMissed),
    homeWifiAbsent ? "YES" : "NO", homeCleanupOk ? "YES" : "NO");
}

static void homeCapture(uint32_t timestampUs, bool valid) {
  if (!homeLogActive) return;
  if (WiFi.softAPgetStationNum()) homeWifiSeen = true;
  else if (homeWifiSeen) homeWifiAbsent = true;
  uint8_t bytes[green_ion::rkt4::kRecordBytes] = {};
  const green_ion::rkt4::Record record = {
    timestampUs, state.pressurePa, state.temperatureC, state.batteryMv, valid};
  green_ion::rkt4::encodeRecord(bytes, record);
  digitalWrite(config::PIN_LORA_CS, HIGH);
  if (!homeFile || homeFile.write(bytes, sizeof(bytes)) != sizeof(bytes)) {
    homeLogActive = false;
    homeCloseFile();
    homeCleanupOk = SD.remove(HOME_TEST_PATH);
    homeResult = 3;
    Serial.println("[HOME] Temporary recording failed; normal flight files were not touched.");
    return;
  }
  ++homeSamples;
  if (valid) ++homeValidSamples;
  if (static_cast<uint32_t>(millis() - homeStartedMs) >= HOME_TEST_DURATION_MS)
    homeVerifyFile();
}

static void homeStart() {
  if (recorderMode == RECORDER_HOME_TEST) {
    homeReply(409, false, "Home test already active; finish first"); return;
  }
  if (state.recording || recorderCore.recording() || logFile || sensorLogFile ||
      recorderMode == RECORDER_RECORDING || recorderMode == RECORDER_MAINTENANCE ||
      otaUploadInProgress) {
    homeReply(409, false, "Cannot test during flight or firmware maintenance"); return;
  }
  if (recorderMode == RECORDER_CALIBRATING) {
    homeReply(409, false, "Keep Feather stationary until calibration completes"); return;
  }
  if (state.sdOk) {
    digitalWrite(config::PIN_LORA_CS, HIGH);
    if (SD.exists(HOME_TEST_PATH) && !SD.remove(HOME_TEST_PATH)) {
      homeReply(500, false, "Unable to clean up prior interrupted home test"); return;
    }
  }
  flightWatcher.cancel();
  recorderCore.resetBuffer();
  recorderMode = RECORDER_HOME_TEST;
  flightState = green_ion::STATE_IDLE;
  homeLogActive = homeWifiSeen = homeWifiAbsent = false;
  homeCleanupOk = true; homeResult = 0;
  homeSamples = homeValidSamples = 0; homeMissed = 0;
  homeRadioStartSent = radioBenchPacketsSent;
  homeRadioStartFailures = radioSendFailures;
  Serial.println("[HOME] Session started. Launch watcher OFF; rocket flight files untouched.");
  homeReply(200, true, "Guided home test started; launch watcher OFF until finish");
}

static void homeStartLog() {
  if (!homeIdle() || homeLogActive) {
    homeReply(409, false, "Home test must be active and temporary recorder idle"); return;
  }
  if (!state.sdOk) {
    homeReply(503, false, "SD card missing"); return;
  }
  digitalWrite(config::PIN_LORA_CS, HIGH);
  if (SD.exists(HOME_TEST_PATH)) {
    homeReply(409, false, "Reserved test file exists; end session to clean it"); return;
  }
  const green_ion::rkt4::Header header = {
    config::SENSOR_RATE_HZ,
    isfinite(calibrationBaselinePa) ? calibrationBaselinePa : 101325.0f,
    state.batteryMv};
  uint8_t bytes[green_ion::rkt4::kHeaderBytes] = {};
  if (!green_ion::rkt4::encodeHeader(bytes, header)) {
    homeReply(500, false, "Invalid RKT4 test header"); return;
  }
  homeFile = SD.open(HOME_TEST_PATH, FILE_WRITE);
  if (!homeFile || homeFile.write(bytes, sizeof(bytes)) != sizeof(bytes)) {
    homeCloseFile();
    homeCleanupOk = SD.remove(HOME_TEST_PATH);
    homeResult = 3;
    homeReply(500, false, "Cannot write temporary test header"); return;
  }
  homeSamples = homeValidSamples = 0;
  homeMissedStart = sampleClock.missedPeriods();
  homeMissed = 0; homeWifiSeen = homeWifiAbsent = false;
  homeResult = 1; homeLogActive = true; homeCleanupOk = false;
  homeStartedMs = millis();
  Serial.println("[HOME] Recording 30 seconds at 50 Hz. Disconnect and reconnect phone Wi-Fi; leave Feather powered.");
  homeReply(202, true, "30-second real-sensor test started; disconnect then reconnect Wi-Fi");
}

static void homeRadio() {
  if (!homeIdle() || homeLogActive || !state.radioOk) {
    homeReply(409, false, "Home mode/radio not ready; check both antennas"); return;
  }
  homeRadioStartSent = radioBenchPacketsSent;
  homeRadioStartFailures = radioSendFailures;
  radioBenchPacketsRemaining = 5;
  lastTelemetryMs = millis() - config::TELEMETRY_INTERVAL_MS;
  Serial.println("[HOME] Sending five radio bench packets. Check actual Uno reception.");
  homeReply(202, true, "Five LoRa packets queued; verify receipt on Uno");
}

static void homeFinish() {
  if (recorderMode != RECORDER_HOME_TEST) {
    homeReply(409, false, "No active home session"); return;
  }
  homeLogActive = false;
  homeCloseFile();
  if (state.sdOk) {
    digitalWrite(config::PIN_LORA_CS, HIGH);
    homeCleanupOk = !SD.exists(HOME_TEST_PATH) || SD.remove(HOME_TEST_PATH);
  } else homeCleanupOk = false;
  if (homeResult == 1) homeResult = 3;
  radioBenchPacketsRemaining = 0;
  resetCalibration(millis());
  Serial.println("[HOME] Session stopped. Stationary calibration restarting.");
  homeReply(homeCleanupOk ? 200 : 500, homeCleanupOk,
    homeCleanupOk ? "Temporary file deleted; recalibrating for flight" :
                    "Home test ended; inspect reserved GIHOME.BIN on SD card");
}
