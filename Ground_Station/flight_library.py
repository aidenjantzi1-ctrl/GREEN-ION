"""Validated, read-only RKT4 v2 import and local analysis library."""

from __future__ import annotations

import csv
import hashlib
import io
import json
import math
import os
from pathlib import Path
import re
import struct
import tempfile
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
        complete_records = (size - HEADER_BYTES) // RECORD_BYTES
        trailing_bytes = (size - HEADER_BYTES) % RECORD_BYTES
        first_timestamp = last_timestamp = previous_timestamp = None
        peak_altitude = 0.0
        invalid_records = timestamp_gaps = 0
        end_battery_mv = start_battery_mv
        for _ in range(complete_records):
            raw = handle.read(RECORD_BYTES)
            timestamp, pressure, _temperature, battery_mv, valid = struct.unpack("<IffHH", raw)
            if first_timestamp is None:
                first_timestamp = timestamp
            if previous_timestamp is not None:
                delta = (timestamp - previous_timestamp) & 0xFFFFFFFF
                if delta > round(4_000_000 / rate):
                    timestamp_gaps += 1
            previous_timestamp = last_timestamp = timestamp
            end_battery_mv = battery_mv
            if valid == 1:
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
            yield {
                "index": index, "timestamp_us": timestamp, "time_s": timestamp / 1_000_000,
                "pressure_pa": pressure, "temperature_c": temperature,
                "battery_v": battery_mv / 1000, "valid": valid == 1,
                "altitude_m_estimated": pressure_altitude(pressure, baseline) if valid == 1 else None,
            }
            index += 1


