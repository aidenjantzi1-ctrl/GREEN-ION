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
            if valid_coordinate(clean.get("latitude"), clean.get("longitude")) and gps_status == "live":
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
        if combined.get("gps_status") == "live" and combined.get("gps_fix_age_ms", 0) > GPS_MAX_AGE_MS:
            combined["gps_status"] = "stale"
        return combined

    def snapshot(self) -> dict:
        with self.lock:
            return self._snapshot_locked(self.clock())

    def recent_history(self, limit: int = 600) -> list[dict]:
        with self.lock:
            return list(self.history[-max(1, min(limit, 1200)):])
