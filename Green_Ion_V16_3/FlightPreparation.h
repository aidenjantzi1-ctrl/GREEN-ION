#pragma once

// V17 firmware-owned one-press flight preparation.
// This never controls ignition, staging or recovery deployment. It only verifies
// the recorder/sensors and arms the pressure-based launch watcher.

enum FlightReadinessPhase : uint8_t {
  FLIGHT_PREP_IDLE = 0,
  FLIGHT_PREP_CRITICAL_CHECK,
  FLIGHT_PREP_SD_INTEGRITY,
  FLIGHT_PREP_SENSOR_START,
  FLIGHT_PREP_SENSOR_WAIT,
  FLIGHT_PREP_RADIO_DIAGNOSTIC,
  FLIGHT_PREP_CALIBRATION_START,
  FLIGHT_PREP_CALIBRATION_WAIT,
  FLIGHT_PREP_SIMULATION,
  FLIGHT_PREP_ARM,
  FLIGHT_PREP_READY,
  FLIGHT_PREP_FAILED,
};

static FlightReadinessPhase flightReadinessPhase = FLIGHT_PREP_IDLE;
static bool flightReadinessActive = false;
static bool flightReadinessSdIntegrityPassed = false;
static bool flightReadinessSimulationPassed = false;
static bool flightReadinessSimulationVerifiedThisBoot = false;
static bool flightReadinessQuick = false;
static uint32_t flightReadinessStartedMs = 0;
static uint32_t flightReadinessPhaseStartedMs = 0;
static uint32_t flightReadinessWarningFlags = 0;
static uint32_t flightReadinessCriticalFlags = 0;
static String flightReadinessFailure;
static uint64_t sdTotalBytesCached = 0;
static uint64_t sdUsedBytesCached = 0;

static uint8_t loraDiagnosticVersion = 0;
static bool loraDiagnosticSpiOk = false;
static bool loraDiagnosticInitOk = false;
static int loraDiagnosticDio0 = -1;
static String loraDiagnosticCode = "NOT_RUN";
static String loraDiagnosticProblem = "Run the passive LoRa diagnostic on the bench.";

static const char *flightReadinessPhaseName() {
  switch (flightReadinessPhase) {
    case FLIGHT_PREP_CRITICAL_CHECK: return "CHECKING CRITICAL HARDWARE";
    case FLIGHT_PREP_SD_INTEGRITY: return "VERIFYING MICROSD RECORDING";
    case FLIGHT_PREP_SENSOR_START:
    case FLIGHT_PREP_SENSOR_WAIT: return "TESTING SENSORS";
    case FLIGHT_PREP_RADIO_DIAGNOSTIC: return "CHECKING OPTIONAL LORA";
    case FLIGHT_PREP_CALIBRATION_START:
    case FLIGHT_PREP_CALIBRATION_WAIT: return "CALIBRATING ALTITUDE";
    case FLIGHT_PREP_SIMULATION: return "SIMULATING RECORDER / LAUNCH DETECTOR";
    case FLIGHT_PREP_ARM: return "ARMING LAUNCH WATCHER";
    case FLIGHT_PREP_READY: return "ARMED — READY FOR LAUNCH";
    case FLIGHT_PREP_FAILED: return "NOT READY";
    default: return "IDLE";
  }
}

static uint8_t flightReadinessProgress() {
  switch (flightReadinessPhase) {
    case FLIGHT_PREP_CRITICAL_CHECK: return 8;
    case FLIGHT_PREP_SD_INTEGRITY: return 20;
    case FLIGHT_PREP_SENSOR_START: return 30;
    case FLIGHT_PREP_SENSOR_WAIT: return 45;
    case FLIGHT_PREP_RADIO_DIAGNOSTIC: return 58;
    case FLIGHT_PREP_CALIBRATION_START: return 65;
    case FLIGHT_PREP_CALIBRATION_WAIT: return 78;
    case FLIGHT_PREP_SIMULATION: return 88;
    case FLIGHT_PREP_ARM: return 96;
    case FLIGHT_PREP_READY: return 100;
    case FLIGHT_PREP_FAILED: return 100;
    default: return 0;
  }
}

