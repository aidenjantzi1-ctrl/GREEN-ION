#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "GreenIonSensorLog.h"

using namespace green_ion::sensor_log;

int main() {
  const uint8_t goldenHeader[kHeaderBytes] = {
    0x47,0x49,0x53,0x31,0x01,0x00,0x20,0x00,
    0x44,0x00,0x32,0x00,0x2a,0x00,0x00,0x00,
    0xf0,0xff,0xff,0xff,0x78,0x56,0x34,0x12,
    0x00,0x00,0x00,0x00,0x7a,0x0e,0x11,0x9d,
  };
  const uint8_t goldenRecord[kRecordBytes] = {
    0x10,0x00,0x00,0x00,0x07,0x00,0x00,0x00,
    0x00,0x00,0xa0,0x3f,0x00,0x00,0x20,0xc0,
    0x0a,0xe8,0x1c,0x41,0xcd,0xcc,0xcc,0x3d,
    0xcd,0xcc,0x4c,0xbe,0x00,0x00,0x60,0x40,
    0x87,0xd3,0x56,0x14,0xf9,0x60,0x30,0xba,
    0x00,0x10,0x80,0x44,0x00,0x00,0x4c,0x41,
    0xfa,0x00,0x00,0x00,0x00,0xff,0xff,0xff,
    0x07,0x00,0x09,0x17,0x3b,0x3c,0x00,0x00,
    0x7e,0x46,0x36,0x68,
  };

  Header header = {50, 42, 0xfffffff0u, 0x12345678u};
  uint8_t encodedHeader[kHeaderBytes];
  assert(encodeHeader(encodedHeader, header));
  assert(memcmp(encodedHeader, goldenHeader, kHeaderBytes) == 0);
  Header decodedHeader = {};
  assert(decodeHeader(encodedHeader, sizeof(encodedHeader), decodedHeader) ==
         SENSOR_LOG_VALID);
  assert(decodedHeader.sampleRateHz == 50 && decodedHeader.flightSequence == 42);
  assert(decodedHeader.startTimestampUs == 0xfffffff0u);
  assert(decodedHeader.rkt4HeaderCrc32 == 0x12345678u);

  Record record = {
    0x10u, 7u, 1.25f, -2.5f, 9.80665f, 0.1f, -0.2f, 3.5f,
    341234567, -1171234567, 1024.5f, 12.75f, 250u, 0xffffff00u,
    static_cast<uint16_t>(FLAG_IMU_VALID | FLAG_GPS_HAS_POSITION |
                          FLAG_GPS_CURRENT),
    9, 23, 59, 60, 0,
  };
  uint8_t encodedRecord[kRecordBytes];
  assert(encodeRecord(encodedRecord, record));
  assert(memcmp(encodedRecord, goldenRecord, kRecordBytes) == 0);
  Record decodedRecord = {};
  assert(decodeRecord(encodedRecord, sizeof(encodedRecord), decodedRecord) ==
         SENSOR_LOG_VALID);
  assert(decodedRecord.timestampUs == 0x10u && decodedRecord.sequence == 7u);
  assert(decodedRecord.accelerationZ == record.accelerationZ);
  assert(decodedRecord.gyroY == record.gyroY);
  assert(decodedRecord.latitudeE7 == record.latitudeE7);
  assert(decodedRecord.longitudeE7 == record.longitudeE7);
  assert(decodedRecord.gpsFixAgeMs == 250u && decodedRecord.satellites == 9);
  assert(decodedRecord.gpsQualityFlags == 0);  // Existing GIS1 bytes stay valid.

  Record altitudeCurrent = record;
  altitudeCurrent.gpsQualityFlags = GPS_QUALITY_KNOWN | GPS_ALTITUDE_CURRENT;
  assert(encodeRecord(encodedRecord, altitudeCurrent));
  assert(encodedRecord[62] == altitudeCurrent.gpsQualityFlags);
  assert(encodedRecord[63] == kGpsQualityMarker);
  assert(decodeRecord(encodedRecord, sizeof(encodedRecord), decodedRecord) ==
         SENSOR_LOG_VALID);
  assert(decodedRecord.gpsQualityFlags == altitudeCurrent.gpsQualityFlags);
  Record altitudeStale = record;
  altitudeStale.gpsQualityFlags = GPS_QUALITY_KNOWN;
  assert(encodeRecord(encodedRecord, altitudeStale));
  assert(decodeRecord(encodedRecord, sizeof(encodedRecord), decodedRecord) ==
         SENSOR_LOG_VALID);
  assert(decodedRecord.gpsQualityFlags == GPS_QUALITY_KNOWN);
  Record invalidQuality = altitudeCurrent;
  invalidQuality.flags &= ~FLAG_GPS_CURRENT;
  assert(!encodeRecord(encodedRecord, invalidQuality));

  uint8_t corruptedHeader[kHeaderBytes];
  memcpy(corruptedHeader, encodedHeader, sizeof(corruptedHeader));
  corruptedHeader[20] ^= 1;
  assert(decodeHeader(corruptedHeader, sizeof(corruptedHeader), decodedHeader) ==
         SENSOR_LOG_BAD_CRC);
  assert(decodeHeader(encodedHeader, 31, decodedHeader) == SENSOR_LOG_SHORT);
  assert(!encodeHeader(encodedHeader, Header{20, 42, 0, 0}));
  assert(!encodeHeader(encodedHeader, Header{50, 1000, 0, 0}));

  uint8_t corruptedRecord[kRecordBytes];
  memcpy(corruptedRecord, encodedRecord, sizeof(corruptedRecord));
  corruptedRecord[8] ^= 1;
  assert(decodeRecord(corruptedRecord, sizeof(corruptedRecord), decodedRecord) ==
         SENSOR_LOG_BAD_CRC);
  assert(decodeRecord(encodedRecord, 67, decodedRecord) == SENSOR_LOG_SHORT);

  Record currentWithoutPosition = record;
  currentWithoutPosition.flags = FLAG_GPS_CURRENT;
  assert(!encodeRecord(encodedRecord, currentWithoutPosition));
  Record invalidImu = record;
  invalidImu.accelerationX = NAN;
  assert(!encodeRecord(encodedRecord, invalidImu));
  Record noPosition = record;
  noPosition.flags = 0;
  noPosition.latitudeE7 = 0;
  noPosition.longitudeE7 = 0;
  noPosition.gpsAltitudeM = 0.0f;
  noPosition.gpsSpeedMps = 0.0f;
  noPosition.gpsFixAgeMs = UINT32_MAX;
  noPosition.lastGpsFixUptimeMs = 0;
  assert(encodeRecord(encodedRecord, noPosition));

  assert(completeRecordCount(32 + 3 * 68 + 9) == 3);
  assert(trailingByteCount(32 + 3 * 68 + 9) == 9);
  assert(completeRecordCount(10) == 0 && trailingByteCount(10) == 10);

  puts("GREEN ION V12 sensor-log golden/error tests passed");
  return 0;
}
