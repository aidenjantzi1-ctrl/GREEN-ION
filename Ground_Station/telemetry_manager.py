"""Source-aware telemetry merging and durable session storage for GREEN ION V12."""

from __future__ import annotations

from dataclasses import dataclass, field
from datetime import datetime, timezone
import json
import math
from pathlib import Path
import threading
import time
import uuid


LIVE_AFTER_SECONDS = {"wifi": 2.5, "radio": 5.0, "simulation": 2.5}
GPS_MAX_AGE_MS = 3000
MEASUREMENT_FIELDS = (
    "uptime_ms", "flight_state", "recording", "altitude_m", "pressure_pa",
    "vertical_speed_mps", "speed_mps", "acceleration_g", "acceleration_x_g",
    "acceleration_y_g", "acceleration_z_g", "rotation_x_dps",
    "rotation_y_dps", "rotation_z_dps", "imu_temperature_c", "battery_v",
    "satellites", "gps_fix_age_ms", "gps_status", "latitude", "longitude",
    "gps_altitude_m", "health", "sd_ok", "imu_ok", "gps_ok", "radio_ok",
    "rssi_dbm", "flags", "log_file", "barometer_ready", "recorder_status",
    "recorder_mode", "calibration_baseline_pa", "calibration_noise_m",
    "imu_test_result", "imu_test_message", "imu_range_32g", "imu_test_samples",
    "imu_test_invalid", "imu_test_mean_g", "imu_test_bias_x_dps",
    "imu_test_bias_y_dps", "imu_test_bias_z_dps", "imu_test_temperature_c",
    "gps_test_result", "gps_test_message", "gps_test_communication",
    "gps_test_nmea", "gps_test_parsed", "hardware_tests_allowed",
)

# Ground Station field contract. "measured" includes values reported by a
# sensor or hardware status endpoint; "calculated" includes firmware or Ground
# Station estimates. A received timestamp is not a GPS fix timestamp.
FIELD_SPECS = {
    "uptime_ms": ("integer", "ms", "measured"),
    "flight_state": ("integer", None, "calculated"),
    "recording": ("boolean", None, "measured"),
    "altitude_m": ("number", "m", "calculated"),
    "pressure_pa": ("number", "Pa", "measured"),
    "vertical_speed_mps": ("number", "m/s", "calculated"),
    "speed_mps": ("number", "m/s", "calculated"),
    "acceleration_g": ("number", "g", "measured"),
    "acceleration_x_g": ("number", "g", "measured"),
    "acceleration_y_g": ("number", "g", "measured"),
    "acceleration_z_g": ("number", "g", "measured"),
    "rotation_x_dps": ("number", "deg/s", "measured"),
    "rotation_y_dps": ("number", "deg/s", "measured"),
    "rotation_z_dps": ("number", "deg/s", "measured"),
    "imu_temperature_c": ("number", "C", "measured"),
    "battery_v": ("number", "V", "measured"),
    "satellites": ("integer", "count", "measured"),
    "gps_fix_age_ms": ("integer", "ms", "calculated"),
    "gps_status": ("string", None, "calculated"),
    "latitude": ("number", "degrees", "measured"),
    "longitude": ("number", "degrees", "measured"),
    "gps_altitude_m": ("number", "m", "measured"),
    "health": ("integer", "bitmask", "calculated"),
    "sd_ok": ("boolean", None, "measured"),
    "imu_ok": ("boolean", None, "measured"),
    "gps_ok": ("boolean", None, "measured"),
    "radio_ok": ("boolean", None, "measured"),
    "rssi_dbm": ("number", "dBm", "measured"),
    "flags": ("integer", "bitmask", "measured"),
    "log_file": ("string", None, "measured"),
    "barometer_ready": ("boolean", None, "measured"),
    "recorder_status": ("string", None, "measured"),
    "recorder_mode": ("string", None, "measured"),
    "calibration_baseline_pa": ("number", "Pa", "calculated"),
    "calibration_noise_m": ("number", "m", "calculated"),
    "imu_test_result": ("string", None, "calculated"),
    "imu_test_message": ("string", None, "calculated"),
    "imu_range_32g": ("boolean", None, "measured"),
    "imu_test_samples": ("integer", "count", "measured"),
    "imu_test_invalid": ("integer", "count", "measured"),
    "imu_test_mean_g": ("number", "g", "calculated"),
    "imu_test_bias_x_dps": ("number", "deg/s", "calculated"),
    "imu_test_bias_y_dps": ("number", "deg/s", "calculated"),
    "imu_test_bias_z_dps": ("number", "deg/s", "calculated"),
    "imu_test_temperature_c": ("number", "C", "measured"),
    "gps_test_result": ("string", None, "calculated"),
    "gps_test_message": ("string", None, "calculated"),
    "gps_test_communication": ("boolean", None, "measured"),
    "gps_test_nmea": ("integer", "count", "measured"),
    "gps_test_parsed": ("integer", "count", "measured"),
    "hardware_tests_allowed": ("boolean", None, "calculated"),
}
assert set(FIELD_SPECS) == set(MEASUREMENT_FIELDS)


