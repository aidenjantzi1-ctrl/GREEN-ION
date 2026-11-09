#pragma once
// GREEN ION V12: bench-only firmware maintenance handlers.
// Included by Green_Ion_V12.ino after the recorder and HTTP server definitions.
// Entering maintenance cancels launch watching. These endpoints must never be
// used at a launch pad or during active recording.
#include <Update.h>
#include <esp_ota_ops.h>

static bool otaPowerReady() {
  return state.batteryMv >= 3600;
}

static const esp_partition_t *otaTargetPartition() {
  return esp_ota_get_next_update_partition(esp_ota_get_running_partition());
}

static bool otaMaintenanceReady() {
  return recorderMode == RECORDER_MAINTENANCE &&
         !state.recording && !recorderCore.recording() &&
         !logFile && !sensorLogFile && otaPowerReady() &&
         otaTargetPartition() != nullptr;
}

// The random token is not a replacement for keeping the AP password private.
// A different browser cannot upload firmware without entering maintenance.
static bool otaTokenValid() {
  return otaMaintenanceToken.length() >= 8 &&
         dashboardServer.arg("token") == otaMaintenanceToken;
}

static void otaReply(int code, bool success, const String &message) {
  dashboardServer.send(code, "application/json",
    String("{\"success\":") + (success ? "true" : "false") +
    ",\"message\":\"" + message + "\"}");
}

static void startFirmwareMaintenance() {
  if (otaUploadInProgress) {
    otaReply(409, false, "Firmware upload already in progress");
    return;
  }
  if (state.recording || recorderCore.recording() || logFile || sensorLogFile) {
    otaReply(409, false, "Active or incomplete flight file; finalize before maintenance");
    return;
  }
  if (!otaPowerReady()) {
    otaReply(409, false,
             "Battery must measure at least 3.60 V; charge LiPo and connect USB power");
    return;
  }
  if (!otaTargetPartition()) {
    otaReply(503, false,
             "No OTA partition: install over USB using an OTA-capable partition scheme");
    return;
  }
  flightWatcher.cancel();
  recorderCore.resetBuffer();
  recorderMode = RECORDER_MAINTENANCE;
  flightState = green_ion::STATE_IDLE;
  otaMaintenanceToken = String(esp_random(), HEX) + String(esp_random(), HEX);
  Serial.println("[OTA] MAINTENANCE active; launch detection OFF.");
  dashboardServer.send(200, "application/json",
    "{\"success\":true,\"token\":\"" + otaMaintenanceToken +
    "\",\"message\":\"Maintenance active; launch watch is OFF until exit or restart\"}");
}

static void exitFirmwareMaintenance() {
  if (!otaTokenValid() || recorderMode != RECORDER_MAINTENANCE) {
    otaReply(403, false, "Enter maintenance from this dashboard before exiting");
    return;
  }
  if (otaUploadInProgress || otaUploadSucceeded) {
    otaReply(409, false, "Cannot exit while firmware is updating or restarting");
    return;
  }
  otaMaintenanceToken = "";
  resetCalibration(millis());
  Serial.println("[OTA] MAINTENANCE exited; starting stationary calibration.");
  otaReply(200, true, "Maintenance exited; stationary calibration restarted");
}

static void abortFirmwareUpload(const String &error) {
  otaUploadError = error;
  if (otaUploadInProgress) Update.abort();
  otaUploadInProgress = false;
  Serial.printf("[OTA] ERROR: %s\n", error.c_str());
}

static void handleFirmwareUpload() {
  HTTPUpload &upload = dashboardServer.upload();
  if (upload.status == UPLOAD_FILE_START) {
    otaUploadSucceeded = false;
    otaUploadInProgress = false;
    otaHeaderChecked = false;
    otaUploadedBytes = 0;
    otaUploadError = "";
    String filename = upload.filename;
    filename.toLowerCase();
    if (!otaTokenValid() || !otaMaintenanceReady()) {
      otaUploadError = "Enter maintenance, then check LiPo and recording state";
      return;
    }
    if (!filename.endsWith(".ino.bin")) {
      otaUploadError = "Choose the Feather .ino.bin image, not a bootloader or merged image";
      return;
    }
    if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) {
      otaUploadError = "Cannot start update (error " + String(Update.getError()) + ")";
      return;
    }
    otaUploadInProgress = true;
    Serial.println("[OTA] Receiving firmware. Do not disconnect USB power.");
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (!otaUploadInProgress) return;
    if (!otaHeaderChecked) {
      otaHeaderChecked = true;
      if (!upload.currentSize || upload.buf[0] != 0xE9) {
        abortFirmwareUpload("Invalid ESP32 app header");
        return;
      }
    }
    const esp_partition_t *target = otaTargetPartition();
    if (!target || upload.currentSize > target->size ||
        otaUploadedBytes > target->size - upload.currentSize) {
      abortFirmwareUpload("Firmware image exceeds OTA partition size");
      return;
    }
    const size_t written = Update.write(upload.buf, upload.currentSize);
    if (written != upload.currentSize) {
      const int error = Update.getError();
      abortFirmwareUpload("Firmware flash write failed (error " + String(error) + ")");
      return;
    }
    otaUploadedBytes += written;
  } else if (upload.status == UPLOAD_FILE_END) {
    if (!otaUploadInProgress) return;
    if (otaUploadedBytes < 200000) {
      abortFirmwareUpload("Firmware image is unexpectedly small");
      return;
    }
    if (!Update.end(true)) {
      const int error = Update.getError();
      abortFirmwareUpload("Firmware verification failed (error " + String(error) + ")");
      return;
    }
    otaUploadSucceeded = true;
    otaUploadInProgress = false;
    Serial.printf("[OTA] Verified %lu bytes; rebooting.\n",
                  static_cast<unsigned long>(otaUploadedBytes));
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    abortFirmwareUpload("Upload interrupted; existing firmware retained");
  }
}

static void finishFirmwareUpload() {
  if (!otaUploadSucceeded) {
    otaReply(400, false, otaUploadError.length()
            ? otaUploadError : String("No valid firmware image received"));
    return;
  }
  dashboardServer.sendHeader("Cache-Control", "no-store");
  dashboardServer.send(200, "application/json",
    "{\"success\":true,\"message\":\"Firmware verified; restarting\",\"bytes\":" +
    String(otaUploadedBytes) + "}");
  delay(650);
  ESP.restart();
}

static void rollbackFirmware() {
  if (!otaTokenValid() || !otaMaintenanceReady()) {
    otaReply(403, false, "Enter maintenance and verify battery and recording state");
    return;
  }
  const esp_partition_t *other = otaTargetPartition();
  esp_app_desc_t description = {};
  if (!other || esp_ota_get_partition_description(other, &description) != ESP_OK) {
    otaReply(409, false, "No previous app image is available in the other OTA slot");
    return;
  }
  // ESP-IDF verifies the image as part of selecting the boot partition.
  if (esp_ota_set_boot_partition(other) != ESP_OK) {
    otaReply(500, false, "Alternate image not bootable; rollback cancelled");
    return;
  }
  Serial.println("[OTA] Previous image selected; rebooting.");
  otaReply(200, true, "Previous firmware selected; restarting");
  delay(650);
  ESP.restart();
}
