#pragma once

#include <stddef.h>
#include <stdint.h>

#include "GreenIonProtocol.h"

namespace green_ion {

template <size_t Capacity>
class TelemetryQueue {
 public:
  static_assert(Capacity > 0, "Telemetry queue capacity must be positive");

  TelemetryQueue() : head_(0), size_(0), dropped_(0) {}

  void push(const TelemetryPacket &packet) {
    if (size_ == Capacity) {
      head_ = (head_ + 1u) % Capacity;
      --size_;
      if (dropped_ != UINT32_MAX) ++dropped_;
    }
    packets_[(head_ + size_) % Capacity] = packet;
    ++size_;
  }

  const TelemetryPacket *front() const {
    return size_ ? &packets_[head_] : NULL;
  }

  bool pop() {
    if (!size_) return false;
    head_ = (head_ + 1u) % Capacity;
    --size_;
    return true;
  }

  size_t size() const { return size_; }
  constexpr size_t capacity() const { return Capacity; }
  uint32_t dropped() const { return dropped_; }

 private:
  TelemetryPacket packets_[Capacity];
  size_t head_;
  size_t size_;
  uint32_t dropped_;
};

inline bool radioDispatchAllowed(bool radioReady, bool radioBusy,
                                 bool storageBusy, bool queueHasData,
                                 uint32_t nowUs, uint32_t nextSampleUs,
                                 uint32_t minimumMarginUs) {
  if (!radioReady || radioBusy || storageBusy || !queueHasData) return false;
  const uint32_t remainingUs = nextSampleUs - nowUs;
  return remainingUs < 0x80000000u && remainingUs >= minimumMarginUs;
}

}  // namespace green_ion
