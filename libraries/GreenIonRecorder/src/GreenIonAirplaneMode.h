#pragma once

#include <math.h>
#include <stdint.h>

namespace green_ion {
namespace airplane {

enum State : uint8_t {
  AIRPLANE_DISABLED = 0,
  AIRPLANE_PREPARING,
  AIRPLANE_READY,
  AIRPLANE_RECORDING,
  AIRPLANE_COMPLETE,
  AIRPLANE_FAULT,
  AIRPLANE_RADIO_SILENCE_LOCKED,
};

enum StartResult : uint8_t {
  START_ALLOWED = 0,
  START_NOT_READY,
  START_SD_UNAVAILABLE,
  START_BAROMETER_UNAVAILABLE,
  START_IMU_UNAVAILABLE,
  START_BATTERY_LOW,
};

struct Readiness {
  bool sdOk;
  bool barometerOk;
  bool imuOk;
  uint16_t batteryMv;
  uint16_t minimumBatteryMv;
};

inline StartResult checkStart(const State state, const Readiness &ready) {
  if (state != AIRPLANE_READY) return START_NOT_READY;
  if (!ready.sdOk) return START_SD_UNAVAILABLE;
  if (!ready.barometerOk) return START_BAROMETER_UNAVAILABLE;
  if (!ready.imuOk) return START_IMU_UNAVAILABLE;
  if (ready.batteryMv < ready.minimumBatteryMv) return START_BATTERY_LOW;
  return START_ALLOWED;
}

inline float cabinPressureAltitudeDeltaM(float pressurePa,
                                         float baselinePressurePa) {
  if (!isfinite(pressurePa) || !isfinite(baselinePressurePa) ||
      pressurePa <= 0.0f || baselinePressurePa <= 0.0f)
    return NAN;
  return 44330.0f *
         (1.0f - powf(pressurePa / baselinePressurePa, 0.190294957f));
}

class Controller {
 public:
  Controller(uint32_t maximumDurationMs)
      : maximumDurationMs_(maximumDurationMs) {}

  bool prepare() {
    if (state_ == AIRPLANE_RECORDING) return false;
    state_ = AIRPLANE_PREPARING;
    return true;
  }

  void calibrationReady(bool ready) {
    if (state_ == AIRPLANE_PREPARING)
      state_ = ready ? AIRPLANE_READY : AIRPLANE_FAULT;
  }

  StartResult start(uint32_t nowMs, const Readiness &ready) {
    const StartResult result = checkStart(state_, ready);
    if (result != START_ALLOWED) return result;
    state_ = AIRPLANE_RECORDING;
    startedMs_ = nowMs;
    radioSilenceLatched_ = true;
    return START_ALLOWED;
  }

  bool automaticStopDue(uint32_t nowMs) const {
    return state_ == AIRPLANE_RECORDING &&
           static_cast<uint32_t>(nowMs - startedMs_) >= maximumDurationMs_;
  }

  bool stop() {
    if (state_ != AIRPLANE_RECORDING) return false;
    state_ = AIRPLANE_COMPLETE;
    return true;
  }

  void fault() { state_ = AIRPLANE_FAULT; }

  void restoreRadioSilence() {
    radioSilenceLatched_ = true;
    if (state_ == AIRPLANE_DISABLED) state_ = AIRPLANE_RADIO_SILENCE_LOCKED;
  }

  bool clearRadioSilenceWhenIdle() {
    if (state_ == AIRPLANE_RECORDING) return false;
    radioSilenceLatched_ = false;
    state_ = AIRPLANE_DISABLED;
    return true;
  }

  State state() const { return state_; }
  bool recording() const { return state_ == AIRPLANE_RECORDING; }
  bool radioSilenceLatched() const { return radioSilenceLatched_; }
  uint32_t elapsedMs(uint32_t nowMs) const {
    return recording() ? static_cast<uint32_t>(nowMs - startedMs_) : 0;
  }
  uint32_t maximumDurationMs() const { return maximumDurationMs_; }

 private:
  uint32_t maximumDurationMs_;
  uint32_t startedMs_ = 0;
  State state_ = AIRPLANE_DISABLED;
  bool radioSilenceLatched_ = false;
};

}  // namespace airplane
}  // namespace green_ion
