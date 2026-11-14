#pragma once

#include <stddef.h>
#include <stdint.h>

namespace green_ion {
namespace wifi_policy {

inline bool validSsidLength(size_t length) {
  return length >= 1 && length <= 32;
}

inline bool validWifiPassphraseLength(size_t length) {
  return length == 0 || (length >= 8 && length <= 63);
}

inline bool validApPassphraseLength(size_t length) {
  return length >= 8 && length <= 63;
}

inline bool networkChangeBlocked(bool recording,
                                 bool launchWatching,
                                 bool homeTest,
                                 bool maintenance,
                                 bool updateInProgress) {
  return recording || launchWatching || homeTest || maintenance || updateInProgress;
}

inline bool shouldUseApSta(bool homeConfigured, bool stationPaused) {
  return homeConfigured && !stationPaused;
}

inline bool shouldRetryStation(bool homeConfigured,
                               bool stationPaused,
                               bool stationConnected,
                               bool attemptInProgress,
                               bool criticalFlightState,
                               uint32_t nowMs,
                               uint32_t retryAtMs) {
  return homeConfigured && !stationPaused && !stationConnected &&
         !attemptInProgress && !criticalFlightState &&
         static_cast<int32_t>(nowMs - retryAtMs) >= 0;
}

}  // namespace wifi_policy
}  // namespace green_ion
