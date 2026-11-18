#pragma once

#include <cstdint>

namespace green_ion {
namespace recovery_policy {

enum class State : uint8_t {
  LOCKED = 0,
  ACTIVE_RECOVERY,
  HIBERNATING,
  WIFI_CONNECTED,
  BATTERY_BLOCKED,
};

enum class TxBlock : uint8_t {
  NONE = 0,
  SAVE_NOT_VERIFIED,
  JOURNALS_NOT_CLEAR,
  WIFI_CONNECTED,
  BATTERY_UNKNOWN,
  BATTERY_LOW,
  RADIO_UNAVAILABLE,
  TX_NOT_AUTHORIZED,
  POSITION_MISSING,
  POSITION_STALE,
  INTERVAL_NOT_DUE,
};

struct Config {
  uint32_t activeWindowMs;
  uint32_t activeIntervalMs;
  uint32_t hibernateIntervalMs;
  uint32_t freshPositionMaxAgeMs;
  uint16_t minimumBatteryMv;
};

struct Input {
  uint32_t nowMs;
  bool saveVerified;
  bool journalsClear;
  bool wifiConnected;
  bool batteryMeasurementAvailable;
  uint16_t batteryMv;
  bool radioAvailable;
  bool txAuthorized;
  bool hasPosition;
  uint32_t positionAgeMs;
};

struct Decision {
  State state;
  bool txDue;
  TxBlock block;
  uint32_t elapsedSinceSaveMs;
  uint32_t nextDueInMs;
};

class Policy {
 public:
  explicit Policy(const Config &config)
      : config_(config),
        started_(false),
        savedAtMs_(0),
        haveSuccessfulTx_(false),
        lastSuccessfulTxMs_(0) {}

  void reset() {
    started_ = false;
    savedAtMs_ = 0;
    haveSuccessfulTx_ = false;
    lastSuccessfulTxMs_ = 0;
  }

  // Call only after the authoritative recorder has stopped, structural package
  // verification succeeded, and recovery journals were cleared.
  void beginVerifiedSave(uint32_t nowMs) {
    started_ = true;
    savedAtMs_ = nowMs;
    haveSuccessfulTx_ = false;
    lastSuccessfulTxMs_ = 0;
  }

  void noteSuccessfulTransmission(uint32_t nowMs) {
    if (!started_) return;
    haveSuccessfulTx_ = true;
    lastSuccessfulTxMs_ = nowMs;
  }

  bool started() const { return started_; }

  Decision evaluate(const Input &input) const {
    Decision out = {State::LOCKED, false, TxBlock::NONE, 0, 0};

    if (!started_ || !input.saveVerified) {
      out.block = TxBlock::SAVE_NOT_VERIFIED;
      return out;
    }
    if (!input.journalsClear) {
      out.block = TxBlock::JOURNALS_NOT_CLEAR;
      return out;
    }

    out.elapsedSinceSaveMs = elapsed(input.nowMs, savedAtMs_);

    if (input.wifiConnected) {
      out.state = State::WIFI_CONNECTED;
      out.block = TxBlock::WIFI_CONNECTED;
      return out;
    }

    const bool hibernating = out.elapsedSinceSaveMs >= config_.activeWindowMs;
    out.state = hibernating ? State::HIBERNATING : State::ACTIVE_RECOVERY;

    if (!input.batteryMeasurementAvailable) {
      out.state = State::BATTERY_BLOCKED;
      out.block = TxBlock::BATTERY_UNKNOWN;
      return out;
    }
    if (input.batteryMv < config_.minimumBatteryMv) {
      out.state = State::BATTERY_BLOCKED;
      out.block = TxBlock::BATTERY_LOW;
      return out;
    }
    if (!input.radioAvailable) {
      out.block = TxBlock::RADIO_UNAVAILABLE;
      return out;
    }
    if (!input.txAuthorized) {
      out.block = TxBlock::TX_NOT_AUTHORIZED;
      return out;
    }
    if (!input.hasPosition) {
      out.block = TxBlock::POSITION_MISSING;
      return out;
    }
    if (input.positionAgeMs > config_.freshPositionMaxAgeMs) {
      out.block = TxBlock::POSITION_STALE;
      return out;
    }

    const uint32_t intervalMs =
        hibernating ? config_.hibernateIntervalMs : config_.activeIntervalMs;
    const uint32_t anchorMs = haveSuccessfulTx_
        ? lastSuccessfulTxMs_
        : (hibernating ? savedAtMs_ + config_.activeWindowMs : savedAtMs_);
    const uint32_t sinceAnchor = elapsed(input.nowMs, anchorMs);
    if (sinceAnchor < intervalMs) {
      out.block = TxBlock::INTERVAL_NOT_DUE;
      out.nextDueInMs = intervalMs - sinceAnchor;
      return out;
    }

    out.txDue = true;
    out.block = TxBlock::NONE;
    return out;
  }

 private:
  static uint32_t elapsed(uint32_t nowMs, uint32_t thenMs) {
    return static_cast<uint32_t>(nowMs - thenMs);
  }

  Config config_;
  bool started_;
  uint32_t savedAtMs_;
  bool haveSuccessfulTx_;
  uint32_t lastSuccessfulTxMs_;
};

}  // namespace recovery_policy
}  // namespace green_ion
