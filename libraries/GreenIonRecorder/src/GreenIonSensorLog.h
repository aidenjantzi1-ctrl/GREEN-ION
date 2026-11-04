#pragma once

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "GreenIonRkt4.h"

namespace green_ion {
namespace sensor_log {

static const uint16_t kVersion = 1;
static const size_t kHeaderBytes = 32;
static const size_t kRecordBytes = 68;

enum RecordFlags : uint16_t {
  FLAG_IMU_VALID = 1u << 0,
  FLAG_GPS_HAS_POSITION = 1u << 1,
  FLAG_GPS_CURRENT = 1u << 2,
};

struct Header {
  uint16_t sampleRateHz;
  uint16_t flightSequence;
  uint32_t startTimestampUs;
  uint32_t rkt4HeaderCrc32;
};

struct Record {
  uint32_t timestampUs;
  uint32_t sequence;
  float accelerationX;
  float accelerationY;
  float accelerationZ;
  float gyroX;
  float gyroY;
  float gyroZ;
  int32_t latitudeE7;
  int32_t longitudeE7;
  float gpsAltitudeM;
  float gpsSpeedMps;
  uint32_t gpsFixAgeMs;
  uint32_t lastGpsFixUptimeMs;
  uint16_t flags;
  uint8_t satellites;
  uint8_t gpsHour;
  uint8_t gpsMinute;
  uint8_t gpsSecond;
};

enum Result : uint8_t {
  SENSOR_LOG_VALID = 0,
  SENSOR_LOG_SHORT,
  SENSOR_LOG_BAD_MAGIC,
  SENSOR_LOG_BAD_VERSION,
  SENSOR_LOG_BAD_HEADER_SIZE,
  SENSOR_LOG_BAD_RECORD_SIZE,
  SENSOR_LOG_BAD_CRC,
  SENSOR_LOG_BAD_FIELDS,
};

static_assert(sizeof(float) == 4, "Sensor log requires IEEE-754 float32");

inline uint32_t crc32(const uint8_t *data, size_t length) {
  uint32_t crc = 0xffffffffu;
  for (size_t i = 0; i < length; ++i) {
    crc ^= data[i];
    for (uint8_t bit = 0; bit < 8; ++bit)
      crc = (crc >> 1) ^ (0xedb88320u & (0u - (crc & 1u)));
  }
  return ~crc;
}

inline void writeI32(uint8_t *out, int32_t value) {
  rkt4::writeU32(out, static_cast<uint32_t>(value));
}

inline int32_t readI32(const uint8_t *in) {
  const uint32_t bits = rkt4::readU32(in);
  int32_t value = 0;
  memcpy(&value, &bits, sizeof(value));
  return value;
}

inline bool headerFieldsValid(const Header &header) {
  return rkt4::allowedRate(header.sampleRateHz) &&
         header.flightSequence <= 999;
}

inline bool recordFieldsValid(const Record &record) {
  const uint16_t knownFlags = FLAG_IMU_VALID | FLAG_GPS_HAS_POSITION |
                              FLAG_GPS_CURRENT;
  if ((record.flags & ~knownFlags) != 0 || record.satellites > 64 ||
      record.gpsHour > 23 || record.gpsMinute > 59 || record.gpsSecond > 60)
    return false;
  if ((record.flags & FLAG_GPS_CURRENT) &&
      !(record.flags & FLAG_GPS_HAS_POSITION))
    return false;
  if (record.flags & FLAG_IMU_VALID) {
    if (!isfinite(record.accelerationX) || !isfinite(record.accelerationY) ||
        !isfinite(record.accelerationZ) || !isfinite(record.gyroX) ||
        !isfinite(record.gyroY) || !isfinite(record.gyroZ))
      return false;
  }
  if (record.flags & FLAG_GPS_HAS_POSITION) {
    if (record.latitudeE7 < -900000000 || record.latitudeE7 > 900000000 ||
        record.longitudeE7 < -1800000000 || record.longitudeE7 > 1800000000 ||
        !isfinite(record.gpsAltitudeM) || !isfinite(record.gpsSpeedMps))
      return false;
  } else if (record.latitudeE7 != 0 || record.longitudeE7 != 0 ||
             record.gpsAltitudeM != 0.0f || record.gpsSpeedMps != 0.0f ||
             record.lastGpsFixUptimeMs != 0 || record.gpsFixAgeMs != UINT32_MAX) {
    return false;
  }
  return true;
}

inline bool encodeHeader(uint8_t out[kHeaderBytes], const Header &header) {
  if (!headerFieldsValid(header)) return false;
  memset(out, 0, kHeaderBytes);
  memcpy(out, "GIS1", 4);
  rkt4::writeU16(out + 4, kVersion);
  rkt4::writeU16(out + 6, static_cast<uint16_t>(kHeaderBytes));
  rkt4::writeU16(out + 8, static_cast<uint16_t>(kRecordBytes));
  rkt4::writeU16(out + 10, header.sampleRateHz);
  rkt4::writeU16(out + 12, header.flightSequence);
  rkt4::writeU32(out + 16, header.startTimestampUs);
  rkt4::writeU32(out + 20, header.rkt4HeaderCrc32);
  rkt4::writeU32(out + 28, crc32(out, 28));
  return true;
}

inline Result decodeHeader(const uint8_t *data, size_t length, Header &out) {
  if (length < kHeaderBytes) return SENSOR_LOG_SHORT;
  if (memcmp(data, "GIS1", 4) != 0) return SENSOR_LOG_BAD_MAGIC;
  if (rkt4::readU16(data + 4) != kVersion) return SENSOR_LOG_BAD_VERSION;
  if (rkt4::readU16(data + 6) != kHeaderBytes)
    return SENSOR_LOG_BAD_HEADER_SIZE;
  if (rkt4::readU16(data + 8) != kRecordBytes)
    return SENSOR_LOG_BAD_RECORD_SIZE;
  if (rkt4::readU32(data + 28) != crc32(data, 28)) return SENSOR_LOG_BAD_CRC;
  Header candidate = {
    rkt4::readU16(data + 10), rkt4::readU16(data + 12),
    rkt4::readU32(data + 16), rkt4::readU32(data + 20),
  };
  if (!headerFieldsValid(candidate)) return SENSOR_LOG_BAD_FIELDS;
  out = candidate;
  return SENSOR_LOG_VALID;
}

inline bool encodeRecord(uint8_t out[kRecordBytes], const Record &record) {
  if (!recordFieldsValid(record)) return false;
  memset(out, 0, kRecordBytes);
  rkt4::writeU32(out, record.timestampUs);
  rkt4::writeU32(out + 4, record.sequence);
  rkt4::writeFloat32(out + 8, record.accelerationX);
  rkt4::writeFloat32(out + 12, record.accelerationY);
  rkt4::writeFloat32(out + 16, record.accelerationZ);
  rkt4::writeFloat32(out + 20, record.gyroX);
  rkt4::writeFloat32(out + 24, record.gyroY);
  rkt4::writeFloat32(out + 28, record.gyroZ);
  writeI32(out + 32, record.latitudeE7);
  writeI32(out + 36, record.longitudeE7);
  rkt4::writeFloat32(out + 40, record.gpsAltitudeM);
  rkt4::writeFloat32(out + 44, record.gpsSpeedMps);
  rkt4::writeU32(out + 48, record.gpsFixAgeMs);
  rkt4::writeU32(out + 52, record.lastGpsFixUptimeMs);
  rkt4::writeU16(out + 56, record.flags);
  out[58] = record.satellites;
  out[59] = record.gpsHour;
  out[60] = record.gpsMinute;
  out[61] = record.gpsSecond;
  rkt4::writeU32(out + 64, crc32(out, 64));
  return true;
}

inline Result decodeRecord(const uint8_t *data, size_t length, Record &out) {
  if (length < kRecordBytes) return SENSOR_LOG_SHORT;
  if (rkt4::readU32(data + 64) != crc32(data, 64)) return SENSOR_LOG_BAD_CRC;
  Record candidate = {
    rkt4::readU32(data), rkt4::readU32(data + 4),
    rkt4::readFloat32(data + 8), rkt4::readFloat32(data + 12),
    rkt4::readFloat32(data + 16), rkt4::readFloat32(data + 20),
    rkt4::readFloat32(data + 24), rkt4::readFloat32(data + 28),
    readI32(data + 32), readI32(data + 36),
    rkt4::readFloat32(data + 40), rkt4::readFloat32(data + 44),
    rkt4::readU32(data + 48), rkt4::readU32(data + 52),
    rkt4::readU16(data + 56), data[58], data[59], data[60], data[61],
  };
  if (!recordFieldsValid(candidate)) return SENSOR_LOG_BAD_FIELDS;
  out = candidate;
  return SENSOR_LOG_VALID;
}

inline size_t completeRecordCount(size_t fileBytes) {
  return fileBytes < kHeaderBytes ? 0 :
         (fileBytes - kHeaderBytes) / kRecordBytes;
}

inline size_t trailingByteCount(size_t fileBytes) {
  return fileBytes < kHeaderBytes ? fileBytes :
         (fileBytes - kHeaderBytes) % kRecordBytes;
}

}  // namespace sensor_log
}  // namespace green_ion