def utc_now() -> str:
    return datetime.now(timezone.utc).isoformat()


def valid_coordinate(latitude: object, longitude: object) -> bool:
    try:
        lat, lon = float(latitude), float(longitude)
    except (TypeError, ValueError):
        return False
    return math.isfinite(lat) and math.isfinite(lon) and -90 <= lat <= 90 and -180 <= lon <= 180


def is_newer_sequence(previous: int, candidate: int) -> bool:
    """Unsigned 32-bit sequence comparison, including rollover."""
    delta = (candidate - previous) & 0xFFFFFFFF
    return 0 < delta < 0x80000000


@dataclass
class SourceState:
    latest: dict = field(default_factory=dict)
    received_monotonic: float = 0.0
    last_sequence: int | None = None
    last_uptime_ms: int | None = None
    received: int = 0
    duplicates: int = 0
    out_of_order: int = 0
    missing_packets: int = 0
    parse_errors: int = 0
    connection_error: str | None = None


class SessionWriter:
    """Append-only raw JSONL plus a restart snapshot; raw data is never rewritten."""

    def __init__(self, directory: Path, session_id: str | None = None) -> None:
        self.directory = directory
        self.directory.mkdir(parents=True, exist_ok=True)
        self.session_id = session_id or datetime.now().strftime("%Y%m%d-%H%M%S") + "-" + uuid.uuid4().hex[:8]
        self.raw_path = self.directory / f"session-{self.session_id}.jsonl"
        self.snapshot_path = self.directory / "current-session.json"
        self.lock = threading.Lock()

    def append(self, source: str, row: dict) -> None:
        event = {"received_utc": utc_now(), "source": source, "data": row}
        encoded = json.dumps(event, separators=(",", ":"), allow_nan=False)
        with self.lock, self.raw_path.open("a", encoding="utf-8", newline="\n") as handle:
            handle.write(encoded + "\n")
            handle.flush()

    def save_snapshot(self, snapshot: dict) -> None:
        temporary = self.snapshot_path.with_suffix(".tmp")
        payload = {"session_id": self.session_id, "saved_utc": utc_now(), "snapshot": snapshot}
        with self.lock:
            temporary.write_text(json.dumps(payload, separators=(",", ":"), allow_nan=False), encoding="utf-8")
            temporary.replace(self.snapshot_path)