static bool flightReadinessContextBusy() {
  return state.recording || recorderCore.recording() || logFile || sensorLogFile ||
         recorderMode == RECORDER_RECORDING ||
         recorderMode == RECORDER_HOME_TEST ||
         recorderMode == RECORDER_MAINTENANCE ||
         otaUploadInProgress || homeWifiOperationActive() ||
         wifiManagementSessionActive();
}

static void updateSdCapacityCache() {
  if (!state.sdOk || recorderCore.recording()) return;
  SdSpiScope bus;
  if (!bus) return;
  sdTotalBytesCached = SD.totalBytes();
  sdUsedBytesCached = SD.usedBytes();
}

static void failFlightReadiness(const String &reason) {
  flightReadinessFailure = reason;
  flightReadinessActive = false;
  flightReadinessPhase = FLIGHT_PREP_FAILED;
  flightWatcher.cancel();
  flightState = green_ion::STATE_IDLE;
  recorderMode = RECORDER_FAULT;
}

static bool runFlightReadinessSdIntegrityTest() {
  if (!state.sdOk || recorderCore.recording()) return false;
  static const char path[] = "/GIPREFLT.BIN";
  static const uint16_t recordCount = 100;  // 2 seconds at 50 Hz.
  SdSpiScope bus;
  if (!bus) return false;

  if (SD.exists(path) && !SD.remove(path)) return false;
  File file = SD.open(path, FILE_WRITE);
  if (!file) return false;

  const green_ion::rkt4::Header header = {
    config::SENSOR_RATE_HZ,
    isfinite(state.pressurePa) ? state.pressurePa : 101325.0f,
    state.batteryMv,
  };
  uint8_t headerBytes[green_ion::rkt4::kHeaderBytes] = {};
  bool ok = green_ion::rkt4::encodeHeader(headerBytes, header) &&
            file.write(headerBytes, sizeof(headerBytes)) == sizeof(headerBytes);
  uint8_t bytes[green_ion::rkt4::kRecordBytes] = {};
  for (uint16_t i = 0; ok && i < recordCount; ++i) {
    const green_ion::rkt4::Record record = {
      static_cast<uint32_t>(i) * config::SENSOR_INTERVAL_US,
      101325.0f - static_cast<float>(i) * 0.75f,
      20.0f + static_cast<float>(i % 5) * 0.1f,
      state.batteryMv,
      true,
    };
    green_ion::rkt4::encodeRecord(bytes, record);
    ok = file.write(bytes, sizeof(bytes)) == sizeof(bytes);
  }
  if (file) {
    file.flush();
    file.close();
  }

  const size_t expectedBytes =
      green_ion::rkt4::kHeaderBytes +
      static_cast<size_t>(recordCount) * green_ion::rkt4::kRecordBytes;
  file = SD.open(path, FILE_READ);
  ok = ok && file && static_cast<size_t>(file.size()) == expectedBytes;
  if (ok) {
    ok = file.read(headerBytes, sizeof(headerBytes)) == sizeof(headerBytes) &&
         green_ion::rkt4::validateHeader(headerBytes, sizeof(headerBytes)) ==
             green_ion::rkt4::RKT4_VALID;
  }
  uint32_t previousTimestamp = 0;
  for (uint16_t i = 0; ok && i < recordCount; ++i) {
    ok = file.read(bytes, sizeof(bytes)) == sizeof(bytes);
    if (!ok) break;
    green_ion::rkt4::Record decoded = {};
    green_ion::rkt4::decodeRecord(bytes, decoded);
    const uint32_t expectedTimestamp =
        static_cast<uint32_t>(i) * config::SENSOR_INTERVAL_US;
    ok = decoded.valid && decoded.timestampUs == expectedTimestamp &&
         decoded.batteryMv == state.batteryMv &&
         (i == 0 ||
          green_ion::rkt4::timestampDeltaUs(decoded.timestampUs,
                                             previousTimestamp) ==
              config::SENSOR_INTERVAL_US);
    previousTimestamp = decoded.timestampUs;
  }
  if (file) file.close();

  const bool removed = !SD.exists(path) || SD.remove(path);
  ok = ok && removed && !SD.exists(path);
  sdTotalBytesCached = SD.totalBytes();
  sdUsedBytesCached = SD.usedBytes();
  return ok;
}

