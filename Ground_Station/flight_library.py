"""Validated, read-only RKT4 v2 import and local analysis library."""

from __future__ import annotations

import csv
from bisect import bisect_left
from datetime import date, datetime, timezone
import hashlib
from html import escape
import io
import json
import math
import os
from pathlib import Path
import re
import shutil
import struct
import tempfile
import threading
import uuid
from typing import BinaryIO

from sensor_log import SensorLogError, csv_row, parse_sensor_log, validate_rkt4_binding, CSV_FIELDS

HEADER_BYTES = 32
RECORD_BYTES = 16
ALLOWED_RATES = (10, 25, 50)
SAFE_NAME = re.compile(r"^[A-Za-z0-9][A-Za-z0-9_.-]{0,63}\.BIN$", re.IGNORECASE)
FLIGHT_STEM = re.compile(r"^(?:V12D|AIRP|TEST)[0-9]{3}$", re.IGNORECASE)
MAX_SENSOR_BYTES = 16 * 1024 * 1024
EXPONENT = 0.190294957


class FlightImportError(ValueError):
    pass


def file_sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        while chunk := handle.read(65536):
            digest.update(chunk)
    return digest.hexdigest()


def pressure_altitude(pressure_pa: float, baseline_pa: float) -> float | None:
    if not math.isfinite(pressure_pa) or not math.isfinite(baseline_pa) or pressure_pa <= 0 or baseline_pa <= 0:
        return None
    return 44330.0 * (1.0 - (pressure_pa / baseline_pa) ** EXPONENT)


def inspect_rkt4(path: Path) -> dict:
    size = path.stat().st_size
    if size < HEADER_BYTES:
        raise FlightImportError("RKT4 file is shorter than its 32-byte header")
    with path.open("rb") as handle:
        header = handle.read(HEADER_BYTES)
        if header[:4] != b"RKT4":
            raise FlightImportError("RKT4 magic is missing")
        version, header_bytes, record_bytes, rate = struct.unpack_from("<HHHH", header, 4)
        baseline_pa = struct.unpack_from("<f", header, 12)[0]
        start_battery_mv = struct.unpack_from("<H", header, 16)[0]
        if version != 2 or header_bytes != HEADER_BYTES or record_bytes != RECORD_BYTES:
            raise FlightImportError("unsupported RKT4 layout")
        if rate not in ALLOWED_RATES:
            raise FlightImportError("unsupported RKT4 sample rate")
        if not math.isfinite(baseline_pa) or baseline_pa <= 0:
            raise FlightImportError("invalid RKT4 baseline pressure")
        complete_records = (size - HEADER_BYTES) // RECORD_BYTES
        trailing_bytes = (size - HEADER_BYTES) % RECORD_BYTES
        first_timestamp = last_timestamp = previous_timestamp = None
        peak_altitude = 0.0
        invalid_records = timestamp_gaps = 0
        end_battery_mv = start_battery_mv
        for _ in range(complete_records):
            raw = handle.read(RECORD_BYTES)
            timestamp, pressure, temperature, battery_mv, valid = struct.unpack("<IffHH", raw)
            if first_timestamp is None:
                first_timestamp = timestamp
            if previous_timestamp is not None:
                delta = (timestamp - previous_timestamp) & 0xFFFFFFFF
                if delta > round(4_000_000 / rate):
                    timestamp_gaps += 1
            previous_timestamp = last_timestamp = timestamp
            end_battery_mv = battery_mv
            if valid == 1 and math.isfinite(pressure) and pressure > 0 and math.isfinite(temperature):
                altitude = pressure_altitude(pressure, baseline_pa)
                if altitude is not None:
                    peak_altitude = max(peak_altitude, altitude)
            else:
                invalid_records += 1
    duration_s = 0.0 if first_timestamp is None else ((last_timestamp - first_timestamp) & 0xFFFFFFFF) / 1_000_000
    warnings = []
    if trailing_bytes:
        warnings.append(f"Interrupted or truncated tail: {trailing_bytes} byte(s)")
    if invalid_records:
        warnings.append(f"Invalid sensor records: {invalid_records}")
    if timestamp_gaps:
        warnings.append(f"Timestamp gaps: {timestamp_gaps}")
    return {
        "format": "RKT4", "version": version, "bytes": size, "sample_rate_hz": rate,
        "baseline_pressure_pa": baseline_pa, "start_battery_v": start_battery_mv / 1000,
        "end_battery_v": end_battery_mv / 1000, "records": complete_records,
        "trailing_bytes": trailing_bytes, "invalid_records": invalid_records,
        "timestamp_gaps": timestamp_gaps, "duration_s": duration_s,
        "peak_altitude_m_estimated": peak_altitude, "warnings": warnings,
        "analysis_version": 1,
        "disclaimer": "Altitude and derived motion values are pressure-derived estimates.",
    }