class TelemetryManager:
    def __init__(self, writer: SessionWriter | None = None, clock=time.monotonic) -> None:
        self.writer = writer
        self.clock = clock
        self.sources = {name: SourceState() for name in ("wifi", "radio", "simulation")}
        self.last_position: dict | None = None
        self.history: list[dict] = []
        self.lock = threading.RLock()
        self.started_utc = utc_now()

    def note_connection(self, source: str, error: str | None) -> None:
        with self.lock:
            self.sources.setdefault(source, SourceState()).connection_error = error

    def note_parse_error(self, source: str, message: str) -> None:
        with self.lock:
            state = self.sources.setdefault(source, SourceState())
            state.parse_errors += 1
            state.connection_error = message

    def ingest(self, source: str, row: dict) -> bool:
        if source not in self.sources:
            raise ValueError(f"unsupported source: {source}")
        if not isinstance(row, dict):
            raise ValueError("telemetry row must be an object")
        now = self.clock()
        clean = {key: value for key, value in row.items() if key in MEASUREMENT_FIELDS or key == "sequence"}
        sequence = clean.get("sequence")
        uptime = clean.get("uptime_ms")
        with self.lock:
            state = self.sources[source]
            if sequence is not None:
                try:
                    sequence = int(sequence) & 0xFFFFFFFF
                except (TypeError, ValueError) as error:
                    raise ValueError("invalid sequence") from error
                clean["sequence"] = sequence
                rebooted = (uptime is not None and state.last_uptime_ms is not None and
                            state.last_uptime_ms - int(uptime) > 10000 and sequence <= 5)
                if state.last_sequence is not None and not rebooted:
                    if sequence == state.last_sequence:
                        state.duplicates += 1
                        return False
                    if not is_newer_sequence(state.last_sequence, sequence):
                        state.out_of_order += 1
                        return False
                    gap = (sequence - state.last_sequence) & 0xFFFFFFFF
                    if gap > 1:
                        state.missing_packets += gap - 1
                state.last_sequence = sequence
            if uptime is not None:
                clean["uptime_ms"] = int(uptime)
                state.last_uptime_ms = int(uptime)
            clean["source"] = source
            clean["received_utc"] = utc_now()
            state.latest = clean
            state.received_monotonic = now
            state.received += 1
            state.connection_error = None
            gps_status = clean.get("gps_status")
            fix_age = clean.get("gps_fix_age_ms")
            current_fix = (isinstance(fix_age, (int, float)) and not isinstance(fix_age, bool)
                           and math.isfinite(fix_age) and 0 <= fix_age <= GPS_MAX_AGE_MS)
            if valid_coordinate(clean.get("latitude"), clean.get("longitude")) and gps_status == "live" and current_fix:
                self.last_position = {
                    "latitude": float(clean["latitude"]),
                    "longitude": float(clean["longitude"]),
                    "received_monotonic": now,
                    "received_utc": clean["received_utc"],
                    "source": source,
                }
            point = {
                key: clean.get(key) for key in
                ("received_utc", "source", "uptime_ms", "altitude_m", "acceleration_g",
                 "vertical_speed_mps", "latitude", "longitude", "gps_status")
            }
            self.history.append(point)
            if len(self.history) > 1200:
                del self.history[:200]
            if self.writer:
                self.writer.append(source, clean)
                self.writer.save_snapshot(self._snapshot_locked(now))
            return True

    def _source_snapshot(self, name: str, now: float) -> dict:
        state = self.sources[name]
        age_ms = round((now - state.received_monotonic) * 1000) if state.received_monotonic else None
        live_limit = LIVE_AFTER_SECONDS[name] * 1000
        status = "live" if age_ms is not None and age_ms <= live_limit else "stale" if age_ms is not None else "missing"
        return {
            "status": status, "age_ms": age_ms, "received": state.received,
            "duplicates": state.duplicates, "out_of_order": state.out_of_order,
            "missing_packets": state.missing_packets, "parse_errors": state.parse_errors,
            "error": state.connection_error,
        }

    def _snapshot_locked(self, now: float) -> dict:
        source_info = {name: self._source_snapshot(name, now) for name in self.sources}
        selected = None
        for name in ("wifi", "radio", "simulation"):
            if source_info[name]["status"] == "live":
                selected = name
                break
        if selected is None:
            for name in ("wifi", "radio", "simulation"):
                if self.sources[name].latest:
                    selected = name
                    break
        combined = dict(self.sources[selected].latest) if selected else {}
        combined["active_source"] = selected
        combined["link_status"] = source_info[selected]["status"] if selected else "missing"
        combined["packet_age_ms"] = source_info[selected]["age_ms"] if selected else None
        combined["sources"] = source_info
        combined["started_utc"] = self.started_utc
        if self.writer:
            combined["session_id"] = self.writer.session_id
        if self.last_position:
            combined["last_position"] = [self.last_position["latitude"], self.last_position["longitude"]]
            combined["last_position_source"] = self.last_position["source"]
            combined["last_position_utc"] = self.last_position["received_utc"]
            combined["last_position_age_ms"] = round((now - self.last_position["received_monotonic"]) * 1000)
        else:
            combined["last_position"] = None
            combined["last_position_age_ms"] = None
        fix_age = combined.get("gps_fix_age_ms")
        if combined.get("gps_status") == "live" and (
            combined["link_status"] != "live" or
            not isinstance(fix_age, (int, float)) or isinstance(fix_age, bool) or
            not math.isfinite(fix_age) or fix_age < 0 or fix_age > GPS_MAX_AGE_MS or
            not valid_coordinate(combined.get("latitude"), combined.get("longitude"))
        ):
            combined["gps_status"] = "stale"
        # LoRa's fixed-width packet has a zero GPS altitude slot even without a
        # position. Keep the raw packet in the session, but do not present that
        # placeholder as a measured altitude in the merged view.
        if combined.get("gps_status") == "missing":
            combined["gps_altitude_m"] = None
        gps_fresh = combined.get("gps_status") == "live"
        coordinate_valid = valid_coordinate(combined.get("latitude"), combined.get("longitude"))
        received_utc = combined.get("received_utc")
        fields = {}
        for name, (kind, unit, origin) in FIELD_SPECS.items():
            value = combined.get(name)
            valid = value is not None
            if kind in ("integer", "number"):
                valid = (not isinstance(value, bool) and isinstance(value, (int, float))
                         and math.isfinite(value))
            elif kind == "boolean":
                valid = isinstance(value, bool)
            elif kind == "string":
                valid = isinstance(value, str) and bool(value)
            if name in ("latitude", "longitude"):
                valid = coordinate_valid
            freshness = "missing" if not valid else combined["link_status"]
            if valid and name in ("latitude", "longitude", "gps_altitude_m", "satellites") and not gps_fresh:
                freshness = "stale"
            fields[name] = {
                "type": kind, "unit": unit, "origin": origin,
                "source": selected if valid else None,
                "received_utc": received_utc if valid else None,
                "valid": valid, "freshness": freshness,
            }
        combined["field_meta"] = fields
        return combined

    def snapshot(self) -> dict:
        with self.lock:
            return self._snapshot_locked(self.clock())

    def recent_history(self, limit: int = 600) -> list[dict]:
        with self.lock:
            return list(self.history[-max(1, min(limit, 1200)):])