static bool simulateLaunchAndLandingDetectors() {
  green_ion::flight::FlightWatcher simulatedLaunch;
  const bool started = simulatedLaunch.begin({
    config::SENSOR_RATE_HZ, 0.0f, 101325.0f, 0.10f});
  if (!started) return false;

  bool launched = false;
  uint32_t t = 0;
  for (uint8_t i = 0; i < 10 && !launched; ++i, t += 20) {
    launched = simulatedLaunch.update({t, 0.0f, 0.0f, 101325.0f, true});
  }
  for (uint8_t i = 0; i < 12 && !launched; ++i, t += 20) {
    const float altitude = 2.0f + static_cast<float>(i) * 0.4f;
    launched = simulatedLaunch.update(
        {t, altitude, 20.0f, 101325.0f - altitude * 12.0f, true});
  }
  if (!launched) return false;

  green_ion::flight::LandingWatcher simulatedLanding;
  if (!simulatedLanding.begin(0, {config::SENSOR_RATE_HZ, 0.0f, 0.10f}))
    return false;

  bool landed = false;
  for (uint32_t ms = 0; ms <= 33000 && !landed; ms += 20) {
    float altitude = 0.0f;
    float speed = 0.0f;
    if (ms < 2000) {
      altitude = 30.0f * static_cast<float>(ms) / 2000.0f;
      speed = 15.0f;
    } else if (ms < 6000) {
      altitude = 30.0f;
      speed = 0.0f;
    } else if (ms < 15000) {
      const float fraction = static_cast<float>(ms - 6000) / 9000.0f;
      altitude = 30.0f - 28.0f * fraction;
      speed = -3.1f;
    } else {
      altitude = 2.0f;
      speed = 0.0f;
    }
    landed = simulatedLanding.update({ms, altitude, speed, true});
  }
  return landed;
}

static uint8_t readLoraVersionRegister() {
  RadioSpiScope bus;
  if (!bus) return 0;
  SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
  digitalWrite(config::PIN_LORA_CS, LOW);
  SPI.transfer(0x42 & 0x7f);  // SX127x RegVersion, read.
  const uint8_t version = SPI.transfer(0);
  digitalWrite(config::PIN_LORA_CS, HIGH);
  SPI.endTransaction();
  return version;
}

static void runPassiveLoraDiagnostic(bool allowReinitialize) {
  loraDiagnosticVersion = readLoraVersionRegister();
  loraDiagnosticSpiOk = loraDiagnosticVersion == 0x12;
  loraDiagnosticDio0 = digitalRead(config::PIN_LORA_DIO0);

  if (loraDiagnosticVersion == 0x00) {
    loraDiagnosticCode = "NO_SPI_RESPONSE_LOW";
    loraDiagnosticProblem =
        "RegVersion read 0x00. Check RFM9x 3V/GND, MISO, SCK and CS/NSS 27; confirm SD CS remains 33.";
  } else if (loraDiagnosticVersion == 0xff) {
    loraDiagnosticCode = "NO_SPI_RESPONSE_HIGH";
    loraDiagnosticProblem =
        "RegVersion read 0xFF. MISO may be floating/high or CS/NSS 27 may not be selecting the radio; check power and SPI wiring.";
  } else if (!loraDiagnosticSpiOk) {
    loraDiagnosticCode = "UNEXPECTED_CHIP_ID";
    char detail[96];
    snprintf(detail, sizeof(detail),
             "RegVersion is 0x%02X, expected SX127x/RFM95 value 0x12. Check the radio model and shared SPI wiring.",
             loraDiagnosticVersion);
    loraDiagnosticProblem = detail;
  } else {
    if (allowReinitialize && !state.radioOk)
      radioRecovery.noteInitialize(millis(), configureRadioHardware());
    loraDiagnosticInitOk = state.radioOk;
    if (state.radioOk) {
      loraDiagnosticCode = "READY";
      loraDiagnosticProblem =
          "SPI chip ID and RadioHead initialization are good. Packet reception still requires matching Uno settings and suitable antennas.";
    } else {
      loraDiagnosticCode = "INIT_FAILED";
      loraDiagnosticProblem =
          "SX127x chip ID is readable but radio initialization failed. Check RESET 15, DIO0 32, CS/NSS 27, and shared SPI wiring/settings.";
    }
  }
  loraDiagnosticInitOk = state.radioOk;
}

