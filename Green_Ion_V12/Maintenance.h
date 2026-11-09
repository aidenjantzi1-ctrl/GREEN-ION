#pragma once
// V12 local-phone maintenance. Included after the recorder support functions
// so OTA work cannot interfere with the primary RKT4 recording path.
// Requires WebServer, Update.h, esp_ota_ops.h and esp_app_desc.h.

static bool maintenanceTokenValid(const String &token) {
  if (token.length() < 24 || token.length() > 64) return false;
  for (size_t i = 0; i < token.length(); ++i) {
    const char c = token[i];
    if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
          (c >= 'A' && c <= 'F'))) return false;
  }
  return true;
}

static bool maintenanceLeaseValid() {
  return maintenanceControllerToken.length() &&
         millis() - maintenanceLeaseAtMs <= MAINTENANCE_LEASE_MS;
}

static bool requireMaintenanceController() {
  const String token = dashboardServer.arg("token");
  if (!maintenanceTokenValid(token) || !maintenanceLeaseValid() ||
      token != maintenanceControllerToken) {
    dashboardServer.send(403, "application/json",
      "{\"success\":false,\"message\":\"Take Control under System before maintenance\"}");
    return false;
  }
  maintenanceLeaseAtMs = millis();
  return true;
}

static bool maintenanceSafeToBegin() {
  // Once an upload begins, the main loop temporarily stops flight watching,
  // sensor sampling, SD logging, and LoRa so flash writes cannot race them.
  return !otaMaintenanceHold && !otaRebootPending &&
         !state.recording && !recorderCore.recording() &&
         recorderMode != RECORDER_RECORDING &&
         hardwareTestRequest == HARDWARE_TEST_NONE &&
         !imuHardwareTest.running() && !gpsHardwareTest.running() &&
         state.batteryMv >= OTA_MIN_BATTERY_MV;
}

static bool otaPartitionsAvailable() {
  const esp_partition_t *next = esp_ota_get_next_update_partition(nullptr);
  return next && next != esp_ota_get_running_partition();
}

static bool previousFirmwareAvailable() {
  const esp_partition_t *previous = esp_ota_get_next_update_partition(nullptr);
  if (!previous || previous == esp_ota_get_running_partition()) return false;
  esp_app_desc_t description = {};
  return esp_ota_get_partition_description(previous, &description) == ESP_OK;
}

static void handleV12FirmwareUpload() {
  HTTPUpload &upload = dashboardServer.upload();
  if (upload.status == UPLOAD_FILE_START) {
    otaLastActivityMs = millis();
    otaUploadError = "";
    otaUploadBytes = 0;
    otaUploadSucceeded = false;
    otaHeaderChecked = false;
    if (!maintenanceTokenValid(dashboardServer.arg("token")) ||
        !maintenanceLeaseValid() ||
        dashboardServer.arg("token") != maintenanceControllerToken) {
      otaUploadError = "Take Control under System before uploading";
      return;
    }
    maintenanceLeaseAtMs = millis();
    if (!maintenanceSafeToBegin()) {
      otaUploadError = "Stop recording and tests; battery must read at least 3.65 V";
      return;
    }
    String name = upload.filename;
    name.toLowerCase();
    if (!name.endsWith(".ino.bin")) {
      otaUploadError = "Select a compiled application file ending .ino.bin";
      return;
    }
    if (!otaPartitionsAvailable()) {
      otaUploadError = "OTA partition unavailable; upload over USB instead";
      return;
    }
    if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) {
      otaUploadError = "ESP32 updater could not start (error " + String(Update.getError()) + ")";
      return;
    }
    otaUploadInProgress = true;
    otaMaintenanceHold = true;
    Serial.println("[OTA] Phone upload started; flight watcher suspended.");
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    otaLastActivityMs = millis();
    if (!otaUploadInProgress) return;
    if (!otaHeaderChecked) {
      otaHeaderChecked = true;
      if (!upload.currentSize || upload.buf[0] != 0xE9) {
        otaUploadError = "Invalid ESP32 application image header";
        Update.abort();
        otaUploadInProgress = false;
        return;
      }
    }
    const size_t written = Update.write(upload.buf, upload.currentSize);
    if (written != upload.currentSize) {
      otaUploadError = "Flash write failed (error " + String(Update.getError()) + ")";
      Update.abort();
      otaUploadInProgress = false;
      return;
    }
    otaUploadBytes += written;
  } else if (upload.status == UPLOAD_FILE_END) {
    otaLastActivityMs = millis();
    if (!otaUploadInProgress) return;
    if (otaUploadBytes < 200000) {
      otaUploadError = "Firmware too small to be a complete application";
      Update.abort();
      otaUploadInProgress = false;
      return;
    }
    if (!Update.end(true)) {
      otaUploadError = "Image verification failed (error " + String(Update.getError()) + ")";
      otaUploadInProgress = false;
      return;
    }
    otaUploadSucceeded = true;
    otaUploadInProgress = false;
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    if (otaUploadInProgress) Update.abort();
    otaUploadInProgress = false;
    otaMaintenanceHold = false;
    otaUploadError = "Firmware upload aborted";
    if (recorderMode != RECORDER_RECORDING) resetCalibration(millis());
    Serial.println("[OTA] Client disconnected; stationary calibration resumed.");
  }
}

