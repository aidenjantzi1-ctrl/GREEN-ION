#pragma once

#include <math.h>
#include <stdint.h>

namespace green_ion {
namespace bench {

enum Result : uint8_t {
  RESULT_IDLE = 0,
  RESULT_RUNNING,
  RESULT_PASS,
  RESULT_FAIL,
  RESULT_INCONCLUSIVE,
  RESULT_DEFERRED,
};

class ImuTest {
 public:
  ImuTest(uint32_t durationMs, uint16_t minimumSamples)
      : durationMs_(durationMs), minimumSamples_(minimumSamples) { reset(); }

  bool valid() const { return durationMs_ > 0 && minimumSamples_ > 0; }

  void begin(uint32_t nowMs, bool detected, bool range32g,
             uint32_t durationOverrideMs = 0, uint16_t minimumOverride = 0) {
    reset();
    startedMs_ = nowMs;
    activeDurationMs_ = durationOverrideMs ? durationOverrideMs : durationMs_;
    activeMinimumSamples_ = minimumOverride ? minimumOverride : minimumSamples_;
    detected_ = detected;
    range32g_ = range32g;
    result_ = detected && range32g && valid() && activeDurationMs_ <= 60000u &&
              activeMinimumSamples_ > 0 ? RESULT_RUNNING : RESULT_FAIL;
  }

  void defer() { reset(); result_ = RESULT_DEFERRED; }

  void observe(uint32_t nowMs, bool sampleValid, float accelerationG,
               float gyroXDps, float gyroYDps, float gyroZDps,
               float temperatureC) {
    if (result_ != RESULT_RUNNING) return;
    if (sampleValid && isfinite(accelerationG) && isfinite(gyroXDps) &&
        isfinite(gyroYDps) && isfinite(gyroZDps) && isfinite(temperatureC)) {
      ++validSamples_;
      sumAccelerationG_ += accelerationG;
      const float deviation = fabsf(accelerationG - 1.0f);
      if (deviation > peakAccelerationDeviationG_)
        peakAccelerationDeviationG_ = deviation;
      sumGyroXDps_ += gyroXDps;
      sumGyroYDps_ += gyroYDps;
      sumGyroZDps_ += gyroZDps;
      sumTemperatureC_ += temperatureC;
      const float peak = fmaxf(fabsf(gyroXDps),
                               fmaxf(fabsf(gyroYDps), fabsf(gyroZDps)));
      if (peak > peakGyroDps_) peakGyroDps_ = peak;
    } else {
      ++invalidSamples_;
    }
    if (static_cast<int32_t>(nowMs - (startedMs_ + activeDurationMs_)) < 0) return;
    finish();
  }

  Result result() const { return result_; }
  bool running() const { return result_ == RESULT_RUNNING; }
  bool detected() const { return detected_; }
  bool range32g() const { return range32g_; }
  uint16_t validSamples() const { return validSamples_; }
  uint16_t invalidSamples() const { return invalidSamples_; }
  float meanAccelerationG() const { return mean(sumAccelerationG_); }
  float gyroBiasXDps() const { return mean(sumGyroXDps_); }
  float gyroBiasYDps() const { return mean(sumGyroYDps_); }
  float gyroBiasZDps() const { return mean(sumGyroZDps_); }
  float meanTemperatureC() const { return mean(sumTemperatureC_); }
  float peakGyroDps() const { return peakGyroDps_; }
  float peakAccelerationDeviationG() const { return peakAccelerationDeviationG_; }
  uint32_t durationMs() const { return activeDurationMs_; }

 private:
  float mean(double sum) const {
    return validSamples_ ? static_cast<float>(sum / validSamples_) : NAN;
  }

  void finish() {
    if (validSamples_ < activeMinimumSamples_) {
      result_ = RESULT_FAIL;
      return;
    }
    const float g = meanAccelerationG();
    const bool stationary = g >= 0.85f && g <= 1.15f &&
                            peakAccelerationDeviationG_ <= 0.25f &&
                            fabsf(gyroBiasXDps()) <= 3.0f &&
                            fabsf(gyroBiasYDps()) <= 3.0f &&
                            fabsf(gyroBiasZDps()) <= 3.0f &&
                            peakGyroDps_ <= 20.0f;
    result_ = stationary ? RESULT_PASS : RESULT_INCONCLUSIVE;
  }

