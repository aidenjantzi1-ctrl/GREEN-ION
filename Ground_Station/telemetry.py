"""GREEN ION V12 telemetry codec and state classification."""

from __future__ import annotations

from dataclasses import dataclass
import struct
import zlib

MAGIC = b"GI12"
VERSION = 2
MESSAGE_TELEMETRY = 1
GPS_CURRENT_MAX_AGE_MS = 3000
PACKET = struct.Struct("<4sBBHIIiiiIiHHhhhhhhHIBBHHHI")
PACKET_BYTES = 70
PAYLOAD_BYTES = 62

FLAG_GPS_HAS_POSITION = 1 << 0
FLAG_GPS_CURRENT = 1 << 1


@dataclass(frozen=True)
class Telemetry:
    sequence: int
    uptime_ms: int
    latitude: float | None
    longitude: float | None
    altitude_m: float
    pressure_pa: int
    gps_altitude_m: float
    speed_mps: float
    acceleration_g: float
    acceleration_x_g: float
    acceleration_y_g: float
    acceleration_z_g: float
    rotation_x_dps: float
    rotation_y_dps: float
    rotation_z_dps: float
    battery_v: float
    gps_fix_age_ms: int
    satellites: int
    flight_state: int
    flags: int
    health: int

    @property
    def gps_status(self) -> str:
        if not self.flags & FLAG_GPS_HAS_POSITION:
            return "missing"
        if self.flags & FLAG_GPS_CURRENT and self.gps_fix_age_ms <= GPS_CURRENT_MAX_AGE_MS:
            return "live"
        return "stale"


def decode_packet(data: bytes) -> Telemetry:
    if len(data) != PACKET_BYTES:
        raise ValueError(f"bad packet size: {len(data)}")
    values = PACKET.unpack(data)
    (magic, version, message_type, payload_bytes, sequence, uptime_ms,
     latitude_e7, longitude_e7, altitude_cm, pressure_pa, gps_altitude_cm,
     speed_cms, acceleration_mg, acceleration_x_mg, acceleration_y_mg,
     acceleration_z_mg, rotation_x_dps10, rotation_y_dps10,
     rotation_z_dps10, battery_mv, fix_age_ms, satellites, flight_state,
     flags, health, _reserved, crc) = values
    if magic != MAGIC:
        raise ValueError("bad magic")
    if version != VERSION:
        raise ValueError("unsupported version")
    if message_type != MESSAGE_TELEMETRY or payload_bytes != PAYLOAD_BYTES:
        raise ValueError("bad message layout")
    if zlib.crc32(data[:-4]) & 0xFFFFFFFF != crc:
        raise ValueError("bad crc")
    has_position = bool(flags & FLAG_GPS_HAS_POSITION)
    return Telemetry(
        sequence=sequence,
        uptime_ms=uptime_ms,
        latitude=latitude_e7 / 1e7 if has_position else None,
        longitude=longitude_e7 / 1e7 if has_position else None,
        altitude_m=altitude_cm / 100,
        pressure_pa=pressure_pa,
        gps_altitude_m=gps_altitude_cm / 100,
        speed_mps=speed_cms / 100,
        acceleration_g=acceleration_mg / 1000,
        acceleration_x_g=acceleration_x_mg / 1000,
        acceleration_y_g=acceleration_y_mg / 1000,
        acceleration_z_g=acceleration_z_mg / 1000,
        rotation_x_dps=rotation_x_dps10 / 10,
        rotation_y_dps=rotation_y_dps10 / 10,
        rotation_z_dps=rotation_z_dps10 / 10,
        battery_v=battery_mv / 1000,
        gps_fix_age_ms=fix_age_ms,
        satellites=satellites,
        flight_state=flight_state,
        flags=flags,
        health=health,
    )


def encode_packet(item: Telemetry) -> bytes:
    flags = item.flags
    latitude_e7 = round(item.latitude * 1e7) if item.latitude is not None else 0
    longitude_e7 = round(item.longitude * 1e7) if item.longitude is not None else 0
    if item.latitude is None or item.longitude is None:
        flags &= ~(FLAG_GPS_HAS_POSITION | FLAG_GPS_CURRENT)
    def i16(value: float) -> int:
        return max(-32768, min(32767, round(value)))

    without_crc = PACKET.pack(
        MAGIC, VERSION, MESSAGE_TELEMETRY, PAYLOAD_BYTES, item.sequence,
        item.uptime_ms, latitude_e7, longitude_e7, round(item.altitude_m * 100),
        max(0, min(0xFFFFFFFF, round(item.pressure_pa))),
        round(item.gps_altitude_m * 100),
        max(0, min(65535, round(item.speed_mps * 100))),
        max(0, min(65535, round(item.acceleration_g * 1000))),
        i16(item.acceleration_x_g * 1000),
        i16(item.acceleration_y_g * 1000),
        i16(item.acceleration_z_g * 1000),
        i16(item.rotation_x_dps * 10),
        i16(item.rotation_y_dps * 10),
        i16(item.rotation_z_dps * 10),
        max(0, min(65535, round(item.battery_v * 1000))),
        item.gps_fix_age_ms, item.satellites, item.flight_state, flags,
        item.health, 0, 0,
    )
    crc = zlib.crc32(without_crc[:-4]) & 0xFFFFFFFF
    return without_crc[:-4] + struct.pack("<I", crc)

