"""Validated, read-only RKT4 v2 import and local analysis library."""

from __future__ import annotations

import csv
import hashlib
import io
import json
import math
from pathlib import Path
import re
import struct
from typing import BinaryIO

HEADER_BYTES = 32
RECORD_BYTES = 16
ALLOWED_RATES = (10, 25, 50)
SAFE_NAME = re.compile(r"^[A-Za-z0-9][A-Za-z0-9_.-]{0,63}\.BIN$", re.IGNORECASE)
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
