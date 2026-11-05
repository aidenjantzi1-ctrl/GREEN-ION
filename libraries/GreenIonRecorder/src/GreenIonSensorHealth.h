#pragma once

#include <stdint.h>

namespace green_ion {

// Fixed-memory policy for a sensor whose hardware driver can be reinitialized.
// Hardware access stays in the sketch so this policy remains host-testable.
class SensorHealth {
 public:
  SensorHealth(uint32_t retryDelayMs, uint8_t readFailureThreshold)
      : retryDelayMs_(retryDelayMs), threshold_(readFailureThreshold),
        nextRetryMs_(0), consecutiveReadFailures_(0), initFailures_(0),
        runtimeLosses_(0), recoveries_(0), ready_(false), started_(false) {}

  bool valid() const { return retryDelayMs_ > 0 && threshold_ > 0; }

  bool begin(uint32_t nowMs, bool initialized) {
    if (!valid()) return false;
    started_ = true;
    ready_ = initialized;
    consecutiveReadFailures_ = 0;
    nextRetryMs_ = nowMs + retryDelayMs_;
    if (!initialized && initFailures_ != UINT32_MAX) ++initFailures_;
    return true;
  }

  // Returns true only when the failure threshold changes the sensor to lost.
  bool noteRead(uint32_t nowMs, bool success) {
    if (!started_ || !ready_) return false;
    if (success) {
      consecutiveReadFailures_ = 0;
      return false;
    }
    if (consecutiveReadFailures_ != UINT8_MAX) ++consecutiveReadFailures_;
    if (consecutiveReadFailures_ < threshold_) return false;
    ready_ = false;
    nextRetryMs_ = nowMs + retryDelayMs_;
    if (runtimeLosses_ != UINT32_MAX) ++runtimeLosses_;
    return true;
  }

  bool shouldAttempt(uint32_t nowMs, bool recorderCritical) const {
    if (!started_ || ready_ || recorderCritical) return false;
    return static_cast<int32_t>(nowMs - nextRetryMs_) >= 0;
  }

  void noteInitialize(uint32_t nowMs, bool initialized) {
    if (!started_) return;
    ready_ = initialized;
    consecutiveReadFailures_ = 0;
    nextRetryMs_ = nowMs + retryDelayMs_;
    if (initialized) {
      if (recoveries_ != UINT32_MAX) ++recoveries_;
    } else if (initFailures_ != UINT32_MAX) {
      ++initFailures_;
    }
  }

  bool ready() const { return ready_; }
  uint8_t consecutiveReadFailures() const { return consecutiveReadFailures_; }
  uint32_t initFailures() const { return initFailures_; }
  uint32_t runtimeLosses() const { return runtimeLosses_; }
  uint32_t recoveries() const { return recoveries_; }
  uint32_t nextRetryMs() const { return nextRetryMs_; }

 private:
  uint32_t retryDelayMs_;
  uint8_t threshold_;
  uint32_t nextRetryMs_;
  uint8_t consecutiveReadFailures_;
  uint32_t initFailures_;
  uint32_t runtimeLosses_;
  uint32_t recoveries_;
  bool ready_;
  bool started_;
};

}  // namespace green_ion
