#pragma once

#include <stddef.h>
#include <stdint.h>

#include "GreenIonRkt4.h"

namespace green_ion {
namespace recorder {

// Minimal deterministic storage boundary. Arduino SD and host tests can both
// implement this without making the recorder core depend on either platform.
class ByteSink {
 public:
  virtual ~ByteSink() {}
  virtual bool write(const uint8_t *data, size_t length) = 0;
  virtual bool flush() = 0;
};

enum CoreStatus : uint8_t {
  CORE_BUFFERING = 0,
  CORE_RECORDING,
  CORE_START_WRITE_FAILED,
  CORE_RECORD_WRITE_FAILED,
  CORE_FLUSH_FAILED,
};

template <size_t Capacity>
class RecordPrebuffer {
 public:
  static_assert(Capacity > 0, "Prebuffer capacity must be positive");

  RecordPrebuffer() : size_(0), next_(0) {}

  void push(const rkt4::Record &record) {
    records_[next_] = record;
    next_ = (next_ + 1) % Capacity;
    if (size_ < Capacity) ++size_;
  }

  size_t size() const { return size_; }
  constexpr size_t capacity() const { return Capacity; }

  const rkt4::Record &oldest(size_t index) const {
    const size_t first = (next_ + Capacity - size_) % Capacity;
    return records_[(first + index) % Capacity];
  }

  void clear() {
    size_ = 0;
    next_ = 0;
  }

 private:
  rkt4::Record records_[Capacity];
  size_t size_;
  size_t next_;
};

template <size_t PrebufferCapacity>
class RecorderCore {
 public:
  RecorderCore() : sink_(NULL), status_(CORE_BUFFERING) {}

  CoreStatus status() const { return status_; }
  bool recording() const { return status_ == CORE_RECORDING; }
  size_t bufferedRecords() const { return prebuffer_.size(); }
  constexpr size_t prebufferCapacity() const { return PrebufferCapacity; }

  bool resetBuffer() {
    if (recording()) return false;
    prebuffer_.clear();
    status_ = CORE_BUFFERING;
    return true;
  }

  void observe(const rkt4::Record &record) {
    if (!recording()) {
      prebuffer_.push(record);
      return;
    }
    uint8_t bytes[rkt4::kRecordBytes];
    rkt4::encodeRecord(bytes, record);
    if (!sink_->write(bytes, sizeof(bytes))) {
      sink_ = NULL;
      status_ = CORE_RECORD_WRITE_FAILED;
      prebuffer_.push(record);
    }
  }

  bool start(ByteSink &sink, const rkt4::Header &header) {
    uint8_t headerBytes[rkt4::kHeaderBytes];
    if (!rkt4::encodeHeader(headerBytes, header) ||
        !sink.write(headerBytes, sizeof(headerBytes))) {
      status_ = CORE_START_WRITE_FAILED;
      return false;
    }
    for (size_t i = 0; i < prebuffer_.size(); ++i) {
      uint8_t recordBytes[rkt4::kRecordBytes];
      rkt4::encodeRecord(recordBytes, prebuffer_.oldest(i));
      if (!sink.write(recordBytes, sizeof(recordBytes))) {
        status_ = CORE_START_WRITE_FAILED;
        return false;
      }
    }
    if (!sink.flush()) {
      status_ = CORE_START_WRITE_FAILED;
      return false;
    }
    prebuffer_.clear();
    sink_ = &sink;
    status_ = CORE_RECORDING;
    return true;
  }

  bool flush() {
    if (!recording()) return false;
    if (sink_->flush()) return true;
    sink_ = NULL;
    status_ = CORE_FLUSH_FAILED;
    return false;
  }

 private:
  ByteSink *sink_;
  CoreStatus status_;
  RecordPrebuffer<PrebufferCapacity> prebuffer_;
};

}  // namespace recorder
}  // namespace green_ion