static green_ion::readiness::Result currentFlightReadinessResult() {
  return green_ion::readiness::evaluate({
    state.sdOk,
    state.barometerOk,
    state.imuOk && imuRange32gVerified,
    state.batteryMv,
    recoveryPending == 0,
    !orphanHomeFile && !homeInterrupted && homeCleanupOk,
    flightReadinessSdIntegrityPassed,
    isfinite(calibrationBaselinePa) && calibrationBaselinePa > 30000.0f &&
        isfinite(calibrationNoiseM),
    flightReadinessSimulationPassed,
    recorderMode == RECORDER_WATCHING &&
        flightWatcher.state() == green_ion::flight::WATCHING,
    gpsPositionCurrent(millis()),
    state.radioOk,
    sensorLogFailures == 0,
    true,  // Preflight-internal SD work is not a flight missed-sample failure.
    validFlightProfileValue(selectedRocket, false)
  });
}

static void beginFlightReadiness() {
  const String requestedMode = dashboardServer.arg("mode");
  if (!requestedMode.isEmpty() && requestedMode != "full" && requestedMode != "quick") {
    dashboardServer.send(400, "application/json",
      "{\"success\":false,\"message\":\"mode must be full or quick\"}");
    return;
  }
  if (flightReadinessContextBusy()) {
    dashboardServer.send(409, "application/json",
      "{\"success\":false,\"message\":\"PREPARE FOR FLIGHT is blocked while recording, Home Test, Wi-Fi setup or firmware maintenance is active\"}");
    return;
  }
  flightWatcher.cancel();
  recorderCore.resetBuffer();
  flightReadinessFailure = "";
  flightReadinessQuick = requestedMode == "quick";
  flightReadinessSdIntegrityPassed = false;
  // Repeat Quick Fly may reuse only the deterministic launch/landing detector
  // self-test from this same boot. SD, battery, critical sensors and the
  // stationary altitude baseline are re-verified every single flight.
  flightReadinessSimulationPassed =
      flightReadinessQuick && flightReadinessSimulationVerifiedThisBoot;
  flightReadinessWarningFlags = 0;
  flightReadinessCriticalFlags = 0;
  flightReadinessStartedMs = millis();
  flightReadinessPhaseStartedMs = flightReadinessStartedMs;
  flightReadinessActive = true;
  flightReadinessPhase = FLIGHT_PREP_CRITICAL_CHECK;
  flightState = green_ion::STATE_IDLE;
  resetCalibration(flightReadinessStartedMs);
  String response = "{\"success\":true,\"mode\":\"";
  response += flightReadinessQuick ? "quick" : "full";
  response += "\",\"message\":\"";
  response += flightReadinessQuick
      ? "Quick Fly started. microSD, battery, critical sensors and a fresh altitude baseline will be verified before automatic arming."
      : "Flight preparation started. Keep GREEN ION stationary; SD recording is the critical path and GPS/LoRa are optional warnings.";
  response += "\"}";
  dashboardServer.send(202, "application/json", response);
}

