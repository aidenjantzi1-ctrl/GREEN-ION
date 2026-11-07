#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "GreenIonSensorRecovery.h"

using namespace green_ion;

static sensor_log::Record sample(uint32_t sequence) {
  return {
    sequence * 20000u, sequence, 1.0f, 2.0f, 3.0f,
    0.1f, 0.2f, 0.3f, 0, 0, 0.0f, 0.0f,
    UINT32_MAX, 0, sensor_log::FLAG_IMU_VALID, 0, 0, 0, 0, 0,
  };
}

int main() {
  uint8_t rktHeader[rkt4::kHeaderBytes];
  assert(rkt4::encodeHeader(
      rktHeader, rkt4::Header{50, 101325.0f, 4000}));
  const sensor_log::Header sensorHeader = {
    50, 23, 500000,
    sensor_log::crc32(rktHeader, sizeof(rktHeader)),
  };
  uint8_t encodedHeader[sensor_log::kHeaderBytes];
  assert(sensor_log::encodeHeader(encodedHeader, sensorHeader));

  sensor_log::Header decoded = {};
  auto clean = sensor_recovery::beginInspection(
      encodedHeader, sizeof(encodedHeader),
      sensor_log::kHeaderBytes + 2 * sensor_log::kRecordBytes, decoded);
  assert(clean.headerResult == sensor_log::SENSOR_LOG_VALID);
  assert(clean.completeRecords == 2 && clean.trailingBytes == 0);
  assert(sensor_recovery::headerMatchesFlight(
      decoded, 23, rktHeader, sizeof(rktHeader)));
  assert(!sensor_recovery::headerMatchesFlight(
      decoded, 24, rktHeader, sizeof(rktHeader)));
  uint8_t changedRktHeader[rkt4::kHeaderBytes];
  memcpy(changedRktHeader, rktHeader, sizeof(changedRktHeader));
  changedRktHeader[16] ^= 1;
  assert(!sensor_recovery::headerMatchesFlight(
      decoded, 23, changedRktHeader, sizeof(changedRktHeader)));

  uint8_t record[sensor_log::kRecordBytes];
  assert(sensor_log::encodeRecord(record, sample(1)));
  assert(sensor_recovery::inspectRecord(clean, record, sizeof(record)));
  assert(sensor_log::encodeRecord(record, sample(2)));
  assert(sensor_recovery::inspectRecord(clean, record, sizeof(record)));
  sensor_recovery::finishInspection(clean);
  assert(sensor_recovery::completeRecordsValid(clean));
  assert(clean.validPrefixRecords == 2 && clean.firstInvalidRecord == UINT32_MAX);

  auto corrupt = sensor_recovery::beginInspection(
      encodedHeader, sizeof(encodedHeader),
      sensor_log::kHeaderBytes + 2 * sensor_log::kRecordBytes + 9, decoded);
  assert(corrupt.completeRecords == 2 && corrupt.trailingBytes == 9);
  assert(sensor_log::encodeRecord(record, sample(1)));
  assert(sensor_recovery::inspectRecord(corrupt, record, sizeof(record)));
  assert(sensor_log::encodeRecord(record, sample(2)));
  record[12] ^= 0x40;
  assert(!sensor_recovery::inspectRecord(corrupt, record, sizeof(record)));
  sensor_recovery::finishInspection(corrupt);
  assert(!sensor_recovery::completeRecordsValid(corrupt));
  assert(corrupt.validPrefixRecords == 1 && corrupt.firstInvalidRecord == 1);
  assert(corrupt.firstInvalidResult == sensor_log::SENSOR_LOG_BAD_CRC);

  auto shortRead = sensor_recovery::beginInspection(
      encodedHeader, sizeof(encodedHeader),
      sensor_log::kHeaderBytes + 2 * sensor_log::kRecordBytes, decoded);
  assert(sensor_log::encodeRecord(record, sample(1)));
  assert(sensor_recovery::inspectRecord(shortRead, record, sizeof(record)));
  sensor_recovery::finishInspection(shortRead);
  assert(shortRead.firstInvalidRecord == 1);
  assert(shortRead.firstInvalidResult == sensor_log::SENSOR_LOG_SHORT);

  encodedHeader[0] = 'X';
  auto badHeader = sensor_recovery::beginInspection(
      encodedHeader, sizeof(encodedHeader), sensor_log::kHeaderBytes, decoded);
  assert(badHeader.headerResult == sensor_log::SENSOR_LOG_BAD_MAGIC);
  assert(!sensor_recovery::completeRecordsValid(badHeader));

  puts("GREEN ION V12 sensor recovery tests passed");
  return 0;
}
