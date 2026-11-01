#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "GreenIonFlightWatcher.h"

using green_ion::flight::FlightWatcher;
using green_ion::flight::LAUNCH_TRIGGERED;
using green_ion::flight::WATCH_DISABLED;
using green_ion::flight::WATCHING;
using green_ion::flight::WatchConfig;
using green_ion::flight::WatchSample;

static WatchSample point(uint32_t timeMs, float altitudeM, float speedMps,
                         float pressurePa = 101300.0f, bool valid = true) {
  return WatchSample{timeMs, altitudeM, speedMps, pressurePa, valid};
}

int main() {
  FlightWatcher watcher;
  assert(!watcher.begin(WatchConfig{0, 0, 101325, 0.2f}));
  assert(watcher.state() == WATCH_DISABLED);
  assert(watcher.begin(WatchConfig{50, 0, 101325, 0.2f}));
  assert(watcher.state() == WATCHING);
  assert(watcher.requiredConfirmations() == 5);
  assert(watcher.requiredGainM() == 1.5f);

  for (uint32_t i = 0; i < 100; ++i)
    assert(!watcher.update(point(i * 20, (static_cast<int>(i % 5) - 2) * 0.05f, 0.2f)));

  assert(!watcher.update(point(2020, 2.0f, 5.0f)));
  assert(watcher.confirmations() == 1);
  assert(!watcher.update(point(2040, 0.1f, 0.1f)));
  assert(watcher.confirmations() == 0);

  for (uint8_t i = 0; i < 4; ++i)
    assert(!watcher.update(point(2060 + i * 20, 2.0f + i, 5.0f)));
  assert(watcher.update(point(2140, 6.0f, 8.0f)));
  assert(watcher.state() == LAUNCH_TRIGGERED);
  assert(watcher.update(point(2160, 0.0f, 0.0f)));

  FlightWatcher invalidReset;
  assert(invalidReset.begin(WatchConfig{10, 100, 90000, 0.4f}));
  assert(invalidReset.requiredConfirmations() == 2);
  assert(invalidReset.requiredGainM() == 2.0f);
  assert(!invalidReset.update(point(0xfffffff0u, 103, 5, 89900)));
  assert(!invalidReset.update(point(0x00000004u, 103, 5, 89900, false)));
  assert(invalidReset.confirmations() == 0);
  assert(!invalidReset.update(point(0x00000018u, 103, 5, 89900)));
  assert(invalidReset.update(point(0x0000002cu, 103.5f, 6, 89800)));

  FlightWatcher gapReset;
  assert(gapReset.begin(WatchConfig{50, 0, 101325, 0.1f}));
  for (uint8_t i = 0; i < 4; ++i)
    assert(!gapReset.update(point(i * 20, 3, 5)));
  assert(!gapReset.update(point(1000, 3, 5)));
  assert(gapReset.confirmations() == 1);

  watcher.cancel();
  assert(watcher.state() == WATCH_DISABLED);
  puts("GREEN ION V12 flight watcher tests passed");
  return 0;
}