static void serviceFlightReadiness(uint32_t nowMs) {
  if (!flightReadinessActive) return;
  switch (flightReadinessPhase) {
    case FLIGHT_PREP_CRITICAL_CHECK:
      if (!state.sdOk) { failFlightReadiness("microSD is unavailable"); return; }
      if (!state.barometerOk) { failFlightReadiness("BMP388 barometer is unavailable"); return; }
      if (!state.imuOk) { failFlightReadiness("IMU is unavailable"); return; }
      if (state.batteryMv < 3600U) { failFlightReadiness("battery is below 3.60 V"); return; }
      if (recoveryPending) { failFlightReadiness("interrupted flight recovery still needs review"); return; }
      if (orphanHomeFile || homeInterrupted || !homeCleanupOk) {
        failFlightReadiness("temporary home-test cleanup is incomplete");
        return;
      }
      flightReadinessPhase = FLIGHT_PREP_SD_INTEGRITY;
      flightReadinessPhaseStartedMs = nowMs;
      break;

    case FLIGHT_PREP_SD_INTEGRITY:
      flightReadinessSdIntegrityPassed = runFlightReadinessSdIntegrityTest();
      if (!flightReadinessSdIntegrityPassed) {
        failFlightReadiness("microSD write/read/RKT4 integrity/delete test failed");
        return;
      }
      flightReadinessPhase = FLIGHT_PREP_SENSOR_START;
      flightReadinessPhaseStartedMs = nowMs;
      break;

    case FLIGHT_PREP_SENSOR_START:
      // Full preparation also checks GPS communication/fix evidence. Quick Fly
      // still runs the critical IMU test but does not wait on optional GPS.
      requestHardwareTest(flightReadinessQuick ? HARDWARE_TEST_IMU : HARDWARE_TEST_ALL);
      flightReadinessPhase = FLIGHT_PREP_SENSOR_WAIT;
      flightReadinessPhaseStartedMs = nowMs;
      break;

    case FLIGHT_PREP_SENSOR_WAIT: {
      const auto imuResult = imuHardwareTest.result();
      const auto gpsResult = gpsHardwareTest.result();
      const bool imuDone = imuResult != green_ion::bench::RESULT_IDLE &&
                           imuResult != green_ion::bench::RESULT_RUNNING;
      const bool gpsDone = gpsResult != green_ion::bench::RESULT_IDLE &&
                           gpsResult != green_ion::bench::RESULT_RUNNING;
      if (!imuDone || (!flightReadinessQuick && !gpsDone)) {
        if (nowMs - flightReadinessPhaseStartedMs < 16000UL) break;
      }
      if (imuHardwareTest.result() != green_ion::bench::RESULT_PASS) {
        failFlightReadiness("IMU stationary/range diagnostic did not pass");
        return;
      }
      flightReadinessPhase = FLIGHT_PREP_RADIO_DIAGNOSTIC;
      flightReadinessPhaseStartedMs = nowMs;
      break;
    }

    case FLIGHT_PREP_RADIO_DIAGNOSTIC:
      // LoRa is a recovery/telemetry aid, never the primary flight record.
      // Full preparation refreshes the passive diagnostic. Quick Fly skips it.
      if (!flightReadinessQuick) runPassiveLoraDiagnostic(true);
      flightReadinessPhase = FLIGHT_PREP_CALIBRATION_START;
      flightReadinessPhaseStartedMs = nowMs;
      break;

    case FLIGHT_PREP_CALIBRATION_START:
      resetCalibration(nowMs);
      flightReadinessPhase = FLIGHT_PREP_CALIBRATION_WAIT;
      flightReadinessPhaseStartedMs = nowMs;
      break;

    case FLIGHT_PREP_CALIBRATION_WAIT:
      if (isfinite(calibrationBaselinePa) && calibrationBaselinePa > 30000.0f &&
          isfinite(calibrationNoiseM)) {
        flightReadinessPhase =
            (flightReadinessQuick && flightReadinessSimulationVerifiedThisBoot)
                ? FLIGHT_PREP_ARM
                : FLIGHT_PREP_SIMULATION;
        if (flightReadinessQuick && flightReadinessSimulationVerifiedThisBoot)
          flightReadinessSimulationPassed = true;
        flightReadinessPhaseStartedMs = nowMs;
      } else if (nowMs - flightReadinessPhaseStartedMs > 15000UL) {
        failFlightReadiness("stable altitude calibration did not complete");
      }
      break;

    case FLIGHT_PREP_SIMULATION:
      flightReadinessSimulationPassed = simulateLaunchAndLandingDetectors();
      if (!flightReadinessSimulationPassed) {
        failFlightReadiness("simulated launch/landing detector self-test failed");
        return;
      }
      flightReadinessSimulationVerifiedThisBoot = true;
      flightReadinessPhase = FLIGHT_PREP_ARM;
      flightReadinessPhaseStartedMs = nowMs;
      break;

    case FLIGHT_PREP_ARM: {
      // Final critical recheck immediately before arming.
      if (!state.sdOk || !state.barometerOk || !state.imuOk ||
          state.batteryMv < 3600U || recoveryPending ||
          !flightReadinessSdIntegrityPassed || !flightReadinessSimulationPassed) {
        failFlightReadiness("critical readiness changed before arming");
        return;
      }
      recorderCore.resetBuffer();
      const bool watching = flightWatcher.begin({
        config::SENSOR_RATE_HZ, 0.0f, calibrationBaselinePa, calibrationNoiseM});
      if (!watching) {
        failFlightReadiness("launch watcher could not arm");
        return;
      }
      recorderMode = RECORDER_WATCHING;
      flightState = green_ion::STATE_ARMED;
      flightReadinessActive = false;
      flightReadinessPhase = FLIGHT_PREP_READY;
      const auto result = currentFlightReadinessResult();
      flightReadinessCriticalFlags = result.criticalFlags;
      flightReadinessWarningFlags = result.warningFlags;
      Serial.printf("[PREFLIGHT] ARMED ready=1 warnings=0x%08lx lora=%s gps=%s sd=PASS\n",
                    static_cast<unsigned long>(result.warningFlags),
                    state.radioOk ? "READY" : "WARNING",
                    gpsPositionCurrent(nowMs) ? "READY" : "WARNING");
      break;
    }

    default:
      break;
  }
}

