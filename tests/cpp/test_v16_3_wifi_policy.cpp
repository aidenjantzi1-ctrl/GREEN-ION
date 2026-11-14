#include <cassert>
#include <cstdint>

#include "GreenIonWifiPolicy.h"

int main() {
  using namespace green_ion::wifi_policy;

  // AP-only when no home network exists.
  assert(!shouldUseApSta(false, false));
  assert(shouldUseApSta(true, false));
  assert(!shouldUseApSta(true, true));

  // WPA passphrases may be empty only for an open network; AP password is never open.
  assert(validSsidLength(1));
  assert(validSsidLength(32));
  assert(!validSsidLength(0));
  assert(!validSsidLength(33));
  assert(validWifiPassphraseLength(0));
  assert(validWifiPassphraseLength(8));
  assert(validWifiPassphraseLength(63));
  assert(!validWifiPassphraseLength(7));
  assert(!validWifiPassphraseLength(64));
  assert(validApPassphraseLength(8));
  assert(!validApPassphraseLength(0));

  // Network configuration is deliberately frozen during recorder-critical states.
  assert(networkChangeBlocked(true, false, false, false, false));
  assert(networkChangeBlocked(false, true, false, false, false));
  assert(networkChangeBlocked(false, false, true, false, false));
  assert(networkChangeBlocked(false, false, false, true, false));
  assert(networkChangeBlocked(false, false, false, false, true));
  assert(!networkChangeBlocked(false, false, false, false, false));

  // Reconnect is bounded by a retry deadline and never runs in a critical flight state.
  assert(!shouldRetryStation(true, false, false, false, true, 1000, 0));
  assert(!shouldRetryStation(true, false, false, true, false, 1000, 0));
  assert(!shouldRetryStation(true, false, true, false, false, 1000, 0));
  assert(!shouldRetryStation(true, true, false, false, false, 1000, 0));
  assert(!shouldRetryStation(false, false, false, false, false, 1000, 0));
  assert(!shouldRetryStation(true, false, false, false, false, 999, 1000));
  assert(shouldRetryStation(true, false, false, false, false, 1000, 1000));
  assert(shouldRetryStation(true, false, false, false, false, 2000, 1000));

  return 0;
}
