#pragma once
// Guided home bench test. Never touches V12D### flight recordings.
static const char HOME_TEST_PATH[] = "/GIHOME.BIN";
static const uint32_t HOME_TEST_DURATION_MS = 30000UL;

static void homeReply(int code, bool ok, const char *message) {
  dashboardServer.send(code, "application/json",
    String("{\"success\":") + (ok ? "true" : "false") +
    ",\"message\":\"" + message + "\"}");
}


static void homeLoadProgress() {
  Preferences prefs;
  if (!prefs.begin("giohome", true)) return;
  if (prefs.getBytesLength("steps") == sizeof(homeStepStatuses))
    prefs.getBytes("steps", homeStepStatuses, sizeof(homeStepStatuses));
  homeInterrupted = prefs.getBool("active", false);
  prefs.end();
  if (homeInterrupted) Serial.println("[HOME] Prior bench session interrupted. Re-run home test; inspect cleanup.");
}

static const char *homeStepName(uint8_t code) {
  switch (code) {
    case 1: return "PASS";
    case 2: return "FAIL";
    case 3: return "INCONCLUSIVE";
    case 4: return "CONFIRMED";
    case 5: return "NOT VERIFIED";
    default: return "NOT RUN";
  }
}

// Only completed steps are persisted. Measured steps are classified on-device.
static void homeStoreStepResult() {
  if (recorderMode != RECORDER_HOME_TEST || homeLogActive) {
    homeReply(409, false, "Home verification not idle"); return;
  }
  const int step = dashboardServer.arg("step").toInt();
  const String action = dashboardServer.arg("action");
  if (step < 1 || step > 9 ||
      (action != "measure" && action != "skip" && action != "confirm")) {
    homeReply(400, false, "Invalid step or action"); return;
  }
  uint8_t code = 3;
  if (action == "skip") code = 5;
  else if (step == 1 || step == 7 || step == 9) {
    if (action != "confirm") { homeReply(400, false, "Manual step requires confirmation"); return; }
    code = 4;
  } else if (step == 6) {
    if (action != "confirm") { homeReply(400, false, "Actual Uno reception must be confirmed"); return; }
    if (radioBenchPacketsSent - homeRadioStartSent < 5 ||
        radioBenchPacketsRemaining ||
        radioSendFailures != homeRadioStartFailures) {
      homeReply(409, false, "Five clean Feather transmissions not completed"); return;
    }
    code = 4; // No receiver-side automatic proof: user-reported.
  } else if (action != "measure") {
    homeReply(400, false, "Measured step must be checked by firmware"); return;
  } else if (step == 2) {
    code = state.batteryMv >= 3500 && state.barometerOk &&
      isfinite(state.pressurePa) && state.pressurePa >= 30000 &&
      state.pressurePa <= 120000 &&
      isfinite(calibrationBaselinePa) &&
      calibrationBaselinePa >= 30000 && calibrationBaselinePa <= 120000 ? 1 : 2;
  } else if (step == 3) {
    if (!homeImuTestStarted || imuHardwareTest.running()) {
      homeReply(409, false, "Run and finish a fresh IMU test in this home session");
      return;
    }
    const auto v = imuHardwareTest.result();
    code = v == green_ion::bench::RESULT_PASS ? 1 :
           v == green_ion::bench::RESULT_FAIL ? 2 : 3;
  } else if (step == 4) {
    if (!homeGpsTestStarted || gpsHardwareTest.running()) {
      homeReply(409, false, "Run and finish a fresh GPS test in this home session");
      return;
    }
    const auto v = gpsHardwareTest.result();
    code = v == green_ion::bench::RESULT_PASS ? 1 :
           v == green_ion::bench::RESULT_FAIL ? 2 : 3;
  } else if (step == 5) code = homeSdVerified ? 1 : 2;
  else if (step == 8) {
    if (homeResult < 2) { homeReply(409, false, "Wait for temporary recording verification"); return; }
    code = homeResult == 3 || !homeCleanupOk ? 2 : homeWifiAbsent ? 1 : 3;
  }
  Preferences prefs;
  if (!prefs.begin("giohome", false)) { homeReply(500, false, "NVS not available"); return; }
  const uint8_t old = homeStepStatuses[step - 1];
  homeStepStatuses[step - 1] = code;
  const bool ok = prefs.putBytes("steps", homeStepStatuses, sizeof(homeStepStatuses)) ==
                  sizeof(homeStepStatuses);
  prefs.end();
  if (!ok) { homeStepStatuses[step - 1] = old; homeReply(500, false, "NVS save failed"); return; }
  dashboardServer.send(200, "application/json", String("{\"success\":true,\"result\":\"") +
    homeStepName(code) + "\",\"step\":" + String(step) +
    ",\"message\":\"Saved on Feather; manual checks remain user-confirmed\"}");
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
  SdSpiScope bus;
  if (!bus) {
    homeResult = 3;
    homeCleanupOk = false;
    Serial.println("[HOME] Shared SPI busy while verifying temporary recording.");
    return;
  }
  homeCloseFile();
  homeMissed = sampleClock.missedPeriods() - homeMissedStart;
  bool good = false;
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
  orphanHomeFile = !homeCleanupOk;
  const auto &spiEvidence = sharedSpi.evidence();
  const bool sharedRadioPass = !homeRadioConcurrentExpected ||
      (radioBenchPacketsSent - homeRadioStartSent >=
           config::HOME_CONCURRENT_RADIO_PACKETS &&
       radioSendFailures == homeRadioStartFailures &&
       radioTxTimeouts == homeRadioTimeoutsStart &&
       spiEvidence.ownershipErrors == homeOwnershipErrorsStart);
  good = good && homeCleanupOk && homeSamples >= 1400 &&
      homeValidSamples >= (static_cast<uint32_t>(homeSamples) * 9UL) / 10UL &&
      homeMissed <= 30 && sharedRadioPass;
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
  SdSpiScope bus;
  if (!bus) {
    homeLogActive = false;
    homeCleanupOk = false;
    homeResult = 3;
    Serial.println("[HOME] Shared SPI ownership unavailable during temporary record; cleanup requires review.");
    return;
  }
  if (!homeFile || homeFile.write(bytes, sizeof(bytes)) != sizeof(bytes)) {
    homeLogActive = false;
    homeCloseFile();
    homeCleanupOk = SD.remove(HOME_TEST_PATH);
    orphanHomeFile = !homeCleanupOk;
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
  if (wifiManagementSessionActive() || homeWifiOperationActive()) {
    homeReply(409, false, "Finish Wi-Fi setup before starting the home test"); return;
  }
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
  // Stop launch detection BEFORE SD cleanup or NVS writes can block the
  // sample loop. On any setup failure, restart calibration rather than
  // silently resuming an old watcher state.
  flightWatcher.cancel();
  recorderMode = RECORDER_HOME_TEST;
  recorderCore.resetBuffer();
  if (state.sdOk) {
    SdSpiScope bus;
    if (!bus) {
      resetCalibration(millis());
      homeReply(503, false, "Shared SPI busy; retry home test after current operation"); return;
    }
    if (SD.exists(HOME_TEST_PATH) && !SD.remove(HOME_TEST_PATH)) {
      orphanHomeFile = true;
      resetCalibration(millis());
      homeReply(500, false, "Unable to clean up prior interrupted home test"); return;
    }
  }
  Preferences prefs;
  if (!prefs.begin("giohome", false)) {
    resetCalibration(millis());
    homeReply(500, false, "Home session NVS unavailable"); return;
  }
  uint8_t clean[sizeof(homeStepStatuses)] = {};
  const bool stored = prefs.putBytes("steps", clean, sizeof(clean)) == sizeof(clean) &&
                      prefs.putBool("active", true);
  prefs.end();
  if (!stored) {
    resetCalibration(millis());
    homeReply(500, false, "Home session NVS save failed"); return;
  }
  memcpy(homeStepStatuses, clean, sizeof(clean));
  homeInterrupted = false; orphanHomeFile = false; homeSdVerified = false;
  homeImuTestStarted = false; homeGpsTestStarted = false;
  flightState = green_ion::STATE_IDLE;
  homeLogActive = homeWifiSeen = homeWifiAbsent = false;
  homeCleanupOk = true; homeResult = 0;
  homeSamples = homeValidSamples = 0; homeMissed = 0;
  homeRadioStartSent = radioBenchPacketsSent;
  homeRadioStartFailures = radioSendFailures;
  homeRadioConcurrentExpected = false;
  homeTelemetryDroppedStart = telemetryQueue.dropped();
  homeSensorDroppedStart = sensorLogCore.droppedRecords();
  homeOwnershipErrorsStart = sharedSpi.evidence().ownershipErrors;
  homeRadioTimeoutsStart = radioTxTimeouts;
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
  SdSpiScope bus;
  if (!bus) {
    homeReply(503, false, "Shared SPI busy; retry temporary recording"); return;
  }
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
    orphanHomeFile = !homeCleanupOk;
    homeResult = 3;
    homeReply(500, false, "Cannot write temporary test header"); return;
  }
  homeSamples = homeValidSamples = 0;
  homeMissedStart = sampleClock.missedPeriods();
  homeMissed = 0; homeWifiSeen = homeWifiAbsent = false;
  homeResult = 1; homeLogActive = true; homeCleanupOk = false;
  homeStartedMs = millis();
  // Exercise the real shared SPI architecture during the 50 Hz temporary
  // recording. Radio remains optional: missing radio evidence never fabricates
  // a PASS and never prevents the SD recorder from being tested.
  homeRadioStartSent = radioBenchPacketsSent;
  homeRadioStartFailures = radioSendFailures;
  homeTelemetryDroppedStart = telemetryQueue.dropped();
  homeSensorDroppedStart = sensorLogCore.droppedRecords();
  homeOwnershipErrorsStart = sharedSpi.evidence().ownershipErrors;
  homeRadioTimeoutsStart = radioTxTimeouts;
  homeRadioConcurrentExpected = state.radioOk;
  if (state.radioOk) {
    radioBenchPacketsRemaining = config::HOME_CONCURRENT_RADIO_PACKETS;
    lastTelemetryMs = homeStartedMs - config::TELEMETRY_INTERVAL_MS;
  }
  Serial.println("[HOME] Recording 30 seconds at 50 Hz while bounded LoRa bench telemetry is scheduled. Disconnect/reconnect phone Wi-Fi; leave Feather powered.");
  homeReply(202, true, "30-second 50 Hz SD + optional LoRa shared-SPI stress test started");
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
  if (state.sdOk) {
    SdSpiScope bus;
    if (bus) {
      homeCloseFile();
      homeCleanupOk = !SD.exists(HOME_TEST_PATH) || SD.remove(HOME_TEST_PATH);
    } else {
      homeCleanupOk = false;
    }
  } else {
    // The handle may still exist after a card fault. Do not attempt an
    // uncoordinated flush; mark cleanup unverified and require inspection.
    homeCleanupOk = false;
  }
  orphanHomeFile = !homeCleanupOk;
  if (homeResult == 1) homeResult = 3;
  radioBenchPacketsRemaining = 0;
  Preferences prefs;
  bool stored = false;
  if (prefs.begin("giohome", false)) {
    stored = prefs.putBool("active", !homeCleanupOk);
    prefs.end();
  }
  homeInterrupted = !homeCleanupOk || !stored;
  resetCalibration(millis());
  Serial.println("[HOME] Session stopped. Stationary calibration restarting.");
  homeReply(homeCleanupOk && !homeInterrupted ? 200 : 500,
    homeCleanupOk && !homeInterrupted,
    homeCleanupOk ? "Temporary file deleted; recalibrating for flight" :
                    "Home test ended; inspect reserved GIHOME.BIN on SD card");
}
