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
SD_CAPACITY_FRESH_MS = 10000
RADIO_FLAG_SD_OK = 1 << 2
RADIO_FLAG_IMU_OK = 1 << 3
RADIO_FLAG_BARO_OK = 1 << 4
RADIO_FLAG_RADIO_OK = 1 << 5
RADIO_FLAG_RECORDING = 1 << 6
# Approximate rested 1-cell LiPo voltage curve. Loaded flight voltage can sag;
# this is only a coarse display estimate, not a capacity measurement.
BATTERY_CURVE = ((3.30, 0), (3.60, 10), (3.70, 25), (3.80, 45),
                 (3.90, 65), (4.00, 80), (4.10, 95), (4.20, 100))
MEASUREMENT_FIELDS = (
    "uptime_ms", "firmware_version", "telemetry_protocol_version",
    "recording_format_version", "flight_state", "recording", "bench_recording",
    "bench_max_ms", "altitude_m", "pressure_pa",
    "vertical_speed_mps", "speed_mps", "gps_speed_mps",
    "relative_altitude_m", "temperature_c", "acceleration_g", "acceleration_x_g",
    "acceleration_y_g", "acceleration_z_g", "rotation_x_dps",
    "rotation_y_dps", "rotation_z_dps", "imu_temperature_c", "battery_v",
    "satellites", "gps_fix_age_ms", "gps_status", "latitude", "longitude",
    "gps_altitude_m", "gps_altitude_valid", "gps_fix_quality", "gps_course_deg",
    "gps_communication", "gps_nmea_sentences", "gps_parsed_sentences",
    "health", "sd_ok", "imu_ok", "gps_ok", "radio_ok",
    "rssi_dbm", "flags", "log_file", "barometer_ready", "recorder_status",
    "recorder_mode", "calibration_baseline_pa", "calibration_noise_m",
    "prebuffer_records", "checkpoint_generation", "checkpoint_failures",
    "checkpoint_committed_bytes", "checkpoint_committed_records", "sample_rate_hz",
    "recorder_written_records", "recorder_written_bytes",
    "sd_capacity_known", "sd_total_bytes", "sd_used_bytes", "sd_free_bytes",
    "sd_capacity_age_ms",
    "barometer_sample_uptime_ms", "imu_sample_uptime_ms",
    "battery_sample_uptime_ms", "gps_fix_uptime_ms",
    "gps_altitude_uptime_ms",
    "gps_utc_hour", "gps_utc_minute", "gps_utc_second", "gps_utc_valid",
    "barometer_init_failures", "barometer_recoveries", "barometer_runtime_losses",
    "metadata_file", "metadata_failures", "sensor_log_status",
    "recovered_records", "recovered_sensor_records", "recovered_trailing_bytes",
    "sensor_recovery_issues", "landing_peak_m", "cabin_pressure_delta_m",
    "airplane_state", "airplane_recording", "airplane_radio_silence",
    "airplane_elapsed_ms", "airplane_max_ms", "airplane_records",
    "protocol", "valid_packets", "invalid_packets",
    "sensor_log_file", "sensor_log_failures", "sensor_log_pending",
    "sensor_log_dropped", "finalization_failures", "missed_sample_periods",
    "recovery_pending", "recovery_status", "recovered_logs",
    "imu_test_result", "imu_test_message", "imu_range_32g", "imu_test_samples",
    "imu_test_duration_ms", "imu_test_peak_deviation_g",
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
    "firmware_version": ("string", None, "measured"),
    "telemetry_protocol_version": ("integer", "version", "measured"),
    "recording_format_version": ("integer", "version", "measured"),
    "flight_state": ("integer", None, "calculated"),
    "recording": ("boolean", None, "measured"),
    "bench_recording": ("boolean", None, "measured"),
    "bench_max_ms": ("integer", "ms", "measured"),
    "altitude_m": ("number", "m", "calculated"),
    "pressure_pa": ("number", "Pa", "measured"),
    "vertical_speed_mps": ("number", "m/s", "calculated"),
    "speed_mps": ("number", "m/s", "measured"),
    "gps_speed_mps": ("number", "m/s", "measured"),
    "relative_altitude_m": ("number", "m", "calculated"),
    "temperature_c": ("number", "C", "measured"),
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
    "gps_altitude_valid": ("boolean", None, "calculated"),
    "gps_fix_quality": ("integer", "code", "measured"),
    "gps_course_deg": ("number", "degrees", "measured"),
    "gps_communication": ("string", None, "calculated"),
    "gps_nmea_sentences": ("integer", "count", "measured"),
    "gps_parsed_sentences": ("integer", "count", "measured"),
    "health": ("integer", "bitmask", "calculated"),
    "sd_ok": ("boolean", None, "measured"),
    "imu_ok": ("boolean", None, "measured"),
    "gps_ok": ("boolean", None, "measured"),
    "radio_ok": ("boolean", None, "measured"),
    "rssi_dbm": ("number", "dBm", "measured"),
    "flags": ("integer", "bitmask", "measured"),
    "log_file": ("string", None, "measured"),
    "barometer_ready": ("boolean", None, "measured"),
    "recorder_status": ("integer", "code", "measured"),
    "recorder_mode": ("integer", "code", "measured"),
    "calibration_baseline_pa": ("number", "Pa", "calculated"),
    "calibration_noise_m": ("number", "m", "calculated"),
    "prebuffer_records": ("integer", "count", "measured"),
    "checkpoint_generation": ("integer", "count", "measured"),
    "checkpoint_failures": ("integer", "count", "measured"),
    "checkpoint_committed_bytes": ("integer", "bytes", "measured"),
    "checkpoint_committed_records": ("integer", "count", "measured"),
    "recorder_written_records": ("integer", "count", "measured"),
    "recorder_written_bytes": ("integer", "bytes", "calculated"),
    "sample_rate_hz": ("integer", "Hz", "calculated"),
    "sd_capacity_known": ("boolean", None, "calculated"),
    "sd_total_bytes": ("integer", "bytes", "measured"),
    "sd_used_bytes": ("integer", "bytes", "measured"),
    "sd_free_bytes": ("integer", "bytes", "calculated"),
    "sd_capacity_age_ms": ("integer", "ms", "calculated"),
    "barometer_sample_uptime_ms": ("integer", "ms", "measured"),
    "imu_sample_uptime_ms": ("integer", "ms", "measured"),
    "battery_sample_uptime_ms": ("integer", "ms", "measured"),
    "gps_fix_uptime_ms": ("integer", "ms", "measured"),
    "gps_altitude_uptime_ms": ("integer", "ms", "measured"),
    "gps_utc_hour": ("integer", "hour", "measured"),
    "gps_utc_minute": ("integer", "minute", "measured"),
    "gps_utc_second": ("integer", "second", "measured"),
    "gps_utc_valid": ("boolean", None, "calculated"),
    "barometer_init_failures": ("integer", "count", "measured"),
    "barometer_recoveries": ("integer", "count", "measured"),
    "barometer_runtime_losses": ("integer", "count", "measured"),
    "metadata_file": ("string", None, "measured"),
    "metadata_failures": ("integer", "count", "measured"),
    "sensor_log_status": ("integer", "code", "measured"),
    "recovered_records": ("integer", "count", "measured"),
    "recovered_sensor_records": ("integer", "count", "measured"),
    "recovered_trailing_bytes": ("integer", "bytes", "measured"),
    "sensor_recovery_issues": ("integer", "count", "measured"),
    "landing_peak_m": ("number", "m", "calculated"),
    "cabin_pressure_delta_m": ("number", "m", "calculated"),
    "airplane_state": ("string", None, "calculated"),
    "airplane_recording": ("boolean", None, "measured"),
    "airplane_radio_silence": ("boolean", None, "measured"),
    "airplane_elapsed_ms": ("integer", "ms", "calculated"),
    "airplane_max_ms": ("integer", "ms", "measured"),
    "airplane_records": ("integer", "count", "measured"),
    "protocol": ("integer", "version", "measured"),
    "valid_packets": ("integer", "count", "measured"),
    "invalid_packets": ("integer", "count", "measured"),
    "sensor_log_file": ("string", None, "measured"),
    "sensor_log_failures": ("integer", "count", "measured"),
    "sensor_log_pending": ("integer", "count", "measured"),
    "sensor_log_dropped": ("integer", "count", "measured"),
    "finalization_failures": ("integer", "count", "measured"),
    "missed_sample_periods": ("integer", "count", "measured"),
    "recovery_pending": ("integer", "flag", "measured"),
    "recovery_status": ("string", None, "measured"),
    "recovered_logs": ("integer", "count", "measured"),
    "imu_test_result": ("string", None, "calculated"),
    "imu_test_message": ("string", None, "calculated"),
    "imu_range_32g": ("boolean", None, "measured"),
    "imu_test_samples": ("integer", "count", "measured"),
    "imu_test_duration_ms": ("integer", "ms", "calculated"),
    "imu_test_peak_deviation_g": ("number", "g", "calculated"),
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

SAMPLE_TIME_FIELDS = {
    "barometer_sample_uptime_ms": ("pressure_pa", "temperature_c", "altitude_m",
                                   "relative_altitude_m", "vertical_speed_mps",
                                   "cabin_pressure_delta_m"),
    "imu_sample_uptime_ms": ("acceleration_g", "acceleration_x_g", "acceleration_y_g",
                             "acceleration_z_g", "rotation_x_dps", "rotation_y_dps",
                             "rotation_z_dps", "imu_temperature_c"),
    "battery_sample_uptime_ms": ("battery_v",),
    "gps_fix_uptime_ms": ("latitude", "longitude", "gps_speed_mps", "gps_course_deg"),
    "gps_altitude_uptime_ms": ("gps_altitude_m",),
}
FIELD_SAMPLE_TIME = {field: clock_field for clock_field, fields in SAMPLE_TIME_FIELDS.items()
                     for field in fields}


def battery_estimate_percent(voltage: object) -> int | None:
    if isinstance(voltage, bool) or not isinstance(voltage, (int, float)):
        return None
    if not math.isfinite(voltage) or not 3.0 <= voltage <= 4.25:
        return None
    if voltage <= BATTERY_CURVE[0][0]:
        return 0
    for (low_v, low_pct), (high_v, high_pct) in zip(BATTERY_CURVE, BATTERY_CURVE[1:]):
        if voltage <= high_v:
            return round(low_pct + (voltage - low_v) * (high_pct - low_pct) / (high_v - low_v))
    return 100


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
        self.receiver_status: dict | None = None
        self.receiver_connected = False
        self.history: list[dict] = []
        self.lock = threading.RLock()
        self.started_utc = utc_now()

    def note_connection(self, source: str, error: str | None) -> None:
        with self.lock:
            self.sources.setdefault(source, SourceState()).connection_error = error

    def note_receiver_status(self, row: dict) -> None:
        """Keep Uno boot evidence separate from actual LoRa telemetry packets."""
        if not isinstance(row, dict) or not isinstance(row.get("ok"), bool):
            raise ValueError("invalid Uno receiver status")
        keys = ("ok", "firmware_version", "protocol", "error", "frequency_mhz",
                "bandwidth_hz", "spreading_factor", "coding_rate", "tx_power_dbm")
        with self.lock:
            self.receiver_status = {key: row[key] for key in keys if key in row}
            self.receiver_status["received_utc"] = utc_now()
            self.receiver_connected = True

    def clear_receiver_status(self) -> None:
        with self.lock:
            self.receiver_status = None
            self.receiver_connected = False

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
        for name, value in (("sequence", sequence), ("uptime_ms", uptime)):
            if value is None:
                continue
            if isinstance(value, bool):
                raise ValueError(f"invalid {name}")
            try:
                normalized = int(value)
            except (TypeError, ValueError, OverflowError) as error:
                raise ValueError(f"invalid {name}") from error
            if not 0 <= normalized <= 0xFFFFFFFF:
                raise ValueError(f"invalid {name}")
            clean[name] = normalized
        sequence = clean.get("sequence")
        uptime = clean.get("uptime_ms")
        for name, value in clean.items():
            if isinstance(value, int) and not isinstance(value, bool) and value.bit_length() > 64:
                clean[name] = None
        if source == "radio" and isinstance(clean.get("flags"), int) and not isinstance(clean["flags"], bool):
            flags = clean["flags"]
            if 0 <= flags <= 0xFFFF:
                clean["sd_ok"] = bool(flags & RADIO_FLAG_SD_OK)
                clean["imu_ok"] = bool(flags & RADIO_FLAG_IMU_OK)
                clean["barometer_ready"] = bool(flags & RADIO_FLAG_BARO_OK)
                clean["radio_ok"] = bool(flags & RADIO_FLAG_RADIO_OK)
                clean["recording"] = bool(flags & RADIO_FLAG_RECORDING)
                if not clean["barometer_ready"]:
                    for field_name in ("pressure_pa", "altitude_m"):
                        clean[field_name] = None
                if not clean["imu_ok"]:
                    for field_name in ("acceleration_g", "acceleration_x_g", "acceleration_y_g",
                                       "acceleration_z_g", "rotation_x_dps", "rotation_y_dps",
                                       "rotation_z_dps"):
                        clean[field_name] = None
        with self.lock:
            state = self.sources[source]
            if sequence is not None:
                clean["sequence"] = sequence
                rebooted = (uptime is not None and state.last_uptime_ms is not None and
                            state.last_uptime_ms - uptime > 10000 and sequence <= 5)
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
                clean["uptime_ms"] = uptime
                state.last_uptime_ms = uptime
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
                phase = clean.get("flight_state")
                quality = clean.get("gps_fix_quality")
                satellites = clean.get("satellites")
                self.last_position = {
                    "latitude": float(clean["latitude"]),
                    "longitude": float(clean["longitude"]),
                    "received_monotonic": now,
                    "received_utc": clean["received_utc"],
                    "source": source,
                    "flight_state": phase if isinstance(phase, int) and not isinstance(phase, bool) and 0 <= phase <= 7 else None,
                    "fix_quality": quality if isinstance(quality, int) and not isinstance(quality, bool) and quality >= 0 else None,
                    "satellites": satellites if isinstance(satellites, int) and not isinstance(satellites, bool) and satellites >= 0 else None,
                }
            point = {
                key: clean.get(key) for key in
                ("received_utc", "source", "sequence", "uptime_ms", "flight_state", "altitude_m",
                 "gps_altitude_m", "vertical_speed_mps", "acceleration_g",
                 "acceleration_x_g", "acceleration_y_g", "acceleration_z_g",
                 "rotation_x_dps", "rotation_y_dps", "rotation_z_dps",
                 "battery_v", "battery_sample_uptime_ms", "pressure_pa", "latitude", "longitude",
                 "gps_status", "gps_fix_age_ms", "gps_fix_uptime_ms")
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
        # The fixed LoRa packet calls GPS ground speed `speed_mps`. Preserve
        # that legacy field, but expose a separate unambiguous view for UI use.
        if selected == "radio" and "gps_speed_mps" not in combined:
            combined["gps_speed_mps"] = combined.get("speed_mps")
        combined["active_source"] = selected
        combined["link_status"] = source_info[selected]["status"] if selected else "missing"
        combined["packet_age_ms"] = source_info[selected]["age_ms"] if selected else None
        combined["sources"] = source_info
        combined["receiver_status"] = dict(self.receiver_status) if self.receiver_connected and self.receiver_status else None
        combined["started_utc"] = self.started_utc
        if self.writer:
            combined["session_id"] = self.writer.session_id
        if self.last_position:
            combined["last_position"] = [self.last_position["latitude"], self.last_position["longitude"]]
            combined["last_position_source"] = self.last_position["source"]
            combined["last_position_utc"] = self.last_position["received_utc"]
            combined["last_position_age_ms"] = round((now - self.last_position["received_monotonic"]) * 1000)
            combined["last_position_phase"] = self.last_position["flight_state"]
            combined["last_position_fix_quality"] = self.last_position["fix_quality"]
            combined["last_position_satellites"] = self.last_position["satellites"]
        else:
            combined["last_position"] = None
            combined["last_position_age_ms"] = None
            combined["last_position_phase"] = None
            combined["last_position_fix_quality"] = None
            combined["last_position_satellites"] = None
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
            combined["gps_speed_mps"] = None
        battery_pct = battery_estimate_percent(combined.get("battery_v"))
        combined["battery_measurement_valid"] = battery_pct is not None
        combined["battery_estimate_pct"] = battery_pct
        battery_v = combined.get("battery_v")
        combined["battery_warning"] = (
            "unavailable" if battery_pct is None else
            "critical" if battery_v < 3.5 else
            "low" if battery_v < 3.65 else "normal"
        )
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
            if valid and name in ("latitude", "longitude", "gps_altitude_m", "gps_speed_mps", "gps_course_deg", "satellites") and not gps_fresh:
                freshness = "stale"
            if valid and name == "gps_altitude_m" and combined.get("gps_altitude_valid") is False:
                freshness = "stale"
            if valid and name in ("sd_total_bytes", "sd_used_bytes", "sd_free_bytes"):
                age = combined.get("sd_capacity_age_ms")
                if combined.get("sd_capacity_known") is not True or combined.get("recording") is True or (
                    not isinstance(age, (int, float)) or not math.isfinite(age) or
                    age > SD_CAPACITY_FRESH_MS
                ):
                    freshness = "stale"
            sample_uptime_ms = None
            sample_time_basis = None
            if valid and name in FIELD_SAMPLE_TIME:
                if selected == "wifi":
                    stamp = combined.get(FIELD_SAMPLE_TIME[name])
                    if isinstance(stamp, int) and not isinstance(stamp, bool) and stamp >= 0:
                        sample_uptime_ms = stamp
                        sample_time_basis = "sensor_sample"
                elif selected in ("radio", "simulation"):
                    stamp = combined.get("uptime_ms")
                    if isinstance(stamp, int) and not isinstance(stamp, bool) and stamp >= 0:
                        sample_uptime_ms = stamp
                        sample_time_basis = "packet_uptime"
            fields[name] = {
                "type": kind, "unit": unit, "origin": origin,
                "source": selected if valid else None,
                "received_utc": received_utc if valid else None,
                "sample_uptime_ms": sample_uptime_ms,
                "sample_time_basis": sample_time_basis,
                "valid": valid, "freshness": freshness,
            }
        fields["battery_estimate_pct"] = {
            "type": "integer", "unit": "%", "origin": "calculated",
            "source": selected if battery_pct is not None else None,
            "received_utc": received_utc if battery_pct is not None else None,
            "sample_uptime_ms": fields["battery_v"]["sample_uptime_ms"] if battery_pct is not None else None,
            "sample_time_basis": fields["battery_v"]["sample_time_basis"] if battery_pct is not None else None,
            "valid": battery_pct is not None,
            "freshness": combined["link_status"] if battery_pct is not None else "missing",
        }
        fields["battery_measurement_valid"] = {
            "type": "boolean", "unit": None, "origin": "calculated",
            "source": selected if combined.get("battery_v") is not None else None,
            "received_utc": received_utc if combined.get("battery_v") is not None else None,
            "sample_uptime_ms": fields["battery_v"]["sample_uptime_ms"],
            "sample_time_basis": fields["battery_v"]["sample_time_basis"],
            "valid": True,
            "freshness": combined["link_status"] if combined.get("battery_v") is not None else "missing",
        }
        fields["battery_warning"] = {
            "type": "string", "unit": None, "origin": "calculated",
            "source": selected if battery_pct is not None else None,
            "received_utc": received_utc if battery_pct is not None else None,
            "sample_uptime_ms": fields["battery_v"]["sample_uptime_ms"] if battery_pct is not None else None,
            "sample_time_basis": fields["battery_v"]["sample_time_basis"] if battery_pct is not None else None,
            "valid": battery_pct is not None,
            "freshness": combined["link_status"] if battery_pct is not None else "missing",
        }
        combined["field_meta"] = fields
        return combined

    def snapshot(self) -> dict:
        with self.lock:
            return self._snapshot_locked(self.clock())

    def recent_history(self, limit: int = 600) -> list[dict]:
        with self.lock:
            return list(self.history[-max(1, min(limit, 1200)):])
