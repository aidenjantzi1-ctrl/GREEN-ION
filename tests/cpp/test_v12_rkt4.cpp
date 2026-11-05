#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "GreenIonRkt4.h"

using namespace green_ion::rkt4;

int main() {
  const uint8_t goldenHeader[kHeaderBytes] = {
    0x52,0x4b,0x54,0x34,0x02,0x00,0x20,0x00,
    0x10,0x00,0x19,0x00,0x80,0xe6,0xc5,0x47,
    0xa0,0x0f,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  };
  const uint8_t goldenRecord[kRecordBytes] = {
    0xf0,0xff,0xff,0xff,0x00,0x50,0xc3,0x47,
    0x00,0x00,0xac,0x41,0x32,0x0f,0x01,0x00,
  };

  Header header = {25, 101325.0f, 4000};
  uint8_t encodedHeader[kHeaderBytes];
  assert(encodeHeader(encodedHeader, header));
  assert(memcmp(encodedHeader, goldenHeader, kHeaderBytes) == 0);
  assert(validateHeader(encodedHeader, sizeof(encodedHeader)) == RKT4_VALID);

  Header decodedHeader = {};
  assert(decodeHeader(encodedHeader, sizeof(encodedHeader), decodedHeader));
  assert(decodedHeader.sampleRateHz == 25);
  assert(decodedHeader.baselinePressurePa == 101325.0f);
  assert(decodedHeader.startBatteryMv == 4000);

  Record record = {0xfffffff0u, 100000.0f, 21.5f, 3890, true};
  uint8_t encodedRecord[kRecordBytes];
  encodeRecord(encodedRecord, record);
  assert(memcmp(encodedRecord, goldenRecord, kRecordBytes) == 0);

  Record decodedRecord = {};
  decodeRecord(encodedRecord, decodedRecord);
  assert(decodedRecord.timestampUs == record.timestampUs);
  assert(decodedRecord.pressurePa == record.pressurePa);
  assert(decodedRecord.temperatureC == record.temperatureC);
  assert(decodedRecord.batteryMv == record.batteryMv);
  assert(decodedRecord.valid);

  uint8_t corrupted[kHeaderBytes];
  memcpy(corrupted, encodedHeader, sizeof(corrupted));
  corrupted[0] = 'X';
  assert(validateHeader(corrupted, sizeof(corrupted)) == RKT4_BAD_MAGIC);
  assert(validateHeader(encodedHeader, 31) == RKT4_SHORT_HEADER);
  assert(!encodeHeader(encodedHeader, Header{20, 101325.0f, 4000}));

  assert(completeRecordCount(32 + 3 * 16 + 7) == 3);
  assert(trailingByteCount(32 + 3 * 16 + 7) == 7);
  assert(completeRecordCount(9) == 0);
  assert(trailingByteCount(9) == 9);
  assert(timestampDeltaUs(0x10u, 0xfffffff0u) == 32u);

  puts("GREEN ION V12 RKT4 C++ golden-vector tests passed");
  return 0;
}

