"""Validate and export GREEN ION V12 GIS1 companion logs.

This tool is read-only. It never modifies the .SEN or matching RKT4 .BIN file.
"""

from __future__ import annotations

import argparse
import csv
from dataclasses import asdict, dataclass
import json
import math
from pathlib import Path
import re
import struct
import sys
import zlib

HEADER_BYTES = 32
RECORD_BYTES = 68
VERSION = 1
ALLOWED_RATES = {10, 25, 50}
FLAG_IMU_VALID = 1 << 0
FLAG_GPS_HAS_POSITION = 1 << 1
FLAG_GPS_CURRENT = 1 << 2
KNOWN_FLAGS = FLAG_IMU_VALID | FLAG_GPS_HAS_POSITION | FLAG_GPS_CURRENT
UINT32_MAX = 0xFFFFFFFF
RECORD = struct.Struct("<II6fii2fIIHBBBB2xI")
CSV_FIELDS = (
    "timestamp_us", "sequence", "imu_status",
    "acceleration_x_mps2", "acceleration_y_mps2", "acceleration_z_mps2",
    "gyro_x_rad_s", "gyro_y_rad_s", "gyro_z_rad_s", "gps_status",
    "latitude_deg", "longitude_deg", "gps_altitude_m", "gps_speed_mps",
    "gps_fix_age_ms", "last_gps_fix_uptime_ms", "satellites", "gps_utc",
)


class SensorLogError(ValueError):
    pass


@dataclass(frozen=True)
class Header:
    sample_rate_hz: int
    flight_sequence: int
    start_timestamp_us: int
    rkt4_header_crc32: int


@dataclass(frozen=True)
class Inspection:
    status: str
    file_bytes: int
    complete_records: int
    valid_prefix_records: int
    trailing_bytes: int
    first_invalid_record: int | None
    first_invalid_reason: str | None
    sequence_gaps: int


def _crc32(data: bytes) -> int:
    return zlib.crc32(data) & UINT32_MAX


def decode_header(data: bytes, expected_sequence: int | None = None) -> Header:
    if len(data) < HEADER_BYTES:
        raise SensorLogError("short_header")
    if data[:4] != b"GIS1":
        raise SensorLogError("bad_magic")
    version, header_bytes, record_bytes, rate, sequence = struct.unpack_from(
        "<HHHHH", data, 4
    )
    if version != VERSION:
        raise SensorLogError("bad_version")
    if header_bytes != HEADER_BYTES or record_bytes != RECORD_BYTES:
        raise SensorLogError("bad_layout")
    if rate not in ALLOWED_RATES or sequence > 999:
        raise SensorLogError("bad_header_fields")
    if expected_sequence is not None and sequence != expected_sequence:
        raise SensorLogError("filename_sequence_mismatch")
    if struct.unpack_from("<I", data, 28)[0] != _crc32(data[:28]):
        raise SensorLogError("bad_header_crc")
    return Header(
        sample_rate_hz=rate,
        flight_sequence=sequence,
        start_timestamp_us=struct.unpack_from("<I", data, 16)[0],
        rkt4_header_crc32=struct.unpack_from("<I", data, 20)[0],
    )


