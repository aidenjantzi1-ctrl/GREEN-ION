#include <assert.h>
#include <stdint.h>

#include "GreenIonSensorHealth.h"

int main() {
  green_ion::SensorHealth health(5000, 3);
  assert(health.valid());
  assert(health.begin(100, false));
  assert(!health.ready());
  assert(health.initFailures() == 1);
  assert(!health.shouldAttempt(5099, false));
  assert(!health.shouldAttempt(5100, true));
  assert(health.shouldAttempt(5100, false));

  health.noteInitialize(5100, false);
  assert(health.initFailures() == 2);
  assert(!health.shouldAttempt(10099, false));
  assert(health.shouldAttempt(10100, false));
  health.noteInitialize(10100, true);
  assert(health.ready());
  assert(health.recoveries() == 1);

  assert(!health.noteRead(10200, false));
  assert(health.consecutiveReadFailures() == 1);
  assert(!health.noteRead(10300, true));
  assert(health.consecutiveReadFailures() == 0);
  assert(!health.noteRead(10400, false));
  assert(!health.noteRead(10500, false));
  assert(health.noteRead(10600, false));
  assert(!health.ready());
  assert(health.runtimeLosses() == 1);
  assert(!health.shouldAttempt(15600, true));
  assert(health.shouldAttempt(15600, false));

  green_ion::SensorHealth rollover(100, 1);
  assert(rollover.begin(UINT32_MAX - 49, true));
  assert(rollover.noteRead(UINT32_MAX - 39, false));
  assert(!rollover.shouldAttempt(49, false));
  assert(rollover.shouldAttempt(60, false));

  green_ion::SensorHealth invalidDelay(0, 1);
  green_ion::SensorHealth invalidThreshold(1000, 0);
  assert(!invalidDelay.valid());
  assert(!invalidThreshold.valid());
  assert(!invalidDelay.begin(0, true));
  assert(!invalidThreshold.begin(0, true));
  return 0;
}
