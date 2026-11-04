#include <assert.h>
#include <stdio.h>

#include "GreenIonRadioRecovery.h"

int main() {
  green_ion::RadioRecovery recovery(10000, 3);
  assert(recovery.valid());
  assert(recovery.begin(1000, false));
  assert(!recovery.ready() && recovery.initFailures() == 1);
  assert(!recovery.shouldAttempt(10999, false));
  assert(!recovery.shouldAttempt(11000, true));
  assert(recovery.shouldAttempt(11000, false));
  recovery.noteInitialize(11000, true);
  assert(recovery.ready() && recovery.recoveries() == 1);

  assert(!recovery.noteSendResult(12000, false));
  assert(!recovery.noteSendResult(13000, true));
  assert(recovery.consecutiveSendFailures() == 0);
  assert(!recovery.noteSendResult(14000, false));
  assert(!recovery.noteSendResult(15000, false));
  assert(recovery.noteSendResult(16000, false));
  assert(!recovery.ready());
  assert(!recovery.shouldAttempt(26000, true));
  assert(recovery.shouldAttempt(26000, false));
  recovery.noteInitialize(26000, false);
  assert(!recovery.ready() && recovery.initFailures() == 2);
  assert(!recovery.shouldAttempt(35999, false));
  assert(recovery.shouldAttempt(36000, false));

  const uint32_t nearWrap = 0xfffffff0u;
  green_ion::RadioRecovery rollover(100, 1);
  assert(rollover.begin(nearWrap, true));
  assert(rollover.noteSendResult(nearWrap, false));
  assert(!rollover.shouldAttempt(nearWrap + 99u, false));
  assert(rollover.shouldAttempt(nearWrap + 100u, false));

  green_ion::RadioRecovery invalidDelay(0, 1);
  green_ion::RadioRecovery invalidThreshold(1000, 0);
  assert(!invalidDelay.valid() && !invalidDelay.begin(0, false));
  assert(!invalidThreshold.valid() && !invalidThreshold.begin(0, false));

  puts("GREEN ION V12 radio-recovery tests passed");
  return 0;
}
