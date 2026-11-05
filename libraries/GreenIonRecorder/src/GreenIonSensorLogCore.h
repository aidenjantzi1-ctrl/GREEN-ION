#pragma once

#include <stddef.h>
#include <stdint.h>

#include "GreenIonRecorderCore.h"
#include "GreenIonSensorLog.h"

namespace green_ion {
namespace sensor_log {

enum CoreStatus : uint8_t {
  SENSOR_CORE_BUFFERING = 0,
  SENSOR_CORE_ACTIVE,
  SENSOR_CORE_START_FAILED,
  SENSOR_CORE_WRITE_FAILED,
  SENSOR_CORE_FLUSH_FAILED,
  SENSOR_CORE_STOPPED,
};

template <size_t Capacity>
class RecordQueue {
 public:
  static_assert(Capacity > 0, "Sensor-log queue capacity must be positive");

  RecordQueue() : head_(0), size_(0), dropped_(0) {}

  void pushLatest(const Record &record, bool countDrop) {
    if (size_ == Capacity) {
      head_ = (head_ + 1u) % Capacity;
      --size_;
      if (countDrop && dropped_ != UINT32_MAX) ++dropped_;
    }
    records_[(head_ + size_) % Capacity] = record;
    ++size_;
  }

  const Record *front() const { return size_ ? &records_[head_] : NULL; }

  bool pop() {
    if (!size_) return false;
    head_ = (head_ + 1u) % Capacity;
    --size_;
    return true;
  }

  size_t size() const { return size_; }
  uint32_t dropped() const { return dropped_; }
  void clear() { head_ = 0; size_ = 0; }

 private:
  Record records_[Capacity];
  size_t head_;
  size_t size_;
  uint32_t dropped_;
};

template <size_t PrebufferCapacity, size_t PendingCapacity>
class SensorLogCore {
 public:
  SensorLogCore() : sink_(NULL), status_(SENSOR_CORE_BUFFERING) {}

  CoreStatus status() const { return status_; }
  bool active() const { return status_ == SENSOR_CORE_ACTIVE; }
  size_t prebufferedRecords() const { return prebuffer_.size(); }
  size_t pendingRecords() const { return pending_.size(); }
  size_t queuedRecords() const {
    return prebuffer_.size() + pending_.size();
  }
  uint32_t droppedRecords() const { return pending_.dropped(); }

  void observe(const Record &record) {
    if (active()) pending_.pushLatest(record, true);
    else if (status_ == SENSOR_CORE_BUFFERING)
      prebuffer_.pushLatest(record, false);
  }

  bool start(recorder::ByteSink &sink, const Header &header) {
    if (status_ != SENSOR_CORE_BUFFERING) return false;
    uint8_t headerBytes[kHeaderBytes];
    if (!encodeHeader(headerBytes, header) ||
        !sink.write(headerBytes, sizeof(headerBytes))) {
      status_ = SENSOR_CORE_START_FAILED;
      return false;
    }
    if (!sink.flush()) {
      status_ = SENSOR_CORE_START_FAILED;
      return false;
    }
    sink_ = &sink;
    status_ = SENSOR_CORE_ACTIVE;
    return true;
  }

  bool serviceOne() {
    if (!active()) return false;
    const bool servingPrebuffer = prebuffer_.front() != NULL;
    const Record *record = servingPrebuffer ? prebuffer_.front() : pending_.front();
    if (!record) return true;
    uint8_t bytes[kRecordBytes];
    if (!encodeRecord(bytes, *record) || !sink_->write(bytes, sizeof(bytes))) {
      sink_ = NULL;
      status_ = SENSOR_CORE_WRITE_FAILED;
      return false;
    }
    if (servingPrebuffer) prebuffer_.pop();
    else pending_.pop();
    return true;
  }

  bool flushIfDrained() {
    if (!active() || queuedRecords()) return false;
    if (sink_->flush()) return true;
    sink_ = NULL;
    status_ = SENSOR_CORE_FLUSH_FAILED;
    return false;
  }

  bool stop() {
    if (!active()) return false;
    while (queuedRecords()) {
      if (!serviceOne()) return false;
    }
    if (!sink_->flush()) {
      sink_ = NULL;
      status_ = SENSOR_CORE_FLUSH_FAILED;
      return false;
    }
    sink_ = NULL;
    status_ = SENSOR_CORE_STOPPED;
    return true;
  }

 private:
  recorder::ByteSink *sink_;
  CoreStatus status_;
  RecordQueue<PrebufferCapacity> prebuffer_;
  RecordQueue<PendingCapacity> pending_;
};

}  // namespace sensor_log
}  // namespace green_ion
