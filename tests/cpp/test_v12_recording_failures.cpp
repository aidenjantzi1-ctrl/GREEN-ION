#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <vector>

#include "GreenIonRecorderCore.h"
#include "GreenIonSensorLogCore.h"

class FaultSink : public green_ion::recorder::ByteSink {
 public:
  bool failWrite = false;
  bool failFlush = false;
  uint32_t writeCalls = 0;
  uint32_t flushCalls = 0;
  std::vector<uint8_t> bytes;

  bool write(const uint8_t *data, size_t length) override {
    ++writeCalls;
    if (failWrite) return false;
    bytes.insert(bytes.end(), data, data + length);
    return true;
  }

  bool flush() override {
    ++flushCalls;
    return !failFlush;
  }
};

struct RecordingHarness {
  green_ion::recorder::RecorderCore<2> primary;
  green_ion::sensor_log::SensorLogCore<2, 2> companion;
  FaultSink primarySink;
  FaultSink companionSink;

  bool start() {
    const bool primaryStarted = primary.start(
        primarySink, green_ion::rkt4::Header{50, 101325.0f, 4000});
    if (!primaryStarted) return false;
    return companion.start(companionSink,
        green_ion::sensor_log::Header{50, 7, 1000, 0x11223344u});
  }

  void observe(uint32_t timestampUs, uint32_t sequence,
               const green_ion::sensor_log::Record &sensor) {
    // This is the firmware's safety-critical order: primary first.
    primary.observe({timestampUs, 101325.0f, 20.0f, 4000, true});
    green_ion::sensor_log::Record synchronized = sensor;
    synchronized.timestampUs = timestampUs;
    synchronized.sequence = sequence;
    companion.observe(synchronized);
  }
};

static green_ion::sensor_log::Record missingSensors() {
  return {
    0, 0, NAN, NAN, NAN, NAN, NAN, NAN,
    0, 0, 0.0f, 0.0f, UINT32_MAX, 0, 0, 0, 0, 0, 0, 0,
  };
}

static green_ion::sensor_log::Record liveSensors(uint16_t flags) {
  const bool hasGps = (flags & green_ion::sensor_log::FLAG_GPS_HAS_POSITION) != 0;
  return {
    0, 0, 1.0f, 2.0f, 3.0f, 0.1f, 0.2f, 0.3f,
    hasGps ? 374219999 : 0, hasGps ? -1220840575 : 0,
    hasGps ? 12.5f : 0.0f, hasGps ? 3.25f : 0.0f,
    hasGps ? 500u : UINT32_MAX, hasGps ? 900u : 0u,
    flags, static_cast<uint8_t>(hasGps ? 9 : 0), 12, 34, 56, 0,
  };
}