class FlightLibrary:
    def __init__(self, root: Path) -> None:
        self.root = root
        self.raw = root / "raw"
        self.meta = root / "metadata"
        self.raw.mkdir(parents=True, exist_ok=True)
        self.meta.mkdir(parents=True, exist_ok=True)

    def _metadata_path(self, identifier: str) -> Path:
        if not re.fullmatch(r"[0-9a-f]{64}", identifier):
            raise FlightImportError("invalid flight identifier")
        return self.meta / f"{identifier}.json"

    def import_stream(self, filename: str, source: BinaryIO, expected_bytes: int | None = None) -> dict:
        if Path(filename).name != filename:
            raise FlightImportError("filename must not contain a directory")
        if not SAFE_NAME.fullmatch(filename):
            raise FlightImportError("filename must be a simple .BIN name")
        temporary = self.root / f"import-{hashlib.sha256(filename.encode()).hexdigest()[:12]}.part"
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
            if expected_bytes is not None and received != expected_bytes:
                raise FlightImportError(f"incomplete transfer: expected {expected_bytes} bytes, received {received}")
            summary = inspect_rkt4(temporary)
            identifier = digest.hexdigest()
            destination = self.raw / f"{identifier}.bin"
            duplicate = destination.exists()
            if duplicate:
                temporary.unlink()
            else:
                temporary.replace(destination)
            metadata_path = self._metadata_path(identifier)
            metadata = {
                "id": identifier, "original_name": filename, "sha256": identifier,
                "raw_file": destination.name, "summary": summary,
            }
            if not metadata_path.exists():
                metadata_path.write_text(json.dumps(metadata, indent=2, allow_nan=False), encoding="utf-8")
            metadata["duplicate"] = duplicate
            return metadata
        except Exception:
            temporary.unlink(missing_ok=True)
            raise

    def list(self) -> list[dict]:
        result = []
        for path in sorted(self.meta.glob("*.json"), key=lambda item: item.stat().st_mtime, reverse=True):
            try:
                result.append(json.loads(path.read_text(encoding="utf-8")))
            except (OSError, json.JSONDecodeError):
                continue
        return result

    def get(self, identifier: str) -> tuple[dict, Path]:
        metadata = json.loads(self._metadata_path(identifier).read_text(encoding="utf-8"))
        raw = (self.raw / metadata["raw_file"]).resolve()
        if raw.parent != self.raw.resolve() or not raw.is_file():
            raise FlightImportError("raw flight file is unavailable")
        return metadata, raw

    def import_sensor_stream(self, identifier: str, filename: str, source: BinaryIO,
                             expected_bytes: int | None = None) -> dict:
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
        duplicate = destination.exists() and hashlib.sha256(destination.read_bytes()).hexdigest() == digest
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
        metadata_path = self._metadata_path(identifier)
        with tempfile.NamedTemporaryFile(mode="w", encoding="utf-8", dir=self.meta,
                                         prefix="sensor-meta-", suffix=".part", delete=False) as output:
            temporary = Path(output.name)
            try:
                json.dump(metadata, output, indent=2, allow_nan=False)
                output.flush()
                os.fsync(output.fileno())
            except Exception:
                temporary.unlink(missing_ok=True)
                raise
        temporary.replace(metadata_path)
        return {**sensor, "duplicate": duplicate}

    def get_sensor(self, identifier: str) -> tuple[dict, Path]:
        metadata, _raw = self.get(identifier)
        sensor = metadata.get("sensor")
        if not isinstance(sensor, dict) or not re.fullmatch(r"[0-9a-f]{64}", str(sensor.get("sha256", ""))):
            raise FlightImportError("sensor companion is unavailable")
        path = (self.raw / f"{sensor['sha256']}.sen").resolve()
        if path.parent != self.raw.resolve() or not path.is_file():
            raise FlightImportError("raw sensor file is unavailable")
        if hashlib.sha256(path.read_bytes()).hexdigest() != sensor["sha256"]:
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

    def json_export(self, identifier: str) -> bytes:
        metadata, raw = self.get(identifier)
        payload = {**metadata, "records": list(iter_records(raw))}
        return json.dumps(payload, indent=2, allow_nan=False).encode()

    def csv_export(self, identifier: str) -> bytes:
        _metadata, raw = self.get(identifier)
        text = io.StringIO(newline="")
        fields = ["index", "timestamp_us", "time_s", "pressure_pa", "temperature_c", "battery_v", "valid", "altitude_m_estimated"]
        writer = csv.DictWriter(text, fieldnames=fields)
        writer.writeheader()
        writer.writerows(iter_records(raw))
        return text.getvalue().encode()

    def report_html(self, identifier: str) -> bytes:
        metadata, _raw = self.get(identifier)
        summary = metadata["summary"]
        warnings = "".join(f"<li>{item}</li>" for item in summary["warnings"]) or "<li>None reported by the parser</li>"
        html = f"""<!doctype html><html><head><meta charset='utf-8'><title>{metadata['original_name']} report</title>
<style>body{{font:15px system-ui;max-width:800px;margin:40px auto;color:#123}}table{{border-collapse:collapse;width:100%}}td{{border-bottom:1px solid #ccd;padding:8px}}@media print{{button{{display:none}}}}</style></head><body>
<button onclick='print()'>Print / Save PDF</button><h1>GREEN ION flight report</h1><h2>{metadata['original_name']}</h2>
<p>SHA-256: <code>{metadata['sha256']}</code></p><table>
<tr><td>Format</td><td>RKT4 v2</td></tr><tr><td>Records</td><td>{summary['records']}</td></tr>
<tr><td>Duration</td><td>{summary['duration_s']:.2f} s</td></tr><tr><td>Sample rate</td><td>{summary['sample_rate_hz']} Hz</td></tr>
<tr><td>Estimated peak altitude</td><td>{summary['peak_altitude_m_estimated']:.2f} m</td></tr>
<tr><td>Battery</td><td>{summary['start_battery_v']:.3f} V to {summary['end_battery_v']:.3f} V</td></tr></table>
<h3>Integrity warnings</h3><ul>{warnings}</ul><p><strong>Evidence note:</strong> {summary['disclaimer']} This report is not physical sensor verification.</p></body></html>"""
        return html.encode()
