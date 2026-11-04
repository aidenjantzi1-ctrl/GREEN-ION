#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <vector>

#include "GreenIonSensorLogCore.h"

using namespace green_ion::sensor_log;

class MockSink : public green_ion::recorder::ByteSink {
 public:
  bool failWrite = false;
  bool failFlush = false;
  std::vector<uint8_t> bytes;

  bool write(const uint8_t *data, size_t length) override {
    if (failWrite) return false;
    bytes.insert(bytes.end(), data, data + length);
    return true;
  }
  bool flush() override { return !failFlush; }
};

static Record record(uint32_t sequence) {
  return Record{
    sequence * 20000u, sequence, 1.0f, 2.0f, 3.0f,
    0.1f, 0.2f, 0.3f, 0, 0, 0.0f, 0.0f,
    UINT32_MAX, 0, FLAG_IMU_VALID, 0, 0, 0, 0,
  };
}

static uint32_t sequenceAt(const std::vector<uint8_t> &bytes, size_t index) {
  return green_ion::rkt4::readU32(
      bytes.data() + kHeaderBytes + index * kRecordBytes + 4);
}

int main() {
  const Header header = {50, 7, 1234, 0x11223344u};
  SensorLogCore<3, 2> core;
  core.observe(record(1));
  core.observe(record(2));
  core.observe(record(3));
  core.observe(record(4));
  assert(core.prebufferedRecords() == 3);
  MockSink sink;
  assert(core.start(sink, header));
  assert(core.active() && core.prebufferedRecords() == 3);
  assert(sink.bytes.size() == kHeaderBytes);
  assert(core.serviceOne());
  assert(core.serviceOne());
  assert(core.serviceOne());
  assert(core.queuedRecords() == 0);
  assert(sink.bytes.size() == kHeaderBytes + 3 * kRecordBytes);
  assert(sequenceAt(sink.bytes, 0) == 2);
  assert(sequenceAt(sink.bytes, 1) == 3);
  assert(sequenceAt(sink.bytes, 2) == 4);

  core.observe(record(5));
  core.observe(record(6));
  core.observe(record(7));
  assert(core.pendingRecords() == 2 && core.droppedRecords() == 1);
  assert(core.serviceOne() && core.pendingRecords() == 1);
  assert(core.serviceOne() && core.pendingRecords() == 0);
  assert(sequenceAt(sink.bytes, 3) == 6);
  assert(sequenceAt(sink.bytes, 4) == 7);
  assert(core.flushIfDrained());
  core.observe(record(8));
  assert(core.stop() && core.status() == SENSOR_CORE_STOPPED);
  assert(sequenceAt(sink.bytes, 5) == 8);

  SensorLogCore<2, 2> startWriteFailure;
  startWriteFailure.observe(record(1));
  MockSink badStart;
  badStart.failWrite = true;
  assert(!startWriteFailure.start(badStart, header));
  assert(startWriteFailure.status() == SENSOR_CORE_START_FAILED);

  SensorLogCore<2, 2> startFlushFailure;
  startFlushFailure.observe(record(1));
  MockSink badStartFlush;
  badStartFlush.failFlush = true;
  assert(!startFlushFailure.start(badStartFlush, header));
  assert(startFlushFailure.status() == SENSOR_CORE_START_FAILED);

  SensorLogCore<2, 2> writeFailure;
  MockSink badWrite;
  assert(writeFailure.start(badWrite, header));
  writeFailure.observe(record(1));
  badWrite.failWrite = true;
  assert(!writeFailure.serviceOne());
  assert(writeFailure.status() == SENSOR_CORE_WRITE_FAILED);

  SensorLogCore<2, 2> flushFailure;
  MockSink badFlush;
  assert(flushFailure.start(badFlush, header));
  badFlush.failFlush = true;
  assert(!flushFailure.flushIfDrained());
  assert(flushFailure.status() == SENSOR_CORE_FLUSH_FAILED);

  puts("GREEN ION V12 sensor-log core tests passed");
  return 0;
}