def _decode_record(data: bytes) -> dict:
    if len(data) != RECORD_BYTES:
        raise SensorLogError("short_record")
    if struct.unpack_from("<I", data, 64)[0] != _crc32(data[:64]):
        raise SensorLogError("bad_record_crc")
    values = RECORD.unpack(data)
    (
        timestamp_us, sequence, ax, ay, az, gx, gy, gz,
        latitude_e7, longitude_e7, gps_altitude_m, gps_speed_mps,
        gps_fix_age_ms, last_gps_fix_uptime_ms, flags, satellites,
        gps_hour, gps_minute, gps_second, _crc,
    ) = values
    if flags & ~KNOWN_FLAGS:
        raise SensorLogError("bad_record_flags")
    imu_valid = bool(flags & FLAG_IMU_VALID)
    gps_has_position = bool(flags & FLAG_GPS_HAS_POSITION)
    gps_current = bool(flags & FLAG_GPS_CURRENT)
    if gps_current and not gps_has_position:
        raise SensorLogError("current_without_position")
    if satellites > 64 or gps_hour > 23 or gps_minute > 59 or gps_second > 60:
        raise SensorLogError("bad_gps_fields")
    if imu_valid and not all(math.isfinite(value) for value in (ax, ay, az, gx, gy, gz)):
        raise SensorLogError("nonfinite_valid_imu")
    if gps_has_position:
        if not (-900000000 <= latitude_e7 <= 900000000):
            raise SensorLogError("bad_latitude")
        if not (-1800000000 <= longitude_e7 <= 1800000000):
            raise SensorLogError("bad_longitude")
        if not math.isfinite(gps_altitude_m) or not math.isfinite(gps_speed_mps):
            raise SensorLogError("nonfinite_gps")
    elif (
        latitude_e7 != 0 or longitude_e7 != 0 or gps_altitude_m != 0.0
        or gps_speed_mps != 0.0 or last_gps_fix_uptime_ms != 0
        or gps_fix_age_ms != UINT32_MAX
    ):
        raise SensorLogError("unlabeled_last_position")
    return {
        "timestamp_us": timestamp_us,
        "sequence": sequence,
        "acceleration": (ax, ay, az),
        "gyro": (gx, gy, gz),
        "imu_valid": imu_valid,
        "gps_has_position": gps_has_position,
        "gps_current": gps_current,
        "latitude_e7": latitude_e7,
        "longitude_e7": longitude_e7,
        "gps_altitude_m": gps_altitude_m,
        "gps_speed_mps": gps_speed_mps,
        "gps_fix_age_ms": gps_fix_age_ms,
        "last_gps_fix_uptime_ms": last_gps_fix_uptime_ms,
        "satellites": satellites,
        "gps_time": (gps_hour, gps_minute, gps_second),
    }


def parse_sensor_log(
    data: bytes, expected_sequence: int | None = None
) -> tuple[Header, list[dict], Inspection]:
    header = decode_header(data, expected_sequence)
    payload_bytes = max(0, len(data) - HEADER_BYTES)
    complete_records, trailing_bytes = divmod(payload_bytes, RECORD_BYTES)
    rows: list[dict] = []
    invalid_index: int | None = None
    invalid_reason: str | None = None
    sequence_gaps = 0
    previous_sequence: int | None = None
    for index in range(complete_records):
        start = HEADER_BYTES + index * RECORD_BYTES
        try:
            record = _decode_record(data[start : start + RECORD_BYTES])
        except SensorLogError as error:
            invalid_index = index
            invalid_reason = str(error)
            break
        if previous_sequence is not None and record["sequence"] != ((previous_sequence + 1) & UINT32_MAX):
            sequence_gaps += 1
        previous_sequence = record["sequence"]
        rows.append(record)
    status = "corrupt" if invalid_index is not None else (
        "torn_tail" if trailing_bytes else "valid"
    )
    return header, rows, Inspection(
        status=status,
        file_bytes=len(data),
        complete_records=complete_records,
        valid_prefix_records=len(rows),
        trailing_bytes=trailing_bytes,
        first_invalid_record=invalid_index,
        first_invalid_reason=invalid_reason,
        sequence_gaps=sequence_gaps,
    )


def iter_valid_prefix_records(path: Path, expected_sequence: int, count: int):
    """Read only an inspected GIS1 valid prefix, retaining one record at a time."""
    with path.open("rb") as handle:
        decode_header(handle.read(HEADER_BYTES), expected_sequence)
        for _ in range(count):
            raw = handle.read(RECORD_BYTES)
            if len(raw) != RECORD_BYTES:
                raise SensorLogError("sensor log became shorter than its inspected valid prefix")
            yield _decode_record(raw)


def validate_rkt4_binding(header: Header, rkt4_data: bytes) -> bool:
    if len(rkt4_data) < 32 or rkt4_data[:4] != b"RKT4":
        return False
    version, header_bytes, record_bytes, rate = struct.unpack_from("<HHHH", rkt4_data, 4)
    return (
        version == 2 and header_bytes == 32 and record_bytes == 16
        and rate == header.sample_rate_hz
        and header.rkt4_header_crc32 == _crc32(rkt4_data[:32])
    )


def _number(value: float) -> str:
    return format(value, ".9g")


