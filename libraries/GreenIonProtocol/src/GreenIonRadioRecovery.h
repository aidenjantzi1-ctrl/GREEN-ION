#pragma once

#include <stdint.h>

namespace green_ion {

class RadioRecovery {
 public:
  RadioRecovery(uint32_t retryDelayMs, uint8_t sendFailureThreshold)
      : retryDelayMs_(retryDelayMs), threshold_(sendFailureThreshold),
        nextRetryMs_(0), consecutiveSendFailures_(0), initFailures_(0),
        recoveries_(0), ready_(false), started_(false) {}

  bool valid() const { return retryDelayMs_ > 0 && threshold_ > 0; }

  bool begin(uint32_t nowMs, bool initialized) {
    if (!valid()) return false;
    started_ = true;
    ready_ = initialized;
    consecutiveSendFailures_ = 0;
    nextRetryMs_ = nowMs + retryDelayMs_;
    if (!initialized && initFailures_ != UINT32_MAX) ++initFailures_;
    return true;
  }

  bool noteSendResult(uint32_t nowMs, bool success) {
    if (!started_ || !ready_) return false;
    if (success) {
      consecutiveSendFailures_ = 0;
      return false;
    }
    if (consecutiveSendFailures_ != UINT8_MAX) ++consecutiveSendFailures_;
    if (consecutiveSendFailures_ < threshold_) return false;
    ready_ = false;
    nextRetryMs_ = nowMs + retryDelayMs_;
    return true;
  }

  bool shouldAttempt(uint32_t nowMs, bool recorderCritical) const {
    if (!started_ || ready_ || recorderCritical) return false;
    return static_cast<int32_t>(nowMs - nextRetryMs_) >= 0;
  }

  void noteInitialize(uint32_t nowMs, bool initialized) {
    if (!started_) return;
    ready_ = initialized;
    consecutiveSendFailures_ = 0;
    nextRetryMs_ = nowMs + retryDelayMs_;
    if (initialized) {
      if (recoveries_ != UINT32_MAX) ++recoveries_;
    } else if (initFailures_ != UINT32_MAX) {
      ++initFailures_;
    }
  }

  bool ready() const { return ready_; }
  uint8_t consecutiveSendFailures() const { return consecutiveSendFailures_; }
  uint32_t initFailures() const { return initFailures_; }
  uint32_t recoveries() const { return recoveries_; }
  uint32_t nextRetryMs() const { return nextRetryMs_; }

 private:
  uint32_t retryDelayMs_;
  uint8_t threshold_;
  uint32_t nextRetryMs_;
  uint8_t consecutiveSendFailures_;
  uint32_t initFailures_;
  uint32_t recoveries_;
  bool ready_;
  bool started_;
};

}  // namespace green_ion
