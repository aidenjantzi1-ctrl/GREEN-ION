#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <vector>

#include "GreenIonRecorderCore.h"

using green_ion::recorder::ByteSink;
using green_ion::recorder::CORE_BUFFERING;
using green_ion::recorder::CORE_FLUSH_FAILED;
using green_ion::recorder::CORE_RECORD_WRITE_FAILED;
using green_ion::recorder::CORE_RECORDING;
using green_ion::recorder::CORE_START_WRITE_FAILED;
using green_ion::recorder::CORE_STOPPED;
using green_ion::recorder::RecorderCore;
using green_ion::rkt4::Header;
using green_ion::rkt4::Record;

class MockSink : public ByteSink {
 public:
  MockSink() : writesBeforeFailure(-1), flushOk(true), writes(0) {}

  bool write(const uint8_t *data, size_t length) override {
    if (writesBeforeFailure >= 0 && writes >= writesBeforeFailure) return false;
    bytes.insert(bytes.end(), data, data + length);
    ++writes;
    return true;
  }

  bool flush() override { return flushOk; }

  int writesBeforeFailure;
  bool flushOk;
  int writes;
  std::vector<uint8_t> bytes;
};

static Record sample(uint32_t timestamp) {
  return Record{timestamp, 101325.0f - timestamp, 20.0f, 4000, true};
}

int main() {
  RecorderCore<3> core;
  assert(core.status() == CORE_BUFFERING);
  assert(core.prebufferCapacity() == 3);

  core.observe(sample(0xfffffff0u));
  core.observe(sample(0xfffffff8u));
  core.observe(sample(0x00000000u));
  core.observe(sample(0x00000008u));
  assert(core.bufferedRecords() == 3);

  RecorderCore<3> resettable;
  resettable.observe(sample(1));
  assert(resettable.resetBuffer());
  assert(resettable.bufferedRecords() == 0);

  MockSink sink;
  assert(core.start(sink, Header{50, 101325.0f, 4000}));
  assert(core.status() == CORE_RECORDING);
  assert(!core.resetBuffer());
  assert(core.bufferedRecords() == 0);
  assert(sink.bytes.size() == green_ion::rkt4::kHeaderBytes + 3 * green_ion::rkt4::kRecordBytes);

  Record decoded = {};
  green_ion::rkt4::decodeRecord(sink.bytes.data() + green_ion::rkt4::kHeaderBytes, decoded);
  assert(decoded.timestampUs == 0xfffffff8u);
  green_ion::rkt4::decodeRecord(sink.bytes.data() + green_ion::rkt4::kHeaderBytes + 2 * green_ion::rkt4::kRecordBytes, decoded);
  assert(decoded.timestampUs == 0x00000008u);

  sink.writesBeforeFailure = sink.writes;
  core.observe(sample(0x10u));
  assert(core.status() == CORE_RECORD_WRITE_FAILED);
  assert(core.bufferedRecords() == 1);

  MockSink retry;
  assert(core.start(retry, Header{50, 101325.0f, 3990}));
  assert(core.status() == CORE_RECORDING);
  assert(retry.bytes.size() == green_ion::rkt4::kHeaderBytes + green_ion::rkt4::kRecordBytes);

  assert(core.stop());
  assert(core.status() == CORE_STOPPED);
  assert(!core.recording());
  assert(!core.stop());

  RecorderCore<3> flushFailure;
  MockSink failingFlush;
  assert(flushFailure.start(failingFlush, Header{50, 101325.0f, 3990}));
  failingFlush.flushOk = false;
  assert(!flushFailure.flush());
  assert(flushFailure.status() == CORE_FLUSH_FAILED);

  RecorderCore<3> stopFailure;
  MockSink failingStop;
  assert(stopFailure.start(failingStop, Header{50, 101325.0f, 3990}));
  failingStop.flushOk = false;
  assert(!stopFailure.stop());
  assert(stopFailure.status() == CORE_FLUSH_FAILED);

  RecorderCore<3> startFailure;
  startFailure.observe(sample(1));
  MockSink failingStart;
  failingStart.writesBeforeFailure = 1;
  assert(!startFailure.start(failingStart, Header{50, 101325.0f, 4000}));
  assert(startFailure.status() == CORE_START_WRITE_FAILED);
  assert(startFailure.bufferedRecords() == 1);

  RecorderCore<3> startFlushFailure;
  startFlushFailure.observe(sample(2));
  MockSink failingStartFlush;
  failingStartFlush.flushOk = false;
  assert(!startFlushFailure.start(failingStartFlush, Header{50, 101325.0f, 4000}));
  assert(startFlushFailure.status() == CORE_START_WRITE_FAILED);
  assert(startFlushFailure.bufferedRecords() == 1);

  puts("GREEN ION V12 recorder core tests passed");
  return 0;
}
