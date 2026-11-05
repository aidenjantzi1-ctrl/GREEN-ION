#include <assert.h>
#include <stdio.h>

#include "GreenIonSampleClock.h"

using green_ion::recorder::SampleClock;

int main() {
  SampleClock clock(20000);
  assert(clock.valid());
  assert(!clock.due(0));
  assert(clock.start(1000));
  assert(!clock.due(20999));
  assert(clock.due(21000));
  assert(clock.nextDeadlineUs() == 41000);
  assert(clock.missedPeriods() == 0);

  // A slightly late loop advances from the prior deadline, not from now.
  assert(clock.due(41500));
  assert(clock.nextDeadlineUs() == 61000);
  assert(clock.missedPeriods() == 0);

  // At the third outstanding deadline, one sample is taken and two are missed.
  assert(clock.due(101000));
  assert(clock.nextDeadlineUs() == 121000);
  assert(clock.missedPeriods() == 2);

  SampleClock rollover(20);
  assert(rollover.start(0xfffffff0u));
  assert(rollover.nextDeadlineUs() == 4u);
  assert(!rollover.due(3u));
  assert(rollover.due(4u));
  assert(rollover.nextDeadlineUs() == 24u);

  SampleClock invalidZero(0);
  assert(!invalidZero.valid());
  assert(!invalidZero.start(0));
  SampleClock invalidHalfRange(0x80000000u);
  assert(!invalidHalfRange.valid());

  puts("GREEN ION V12 sample-clock tests passed");
  return 0;
}
