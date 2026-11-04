#pragma once

#include <stddef.h>
#include <stdint.h>

#include "GreenIonSensorLog.h"

namespace green_ion {
namespace sensor_recovery {

struct Inspection {
  sensor_log::Result headerResult;
  uint32_t completeRecords;
  uint8_t trailingBytes;
  uint32_t recordsInspected;
  uint32_t validPrefixRecords;
  uint32_t firstInvalidRecord;
  sensor_log::Result firstInvalidResult;
};

inline Inspection beginInspection(const uint8_t *header, size_t headerBytes,
                                  uint32_t fileBytes,
                                  sensor_log::Header &decodedHeader) {
  Inspection result = {
    sensor_log::decodeHeader(header, headerBytes, decodedHeader),
    static_cast<uint32_t>(sensor_log::completeRecordCount(fileBytes)),
    static_cast<uint8_t>(sensor_log::trailingByteCount(fileBytes)),
    0,
    0,
    UINT32_MAX,
    sensor_log::SENSOR_LOG_VALID,
  };
  return result;
}

inline bool headerMatchesFlight(const sensor_log::Header &header,
                                uint16_t expectedFlightSequence,
                                const uint8_t *rkt4Header,
                                size_t rkt4HeaderBytes) {
  return rkt4HeaderBytes == rkt4::kHeaderBytes &&
         rkt4::validateHeader(rkt4Header, rkt4HeaderBytes) == rkt4::RKT4_VALID &&
         header.flightSequence == expectedFlightSequence &&
         header.rkt4HeaderCrc32 ==
             sensor_log::crc32(rkt4Header, rkt4HeaderBytes);
}

inline bool inspectRecord(Inspection &inspection, const uint8_t *record,
                          size_t recordBytes) {
  if (inspection.headerResult != sensor_log::SENSOR_LOG_VALID ||
      inspection.firstInvalidRecord != UINT32_MAX)
    return false;
  sensor_log::Record decoded = {};
  const sensor_log::Result result =
      sensor_log::decodeRecord(record, recordBytes, decoded);
  const uint32_t index = inspection.recordsInspected++;
  if (result != sensor_log::SENSOR_LOG_VALID) {
    inspection.firstInvalidRecord = index;
    inspection.firstInvalidResult = result;
    return false;
  }
  ++inspection.validPrefixRecords;
  return true;
}

inline void finishInspection(Inspection &inspection) {
  if (inspection.headerResult == sensor_log::SENSOR_LOG_VALID &&
      inspection.firstInvalidRecord == UINT32_MAX &&
      inspection.recordsInspected < inspection.completeRecords) {
    inspection.firstInvalidRecord = inspection.recordsInspected;
    inspection.firstInvalidResult = sensor_log::SENSOR_LOG_SHORT;
  }
}

inline bool completeRecordsValid(const Inspection &inspection) {
  return inspection.headerResult == sensor_log::SENSOR_LOG_VALID &&
         inspection.recordsInspected == inspection.completeRecords &&
         inspection.firstInvalidRecord == UINT32_MAX;
}

}  // namespace sensor_recovery
}  // namespace green_ion
