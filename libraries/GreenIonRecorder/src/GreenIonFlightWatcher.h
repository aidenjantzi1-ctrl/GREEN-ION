#pragma once

#include <math.h>
#include <stdint.h>

namespace green_ion {
namespace flight {

struct WatchConfig {
  uint16_t sampleRateHz;
  float baselineAltitudeM;
  float baselinePressurePa;
  float altitudeNoiseM;
};

struct WatchSample {
  uint32_t timestampMs;
  float altitudeM;
  float verticalSpeedMps;
  float pressurePa;
  bool valid;
};

enum WatchState : uint8_t {
  WATCH_DISABLED = 0,
  WATCHING,
  LAUNCH_TRIGGERED,
};

// Portable launch detector derived from the V11 recorder thresholds. It only
// decides when recording should start; it cannot control energetic hardware.
class FlightWatcher {
 public:
  FlightWatcher()
      : state_(WATCH_DISABLED), confirmations_(0), requiredConfirmations_(0),
        requiredGainM_(0), maximumGapMs_(0), lastSampleMs_(0), haveLast_(false) {}

  bool begin(const WatchConfig &config) {
    if (config.sampleRateHz == 0 || config.sampleRateHz > 200 ||
        !isfinite(config.baselineAltitudeM) ||
        !isfinite(config.baselinePressurePa) || config.baselinePressurePa <= 0 ||
        !isfinite(config.altitudeNoiseM) || config.altitudeNoiseM < 0) {
      state_ = WATCH_DISABLED;
      return false;
    }
    config_ = config;
    requiredConfirmations_ = config.sampleRateHz / 10;
    if (requiredConfirmations_ < 2) requiredConfirmations_ = 2;
    requiredGainM_ = fmaxf(1.5f, config.altitudeNoiseM * 5.0f);
    maximumGapMs_ = 3000u / config.sampleRateHz;
    if (maximumGapMs_ < 60u) maximumGapMs_ = 60u;
    confirmations_ = 0;
    haveLast_ = false;
    state_ = WATCHING;
    return true;
  }

  void cancel() {
    state_ = WATCH_DISABLED;
    confirmations_ = 0;
    haveLast_ = false;
  }

  bool update(const WatchSample &sample) {
    if (state_ == LAUNCH_TRIGGERED) return true;
    if (state_ != WATCHING) return false;
    if (!sample.valid || !isfinite(sample.altitudeM) ||
        !isfinite(sample.verticalSpeedMps) || !isfinite(sample.pressurePa)) {
      resetConfirmation();
      return false;
    }
    if (haveLast_ && static_cast<uint32_t>(sample.timestampMs - lastSampleMs_) > maximumGapMs_)
      confirmations_ = 0;
    lastSampleMs_ = sample.timestampMs;
    haveLast_ = true;

    const float gainM = sample.altitudeM - config_.baselineAltitudeM;
    const bool rising = gainM >= requiredGainM_ &&
                        sample.verticalSpeedMps >= 4.0f &&
                        sample.pressurePa < config_.baselinePressurePa;
    confirmations_ = rising ? static_cast<uint8_t>(confirmations_ + 1) : 0;
    if (confirmations_ >= requiredConfirmations_) {
      state_ = LAUNCH_TRIGGERED;
      return true;
    }
    return false;
  }

  WatchState state() const { return state_; }
  uint8_t confirmations() const { return confirmations_; }
  uint8_t requiredConfirmations() const { return requiredConfirmations_; }
  float requiredGainM() const { return requiredGainM_; }

 private:
  void resetConfirmation() {
    confirmations_ = 0;
    haveLast_ = false;
  }

  WatchConfig config_;
  WatchState state_;
  uint8_t confirmations_;
  uint8_t requiredConfirmations_;
  float requiredGainM_;
  uint32_t maximumGapMs_;
  uint32_t lastSampleMs_;
  bool haveLast_;
};

}  // namespace flight
}  // namespace green_ion