  void reset() {
    result_ = RESULT_IDLE;
    startedMs_ = 0;
    detected_ = false;
    range32g_ = false;
    validSamples_ = 0;
    invalidSamples_ = 0;
    sumAccelerationG_ = 0;
    sumGyroXDps_ = sumGyroYDps_ = sumGyroZDps_ = 0;
    sumTemperatureC_ = 0;
    peakGyroDps_ = 0;
    peakAccelerationDeviationG_ = 0;
    activeDurationMs_ = durationMs_;
    activeMinimumSamples_ = minimumSamples_;
  }

  uint32_t durationMs_;
  uint16_t minimumSamples_;
  uint32_t activeDurationMs_;
  uint16_t activeMinimumSamples_;
  Result result_;
  uint32_t startedMs_;
  bool detected_;
  bool range32g_;
  uint16_t validSamples_;
  uint16_t invalidSamples_;
  double sumAccelerationG_;
  double sumGyroXDps_;
  double sumGyroYDps_;
  double sumGyroZDps_;
  double sumTemperatureC_;
  float peakGyroDps_;
  float peakAccelerationDeviationG_;
};

class GpsTest {
 public:
  explicit GpsTest(uint32_t durationMs) : durationMs_(durationMs) { reset(); }

  bool valid() const { return durationMs_ > 0; }

  void begin(uint32_t nowMs, bool detected, uint32_t nmeaCount,
             uint32_t parsedCount) {
    reset();
    startedMs_ = nowMs;
    detected_ = detected;
    startNmeaCount_ = nmeaCount;
    startParsedCount_ = parsedCount;
    result_ = detected && valid() ? RESULT_RUNNING : RESULT_FAIL;
  }

  void defer() { reset(); result_ = RESULT_DEFERRED; }

  void update(uint32_t nowMs, uint32_t nmeaCount, uint32_t parsedCount,
              bool currentFix, uint8_t satellites, uint32_t fixAgeMs) {
    if (result_ != RESULT_RUNNING) return;
    nmeaReceived_ = nmeaCount - startNmeaCount_;
    parsedSentences_ = parsedCount - startParsedCount_;
    currentFix_ = currentFix;
    satellites_ = satellites;
    fixAgeMs_ = fixAgeMs;
    if (static_cast<int32_t>(nowMs - (startedMs_ + durationMs_)) < 0) return;
    communicationOk_ = nmeaReceived_ > 0 && parsedSentences_ > 0;
    if (!communicationOk_) result_ = RESULT_FAIL;
    else result_ = currentFix_ ? RESULT_PASS : RESULT_INCONCLUSIVE;
  }

  Result result() const { return result_; }
  bool running() const { return result_ == RESULT_RUNNING; }
  bool detected() const { return detected_; }
  bool communicationOk() const { return communicationOk_; }
  bool currentFix() const { return currentFix_; }
  uint32_t nmeaReceived() const { return nmeaReceived_; }
  uint32_t parsedSentences() const { return parsedSentences_; }
  uint8_t satellites() const { return satellites_; }
  uint32_t fixAgeMs() const { return fixAgeMs_; }

 private:
  void reset() {
    result_ = RESULT_IDLE;
    startedMs_ = 0;
    detected_ = false;
    communicationOk_ = false;
    currentFix_ = false;
    startNmeaCount_ = startParsedCount_ = 0;
    nmeaReceived_ = parsedSentences_ = 0;
    satellites_ = 0;
    fixAgeMs_ = UINT32_MAX;
  }

  uint32_t durationMs_;
  Result result_;
  uint32_t startedMs_;
  bool detected_;
  bool communicationOk_;
  bool currentFix_;
  uint32_t startNmeaCount_;
  uint32_t startParsedCount_;
  uint32_t nmeaReceived_;
  uint32_t parsedSentences_;
  uint8_t satellites_;
  uint32_t fixAgeMs_;
};

}  // namespace bench
}  // namespace green_ion
