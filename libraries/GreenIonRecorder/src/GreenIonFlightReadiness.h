#pragma once
#include <stdint.h>

namespace green_ion {
namespace readiness {

enum CriticalFlag : uint32_t {
  CRITICAL_NONE            = 0,
  CRITICAL_SD              = 1UL << 0,
  CRITICAL_BAROMETER       = 1UL << 1,
  CRITICAL_IMU             = 1UL << 2,
  CRITICAL_BATTERY         = 1UL << 3,
  CRITICAL_RECOVERY        = 1UL << 4,
  CRITICAL_TEMP_CLEANUP    = 1UL << 5,
  CRITICAL_SD_INTEGRITY    = 1UL << 6,
  CRITICAL_CALIBRATION     = 1UL << 7,
  CRITICAL_LAUNCH_SIM      = 1UL << 8,
  CRITICAL_WATCHER         = 1UL << 9,
};

enum WarningFlag : uint32_t {
  WARNING_NONE             = 0,
  WARNING_GPS              = 1UL << 0,
  WARNING_LORA             = 1UL << 1,
  WARNING_SECONDARY_LOG    = 1UL << 2,
  WARNING_MISSED_SAMPLES   = 1UL << 3,
  WARNING_PROFILE          = 1UL << 4,
};

struct Input {
  bool sd;
  bool barometer;
  bool imu;
  uint16_t batteryMv;
  bool recoveryClear;
  bool tempCleanupClear;
  bool sdIntegrity;
  bool calibrated;
  bool launchSimulation;
  bool watcherActive;
  bool gps;
  bool radio;
  bool secondaryLogHealthy;
  bool noMissedSamples;
  bool profileSet;
};

struct Result {
  uint32_t criticalFlags;
  uint32_t warningFlags;
  bool ready;
};

inline Result evaluate(const Input &in) {
  uint32_t critical = 0;
  uint32_t warnings = 0;
  if (!in.sd) critical |= CRITICAL_SD;
  if (!in.barometer) critical |= CRITICAL_BAROMETER;
  if (!in.imu) critical |= CRITICAL_IMU;
  if (in.batteryMv < 3600U) critical |= CRITICAL_BATTERY;
  if (!in.recoveryClear) critical |= CRITICAL_RECOVERY;
  if (!in.tempCleanupClear) critical |= CRITICAL_TEMP_CLEANUP;
  if (!in.sdIntegrity) critical |= CRITICAL_SD_INTEGRITY;
  if (!in.calibrated) critical |= CRITICAL_CALIBRATION;
  if (!in.launchSimulation) critical |= CRITICAL_LAUNCH_SIM;
  if (!in.watcherActive) critical |= CRITICAL_WATCHER;

  // GPS and LoRa are recovery/telemetry aids. They must never block local
  // 50 Hz microSD recording or the pressure-based launch detector.
  if (!in.gps) warnings |= WARNING_GPS;
  if (!in.radio) warnings |= WARNING_LORA;
  if (!in.secondaryLogHealthy) warnings |= WARNING_SECONDARY_LOG;
  if (!in.noMissedSamples) warnings |= WARNING_MISSED_SAMPLES;
  if (!in.profileSet) warnings |= WARNING_PROFILE;
  return {critical, warnings, critical == 0};
}

}  // namespace readiness
}  // namespace green_ion
