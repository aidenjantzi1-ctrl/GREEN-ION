#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>

namespace green_ion {
namespace rkt4 {

static const uint16_t kVersion = 2;
static const size_t kHeaderBytes = 32;
static const size_t kRecordBytes = 16;

struct Header {
  uint16_t sampleRateHz;
  float baselinePressurePa;
  uint16_t startBatteryMv;
};

struct Record {
  uint32_t timestampUs;
  float pressurePa;
  float temperatureC;
  uint16_t batteryMv;
  bool valid;
};

enum ValidationResult : uint8_t {
  RKT4_VALID = 0,
  RKT4_SHORT_HEADER,
  RKT4_BAD_MAGIC,
  RKT4_BAD_VERSION,
  RKT4_BAD_HEADER_SIZE,
  RKT4_BAD_RECORD_SIZE,
  RKT4_BAD_SAMPLE_RATE,
};

static_assert(sizeof(float) == 4, "RKT4 requires IEEE-754 float32");

inline void writeU16(uint8_t *out, uint16_t value) {
  out[0] = static_cast<uint8_t>(value);
  out[1] = static_cast<uint8_t>(value >> 8);
}

inline void writeU32(uint8_t *out, uint32_t value) {
  out[0] = static_cast<uint8_t>(value);
  out[1] = static_cast<uint8_t>(value >> 8);
  out[2] = static_cast<uint8_t>(value >> 16);
  out[3] = static_cast<uint8_t>(value >> 24);
}

inline uint16_t readU16(const uint8_t *in) {
  return static_cast<uint16_t>(in[0]) |
         (static_cast<uint16_t>(in[1]) << 8);
}

inline uint32_t readU32(const uint8_t *in) {
  return static_cast<uint32_t>(in[0]) |
         (static_cast<uint32_t>(in[1]) << 8) |
         (static_cast<uint32_t>(in[2]) << 16) |
         (static_cast<uint32_t>(in[3]) << 24);
}

inline void writeFloat32(uint8_t *out, float value) {
  uint32_t bits = 0;
  memcpy(&bits, &value, sizeof(bits));
  writeU32(out, bits);
}

inline float readFloat32(const uint8_t *in) {
  const uint32_t bits = readU32(in);
  float value = 0;
  memcpy(&value, &bits, sizeof(value));
  return value;
}

inline bool allowedRate(uint16_t rateHz) {
  return rateHz == 10 || rateHz == 25 || rateHz == 50;
}

inline bool encodeHeader(uint8_t out[kHeaderBytes], const Header &header) {
  if (!allowedRate(header.sampleRateHz)) return false;
  memset(out, 0, kHeaderBytes);
  memcpy(out, "RKT4", 4);
  writeU16(out + 4, kVersion);
  writeU16(out + 6, static_cast<uint16_t>(kHeaderBytes));
  writeU16(out + 8, static_cast<uint16_t>(kRecordBytes));
  writeU16(out + 10, header.sampleRateHz);
  writeFloat32(out + 12, header.baselinePressurePa);
  writeU16(out + 16, header.startBatteryMv);
  return true;
}

inline ValidationResult validateHeader(const uint8_t *data, size_t length) {
  if (length < kHeaderBytes) return RKT4_SHORT_HEADER;
  if (memcmp(data, "RKT4", 4) != 0) return RKT4_BAD_MAGIC;
  if (readU16(data + 4) != kVersion) return RKT4_BAD_VERSION;
  if (readU16(data + 6) != kHeaderBytes) return RKT4_BAD_HEADER_SIZE;
  if (readU16(data + 8) != kRecordBytes) return RKT4_BAD_RECORD_SIZE;
  if (!allowedRate(readU16(data + 10))) return RKT4_BAD_SAMPLE_RATE;
  return RKT4_VALID;
}

inline bool decodeHeader(const uint8_t *data, size_t length, Header &out) {
  if (validateHeader(data, length) != RKT4_VALID) return false;
  out.sampleRateHz = readU16(data + 10);
  out.baselinePressurePa = readFloat32(data + 12);
  out.startBatteryMv = readU16(data + 16);
  return true;
}

inline void encodeRecord(uint8_t out[kRecordBytes], const Record &record) {
  writeU32(out, record.timestampUs);
  writeFloat32(out + 4, record.pressurePa);
  writeFloat32(out + 8, record.temperatureC);
  writeU16(out + 12, record.batteryMv);
  writeU16(out + 14, record.valid ? 1 : 0);
}

inline void decodeRecord(const uint8_t data[kRecordBytes], Record &out) {
  out.timestampUs = readU32(data);
  out.pressurePa = readFloat32(data + 4);
  out.temperatureC = readFloat32(data + 8);
  out.batteryMv = readU16(data + 12);
  out.valid = readU16(data + 14) == 1;
}

inline size_t completeRecordCount(size_t fileBytes) {
  return fileBytes < kHeaderBytes ? 0 : (fileBytes - kHeaderBytes) / kRecordBytes;
}

inline size_t trailingByteCount(size_t fileBytes) {
  return fileBytes < kHeaderBytes ? fileBytes : (fileBytes - kHeaderBytes) % kRecordBytes;
}

inline uint32_t timestampDeltaUs(uint32_t newer, uint32_t older) {
  return newer - older;  // Unsigned subtraction intentionally handles micros() rollover.
}

}  // namespace rkt4
}  // namespace green_ion

