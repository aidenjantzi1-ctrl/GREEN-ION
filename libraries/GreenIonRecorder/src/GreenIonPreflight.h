#pragma once
// Recorder-only preflight advisory. Does not actuate or authorize flight.
#include <stdint.h>
namespace green_ion {
namespace preflight {
enum Flag : uint32_t {
  SD_UNAVAILABLE      = 1UL << 0,
  BARO_UNAVAILABLE    = 1UL << 1,
  BATTERY_LOW         = 1UL << 2,
  CALIBRATION_MISSING = 1UL << 3,
  WATCH_NOT_ACTIVE    = 1UL << 4,
  MAINTENANCE_ACTIVE  = 1UL << 5,
  RECOVERY_PENDING    = 1UL << 6,
  TEMP_CLEANUP        = 1UL << 7,
  IMU_UNAVAILABLE     = 1UL << 8,
  GPS_NOT_CURRENT     = 1UL << 9,
  RADIO_UNAVAILABLE   = 1UL << 10,
  MISSED_SAMPLES      = 1UL << 11,
  SECONDARY_ERRORS    = 1UL << 12,
  CALIBRATION_OLD     = 1UL << 13,
  PROFILE_UNSET       = 1UL << 14,
};
static constexpr uint32_t kBlockingMask =
    SD_UNAVAILABLE | BARO_UNAVAILABLE | BATTERY_LOW | CALIBRATION_MISSING |
    WATCH_NOT_ACTIVE | MAINTENANCE_ACTIVE | RECOVERY_PENDING | TEMP_CLEANUP;
struct Input {
  bool sd, barometer, calibrated, watcher, maintenance, recovery, tempCleanup;
  bool imu, gpsCurrent, radio, missed, secondaryErrors, calibrationOld, profileSet;
  uint16_t batteryMv;
};
struct Result {
  uint32_t flags;
  const char *label;
};
inline Result evaluate(const Input &in) {
  uint32_t mask = 0;
  if (!in.sd) mask |= SD_UNAVAILABLE;
  if (!in.barometer) mask |= BARO_UNAVAILABLE;
  if (in.batteryMv < 3600U) mask |= BATTERY_LOW;
  if (!in.calibrated) mask |= CALIBRATION_MISSING;
  if (!in.watcher) mask |= WATCH_NOT_ACTIVE;
  if (in.maintenance) mask |= MAINTENANCE_ACTIVE;
  if (in.recovery) mask |= RECOVERY_PENDING;
  if (in.tempCleanup) mask |= TEMP_CLEANUP;
  if (!in.imu) mask |= IMU_UNAVAILABLE;
  if (!in.gpsCurrent) mask |= GPS_NOT_CURRENT;
  if (!in.radio) mask |= RADIO_UNAVAILABLE;
  if (in.missed) mask |= MISSED_SAMPLES;
  if (in.secondaryErrors) mask |= SECONDARY_ERRORS;
  if (in.calibrationOld) mask |= CALIBRATION_OLD;
  if (!in.profileSet) mask |= PROFILE_UNSET;
  return {mask, (mask & kBlockingMask) ? "BLOCKED" :
                mask ? "CHECK_OPTIONAL" : "READY_FOR_REVIEW"};
}
}  // namespace preflight
}  // namespace green_ion
