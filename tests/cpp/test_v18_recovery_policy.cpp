#include <cassert>
#include <cstdint>

#include "GreenIonRecoveryPolicy.h"

using green_ion::recovery_policy::Config;
using green_ion::recovery_policy::Decision;
using green_ion::recovery_policy::Input;
using green_ion::recovery_policy::Policy;
using green_ion::recovery_policy::State;
using green_ion::recovery_policy::TxBlock;

static Config config() {
  return {
    300000UL,  // five-minute active recovery window
    1000UL,    // active recovery opportunity once per second
    180000UL,  // hibernation opportunity every three minutes
    30000UL,   // only transmit a position no older than 30 seconds
    3500,      // conservative existing bench threshold; hardware validation required
  };
}

static Input eligible(uint32_t nowMs) {
  return {
    nowMs,
    true,   // verified save
    true,   // journals clear
    false,  // Wi-Fi not reconnected
    true,   // battery measurement available
    3900,
    true,   // radio initialized
    true,   // antenna/TX authorization explicitly granted
    true,   // GPS/last-known position available
    1000,
  };
}

static void assertBlocked(const Decision &d, TxBlock block) {
  assert(!d.txDue);
  assert(d.block == block);
}

int main() {
  Policy policy(config());

  // Recovery stays locked before independently verified save/journal cleanup.
  assertBlocked(policy.evaluate(eligible(1000)), TxBlock::SAVE_NOT_VERIFIED);
  policy.beginVerifiedSave(1000);
  Input input = eligible(1000);
  input.saveVerified = false;
  assertBlocked(policy.evaluate(input), TxBlock::SAVE_NOT_VERIFIED);
  input = eligible(1000);
  input.journalsClear = false;
  assertBlocked(policy.evaluate(input), TxBlock::JOURNALS_NOT_CLEAR);

  // Active recovery begins only after save verification and is non-blocking.
  input = eligible(1999);
  Decision d = policy.evaluate(input);
  assert(d.state == State::ACTIVE_RECOVERY);
  assertBlocked(d, TxBlock::INTERVAL_NOT_DUE);
  assert(d.nextDueInMs == 1);
  input.nowMs = 2000;
  d = policy.evaluate(input);
  assert(d.state == State::ACTIVE_RECOVERY);
  assert(d.txDue);
  policy.noteSuccessfulTransmission(input.nowMs);
  input.nowMs = 2999;
  assertBlocked(policy.evaluate(input), TxBlock::INTERVAL_NOT_DUE);
  input.nowMs = 3000;
  assert(policy.evaluate(input).txDue);

  // Exactly five minutes after verified save the policy moves to hibernating.
  input = eligible(300999);
  assert(policy.evaluate(input).state == State::ACTIVE_RECOVERY);
  input.nowMs = 301000;
  d = policy.evaluate(input);
  assert(d.state == State::HIBERNATING);

  // Last successful active TX controls the first hibernation due time.
  policy.noteSuccessfulTransmission(300000);
  input.nowMs = 479999;
  d = policy.evaluate(input);
  assert(d.state == State::HIBERNATING);
  assertBlocked(d, TxBlock::INTERVAL_NOT_DUE);
  assert(d.nextDueInMs == 1);
  input.nowMs = 480000;
  assert(policy.evaluate(input).txDue);

  // No successful active TX: first hibernation opportunity is three minutes
  // after hibernation begins, not immediately at the five-minute boundary.
  Policy quiet(config());
  quiet.beginVerifiedSave(5000);
  input = eligible(305000 + 179999);
  assertBlocked(quiet.evaluate(input), TxBlock::INTERVAL_NOT_DUE);
  input.nowMs = 305000 + 180000;
  d = quiet.evaluate(input);
  assert(d.state == State::HIBERNATING);
  assert(d.txDue);

  // millis() wrap is handled using unsigned elapsed arithmetic.
  Policy wrap(config());
  wrap.beginVerifiedSave(0xfffffff0u);
  input = eligible(0x000003d8u);  // 1000 ms after 0xfffffff0
  d = wrap.evaluate(input);
  assert(d.state == State::ACTIVE_RECOVERY);
  assert(d.txDue);

  // Missing/stale GPS never fabricates a recoverable position.
  input = eligible(500000);
  input.hasPosition = false;
  assertBlocked(policy.evaluate(input), TxBlock::POSITION_MISSING);
  input = eligible(500000);
  input.positionAgeMs = 30001;
  assertBlocked(policy.evaluate(input), TxBlock::POSITION_STALE);

  // Lost radio and unknown antenna authorization prevent all TX.
  input = eligible(500000);
  input.radioAvailable = false;
  assertBlocked(policy.evaluate(input), TxBlock::RADIO_UNAVAILABLE);
  input = eligible(500000);
  input.txAuthorized = false;
  assertBlocked(policy.evaluate(input), TxBlock::TX_NOT_AUTHORIZED);

  // Battery status is explicit; unknown or below existing 3.5 V bench threshold
  // prevents recovery transmission rather than guessing remaining endurance.
  input = eligible(500000);
  input.batteryMeasurementAvailable = false;
  d = policy.evaluate(input);
  assert(d.state == State::BATTERY_BLOCKED);
  assertBlocked(d, TxBlock::BATTERY_UNKNOWN);
  input = eligible(500000);
  input.batteryMv = 3499;
  d = policy.evaluate(input);
  assert(d.state == State::BATTERY_BLOCKED);
  assertBlocked(d, TxBlock::BATTERY_LOW);

  // Wi-Fi reconnect disarms hibernation TX without altering save evidence.
  input = eligible(500000);
  input.wifiConnected = true;
  d = policy.evaluate(input);
  assert(d.state == State::WIFI_CONNECTED);
  assertBlocked(d, TxBlock::WIFI_CONNECTED);

  // A new flight/reboot reset cannot inherit a prior flight's recovery timer.
  policy.reset();
  assertBlocked(policy.evaluate(eligible(600000)), TxBlock::SAVE_NOT_VERIFIED);
  policy.beginVerifiedSave(600000);
  input = eligible(600999);
  assertBlocked(policy.evaluate(input), TxBlock::INTERVAL_NOT_DUE);
  input.nowMs = 601000;
  assert(policy.evaluate(input).txDue);

  // Failed save on a second flight cannot inherit the previous flight's
  // successful recovery state.
  policy.reset();
  input = eligible(700000);
  input.saveVerified = false;
  assertBlocked(policy.evaluate(input), TxBlock::SAVE_NOT_VERIFIED);

  return 0;
}