static void finishV12FirmwareUpload() {
  if (!otaUploadSucceeded) {
    if (otaUploadInProgress) Update.abort();
    otaUploadInProgress = false;
    otaMaintenanceHold = false;
    if (recorderMode != RECORDER_RECORDING) resetCalibration(millis());
    Serial.println("[OTA] Upload rejected or failed; autonomous calibration resumed.");
    dashboardServer.send(400, "application/json",
      "{\"success\":false,\"message\":\"" +
      (otaUploadError.length() ? otaUploadError : String("Firmware upload incomplete")) + "\"}");
    return;
  }
  dashboardServer.sendHeader("Cache-Control", "no-store");
  dashboardServer.send(200, "application/json",
    "{\"success\":true,\"message\":\"Firmware verified; restarting shortly\",\"bytes\":" +
    String(otaUploadBytes) + "}");
  otaRebootAtMs = millis() + 1200;  // Finish sending the HTTP reply first.
  otaRebootPending = true;
  Serial.printf("[OTA] Verified %lu bytes; restarting.\n",
                static_cast<unsigned long>(otaUploadBytes));
}

static void handleV12Rollback() {
  if (!requireMaintenanceController()) return;
  if (!maintenanceSafeToBegin()) {
    dashboardServer.send(409, "application/json",
      "{\"success\":false,\"message\":\"Rollback needs idle recorder, no tests and battery >= 3.65 V\"}");
    return;
  }
  if (!previousFirmwareAvailable()) {
    dashboardServer.send(409, "application/json",
      "{\"success\":false,\"message\":\"No valid previous firmware in the other OTA slot\"}");
    return;
  }
  const esp_partition_t *previous = esp_ota_get_next_update_partition(nullptr);
  if (esp_ota_set_boot_partition(previous) != ESP_OK) {
    dashboardServer.send(500, "application/json",
      "{\"success\":false,\"message\":\"Could not select previous firmware partition\"}");
    return;
  }
  otaMaintenanceHold = true;
  dashboardServer.sendHeader("Cache-Control", "no-store");
  dashboardServer.send(200, "application/json",
    "{\"success\":true,\"message\":\"Previous firmware selected; restarting shortly\"}");
  otaRebootAtMs = millis() + 1200;
  otaRebootPending = true;
  Serial.println("[OTA] Rollback selected; restarting.");
}

static void printConsoleStatus() {
  const char *mode = recorderMode == RECORDER_CALIBRATING ? "CALIBRATING" :
      recorderMode == RECORDER_WATCHING ? "READY / WATCHING" :
      recorderMode == RECORDER_RECORDING ? "RECORDING" :
      recorderMode == RECORDER_COMPLETE ? "COMPLETE" : "CHECK RECORDER";
  Serial.println();
  Serial.println("========== GREEN ION V12 ==========");
  Serial.printf("Recorder : %s  |  SD: %s  |  battery: %.3f V\n",
                mode, state.sdOk ? "OK" : "CHECK", state.batteryMv / 1000.0f);
  Serial.printf("Sensors  : BMP388 %s  |  IMU %s  |  GPS %s  |  LoRa %s\n",
                state.barometerOk ? "OK" : "MISSING",
                state.imuOk ? "OK" : "MISSING",
                state.gpsOk ? "OK" : "MISSING",
                state.radioOk ? "OK" : "MISSING");
  Serial.printf("Altitude : %.1f m relative  |  GPS: %s (%u satellites)\n",
                watcherAltitudeM,
                gpsPositionCurrent(millis()) ? "LIVE" :
                  (state.gpsHasPosition ? "STALE" : "NO FIX"), state.satellites);
  Serial.printf("Flight   : %s  |  missed periods: %lu\n",
                logFilename.length() ? logFilename.c_str() : "none",
                static_cast<unsigned long>(sampleClock.missedPeriods()));
  Serial.printf("Wi-Fi    : %s  |  http://192.168.4.1\n", config::DASHBOARD_SSID);
  Serial.println("Commands : status | help | test imu/gps/all/status | bench imu/gps/all/off | radio test/retry/status");
  Serial.println("====================================");
}
