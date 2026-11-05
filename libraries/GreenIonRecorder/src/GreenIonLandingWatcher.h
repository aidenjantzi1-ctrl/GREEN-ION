#pragma once

#include <math.h>
#include <stdint.h>

namespace green_ion {
namespace flight {

struct LandingConfig {
  uint16_t sampleRateHz;
  float baselineAltitudeM;
  float altitudeNoiseM;
};

struct LandingSample {
  uint32_t timestampMs;
  float altitudeM;
  float verticalSpeedMps;
  bool valid;
};

enum LandingState : uint8_t {
  LANDING_DISABLED = 0,
  LANDING_MONITORING,
  LANDING_TRIGGERED,
};

// Pressure-only recording finalization aid derived from V11. It never controls
// recovery deployment, ignition, staging, or any other energetic hardware.
class LandingWatcher {
 public:
  LandingWatcher()
      : state_(LANDING_DISABLED), flightStartMs_(0), lastSampleMs_(0),
        maximumGapMs_(0), stableSinceMs_(0), peakAltitudeM_(NAN),
        stableAnchorM_(NAN), speedLimitMps_(NAN), stableDurationMs_(0),
        haveLast_(false), haveStable_(false) {}

  bool begin(uint32_t nowMs, const LandingConfig &config) {
    if (config.sampleRateHz == 0 || config.sampleRateHz > 200 ||
        !isfinite(config.baselineAltitudeM) ||
        !isfinite(config.altitudeNoiseM) || config.altitudeNoiseM < 0) {
      state_ = LANDING_DISABLED;
      return false;
    }
    config_ = config;
    flightStartMs_ = nowMs;
    maximumGapMs_ = 3000u / config.sampleRateHz;
    if (maximumGapMs_ < 60u) maximumGapMs_ = 60u;
    speedLimitMps_ = 0.7f + config.altitudeNoiseM * 1.5f;
    if (speedLimitMps_ < 0.9f) speedLimitMps_ = 0.9f;
    if (speedLimitMps_ > 2.0f) speedLimitMps_ = 2.0f;
    float stableMs = 12000.0f + config.altitudeNoiseM * 8000.0f;
    if (stableMs < 14000.0f) stableMs = 14000.0f;
    if (stableMs > 26000.0f) stableMs = 26000.0f;
    stableDurationMs_ = static_cast<uint32_t>(stableMs);
    peakAltitudeM_ = config.baselineAltitudeM;
    stableAnchorM_ = NAN;
    haveLast_ = false;
    haveStable_ = false;
    state_ = LANDING_MONITORING;
    return true;
  }

  void cancel() {
    state_ = LANDING_DISABLED;
    haveLast_ = false;
    haveStable_ = false;
  }

  bool update(const LandingSample &sample) {
    if (state_ == LANDING_TRIGGERED) return true;
    if (state_ != LANDING_MONITORING) return false;
    if (haveLast_ &&
        static_cast<uint32_t>(sample.timestampMs - lastSampleMs_) > maximumGapMs_)
      resetStability();
    lastSampleMs_ = sample.timestampMs;
    haveLast_ = true;
    if (!sample.valid || !isfinite(sample.altitudeM) ||
        !isfinite(sample.verticalSpeedMps)) {
      resetStability();
      return false;
    }
    if (!isfinite(peakAltitudeM_) || sample.altitudeM > peakAltitudeM_)
      peakAltitudeM_ = sample.altitudeM;

    const uint32_t elapsedMs = sample.timestampMs - flightStartMs_;
    const bool descended = peakAltitudeM_ - config_.baselineAltitudeM >= 12.0f &&
                           peakAltitudeM_ - sample.altitudeM >= 8.0f;
    const bool stable = elapsedMs > 15000u && descended &&
                        fabsf(sample.verticalSpeedMps) <= speedLimitMps_;
    if (!stable) {
      resetStability();
      return false;
    }
    if (!haveStable_ || !isfinite(stableAnchorM_) ||
        fabsf(sample.altitudeM - stableAnchorM_) > 2.0f) {
      stableSinceMs_ = sample.timestampMs;
      stableAnchorM_ = sample.altitudeM;
      haveStable_ = true;
      return false;
    }
    if (static_cast<uint32_t>(sample.timestampMs - stableSinceMs_) >=
        stableDurationMs_) {
      state_ = LANDING_TRIGGERED;
      return true;
    }
    return false;
  }

  LandingState state() const { return state_; }
  float peakAltitudeM() const { return peakAltitudeM_; }
  float speedLimitMps() const { return speedLimitMps_; }
  uint32_t stableDurationMs() const { return stableDurationMs_; }

 private:
  void resetStability() {
    haveStable_ = false;
    stableAnchorM_ = NAN;
  }

  LandingConfig config_;
  LandingState state_;
  uint32_t flightStartMs_;
  uint32_t lastSampleMs_;
  uint32_t maximumGapMs_;
  uint32_t stableSinceMs_;
  float peakAltitudeM_;
  float stableAnchorM_;
  float speedLimitMps_;
  uint32_t stableDurationMs_;
  bool haveLast_;
  bool haveStable_;
};

}  // namespace flight
}  // namespace green_ion