static void runLoraDiagnosticEndpoint() {
  if (state.recording || recorderCore.recording() ||
      recorderMode == RECORDER_RECORDING ||
      recorderMode == RECORDER_HOME_TEST ||
      recorderMode == RECORDER_MAINTENANCE || otaUploadInProgress) {
    dashboardServer.send(409, "application/json",
      "{\"success\":false,\"message\":\"LoRa diagnostics are bench-only and disabled during recording/test/maintenance\"}");
    return;
  }
  runPassiveLoraDiagnostic(true);
  const bool transmit = dashboardServer.arg("transmit") == "1";
  if (transmit) {
    if (dashboardServer.arg("antenna") != "confirmed") {
      dashboardServer.send(400, "application/json",
        "{\"success\":false,\"message\":\"Transmission refused: confirm a suitable antenna is attached first\"}");
      return;
    }
    if (!state.radioOk) {
      dashboardServer.send(409, "application/json",
        "{\"success\":false,\"message\":\"Radio did not initialize, so no packet was transmitted\"}");
      return;
    }
    radioTransmitAuthorized = true;
    radioBenchPacketsRemaining = 3;
    lastTelemetryMs = millis() - config::TELEMETRY_INTERVAL_MS;
  }

  String body = "{\"success\":true,\"code\":\"" + loraDiagnosticCode + "\"";
  body += ",\"chip_version\":" + String(loraDiagnosticVersion);
  body += ",\"expected_chip_version\":18";
  body += ",\"spi_ok\":" + String(loraDiagnosticSpiOk ? "true" : "false");
  body += ",\"radio_init_ok\":" + String(state.radioOk ? "true" : "false");
  body += ",\"dio0_level\":" + String(loraDiagnosticDio0);
  body += ",\"feather_pins\":{\"sck\":5,\"mosi\":19,\"miso\":21,\"sd_cs\":33,\"lora_cs\":27,\"reset\":15,\"dio0\":32}";
  body += ",\"settings\":{\"frequency_mhz\":" + String(config::LORA_FREQUENCY_MHZ, 1);
  body += ",\"bandwidth_hz\":" + String(config::LORA_SIGNAL_BANDWIDTH_HZ);
  body += ",\"spreading_factor\":" + String(config::LORA_SPREADING_FACTOR);
  body += ",\"coding_rate\":\"4/" + String(config::LORA_CODING_RATE_DENOMINATOR) + "\"}";
  body += ",\"antenna_detectable\":false";
  body += ",\"transmit_authorized\":" + String(radioTransmitAuthorized ? "true" : "false");
  body += ",\"transmit_started\":" + String(transmit && state.radioOk ? "true" : "false");
  body += ",\"probable_problem\":\"" + loraDiagnosticProblem + "\"}";
  dashboardServer.send(200, "application/json", body);
}
