#pragma once

#include <math.h>
#include <stdint.h>

namespace green_ion {

// The GPS parser owns the timestamp of its last positive fix. A later parsed
// sentence must not reset the recorder's position age to zero.
inline bool gpsFixTimestamp(uint32_t nowMs, double parserAgeSeconds,
                            uint32_t currentLimitMs, uint32_t *fixMs) {
  if (!fixMs || !isfinite(parserAgeSeconds) || parserAgeSeconds < 0.0) return false;
  const double ageMs = parserAgeSeconds * 1000.0;
  if (!isfinite(ageMs) || ageMs > currentLimitMs) return false;
  *fixMs = nowMs - static_cast<uint32_t>(ageMs);
  return true;
}

}  // namespace green_ion