def iter_records(path: Path):
    with path.open("rb") as handle:
        header = handle.read(HEADER_BYTES)
        baseline = struct.unpack_from("<f", header, 12)[0]
        index = 0
        while len(raw := handle.read(RECORD_BYTES)) == RECORD_BYTES:
            timestamp, pressure, temperature, battery_mv, valid = struct.unpack("<IffHH", raw)
            sample_valid = valid == 1 and math.isfinite(pressure) and pressure > 0 and math.isfinite(temperature)
            yield {
                "index": index, "timestamp_us": timestamp, "time_s": timestamp / 1_000_000,
                "pressure_pa": pressure if math.isfinite(pressure) else None,
                "temperature_c": temperature if math.isfinite(temperature) else None,
                "battery_v": battery_mv / 1000, "valid": sample_valid,
                "altitude_m_estimated": pressure_altitude(pressure, baseline) if sample_valid else None,
            }
            index += 1


class FlightLibrary:
    def __init__(self, root: Path) -> None:
        self.root = root
        self.raw = root / "raw"
        self.meta = root / "metadata"
        self.raw.mkdir(parents=True, exist_ok=True)
        self.meta.mkdir(parents=True, exist_ok=True)
        self._lock = threading.RLock()

    def _metadata_path(self, identifier: str) -> Path:
        if not re.fullmatch(r"[0-9a-f]{64}", identifier):
            raise FlightImportError("invalid flight identifier")
        return self.meta / f"{identifier}.json"

    def _save_metadata(self, identifier: str, metadata: dict) -> None:
        with tempfile.NamedTemporaryFile(mode="w", encoding="utf-8", dir=self.meta,
                                         prefix="flight-meta-", suffix=".part", delete=False) as output:
            temporary = Path(output.name)
            try:
                json.dump(metadata, output, indent=2, allow_nan=False)
                output.flush()
                os.fsync(output.fileno())
            except Exception:
                temporary.unlink(missing_ok=True)
                raise
        temporary.replace(self._metadata_path(identifier))

    def import_stream(self, filename: str, source: BinaryIO, expected_bytes: int | None = None,
                      source_kind: str = "manual_upload") -> dict:
        if source_kind not in {"manual_upload", "feather_sd", "bench_verification"}:
            raise FlightImportError("invalid flight import source")
        with self._lock:
            return self._import_stream(filename, source, expected_bytes, source_kind)

    def _import_stream(self, filename: str, source: BinaryIO, expected_bytes: int | None,
                       source_kind: str) -> dict:
        if Path(filename).name != filename:
            raise FlightImportError("filename must not contain a directory")
        if not SAFE_NAME.fullmatch(filename):
            raise FlightImportError("filename must be a simple .BIN name")
        fd, temporary_name = tempfile.mkstemp(prefix="flight-", suffix=".part", dir=self.root)
        os.close(fd)
        temporary = Path(temporary_name)
        digest = hashlib.sha256()
        received = 0
        try:
            with temporary.open("wb") as output:
                while chunk := source.read(65536):
                    received += len(chunk)
                    if received > 256 * 1024 * 1024:
                        raise FlightImportError("file exceeds the 256 MB safety limit")
                    output.write(chunk)
                    digest.update(chunk)
                output.flush()
                os.fsync(output.fileno())
            if expected_bytes is not None and received != expected_bytes:
                raise FlightImportError(f"incomplete transfer: expected {expected_bytes} bytes, received {received}")
            summary = inspect_rkt4(temporary)
            identifier = digest.hexdigest()
            destination = self.raw / f"{identifier}.bin"
            duplicate = destination.exists()
            if duplicate:
                if file_sha256(destination) != identifier:
                    raise FlightImportError("stored raw flight checksum does not match its identity")
                temporary.unlink()
            else:
                temporary.replace(destination)
            metadata_path = self._metadata_path(identifier)
            metadata = {
                "id": identifier, "original_name": filename, "sha256": identifier,
                "raw_file": destination.name, "summary": summary,
                "imported_utc": datetime.now(timezone.utc).isoformat(),
                "source": source_kind, "label": "", "rocket_profile": "", "flight_date": None,
            }
            if not metadata_path.exists():
                self._save_metadata(identifier, metadata)
            else:
                metadata = json.loads(metadata_path.read_text(encoding="utf-8"))
            metadata["duplicate"] = duplicate
            return metadata
        except Exception:
            temporary.unlink(missing_ok=True)
            raise

    def list(self) -> list[dict]:
        result = []
        for path in sorted(self.meta.glob("*.json"), key=lambda item: item.lstat().st_mtime, reverse=True):
            try:
                if path.is_symlink():
                    continue
                item = json.loads(path.read_text(encoding="utf-8"))
                if (not isinstance(item, dict) or item.get("id") != path.stem or
                        item.get("sha256") != path.stem or item.get("raw_file") != f"{path.stem}.bin" or
                        not isinstance(item.get("summary"), dict)):
                    continue
                result.append(item)
            except (OSError, json.JSONDecodeError):
                continue
        return result

    def get(self, identifier: str) -> tuple[dict, Path]:
        metadata_path = self._metadata_path(identifier)
        if metadata_path.is_symlink():
            raise FlightImportError("stored flight metadata is a symlink")
        metadata = json.loads(metadata_path.read_text(encoding="utf-8"))
        if (not isinstance(metadata, dict) or metadata.get("id") != identifier or
                metadata.get("sha256") != identifier or metadata.get("raw_file") != f"{identifier}.bin" or
                not isinstance(metadata.get("original_name"), str) or
                not isinstance(metadata.get("summary"), dict)):
            raise FlightImportError("stored flight metadata no longer matches its raw recording")
        raw = (self.raw / metadata["raw_file"]).resolve()
        if raw.parent != self.raw.resolve() or not raw.is_file():
            raise FlightImportError("raw flight file is unavailable")
        if file_sha256(raw) != identifier:
            raise FlightImportError("raw flight checksum no longer matches import metadata")
        return metadata, raw

    def update_details(self, identifier: str, label: str, rocket_profile: str,
                       flight_date: str | None) -> dict:
        if not isinstance(label, str) or not isinstance(rocket_profile, str):
            raise FlightImportError("label and rocket profile must be text")
        if len(label) > 80 or len(rocket_profile) > 60 or any(ord(char) < 32 for char in label + rocket_profile):
            raise FlightImportError("flight label or rocket profile is too long or contains a control character")
        if flight_date is not None:
            if not isinstance(flight_date, str):
                raise FlightImportError("flight date must be YYYY-MM-DD or empty")
            if flight_date == "":
                flight_date = None
            else:
                if not re.fullmatch(r"\d{4}-\d{2}-\d{2}", flight_date):
                    raise FlightImportError("flight date must be YYYY-MM-DD or empty")
                try:
                    flight_date = date.fromisoformat(flight_date).isoformat()
                except ValueError as error:
                    raise FlightImportError("flight date must be YYYY-MM-DD or empty") from error
        with self._lock:
            metadata, _raw = self.get(identifier)
            metadata.update(label=label.strip(), rocket_profile=rocket_profile.strip(),
                            flight_date=flight_date)
            self._save_metadata(identifier, metadata)
            return metadata

    def trash_flight(self, identifier: str) -> dict:
        """Hide a laptop copy only after a verified copy is safely in local Trash."""
        with self._lock:
            self._metadata_path(identifier)  # validate identifier
            trash_root = self.root / "trash"
            trash_root.mkdir(exist_ok=True)
            destination = trash_root / identifier
            if destination.exists():
                raise FlightImportError("this flight already has a Trash copy; restore it first")
            metadata, raw = self.get(identifier)
            sensor_path = self.get_sensor(identifier)[1] if metadata.get("sensor") else None
            stage = self.root / f".trash-stage-{uuid.uuid4().hex}"
            try:
                staged = FlightLibrary(stage)
                shutil.copy2(raw, staged.raw / raw.name)
                shutil.copy2(self._metadata_path(identifier), staged._metadata_path(identifier))
                if sensor_path:
                    shutil.copy2(sensor_path, staged.raw / sensor_path.name)
                staged.get(identifier)
                if sensor_path:
                    staged.get_sensor(identifier)
                stage.replace(destination)
            except Exception:
                shutil.rmtree(stage, ignore_errors=True)
                raise
            self._metadata_path(identifier).unlink()
            raw.unlink()
            if sensor_path:
                shared = any((item.get("sensor") or {}).get("sha256") == metadata["sensor"]["sha256"]
                             for item in self.list())
                if not shared:
                    sensor_path.unlink(missing_ok=True)
            return {"id": identifier, "original_name": metadata["original_name"],
                    "trash_location": f"flights/trash/{identifier}", "feather_sd_unchanged": True}

    def list_trash(self) -> list[dict]:
        result = []
        for path in sorted((self.root / "trash").glob("*")):
            if not re.fullmatch(r"[0-9a-f]{64}", path.name) or not path.is_dir() or path.is_symlink():
                continue
            try:
                trashed = FlightLibrary(path)
                metadata, _raw = trashed.get(path.name)
                if metadata.get("sensor"):
                    trashed.get_sensor(path.name)
                result.append({"id": path.name, "original_name": metadata["original_name"],
                               "label": metadata.get("label") or ""})
            except (OSError, ValueError, KeyError, json.JSONDecodeError):
                result.append({"id": path.name, "original_name": "Unreadable Trash entry",
                               "label": "", "integrity": "invalid"})
        return result

    def restore_trash(self, identifier: str) -> dict:
        with self._lock:
            self._metadata_path(identifier)  # validate identifier before constructing a path
            trash_path = self.root / "trash" / identifier
            if not trash_path.is_dir() or trash_path.is_symlink():
                raise FlightImportError("flight Trash entry is unavailable")
            trashed = FlightLibrary(trash_path)
            metadata, raw = trashed.get(identifier)
            sensor_path = trashed.get_sensor(identifier)[1] if metadata.get("sensor") else None
            if self._metadata_path(identifier).exists():
                raise FlightImportError("active flight metadata already exists; Trash restore would conflict")
            copies = [(raw, self.raw / raw.name)]
            if sensor_path:
                copies.append((sensor_path, self.raw / sensor_path.name))
            copies.append((trashed._metadata_path(identifier), self._metadata_path(identifier)))
            for source, target in copies:
                if target.exists() and (target.is_symlink() or file_sha256(target) != file_sha256(source)):
                    raise FlightImportError(f"active file conflicts with Trash restore: {target.name}")
            created = []
            try:
                for source, target in copies:
                    if target.exists():
                        continue
                    with source.open("rb") as input_file, target.open("xb") as output:
                        created.append(target)
                        shutil.copyfileobj(input_file, output, length=65536)
                        output.flush()
                        os.fsync(output.fileno())
                self.get(identifier)
                if sensor_path:
                    self.get_sensor(identifier)
            except Exception:
                for target in created:
                    target.unlink(missing_ok=True)
                raise
            shutil.rmtree(trash_path)
            return {"id": identifier, "original_name": metadata["original_name"]}

    def import_sensor_stream(self, identifier: str, filename: str, source: BinaryIO,
                             expected_bytes: int | None = None) -> dict:
        with self._lock:
            return self._import_sensor_stream(identifier, filename, source, expected_bytes)

    def _import_sensor_stream(self, identifier: str, filename: str, source: BinaryIO,
                              expected_bytes: int | None) -> dict:
        metadata, raw = self.get(identifier)
        bin_stem = Path(metadata["original_name"]).stem
        if not FLIGHT_STEM.fullmatch(bin_stem) or filename.upper() != bin_stem.upper() + ".SEN":
            raise FlightImportError("sensor filename must match a V12D, AIRP or TEST flight BIN")
        if expected_bytes is not None and (expected_bytes < 0 or expected_bytes > MAX_SENSOR_BYTES):
            raise FlightImportError("sensor file exceeds the 16 MB safety limit")
        data = bytearray()
        while chunk := source.read(min(65536, MAX_SENSOR_BYTES + 1 - len(data))):
            data.extend(chunk)
            if len(data) > MAX_SENSOR_BYTES:
                raise FlightImportError("sensor file exceeds the 16 MB safety limit")
        if expected_bytes is not None and len(data) != expected_bytes:
            raise FlightImportError(f"incomplete sensor transfer: expected {expected_bytes} bytes, received {len(data)}")
        try:
            header, rows, inspection = parse_sensor_log(data, int(bin_stem[-3:]))
        except SensorLogError as error:
            raise FlightImportError(f"GIS1 sensor header invalid: {error}") from error
        with raw.open("rb") as handle:
            rkt4_header = handle.read(HEADER_BYTES)
            if not validate_rkt4_binding(header, rkt4_header):
                raise FlightImportError("sensor header does not bind to the RKT4 header and sample rate")
            record_count = (raw.stat().st_size - HEADER_BYTES) // RECORD_BYTES
            if record_count <= 0:
                raise FlightImportError("RKT4 has no samples for sensor time alignment")
            first_us = struct.unpack("<I", handle.read(4))[0]
            handle.seek(HEADER_BYTES + (record_count - 1) * RECORD_BYTES)
            last_us = struct.unpack("<I", handle.read(4))[0]
        span_us = (last_us - first_us) & 0xFFFFFFFF
        start_offset_us = (header.start_timestamp_us - first_us) & 0xFFFFFFFF
        if start_offset_us > span_us + 1_000_000:
            raise FlightImportError("sensor start time is outside the RKT4 recording window")
        if rows and ((rows[0]["timestamp_us"] - first_us) & 0xFFFFFFFF) > span_us + 1_000_000:
            raise FlightImportError("sensor samples are outside the RKT4 recording window")
        digest = hashlib.sha256(data).hexdigest()
        previous = metadata.get("sensor")
        if previous and previous.get("sha256") != digest:
            raise FlightImportError("this flight already has a different sensor companion")
        destination = self.raw / f"{digest}.sen"
        duplicate = destination.exists() and file_sha256(destination) == digest
        if not duplicate:
            with tempfile.NamedTemporaryFile(dir=self.raw, prefix="sensor-", suffix=".part", delete=False) as output:
                temporary = Path(output.name)
                try:
                    output.write(data)
                    output.flush()
                    os.fsync(output.fileno())
                except Exception:
                    temporary.unlink(missing_ok=True)
                    raise
            temporary.replace(destination)
        sensor = {
            "original_name": filename, "sha256": digest, "raw_file": destination.name,
            "bytes": len(data), "header": {
                "sample_rate_hz": header.sample_rate_hz,
                "flight_sequence": header.flight_sequence,
                "start_timestamp_us": header.start_timestamp_us,
                "rkt4_header_crc32": header.rkt4_header_crc32,
            },
            "inspection": {
                "status": inspection.status, "complete_records": inspection.complete_records,
                "valid_prefix_records": inspection.valid_prefix_records,
                "trailing_bytes": inspection.trailing_bytes,
                "first_invalid_record": inspection.first_invalid_record,
                "first_invalid_reason": inspection.first_invalid_reason,
                "sequence_gaps": inspection.sequence_gaps,
            },
            "rkt4_binding": "valid", "timestamp_alignment": "within_recording_window",
        }
        metadata["sensor"] = sensor
        self._save_metadata(identifier, metadata)
        return {**sensor, "duplicate": duplicate}

    def get_sensor(self, identifier: str) -> tuple[dict, Path]:
        metadata, _raw = self.get(identifier)
        sensor = metadata.get("sensor")
        if not isinstance(sensor, dict) or not re.fullmatch(r"[0-9a-f]{64}", str(sensor.get("sha256", ""))):
            raise FlightImportError("sensor companion is unavailable")
        path = (self.raw / f"{sensor['sha256']}.sen").resolve()
        if path.parent != self.raw.resolve() or not path.is_file():
            raise FlightImportError("raw sensor file is unavailable")
        if file_sha256(path) != sensor["sha256"]:
            raise FlightImportError("raw sensor checksum no longer matches import metadata")
        return sensor, path

    def sensor_csv_export(self, identifier: str) -> bytes:
        sensor, path = self.get_sensor(identifier)
        _header, rows, _inspection = parse_sensor_log(path.read_bytes(), sensor["header"]["flight_sequence"])
        text = io.StringIO(newline="")
        writer = csv.DictWriter(text, fieldnames=CSV_FIELDS)
        writer.writeheader()
        writer.writerows(csv_row(row) for row in rows)
        return text.getvalue().encode()

    def analysis_timeline(self, identifier: str, max_points: int = 1500) -> dict:
        if not 100 <= max_points <= 5000:
            raise FlightImportError("timeline point limit must be between 100 and 5000")
        metadata, raw = self.get(identifier)
        total = metadata["summary"]["records"]
        stride = max(1, math.ceil(total / max_points))
        first_us = None
        previous_valid = None
        peak_altitude = -math.inf
        peak_point = None
        peak_velocity = None
        points = []
        last_point = None
        for row in iter_records(raw):
            if first_us is None:
                first_us = row["timestamp_us"]
            offset_us = (row["timestamp_us"] - first_us) & 0xFFFFFFFF
            speed = None
            if row["valid"] and row["altitude_m_estimated"] is not None:
                if previous_valid is not None:
                    delta_us = (row["timestamp_us"] - previous_valid[0]) & 0xFFFFFFFF
                    if 0 < delta_us <= 4_000_000 / metadata["summary"]["sample_rate_hz"]:
                        speed = (row["altitude_m_estimated"] - previous_valid[1]) * 1_000_000 / delta_us
                        if speed > 0:
                            peak_velocity = max(peak_velocity if peak_velocity is not None else speed, speed)
                previous_valid = (row["timestamp_us"], row["altitude_m_estimated"])
            else:
                previous_valid = None
            point = {**row, "elapsed_s": offset_us / 1_000_000,
                     "vertical_speed_mps_estimated": speed}
            if row["altitude_m_estimated"] is not None and row["altitude_m_estimated"] > peak_altitude:
                peak_altitude = row["altitude_m_estimated"]
                peak_point = point
            if row["index"] % stride == 0:
                points.append(point)
            last_point = point
        if last_point is not None and (not points or points[-1]["index"] != last_point["index"]):
            points.append(last_point)
        if peak_point is not None and all(point["index"] != peak_point["index"] for point in points):
            points.append(peak_point)
            points.sort(key=lambda point: point["index"])
        sensor_status = "missing"
        sensor_gaps = 0
        peak_acceleration_g = None
        peak_gps_altitude_m = None
        last_current_gps_fix = None
        if metadata.get("sensor"):
            sensor, sensor_path = self.get_sensor(identifier)
            header, rows, inspection = parse_sensor_log(sensor_path.read_bytes(),
                                                        sensor["header"]["flight_sequence"])
            with raw.open("rb") as handle:
                if not validate_rkt4_binding(header, handle.read(HEADER_BYTES)):
                    raise FlightImportError("stored sensor companion no longer matches this RKT4 flight")
            sensor_status = inspection.status
            sensor_gaps = inspection.sequence_gaps
            pairs = sorted(((((row["timestamp_us"] - first_us) & 0xFFFFFFFF), row) for row in rows),
                           key=lambda pair: pair[0]) if first_us is not None else []
            stamps = [pair[0] for pair in pairs]
            tolerance_us = round(500_000 / header.sample_rate_hz)
            for row in rows:
                if row["imu_valid"]:
                    magnitude = math.sqrt(sum(axis * axis for axis in row["acceleration"])) / 9.80665
                    peak_acceleration_g = max(peak_acceleration_g or 0.0, magnitude)
                if row["gps_current"]:
                    last_current_gps_fix = [row["latitude_e7"] / 1e7, row["longitude_e7"] / 1e7]
                    if row["gps_altitude_m"] is not None and math.isfinite(row["gps_altitude_m"]):
                        peak_gps_altitude_m = max(peak_gps_altitude_m if peak_gps_altitude_m is not None else row["gps_altitude_m"], row["gps_altitude_m"])
            for point in points:
                target = round(point["elapsed_s"] * 1_000_000)
                index = bisect_left(stamps, target)
                nearby = [pairs[i] for i in (index - 1, index) if 0 <= i < len(pairs)]
                if not nearby:
                    point["sensor"] = None
                    continue
                stamp, row = min(nearby, key=lambda pair: abs(pair[0] - target))
                if abs(stamp - target) > tolerance_us:
                    point["sensor"] = None
                    continue
                point["sensor"] = {
                    "time_delta_ms": round((stamp - target) / 1000, 2),
                    "imu_valid": row["imu_valid"],
                    "acceleration_mps2": list(row["acceleration"]) if row["imu_valid"] else None,
                    "gyro_rad_s": list(row["gyro"]) if row["imu_valid"] else None,
                    "gps_status": "live" if row["gps_current"] else "stale" if row["gps_has_position"] else "missing",
                    "latitude": row["latitude_e7"] / 1e7 if row["gps_has_position"] else None,
                    "longitude": row["longitude_e7"] / 1e7 if row["gps_has_position"] else None,
                    "gps_altitude_m": row["gps_altitude_m"] if row["gps_has_position"] else None,
                    "gps_speed_mps": row["gps_speed_mps"] if row["gps_has_position"] else None,
                }
        return {
            "flight_id": identifier, "analysis_version": 2, "source": "onboard_rkt4",
            "total_records": total, "displayed_records": len(points), "display_stride": stride,
            "sensor_status": sensor_status, "sensor_sequence_gaps": sensor_gaps,
            "estimated_peak_index": peak_point["index"] if peak_point else None,
            "estimated_apogee_elapsed_s": peak_point["elapsed_s"] if peak_point else None,
            "peak_vertical_speed_mps_estimated": peak_velocity,
            "peak_acceleration_g": peak_acceleration_g,
            "peak_current_gps_altitude_m": peak_gps_altitude_m,
            "last_current_gps_fix": last_current_gps_fix,
            "warnings": metadata["summary"]["warnings"] +
                ([f"GIS1 companion status: {sensor_status}; only its valid prefix is shown"]
                 if sensor_status not in ("missing", "valid") else []),
            "points": points,
        }

    def compare(self, identifiers: list[str]) -> dict:
        if not isinstance(identifiers, list) or not 2 <= len(identifiers) <= 4 or len(set(identifiers)) != len(identifiers):
            raise FlightImportError("compare two to four distinct flights")
        flights = []
        for identifier in identifiers:
            metadata, _raw = self.get(identifier)
            timeline = self.analysis_timeline(identifier)
            summary = metadata["summary"]
            flights.append({
                "id": identifier, "label": metadata.get("label") or metadata["original_name"],
                "original_name": metadata["original_name"],
                "rocket_profile": metadata.get("rocket_profile") or None,
                "flight_date": metadata.get("flight_date"),
                "peak_barometric_altitude_m_estimated": summary["peak_altitude_m_estimated"],
                "peak_current_gps_altitude_m": timeline["peak_current_gps_altitude_m"],
                "estimated_apogee_elapsed_s": timeline["estimated_apogee_elapsed_s"],
                "duration_s": summary["duration_s"],
                "peak_vertical_speed_mps_estimated": timeline["peak_vertical_speed_mps_estimated"],
                "peak_imu_magnitude_g": timeline["peak_acceleration_g"],
                "battery_start_v": summary["start_battery_v"],
                "battery_end_v": summary["end_battery_v"],
                "last_current_gps_fix": timeline["last_current_gps_fix"],
                "sensor_status": timeline["sensor_status"],
                "rkt4_warnings": list(summary["warnings"]),
                "sensor_sequence_gaps": timeline["sensor_sequence_gaps"],
                "sha256": metadata["sha256"],
            })
        return {"analysis_version": 2, "source": "onboard_recordings",
                "gps_note": "Last current GPS fix is not a confirmed landing position.",
                "flights": flights}

    def json_export(self, identifier: str) -> bytes:
        return b"".join(self.iter_json_export(identifier))

    def iter_json_export(self, identifier: str, *, _verified: tuple[dict, Path] | None = None):
        """Yield a complete JSON report without retaining all RKT4 records."""
        metadata, raw = _verified if _verified is not None else self.get(identifier)
        prefix = json.dumps(metadata, allow_nan=False)

        def chunks():
            yield (prefix[:-1] + ',"records":[').encode()
            buffer = io.StringIO()
            first = True
            for row in iter_records(raw):
                if not first:
                    buffer.write(",")
                buffer.write(json.dumps(row, separators=(",", ":"), allow_nan=False))
                first = False
                if buffer.tell() >= 65536:
                    yield buffer.getvalue().encode()
                    buffer.seek(0)
                    buffer.truncate(0)
            if buffer.tell():
                yield buffer.getvalue().encode()
            yield b"]}"

        return chunks()

    def csv_export(self, identifier: str) -> bytes:
        return b"".join(self.iter_csv_export(identifier))

    def iter_csv_export(self, identifier: str, *, _verified: tuple[dict, Path] | None = None):
        """Yield bounded CSV chunks for full-size recordings."""
        _metadata, raw = _verified if _verified is not None else self.get(identifier)
        fields = ["index", "timestamp_us", "time_s", "pressure_pa", "temperature_c", "battery_v", "valid", "altitude_m_estimated"]

        def chunks():
            text = io.StringIO(newline="")
            writer = csv.DictWriter(text, fieldnames=fields)
            writer.writeheader()
            for row in iter_records(raw):
                writer.writerow(row)
                if text.tell() >= 65536:
                    yield text.getvalue().encode()
                    text.seek(0)
                    text.truncate(0)
            if text.tell():
                yield text.getvalue().encode()

        return chunks()

    def report_html(self, identifier: str) -> bytes:
        metadata, _raw = self.get(identifier)
        summary = metadata["summary"]
        timeline = self.analysis_timeline(identifier)
        warnings = "".join(f"<li>{escape(item)}</li>" for item in timeline["warnings"]) or "<li>None reported by the parser</li>"
        points = [(point["elapsed_s"], point["altitude_m_estimated"])
                  for point in timeline["points"] if point["altitude_m_estimated"] is not None]
        if points:
            duration = max(points[-1][0], 1e-6)
            lowest = min(point[1] for point in points)
            highest = max(point[1] for point in points)
            altitude_span = max(highest - lowest, 1e-6)
            coordinates = " ".join(f"{20 + t / duration * 760:.1f},{180 - (altitude - lowest) / altitude_span * 150:.1f}"
                                   for t, altitude in points)
            chart = (f"<svg role='img' aria-label='Estimated barometric altitude over time' "
                     f"viewBox='0 0 800 200' width='100%' height='200'>"
                     f"<polyline fill='none' stroke='#1460a0' stroke-width='2' points='{coordinates}'/></svg>")
        else:
            chart = "<p>No valid barometric samples for a chart.</p>"
        label = escape(metadata.get("label") or metadata["original_name"])
        rocket = escape(metadata.get("rocket_profile") or "Unknown")
        flight_date = escape(metadata.get("flight_date") or "Unknown")
        source = escape(metadata.get("source") or "Unknown")
        imported = escape(metadata.get("imported_utc") or "Unknown")
        sensor = escape(timeline["sensor_status"])
        peak_imu = (f"{timeline['peak_acceleration_g']:.2f} g" if timeline["peak_acceleration_g"] is not None
                    else "Unavailable")
        html = f"""<!doctype html><html><head><meta charset='utf-8'><title>{label} report</title>
<style>body{{font:15px system-ui;max-width:800px;margin:40px auto;color:#123}}table{{border-collapse:collapse;width:100%}}td{{border-bottom:1px solid #ccd;padding:8px}}@media print{{button{{display:none}}}}</style></head><body>
<button onclick='print()'>Print / Save PDF</button><h1>GREEN ION flight report</h1><h2>{label}</h2>
<p>SHA-256: <code>{metadata['sha256']}</code></p><table>
<tr><td>Original file</td><td>{escape(metadata['original_name'])}</td></tr><tr><td>Rocket profile</td><td>{rocket}</td></tr>
<tr><td>Flight date supplied by user</td><td>{flight_date}</td></tr><tr><td>Import source</td><td>{source}</td></tr><tr><td>Import UTC</td><td>{imported}</td></tr>
<tr><td>Format</td><td>RKT4 v2</td></tr><tr><td>Records</td><td>{summary['records']}</td></tr>
<tr><td>Duration</td><td>{summary['duration_s']:.2f} s</td></tr><tr><td>Sample rate</td><td>{summary['sample_rate_hz']} Hz</td></tr>
<tr><td>Estimated peak altitude</td><td>{summary['peak_altitude_m_estimated']:.2f} m</td></tr>
<tr><td>Estimated apogee time after recording began</td><td>{'Unavailable' if timeline['estimated_apogee_elapsed_s'] is None else f"{timeline['estimated_apogee_elapsed_s']:.2f} s"}</td></tr>
<tr><td>Peak current GPS altitude</td><td>{'Unavailable' if timeline['peak_current_gps_altitude_m'] is None else f"{timeline['peak_current_gps_altitude_m']:.2f} m"}</td></tr>
<tr><td>Estimated peak vertical speed</td><td>{'Unavailable' if timeline['peak_vertical_speed_mps_estimated'] is None else f"{timeline['peak_vertical_speed_mps_estimated']:.2f} m/s"}</td></tr>
<tr><td>Peak IMU acceleration magnitude</td><td>{peak_imu}</td></tr><tr><td>GIS1 companion</td><td>{sensor}</td></tr>
<tr><td>Battery</td><td>{summary['start_battery_v']:.3f} V to {summary['end_battery_v']:.3f} V</td></tr></table>
<h3>Estimated barometric altitude</h3>{chart}<h3>Integrity warnings</h3><ul>{warnings}</ul><p><strong>Evidence note:</strong> {escape(summary['disclaimer'])} Velocity is a direct difference between neighboring valid pressure-derived samples. IMU magnitude includes gravity and is not net thrust acceleration. The chart is reduced for display and retains the barometric peak. Save PDF through the browser print dialog; this page does not generate a native PDF. This report is not physical sensor verification.</p></body></html>"""
        return html.encode()