def csv_row(record: dict) -> dict[str, str | int]:
    imu_valid = record["imu_valid"]
    gps_has_position = record["gps_has_position"]
    gps_status = "live" if record["gps_current"] else (
        "stale" if gps_has_position else "missing"
    )
    acceleration = record["acceleration"]
    gyro = record["gyro"]
    hour, minute, second = record["gps_time"]
    return {
        "timestamp_us": record["timestamp_us"],
        "sequence": record["sequence"],
        "imu_status": "valid" if imu_valid else "missing",
        "acceleration_x_mps2": _number(acceleration[0]) if imu_valid else "",
        "acceleration_y_mps2": _number(acceleration[1]) if imu_valid else "",
        "acceleration_z_mps2": _number(acceleration[2]) if imu_valid else "",
        "gyro_x_rad_s": _number(gyro[0]) if imu_valid else "",
        "gyro_y_rad_s": _number(gyro[1]) if imu_valid else "",
        "gyro_z_rad_s": _number(gyro[2]) if imu_valid else "",
        "gps_status": gps_status,
        "latitude_deg": f"{record['latitude_e7'] / 1e7:.7f}" if gps_has_position else "",
        "longitude_deg": f"{record['longitude_e7'] / 1e7:.7f}" if gps_has_position else "",
        "gps_altitude_m": _number(record["gps_altitude_m"]) if gps_has_position else "",
        "gps_speed_mps": _number(record["gps_speed_mps"]) if gps_has_position else "",
        "gps_fix_age_ms": record["gps_fix_age_ms"] if gps_has_position else "",
        "last_gps_fix_uptime_ms": record["last_gps_fix_uptime_ms"] if gps_has_position else "",
        "satellites": record["satellites"],
        "gps_utc": f"{hour:02d}:{minute:02d}:{second:02d}",
    }


def write_csv(records: list[dict], output: Path) -> None:
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=CSV_FIELDS)
        writer.writeheader()
        writer.writerows(csv_row(record) for record in records)


def _sequence_from_name(path: Path) -> int | None:
    match = re.fullmatch(r"(?:V12D|AIRP|TEST)(\d{3})\.SEN", path.name, re.IGNORECASE)
    return int(match.group(1)) if match else None


def _rkt4_sequence_from_name(path: Path) -> int | None:
    match = re.fullmatch(r"(?:V12D|AIRP|TEST)(\d{3})\.BIN", path.name, re.IGNORECASE)
    return int(match.group(1)) if match else None


def main() -> int:
    parser = argparse.ArgumentParser(description="Validate and export a GREEN ION GIS1 sensor log")
    parser.add_argument("sensor_log", type=Path, help="V12Dxxx.SEN, AIRPxxx.SEN or TESTxxx.SEN input")
    parser.add_argument("--rkt4", type=Path, help="matching V12Dxxx.BIN, AIRPxxx.BIN or TESTxxx.BIN")
    parser.add_argument("--csv", type=Path, help="output CSV path; defaults beside the input")
    parser.add_argument("--json", type=Path, help="write integrity/header summary as JSON")
    parser.add_argument("--allow-partial", action="store_true", help="export the valid prefix of corrupt or torn input")
    args = parser.parse_args()
    try:
        header, records, inspection = parse_sensor_log(
            args.sensor_log.read_bytes(), _sequence_from_name(args.sensor_log)
        )
        binding = "not_checked"
        if args.rkt4:
            rkt4_sequence = _rkt4_sequence_from_name(args.rkt4)
            binding_valid = validate_rkt4_binding(header, args.rkt4.read_bytes())
            if rkt4_sequence is not None and rkt4_sequence != header.flight_sequence:
                binding_valid = False
            binding = "valid" if binding_valid else "invalid"
        summary = {"header": asdict(header), "inspection": asdict(inspection), "rkt4_binding": binding}
        print(json.dumps(summary, indent=2, sort_keys=True))
        if args.json:
            payload = dict(summary)
            payload["records"] = [csv_row(record) for record in records]
            args.json.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        if binding == "invalid":
            raise SensorLogError("rkt4_binding_mismatch")
        if inspection.status != "valid" and not args.allow_partial:
            raise SensorLogError("partial_export_requires_--allow-partial")
        output = args.csv or args.sensor_log.with_suffix(".csv")
        write_csv(records, output)
        print(f"CSV: {output}")
        return 0
    except (OSError, SensorLogError) as error:
        print(f"GIS1 export failed: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
