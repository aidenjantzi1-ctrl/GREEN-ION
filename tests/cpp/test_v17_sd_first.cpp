#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "GreenIonCheckpoint.h"
#include "GreenIonFlightReadiness.h"
#include "GreenIonFlightWatcher.h"
#include "GreenIonRecorderCore.h"

class MemorySink : public green_ion::recorder::ByteSink {
 public:
  bool failWrites = false;
  bool failFlush = false;
  std::vector<uint8_t> bytes;

  bool write(const uint8_t *data, size_t length) override {
    if (failWrites) return false;
    bytes.insert(bytes.end(), data, data + length);
    return true;
  }
  bool flush() override { return !failFlush; }
};

static green_ion::rkt4::Record sample(uint32_t us, float pressure) {
  return {us, pressure, 20.0f, 3900, true};
}

int main() {
  // Core requirement: launch detection and primary RKT4 recording have no
  // dependency on a radio object or telemetry success.
  green_ion::flight::FlightWatcher watcher;
  assert(watcher.begin({50, 0.0f, 101325.0f, 0.10f}));
  green_ion::recorder::RecorderCore<150> recorder;
  uint32_t us=0, ms=0;

  for (int i=0;i<150;i++,us+=20000,ms+=20) {
    recorder.observe(sample(us,101325.0f));
    assert(!watcher.update({ms,0.0f,0.0f,101325.0f,true}));
  }
  assert(recorder.bufferedRecords()==150);

  bool launch=false;
  for (int i=0;i<12 && !launch;i++,us+=20000,ms+=20) {
    const float alt=2.0f+i*0.45f;
    recorder.observe(sample(us,101325.0f-alt*12.0f));
    launch=watcher.update({ms,alt,22.0f,101325.0f-alt*12.0f,true});
  }
  assert(launch);

  MemorySink sd;
  assert(recorder.start(sd,{50,101325.0f,3900}));
  assert(recorder.recording());
  for(int i=0;i<300;i++,us+=20000) recorder.observe(sample(us,100900.0f-i*0.25f));
  assert(recorder.flush());
  assert(recorder.stop());
  assert(green_ion::rkt4::validateHeader(sd.bytes.data(),sd.bytes.size())==
         green_ion::rkt4::RKT4_VALID);
  assert(green_ion::rkt4::completeRecordCount(sd.bytes.size())>=450);

  // Readiness is still true when BOTH optional links are absent.
  green_ion::readiness::Input ready = {
    true,true,true,3900,true,true,true,true,true,true,
    false,false,true,true,true
  };
  auto readiness=green_ion::readiness::evaluate(ready);
  assert(readiness.ready);
  assert(readiness.warningFlags & green_ion::readiness::WARNING_GPS);
  assert(readiness.warningFlags & green_ion::readiness::WARNING_LORA);

  // SD write loss is correctly fatal to the authoritative recorder.
  green_ion::recorder::RecorderCore<4> failedRecorder;
  MemorySink failingSd;
  assert(failedRecorder.start(failingSd,{50,101325.0f,3900}));
  failingSd.failWrites=true;
  failedRecorder.observe(sample(20000,101300.0f));
  assert(!failedRecorder.recording());
  assert(failedRecorder.status()==green_ion::recorder::CORE_RECORD_WRITE_FAILED);

  // Power-loss model: a torn file remains inspectable and a valid checkpoint
  // selects the last committed complete RKT4 boundary.
  uint8_t header[green_ion::rkt4::kHeaderBytes]={};
  assert(green_ion::rkt4::encodeHeader(header,{50,101325.0f,3900}));
  const uint32_t committed=green_ion::rkt4::kHeaderBytes+100*green_ion::rkt4::kRecordBytes;
  const uint32_t tornBytes=committed+7;
  const auto inspection=green_ion::recovery::inspectRkt4(header,sizeof(header),tornBytes);
  assert(inspection.headerResult==green_ion::rkt4::RKT4_VALID);
  assert(inspection.completeRecords==100);
  assert(inspection.trailingBytes==7);

  const green_ion::recovery::Checkpoint cp={
    4,committed,100,2000,0,0,false,0
  };
  uint8_t slot[green_ion::recovery::kCheckpointBytes]={};
  assert(green_ion::recovery::encodeCheckpoint(slot,cp));
  green_ion::recovery::Checkpoint selected={};
  assert(green_ion::recovery::selectNewestCheckpoint(
      slot,sizeof(slot),nullptr,0,tornBytes,selected));
  assert(selected.committedBytes==committed);
  assert(selected.recordCount==100);
}
