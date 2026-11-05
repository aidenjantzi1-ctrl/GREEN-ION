#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "GreenIonLandingWatcher.h"

using namespace green_ion::flight;

static LandingSample sample(uint32_t ms, float altitude, float speed,
                            bool valid = true) {
  return LandingSample{ms, altitude, speed, valid};
}

static void establishDescent(LandingWatcher &watcher, uint32_t start = 0) {
  assert(!watcher.update(sample(start + 1000, 15.0f, 10.0f)));
  assert(!watcher.update(sample(start + 5000, 4.0f, -3.0f)));
}

int main() {
  LandingWatcher stationary;
  assert(stationary.begin(0, LandingConfig{10, 0.0f, 0.0f}));
  for (uint32_t ms = 100; ms <= 40000; ms += 100)
    assert(!stationary.update(sample(ms, 0.2f, 0.0f)));

  LandingWatcher nominal;
  assert(nominal.begin(0, LandingConfig{10, 0.0f, 0.0f}));
  assert(nominal.speedLimitMps() == 0.9f);
  assert(nominal.stableDurationMs() == 14000u);
  establishDescent(nominal);
  assert(!nominal.update(sample(16000, 2.0f, 0.2f)));
  for (uint32_t ms = 16100; ms < 30000; ms += 100)
    assert(!nominal.update(sample(ms, 2.1f, 0.1f)));
  assert(nominal.update(sample(30000, 2.0f, 0.0f)));
  assert(nominal.state() == LANDING_TRIGGERED);

  LandingWatcher noisy;
  assert(noisy.begin(0, LandingConfig{10, 0.0f, 1.0f}));
  assert(noisy.speedLimitMps() == 2.0f);
  assert(noisy.stableDurationMs() == 20000u);

  LandingWatcher invalidReset;
  assert(invalidReset.begin(0, LandingConfig{10, 0.0f, 0.0f}));
  establishDescent(invalidReset);
  assert(!invalidReset.update(sample(16000, 2.0f, 0.0f)));
  assert(!invalidReset.update(sample(20000, NAN, NAN, false)));
  assert(!invalidReset.update(sample(21000, 2.0f, 0.0f)));
  for (uint32_t ms = 21100; ms < 35000; ms += 100)
    assert(!invalidReset.update(sample(ms, 2.0f, 0.0f)));
  assert(invalidReset.update(sample(35000, 2.0f, 0.0f)));

  LandingWatcher anchorReset;
  assert(anchorReset.begin(0, LandingConfig{10, 0.0f, 0.0f}));
  establishDescent(anchorReset);
  assert(!anchorReset.update(sample(16000, 2.0f, 0.0f)));
  for (uint32_t ms = 16100; ms < 20000; ms += 100)
    assert(!anchorReset.update(sample(ms, 2.0f, 0.0f)));
  assert(!anchorReset.update(sample(20000, 4.1f, 0.0f)));
  for (uint32_t ms = 20100; ms < 34000; ms += 100)
    assert(!anchorReset.update(sample(ms, 4.0f, 0.0f)));
  assert(anchorReset.update(sample(34000, 4.0f, 0.0f)));

  LandingWatcher gapReset;
  assert(gapReset.begin(0, LandingConfig{10, 0.0f, 0.0f}));
  establishDescent(gapReset);
  assert(!gapReset.update(sample(16000, 2.0f, 0.0f)));
  assert(!gapReset.update(sample(17000, 2.0f, 0.0f)));  // 1 s exceeds 300 ms.
  for (uint32_t ms = 17100; ms < 31000; ms += 100)
    assert(!gapReset.update(sample(ms, 2.0f, 0.0f)));
  assert(gapReset.update(sample(31000, 2.0f, 0.0f)));

  LandingWatcher rollover;
  const uint32_t start = 0xffffe000u;
  assert(rollover.begin(start, LandingConfig{10, 0.0f, 0.0f}));
  establishDescent(rollover, start);
  assert(!rollover.update(sample(start + 16000u, 2.0f, 0.0f)));
  for (uint32_t elapsed = 16100; elapsed < 30000; elapsed += 100)
    assert(!rollover.update(sample(start + elapsed, 2.0f, 0.0f)));
  assert(rollover.update(sample(start + 30000u, 2.0f, 0.0f)));

  LandingWatcher invalid;
  assert(!invalid.begin(0, LandingConfig{0, 0.0f, 0.0f}));
  assert(!invalid.begin(0, LandingConfig{10, 0.0f, NAN}));

  puts("GREEN ION V12 landing-watcher tests passed");
  return 0;
}
