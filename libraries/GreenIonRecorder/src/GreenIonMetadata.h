#pragma once

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "GreenIonRkt4.h"

namespace green_ion {
namespace metadata {

enum EncodeResult : uint8_t {
  METADATA_VALID = 0,
  METADATA_INVALID_FILENAME,
  METADATA_INVALID_VALUES,
  METADATA_BUFFER_TOO_SMALL,
};

struct FlightStart {
  const char *logFilename;
  uint16_t sampleRateHz;
  float baselinePressurePa;
  float calibrationNoiseM;
  uint16_t startBatteryMv;
  uint16_t prebufferRecords;
};

inline bool validLogFilename(const char *name) {
  if (!name) return false;
  if (name[0] == '/') ++name;
  if (strlen(name) != 11 || memcmp(name, "V12D", 4) != 0 ||
      memcmp(name + 7, ".BIN", 4) != 0) return false;
  return name[4] >= '0' && name[4] <= '9' &&
         name[5] >= '0' && name[5] <= '9' &&
         name[6] >= '0' && name[6] <= '9';
}

inline EncodeResult encodeFlightStart(char *out, size_t capacity,
                                      const FlightStart &metadata,
                                      size_t &written) {
  written = 0;
  if (!validLogFilename(metadata.logFilename)) return METADATA_INVALID_FILENAME;
  if (!rkt4::allowedRate(metadata.sampleRateHz) ||
      !isfinite(metadata.baselinePressurePa) || metadata.baselinePressurePa <= 0 ||
      !isfinite(metadata.calibrationNoiseM) || metadata.calibrationNoiseM < 0 ||
      metadata.startBatteryMv == 0 || metadata.prebufferRecords == 0)
    return METADATA_INVALID_VALUES;
  const char *leaf = metadata.logFilename[0] == '/' ? metadata.logFilename + 1
                                                    : metadata.logFilename;
  const int needed = snprintf(
      out, capacity,
      "GREEN-ION-METADATA-1\n"
      "status=recording\n"
      "file=%s\n"
      "rkt4_version=%u\n"
      "sample_rate_hz=%u\n"
      "baseline_pressure_pa=%.1f\n"
      "calibration_noise_m=%.3f\n"
      "start_battery_mv=%u\n"
      "prebuffer_records=%u\n"
      "checkpoint_format=GIC1\n",
      leaf, static_cast<unsigned int>(rkt4::kVersion),
      static_cast<unsigned int>(metadata.sampleRateHz),
      static_cast<double>(metadata.baselinePressurePa),
      static_cast<double>(metadata.calibrationNoiseM),
      static_cast<unsigned int>(metadata.startBatteryMv),
      static_cast<unsigned int>(metadata.prebufferRecords));
  if (needed < 0 || static_cast<size_t>(needed) >= capacity)
    return METADATA_BUFFER_TOO_SMALL;
  written = static_cast<size_t>(needed);
  return METADATA_VALID;
}

}  // namespace metadata
}  // namespace green_ion