int main() {
  // Missing IMU/GPS data remains explicitly invalid in the companion while the
  // essential pressure recorder continues and flushes normally.
  RecordingHarness optionalMissing;
  assert(optionalMissing.start());
  optionalMissing.observe(1000, 1, missingSensors());
  assert(optionalMissing.companion.serviceOne());
  assert(optionalMissing.primary.recording());
  assert(optionalMissing.primary.flush());
  green_ion::sensor_log::Record decoded = {};
  assert(green_ion::sensor_log::decodeRecord(
      optionalMissing.companionSink.bytes.data() +
          green_ion::sensor_log::kHeaderBytes,
      green_ion::sensor_log::kRecordBytes, decoded) ==
      green_ion::sensor_log::SENSOR_LOG_VALID);
  assert(decoded.flags == 0 && decoded.gpsFixAgeMs == UINT32_MAX);

  // Mid-run GPS loss preserves the last-known stale position before switching
  // to an explicitly missing row; none of these transitions stop RKT4.
  RecordingHarness gpsLoss;
  assert(gpsLoss.start());
  const uint16_t liveFlags = green_ion::sensor_log::FLAG_IMU_VALID |
      green_ion::sensor_log::FLAG_GPS_HAS_POSITION |
      green_ion::sensor_log::FLAG_GPS_CURRENT;
  const uint16_t staleFlags = green_ion::sensor_log::FLAG_IMU_VALID |
      green_ion::sensor_log::FLAG_GPS_HAS_POSITION;
  gpsLoss.observe(1100, 1, liveSensors(liveFlags));
  assert(gpsLoss.companion.serviceOne());
  gpsLoss.observe(1200, 2, liveSensors(staleFlags));
  assert(gpsLoss.companion.serviceOne());
  gpsLoss.observe(1300, 3, liveSensors(green_ion::sensor_log::FLAG_IMU_VALID));
  assert(gpsLoss.companion.serviceOne());
  assert(gpsLoss.primary.recording() && gpsLoss.primary.flush());
  const uint8_t *gpsRecords = gpsLoss.companionSink.bytes.data() +
      green_ion::sensor_log::kHeaderBytes;
  assert(green_ion::sensor_log::decodeRecord(
      gpsRecords, green_ion::sensor_log::kRecordBytes, decoded) ==
      green_ion::sensor_log::SENSOR_LOG_VALID);
  assert(decoded.flags & green_ion::sensor_log::FLAG_GPS_CURRENT);
  assert(green_ion::sensor_log::decodeRecord(
      gpsRecords + green_ion::sensor_log::kRecordBytes,
      green_ion::sensor_log::kRecordBytes, decoded) ==
      green_ion::sensor_log::SENSOR_LOG_VALID);
  assert((decoded.flags & green_ion::sensor_log::FLAG_GPS_HAS_POSITION) &&
         !(decoded.flags & green_ion::sensor_log::FLAG_GPS_CURRENT));
  assert(green_ion::sensor_log::decodeRecord(
      gpsRecords + 2 * green_ion::sensor_log::kRecordBytes,
      green_ion::sensor_log::kRecordBytes, decoded) ==
      green_ion::sensor_log::SENSOR_LOG_VALID);
  assert(!(decoded.flags & green_ion::sensor_log::FLAG_GPS_HAS_POSITION));

  // A mid-run companion write failure is isolated: primary samples written
  // before and after the failure remain recordable and flushable.
  RecordingHarness companionWriteFailure;
  assert(companionWriteFailure.start());
  companionWriteFailure.observe(2000, 2, liveSensors(liveFlags));
  companionWriteFailure.companionSink.failWrite = true;
  assert(!companionWriteFailure.companion.serviceOne());
  assert(companionWriteFailure.companion.status() ==
         green_ion::sensor_log::SENSOR_CORE_WRITE_FAILED);
  assert(companionWriteFailure.primary.recording());
  companionWriteFailure.observe(3000, 3, missingSensors());
  assert(companionWriteFailure.primary.recording());
  assert(companionWriteFailure.primary.flush());
  assert(green_ion::rkt4::completeRecordCount(
      companionWriteFailure.primarySink.bytes.size()) == 2);

  // A companion flush failure also remains secondary and cannot poison the
  // already successful primary flush path.
  RecordingHarness companionFlushFailure;
  assert(companionFlushFailure.start());
  companionFlushFailure.observe(4000, 4,
      liveSensors(green_ion::sensor_log::FLAG_IMU_VALID));
  assert(companionFlushFailure.companion.serviceOne());
  assert(companionFlushFailure.primary.flush());
  companionFlushFailure.companionSink.failFlush = true;
  assert(!companionFlushFailure.companion.flushIfDrained());
  assert(companionFlushFailure.companion.status() ==
         green_ion::sensor_log::SENSOR_CORE_FLUSH_FAILED);
  assert(companionFlushFailure.primary.recording());

  // Primary SD failures are different: they are fatal and explicitly latched.
  RecordingHarness primaryWriteFailure;
  assert(primaryWriteFailure.start());
  primaryWriteFailure.primarySink.failWrite = true;
  primaryWriteFailure.observe(5000, 5, missingSensors());
  assert(!primaryWriteFailure.primary.recording());
  assert(primaryWriteFailure.primary.status() ==
         green_ion::recorder::CORE_RECORD_WRITE_FAILED);

  RecordingHarness primaryFlushFailure;
  assert(primaryFlushFailure.start());
  primaryFlushFailure.primarySink.failFlush = true;
  assert(!primaryFlushFailure.primary.flush());
  assert(primaryFlushFailure.primary.status() ==
         green_ion::recorder::CORE_FLUSH_FAILED);

  puts("GREEN ION V12 coordinated recording failure tests passed");
  return 0;
}
