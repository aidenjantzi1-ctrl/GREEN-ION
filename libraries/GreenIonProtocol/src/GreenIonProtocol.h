#pragma once

#include <stdint.h>
#include <stddef.h>
#include <string.h>

namespace green_ion {

static const uint8_t kProtocolVersion = 2;
static const uint8_t kMessageTelemetry = 1;
static const uint16_t kPayloadBytes = 62;
static const uint32_t kGpsCurrentMaxAgeMs = 3000;
static const uint16_t kBenchRadioMarker = 0xB12Cu;

enum TelemetryFlags : uint16_t {
  FLAG_GPS_HAS_POSITION = 1u << 0,
  FLAG_GPS_CURRENT = 1u << 1,
  FLAG_SD_OK = 1u << 2,
  FLAG_IMU_OK = 1u << 3,
  FLAG_BARO_OK = 1u << 4,
  FLAG_RADIO_OK = 1u << 5,
  FLAG_RECORDING = 1u << 6,
  FLAG_BUZZER_ACTIVE = 1u << 7,
};

enum HealthBits : uint16_t {
  HEALTH_SD_WRITE_ERROR = 1u << 0,
  HEALTH_GPS_INIT_ERROR = 1u << 1,
  HEALTH_IMU_INIT_ERROR = 1u << 2,
  HEALTH_BARO_INIT_ERROR = 1u << 3,
  HEALTH_RADIO_INIT_ERROR = 1u << 4,
  HEALTH_IMU_READ_ERROR = 1u << 5,
  HEALTH_RADIO_SEND_ERROR = 1u << 6,
  HEALTH_BARO_READ_ERROR = 1u << 7,
  // V16.2 adds backward-compatible diagnostic bits. The 70-byte packet and
  // protocol version remain unchanged; older receivers may safely ignore them.
  HEALTH_SD_INIT_ERROR = 1u << 8,
  HEALTH_GPS_RUNTIME_ERROR = 1u << 9,
  HEALTH_IMU_RUNTIME_LOST = 1u << 10,
  HEALTH_SENSOR_LOG_DROPPED = 1u << 11,
};

enum FlightState : uint8_t {
  STATE_BOOT = 0,
  STATE_IDLE = 1,
  STATE_ARMED = 2,
  STATE_ASCENT = 3,
  STATE_COAST = 4,
  STATE_DESCENT = 5,
  STATE_LANDED = 6,
  STATE_RECOVERY = 7,
};

#pragma pack(push, 1)
struct TelemetryPacket {
  char magic[4];                 // "GI12"
  uint8_t version;
  uint8_t messageType;
  uint16_t payloadBytes;
  uint32_t sequence;
  uint32_t uptimeMs;
  int32_t latitudeE7;           // last valid position, degrees * 1e7
  int32_t longitudeE7;
  int32_t altitudeCm;           // barometric altitude
  uint32_t pressurePa;
  int32_t gpsAltitudeCm;
  uint16_t speedCms;
  uint16_t accelerationMg;      // magnitude, milli-g
  int16_t accelerationXMg;
  int16_t accelerationYMg;
  int16_t accelerationZMg;
  int16_t rotationXDps10;       // angular rate, 0.1 degree/second
  int16_t rotationYDps10;
  int16_t rotationZDps10;
  uint16_t batteryMv;
  uint32_t gpsFixAgeMs;
  uint8_t satellites;
  uint8_t flightState;
  uint16_t flags;
  uint16_t health;
  uint16_t reserved;
  uint32_t crc32;
};
#pragma pack(pop)

static_assert(sizeof(TelemetryPacket) == 70, "Telemetry packet layout changed");

enum ValidationResult : uint8_t {
  VALID = 0,
  BAD_SIZE,
  BAD_MAGIC,
  BAD_VERSION,
  BAD_TYPE,
  BAD_LENGTH,
  BAD_CRC,
};

inline uint32_t crc32(const uint8_t *data, size_t length) {
  uint32_t crc = 0xFFFFFFFFu;
  for (size_t i = 0; i < length; ++i) {
    crc ^= data[i];
    for (uint8_t bit = 0; bit < 8; ++bit) {
      crc = (crc >> 1) ^ (0xEDB88320u & (uint32_t)(-(int32_t)(crc & 1u)));
    }
  }
  return ~crc;
}

inline void initialize(TelemetryPacket &packet) {
  memset(&packet, 0, sizeof(packet));
  memcpy(packet.magic, "GI12", 4);
  packet.version = kProtocolVersion;
  packet.messageType = kMessageTelemetry;
  packet.payloadBytes = kPayloadBytes;
}

inline void seal(TelemetryPacket &packet) {
  packet.crc32 = crc32(reinterpret_cast<const uint8_t *>(&packet),
                       offsetof(TelemetryPacket, crc32));
}

inline ValidationResult validate(const uint8_t *data, size_t length) {
  if (length != sizeof(TelemetryPacket)) return BAD_SIZE;
  const TelemetryPacket *packet = reinterpret_cast<const TelemetryPacket *>(data);
  if (memcmp(packet->magic, "GI12", 4) != 0) return BAD_MAGIC;
  if (packet->version != kProtocolVersion) return BAD_VERSION;
  if (packet->messageType != kMessageTelemetry) return BAD_TYPE;
  if (packet->payloadBytes != kPayloadBytes) return BAD_LENGTH;
  const uint32_t expected = crc32(data, offsetof(TelemetryPacket, crc32));
  return packet->crc32 == expected ? VALID : BAD_CRC;
}

inline bool hasPosition(const TelemetryPacket &packet) {
  return (packet.flags & FLAG_GPS_HAS_POSITION) != 0;
}

inline bool hasCurrentPosition(const TelemetryPacket &packet) {
  return hasPosition(packet) && (packet.flags & FLAG_GPS_CURRENT) != 0 &&
         packet.gpsFixAgeMs <= kGpsCurrentMaxAgeMs;
}

inline bool isBenchRadioPacket(const TelemetryPacket &packet) {
  return packet.reserved == kBenchRadioMarker;
}

}  // namespace green_ion

