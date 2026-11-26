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

// V18 flight-power policy is deliberately separate from Wi-Fi configuration.
// Configuration stays frozen in critical states; this only decides whether the
// already-running radio may be powered down after the SD recorder has started.
inline bool shouldDisableForFlight(bool recording,
                                   bool suppressionRequested,
                                   bool alreadySuppressed,
                                   uint32_t nowMs,
                                   uint32_t requestedAtMs,
                                   uint32_t settleDelayMs) {
  return recording && suppressionRequested && !alreadySuppressed &&
         static_cast<uint32_t>(nowMs - requestedAtMs) >= settleDelayMs;
}

// Wi-Fi may return only after the primary recorder has stopped and the completed
// flight package has independently passed SD readback verification.
inline bool shouldRestoreAfterVerifiedSave(bool recording,
                                           bool saveVerified,
                                           bool alreadySuppressed) {
  return !recording && saveVerified && alreadySuppressed;
}

}  // namespace wifi_policy
}  // namespace green_ion
