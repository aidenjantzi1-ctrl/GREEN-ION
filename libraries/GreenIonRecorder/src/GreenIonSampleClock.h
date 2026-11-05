#pragma once

#include <stdint.h>

namespace green_ion {
namespace recorder {

// Fixed-rate unsigned-micros scheduler. Calls must occur at least once per
// 2^31 microseconds so before/after remains unambiguous across rollover.
class SampleClock {
 public:
  explicit SampleClock(uint32_t intervalUs)
      : intervalUs_(intervalUs), nextUs_(0), missedPeriods_(0), started_(false) {}

  bool valid() const { return intervalUs_ > 0 && intervalUs_ < 0x80000000u; }

  bool start(uint32_t nowUs) {
    if (!valid()) return false;
    nextUs_ = nowUs + intervalUs_;
    missedPeriods_ = 0;
    started_ = true;
    return true;
  }

  bool due(uint32_t nowUs) {
    if (!started_ || !valid()) return false;
    const uint32_t lateUs = nowUs - nextUs_;
    if (lateUs >= 0x80000000u) return false;
    const uint32_t elapsedPeriods = lateUs / intervalUs_ + 1u;
    const uint32_t newlyMissed = elapsedPeriods - 1u;
    if (UINT32_MAX - missedPeriods_ < newlyMissed)
      missedPeriods_ = UINT32_MAX;
    else
      missedPeriods_ += newlyMissed;
    nextUs_ += elapsedPeriods * intervalUs_;
    return true;
  }

  uint32_t intervalUs() const { return intervalUs_; }
  uint32_t nextDeadlineUs() const { return nextUs_; }
  uint32_t missedPeriods() const { return missedPeriods_; }
  bool started() const { return started_; }

 private:
  uint32_t intervalUs_;
  uint32_t nextUs_;
  uint32_t missedPeriods_;
  bool started_;
};

}  // namespace recorder
}  // namespace green_ion
