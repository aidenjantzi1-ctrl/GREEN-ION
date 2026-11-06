"""Offline GREEN ION V12 Windows Ground Station.

The default mode keeps the local dashboard running while it independently
reconnects to an Arduino Uno receiver and the Feather Wi-Fi status endpoint.
Simulation is available only through the explicit ``--sample`` option.
"""

from __future__ import annotations

import argparse
from collections import deque
from datetime import datetime, timezone
from http.server import ThreadingHTTPServer, SimpleHTTPRequestHandler
import json
import math
import ipaddress
import os
from pathlib import Path
import re
import ssl
import threading
import time
import tempfile
import uuid
import zipfile
from urllib.error import URLError
from urllib.request import Request, urlopen
from urllib.parse import parse_qs, quote, urlparse
import webbrowser

from flight_library import FlightImportError, FlightLibrary, MAX_SENSOR_BYTES
from companion_security import CompanionSecurity
from session_library import MAX_SESSION_BYTES, SessionImportError, SessionLibrary
from library_backup import BackupError, MAX_ARCHIVE_BYTES, create_backup, restore_backup
from sensor_log import SensorLogError
from telemetry_manager import SessionWriter, TelemetryManager, valid_coordinate

ROOT = Path(__file__).resolve().parent
GROUND_STATION_VERSION = "V12-development"
DATA_DIR = ROOT / "data"
MANAGER = TelemetryManager(SessionWriter(DATA_DIR))
STOP = threading.Event()
CONFIG = {"serial_port": "auto", "feather_url": "http://192.168.4.1"}
CONFIG_LOCK = threading.Lock()
FLIGHTS = FlightLibrary(DATA_DIR / "flights")
SESSIONS = SessionLibrary(DATA_DIR)
PREFLIGHT_DIR = DATA_DIR / "preflight"
PREFLIGHT_LOCK = threading.Lock()
DIAGNOSTIC_LOG = DATA_DIR / "diagnostic-history.jsonl"
DIAGNOSTIC_LOCK = threading.Lock()


class LimitedReader:
    def __init__(self, source, remaining: int) -> None:
        self.source = source
        self.remaining = remaining

    def read(self, size: int = -1) -> bytes:
        if self.remaining <= 0:
            return b""
        amount = self.remaining if size < 0 else min(size, self.remaining)
        data = self.source.read(amount)
        self.remaining -= len(data)
        return data


def json_response(handler: SimpleHTTPRequestHandler, payload: object, status: int = 200) -> None:
    body = json.dumps(payload, allow_nan=False, separators=(",", ":")).encode()
    handler.send_response(status)
    handler.send_header("Content-Type", "application/json")
    handler.send_header("Cache-Control", "no-store")
    handler.send_header("Content-Length", str(len(body)))
    handler.end_headers()
    handler.wfile.write(body)


def bytes_response(handler: SimpleHTTPRequestHandler, body: bytes, content_type: str,
                   filename: str | None = None) -> None:
    handler.send_response(200)
    handler.send_header("Content-Type", content_type)
    handler.send_header("Cache-Control", "no-store")
    if filename:
        handler.send_header("Content-Disposition", f'attachment; filename="{filename}"')
    handler.send_header("Content-Length", str(len(body)))
    handler.end_headers()
    handler.wfile.write(body)


def file_response(handler: SimpleHTTPRequestHandler, path: Path, content_type: str,
                  filename: str) -> None:
    """Serve a verified original with bounded memory use."""
    handler.send_response(200)
    handler.send_header("Content-Type", content_type)
    handler.send_header("Cache-Control", "no-store")
    handler.send_header("Content-Disposition", f'attachment; filename="{filename}"')
    handler.send_header("Content-Length", str(path.stat().st_size))
    handler.end_headers()
    with path.open("rb") as source:
        while chunk := source.read(65536):
            handler.wfile.write(chunk)


def stream_response(handler: SimpleHTTPRequestHandler, chunks, content_type: str,
                    filename: str) -> None:
    """Stream a derived export to EOF without allocating its full size."""
    handler.send_response(200)
    handler.send_header("Content-Type", content_type)
    handler.send_header("Cache-Control", "no-store")
    handler.send_header("Content-Disposition", f'attachment; filename="{filename}"')
    handler.send_header("Connection", "close")
    handler.close_connection = True
    handler.end_headers()
    for chunk in chunks:
        handler.wfile.write(chunk)


def feather_post(path: str, timeout: float = 4.0, base_url: str | None = None) -> tuple[int, dict]:
    with CONFIG_LOCK:
        base = base_url or CONFIG["feather_url"]
    request = Request(base + path, data=b"", method="POST", headers={"Accept": "application/json"})
    try:
        with urlopen(request, timeout=timeout) as response:
            return response.status, json.loads(response.read(65536))
    except URLError as error:
        if getattr(error, "code", None) is not None:
            try:
                return error.code, json.loads(error.read(65536))
            except (AttributeError, json.JSONDecodeError):
                pass
        raise


def feather_status() -> tuple[str, dict]:
    with CONFIG_LOCK:
        base = CONFIG["feather_url"]
    request = Request(base + "/api/status", headers={"Accept": "application/json"})
    with urlopen(request, timeout=1.5) as response:
        return base, json.loads(response.read(65536))


def feather_capacity() -> dict:
    """Refresh SD capacity through the Feather's idle-only file-list endpoint."""
    with CONFIG_LOCK:
        base = CONFIG["feather_url"]
    with urlopen(Request(base + "/api/files", headers={"Accept": "application/json"}), timeout=3.0) as response:
        payload = json.loads(response.read(131072))
    if not isinstance(payload, dict):
        raise ValueError("Feather capacity response was not an object")
    return payload


def prepare_recorder() -> tuple[int, dict]:
    """Restart stationary calibration only with current V12 Feather evidence."""
    snapshot = MANAGER.snapshot()
    mode = snapshot.get("recorder_mode")
    mode_meta = snapshot.get("field_meta", {}).get("recorder_mode", {})
    if (snapshot.get("active_source") != "wifi" or
            snapshot.get("sources", {}).get("wifi", {}).get("status") != "live" or
            mode_meta.get("freshness") != "live"):
        return 409, {"success": False, "error": "A live V12 Feather Wi-Fi recorder status is required."}
    stationary_phase = snapshot.get("flight_state") in (0, 1)
    completed_flight = mode == 3 and snapshot.get("flight_state") == 6
    if snapshot.get("recording") or mode == 2 or not (stationary_phase or completed_flight):
        return 409, {"success": False, "error": "Cannot restart calibration during a flight or recording."}
    if mode not in (0, 1, 3, 4):
        return 409, {"success": False, "error": "Recorder mode is unknown; calibration was not changed."}
    try:
        base, current = feather_status()
        if (not isinstance(current, dict) or current.get("recording") is True or
                current.get("recorder_mode") not in (0, 1, 3, 4) or
                (current.get("flight_state") not in (0, 1) and not
                 (current.get("recorder_mode") == 3 and current.get("flight_state") == 6))):
            return 409, {"success": False, "error": "Feather is no longer stationary and idle; calibration was not sent."}
        return feather_post("/api/calibrate", base_url=base)
    except (OSError, URLError, TimeoutError, ValueError, json.JSONDecodeError) as error:
        return 502, {"success": False, "error": f"Feather calibration request failed: {error}"}


def stop_recorder(confirmation: str, filename: str) -> tuple[int, dict]:
    """Manually finalize the current V12 recording after explicit user confirmation."""
    if confirmation != "STOP AND SAVE":
        return 400, {"success": False, "error": "Type STOP AND SAVE to confirm manual finalization."}
    snapshot = MANAGER.snapshot()
    fields = snapshot.get("field_meta", {})
    if (snapshot.get("active_source") != "wifi" or
            snapshot.get("sources", {}).get("wifi", {}).get("status") != "live" or
            any(fields.get(name, {}).get("freshness") != "live"
                for name in ("recording", "recorder_mode", "log_file"))):
        return 409, {"success": False, "error": "Fresh V12 Feather Wi-Fi recorder and file status is required."}
    if snapshot.get("recording") is not True or snapshot.get("recorder_mode") != 2:
        return 409, {"success": False, "error": "No active V12 flight recording is available to stop."}
    current_file = snapshot.get("log_file")
    if not isinstance(current_file, str) or not current_file or filename != current_file:
        return 409, {"success": False, "error": "Flight file changed or is unknown; refresh Mission Control."}
    try:
        base, current = feather_status()
        if (not isinstance(current, dict) or current.get("recording") is not True or
                current.get("recorder_mode") != 2 or
                current.get("log_file") != filename):
            return 409, {"success": False, "error": "Feather recording changed; Stop/Save was not sent."}
        return feather_post("/api/stop", base_url=base)
    except (OSError, URLError, TimeoutError, ValueError, json.JSONDecodeError) as error:
        return 502, {"success": False, "error": f"Feather stop/save request failed: {error}"}


BENCH_FILE = re.compile(r"^/TEST[0-9]{3}\.BIN$")


def bench_battery_ready(value: object) -> bool:
    return (isinstance(value, (int, float)) and not isinstance(value, bool) and
            math.isfinite(value) and 3.5 <= value <= 4.25)


def start_bench_recording(confirmation: str) -> tuple[int, dict]:
    if confirmation != "START BENCH TEST":
        return 400, {"success": False, "error": "Type START BENCH TEST to confirm a motor-free bench recording."}
    snapshot = MANAGER.snapshot()
    fields = snapshot.get("field_meta", {})
    required = ("firmware_version", "recording", "recorder_mode", "sd_ok", "battery_v")
    if (snapshot.get("active_source") != "wifi" or
            snapshot.get("sources", {}).get("wifi", {}).get("status") != "live" or
            any(fields.get(name, {}).get("freshness") != "live" for name in required) or
            not str(snapshot.get("firmware_version", "")).startswith("V12") or
            snapshot.get("recording") is not False or snapshot.get("recorder_mode") != 1 or
            snapshot.get("sd_ok") is not True or not bench_battery_ready(snapshot.get("battery_v"))):
        return 409, {"success": False, "error": "Live calibrated V12 Feather, idle recorder, ready SD and battery at least 3.5 V are required."}
    try:
        base, current = feather_status()
        if (not isinstance(current, dict) or
                not str(current.get("firmware_version", "")).startswith("V12") or
                current.get("recording") is not False or current.get("recorder_mode") != 1 or
                current.get("sd_ok") is not True or not bench_battery_ready(current.get("battery_v"))):
            return 409, {"success": False, "error": "Feather readiness changed; bench recording was not started."}
        return feather_post("/api/bench/start", base_url=base)
    except (OSError, URLError, TimeoutError, ValueError, json.JSONDecodeError) as error:
        return 502, {"success": False, "error": f"Could not start Feather bench recording: {error}"}


def stop_bench_recording(confirmation: str, filename: str) -> tuple[int, dict]:
    if confirmation != "STOP BENCH TEST" or not isinstance(filename, str) or not BENCH_FILE.fullmatch(filename):
        return 400, {"success": False, "error": "A current TESTnnn.BIN file and STOP BENCH TEST confirmation are required."}
    return stop_recorder("STOP AND SAVE", filename)


def verify_bench_recording(filename: str) -> tuple[int, dict]:
    """Import a completed TEST BIN without modifying the Feather's SD card."""
    if not isinstance(filename, str) or not BENCH_FILE.fullmatch(filename):
        return 400, {"test": "bench_file_integrity", "status": "NOT TESTED",
                     "message": "Select an exact TESTnnn.BIN file from current Feather status."}
    snapshot = MANAGER.snapshot()
    fields = snapshot.get("field_meta", {})
    if (snapshot.get("active_source") != "wifi" or
            snapshot.get("sources", {}).get("wifi", {}).get("status") != "live" or
            any(fields.get(name, {}).get("freshness") != "live" for name in
                ("firmware_version", "recording", "log_file")) or
            not str(snapshot.get("firmware_version", "")).startswith("V12") or
            snapshot.get("recording") is not False or snapshot.get("log_file") != filename):
        return 409, {"test": "bench_file_integrity", "status": "NOT TESTED",
                     "message": "Current idle V12 Feather Wi-Fi status must identify the exact TEST file."}
    try:
        base, current = feather_status()
        if (not isinstance(current, dict) or
                not str(current.get("firmware_version", "")).startswith("V12") or
                current.get("recording") is not False or current.get("log_file") != filename):
            return 409, {"test": "bench_file_integrity", "status": "NOT TESTED",
                         "message": "Feather file or recorder state changed; no download was started."}
        url = base + "/api/download?name=" + quote(filename.lstrip("/"))
        with urlopen(Request(url, headers={"Accept": "application/octet-stream"}), timeout=20.0) as response:
            expected_header = response.headers.get("X-Green-Ion-Bytes") or response.headers.get("Content-Length")
            expected = int(expected_header) if expected_header else None
            flight = FLIGHTS.import_stream(filename.lstrip("/"), response, expected_bytes=expected,
                                           source_kind="bench_verification")
        summary = flight["summary"]
        status = ("PASS" if summary["records"] > 0 and summary["duration_s"] >= 5 and
                  not summary["warnings"] else "WARNING")
        message = (f"{filename}: {summary['records']} RKT4 records over {summary['duration_s']:.2f} s; "
                   f"{summary['timestamp_gaps']} timestamp gaps; {summary['invalid_records']} invalid sensor records; "
                   f"{summary['trailing_bytes']} trailing bytes; "
                   f"warnings: {', '.join(summary['warnings']) if summary['warnings'] else 'none'}; "
                   f"SHA-256 {flight['sha256']}. "
                   "This validates the downloaded file only; a physical Wi-Fi disconnect is not proven.")
        return 200, {"test": "bench_file_integrity", "status": status, "message": message,
                     "file": filename, "sha256": flight["sha256"], "summary": summary,
                     "hardware_test_started": False}
    except FlightImportError as error:
        return 422, {"test": "bench_file_integrity", "status": "FAIL", "file": filename,
                     "message": f"RKT4 validation failed for {filename}: {error}. Keep the original SD file."}
    except (OSError, URLError, TimeoutError, ValueError, json.JSONDecodeError) as error:
        return 502, {"test": "bench_file_integrity", "status": "NOT TESTED", "file": filename,
                     "message": f"Could not download {filename}: {error}. Keep the original SD file."}


def battery_stability(points: list[dict], source: str | None,
                      now: datetime | None = None) -> tuple[str, str]:
    """Classify a short stationary voltage window using distinct sensor samples."""
    if source not in ("wifi", "radio"):
        return "NOT TESTED", "Live hardware battery samples are unavailable."
    now = now or datetime.now(timezone.utc)
    samples: dict[int, tuple[datetime, float]] = {}
    for point in points:
        if point.get("source") != source:
            continue
        stamp = point.get("battery_sample_uptime_ms") if source == "wifi" else point.get("uptime_ms")
        voltage = point.get("battery_v")
        if (not isinstance(stamp, int) or isinstance(stamp, bool) or
                not isinstance(voltage, (int, float)) or isinstance(voltage, bool) or
                not math.isfinite(voltage) or not 3.0 <= voltage <= 4.25):
            continue
        try:
            received = datetime.fromisoformat(point["received_utc"])
            age = (now - received).total_seconds()
        except (KeyError, TypeError, ValueError):
            continue
        if 0 <= age <= 20:
            samples[stamp] = (received, float(voltage))
    ordered = sorted(samples.values())
    if len(ordered) < 5 or (ordered[-1][0] - ordered[0][0]).total_seconds() < 2:
        return "NOT TESTED", "Collect at least five distinct live battery samples over two stationary seconds."
    values = [value for _, value in ordered]
    spread = max(values) - min(values)
    message = (f"{len(values)} distinct samples over {(ordered[-1][0]-ordered[0][0]).total_seconds():.1f} s; "
               f"voltage range {spread:.3f} V. This is not a battery endurance test.")
    return ("PASS" if spread <= 0.08 else "WARNING"), message


def gps_position_stability(points: list[dict], now: datetime | None = None) -> tuple[str, str]:
    """Check stationary GPS scatter from distinct, recent Feather fixes."""
    now = now or datetime.now(timezone.utc)
    fixes: dict[int, tuple[datetime, float, float]] = {}
    for point in points:
        if (point.get("source") != "wifi" or point.get("gps_status") != "live" or
                not valid_coordinate(point.get("latitude"), point.get("longitude"))):
            continue
        stamp = point.get("gps_fix_uptime_ms")
        fix_age = point.get("gps_fix_age_ms")
        if (not isinstance(stamp, int) or isinstance(stamp, bool) or
                not isinstance(fix_age, (int, float)) or isinstance(fix_age, bool) or
                not math.isfinite(fix_age) or not 0 <= fix_age <= 3000):
            continue
        try:
            received = datetime.fromisoformat(point["received_utc"])
            age_seconds = (now - received).total_seconds()
        except (KeyError, TypeError, ValueError):
            continue
        if 0 <= age_seconds <= 30:
            fixes[stamp] = (received, float(point["latitude"]), float(point["longitude"]))
    ordered = sorted(fixes.values())
    if len(ordered) < 5 or (ordered[-1][0] - ordered[0][0]).total_seconds() < 4:
        return "NOT TESTED", "Collect at least five distinct fresh Feather GPS fixes over four stationary seconds."
    radius = 6371008.8
    maximum = 0.0
    for index, (_, lat_a, lon_a) in enumerate(ordered):
        phi_a = math.radians(lat_a)
        for _, lat_b, lon_b in ordered[index + 1:]:
            delta_phi = math.radians(lat_b - lat_a)
            delta_lambda = math.radians(lon_b - lon_a)
            haversine = math.sin(delta_phi / 2) ** 2 + math.cos(phi_a) * math.cos(math.radians(lat_b)) * math.sin(delta_lambda / 2) ** 2
            maximum = max(maximum, 2 * radius * math.asin(min(1.0, math.sqrt(haversine))))
    status = "PASS" if maximum <= 30 else "WARNING"
    return status, (f"{len(ordered)} distinct fixes over {(ordered[-1][0]-ordered[0][0]).total_seconds():.1f} s; "
                    f"maximum pairwise separation {maximum:.1f} m (30 m software warning threshold). "
                    "Stability does not prove position accuracy.")


def preflight_report(actions: dict | None = None) -> dict:
    snapshot = MANAGER.snapshot()
    wifi = snapshot["sources"]["wifi"]
    radio = snapshot["sources"]["radio"]
    checks = []
    corrections = {
        "Feather Wi-Fi": "Reconnect Windows to Feather Wi-Fi and verify the configured Feather address in Settings.",
        "Feather API": "Check Feather power and open its /api/status address from the laptop.",
        "Telemetry source freshness": "Reconnect a telemetry link; inspect source errors in Diagnostics.",
        "Feather firmware version": "Flash the matching V12 Feather firmware, then reconnect and verify its reported version.",
        "RKT4 recording format": "Use V12 recorder firmware that reports RKT4 v2 before relying on Analysis imports.",
        "Recorder sample rate": "Check the Feather configuration and firmware build; expected rate is 50 Hz.",
        "Uno USB port": "Connect the Uno by USB and select its COM port in Settings; USB detection does not prove radio reception.",
        "Uno firmware version": "Connect the Uno over USB and verify its V12 boot-status version.",
        "Uno receiver protocol": "Install a matching v2 receiver sketch and inspect its USB boot status.",
        "Ground LoRa module": "Inspect the Uno RFM95W wiring, power and antenna; then restart the receiver.",
        "LoRa packets": "With both radios and antennas connected, confirm actual packets in Diagnostics. Wi-Fi-only operation remains available.",
        "Telemetry protocol": "Use matching V12 Feather and Uno protocol v2 firmware.",
        "Feather LoRa module": "Check Feather RFM95W wiring and antenna; Wi-Fi-only operation remains available.",
        "SD initialization": "Power down before reseating the SD card; inspect its wiring and format, then rerun Preflight.",
        "SD free space": "Load the idle Feather recording list to refresh capacity; if space is low, copy and verify files before managing the SD card.",
        "SD write/read/cleanup": "Inspect SD wiring, free space and the reported temporary-file cleanup result; retain all flight files.",
        "BMP388": "Inspect the BMP388 connection and power, then retry with the Feather stationary.",
        "Stationary barometer baseline": "Keep the Feather motionless, prepare the recorder, and retry calibration if pressure noise remains high.",
        "Autonomous recorder watcher": "Check SD and barometer results, then prepare the recorder and confirm WATCHING mode.",
        "LSM6DSO32": "Keep the Feather still, run the IMU test, and check its cable, 32 g range and gyro bias result.",
        "PA1010D communication": "Run the GPS test; inspect its cable and NMEA counts. Move outdoors only to test satellite fix.",
        "Stationary sensor test request": "Keep the Feather motionless and rerun Full Preflight after the recorder is idle.",
        "GPS fix": "Move outdoors under open sky and wait for a fresh fix; GPS communication is checked separately.",
        "Stationary GPS position stability": "Keep the Feather still outdoors for multiple fixes; inspect sky view and antenna placement if positions jump.",
        "GPS UTC time": "Wait for recent RMC date and NMEA time; check GPS reception and the laptop clock if they disagree.",
        "Battery": "Charge or replace the LiPo and recheck its voltage before recording.",
        "Stationary battery stability": "Collect five distinct live samples over two stationary seconds; inspect power wiring if voltage varies.",
        "Ground persistence": "Check Ground Station data-directory write access and available laptop disk space.",
        "Interrupted-file recovery scan": "If recovery items are pending, preserve the SD card and inspect BIN, journal and RCV files before another recording.",
    }
    def add(name: str, status: str, message: str) -> None:
        checks.append({"name": name, "status": status, "message": message,
                       "correction": corrections.get(name) if status != "PASS" else None})
    def live(name: str) -> bool:
        meta = snapshot.get("field_meta", {}).get(name, {})
        return (snapshot.get("active_source") != "simulation" and
                meta.get("valid") is True and meta.get("freshness") == "live")
    add("Feather Wi-Fi", "PASS" if wifi["status"] == "live" else "FAIL",
        "Live Feather status is available." if wifi["status"] == "live" else (wifi["error"] or "No live Feather status."))
    add("Feather API", "PASS" if wifi["status"] == "live" else "NOT TESTED",
        "Fresh /api/status response received." if wifi["status"] == "live" else
        "Connect Windows to Feather Wi-Fi and confirm the status endpoint responds.")
    add("Ground Station server", "PASS", "This report was served by the local Ground Station.")
    add("Ground Station version", "PASS", f"Local Ground Station {GROUND_STATION_VERSION}.")
    add("Telemetry source freshness", "WARNING" if snapshot.get("active_source") == "simulation" else
        "PASS" if snapshot["link_status"] == "live" else
        "WARNING" if snapshot["link_status"] == "stale" else "NOT TESTED",
        f"Selected source: {snapshot.get('active_source') or 'none'}; link is {snapshot['link_status']}."
        + (" Simulation is software evidence only." if snapshot.get("active_source") == "simulation" else ""))
    version = snapshot.get("firmware_version") if live("firmware_version") else None
    add("Feather firmware version", "PASS" if isinstance(version, str) and version.startswith("V12") else
        "WARNING" if version is not None else "NOT TESTED",
        f"Reported firmware: {version}." if version is not None else
        "V12 firmware version has not been reported by a live Feather connection.")
    format_version = snapshot.get("recording_format_version") if live("recording_format_version") else None
    add("RKT4 recording format", "PASS" if format_version == 2 else
        "WARNING" if format_version is not None else "NOT TESTED",
        f"Reported recording format version: {format_version}; expected RKT4 v2."
        if format_version is not None else "Connect the V12 Feather to verify recording format v2.")
    sample_rate = snapshot.get("sample_rate_hz") if live("sample_rate_hz") else None
    add("Recorder sample rate", "PASS" if sample_rate == 50 else
        "WARNING" if sample_rate is not None else "NOT TESTED",
        f"Reported sample rate: {sample_rate} Hz; current V12 configuration expects 50 Hz."
        if sample_rate is not None else "Current sample rate is unavailable.")
    ports = available_ports()
    receiver = snapshot.get("receiver_status") or {}
    identified_uno = [port for port in ports if
                      "arduino" in (port["description"] + port["hwid"]).lower() or
                      "uno" in port["description"].lower()]
    add("Uno USB port", "PASS" if receiver or radio["status"] == "live" or len(identified_uno) == 1 else
        "WARNING" if ports else "NOT TESTED",
        "Validated Uno telemetry confirms an active USB connection." if radio["status"] == "live" else
        "Uno boot status was received over USB; LoRa packet reception is separate." if receiver else
        f"{len(ports)} serial port(s) detected; select and confirm the Uno COM port in Settings."
        if ports else "Connect the Arduino Uno by USB; the LoRa module can still be disconnected.")
    receiver_version = receiver.get("firmware_version")
    add("Uno firmware version", "PASS" if isinstance(receiver_version, str) and receiver_version.startswith("V12") else
        "WARNING" if receiver_version is not None else "NOT TESTED",
        f"Uno reports {receiver_version}." if receiver_version is not None else "No Uno boot-status version received.")
    add("Uno receiver protocol", "PASS" if receiver.get("protocol") == 2 or radio["status"] == "live" else
        "WARNING" if receiver else "NOT TESTED",
        f"Uno boot status reports protocol v{receiver.get('protocol')}; expected v2."
        if receiver else "No validated Uno boot status or telemetry; USB detection alone is insufficient.")
    add("Ground LoRa module", "PASS" if receiver.get("ok") is True else
        "FAIL" if receiver.get("ok") is False else "NOT TESTED",
        "Uno reports its LoRa module initialized; packet reception is checked separately."
        if receiver.get("ok") is True else
        f"Uno reported {receiver.get('error', 'radio initialization failure')}; check module wiring and antenna."
        if receiver else "No Uno radio initialization result received over USB.")
    add("LoRa packets", "PASS" if radio["status"] == "live" else "NOT TESTED",
        "Validated radio packets received." if radio["status"] == "live"
        else "Connect both LoRa modules and antennas, then verify actual packets; USB alone is insufficient.")
    wifi_protocol = snapshot.get("telemetry_protocol_version") if live("telemetry_protocol_version") else None
    radio_protocol = snapshot.get("protocol") if live("protocol") else None
    protocol = wifi_protocol if wifi_protocol is not None else radio_protocol
    add("Telemetry protocol", "PASS" if protocol == 2 else
        "WARNING" if protocol is not None else "NOT TESTED",
        f"Reported telemetry protocol v{protocol}; expected v2." if protocol is not None
        else "No current V12 protocol version reported by Feather or Uno.")
    add("Feather LoRa module", "PASS" if live("radio_ok") and snapshot.get("radio_ok") is True else
        "FAIL" if live("radio_ok") and snapshot.get("radio_ok") is False else "NOT TESTED",
        "Feather reports its radio initialized; packet reception is checked separately."
        if live("radio_ok") and snapshot.get("radio_ok") is True else "Current Feather radio initialization evidence is unavailable or reports a fault.")
    add("SD initialization", "PASS" if live("sd_ok") and snapshot.get("sd_ok") is True else
        "FAIL" if live("sd_ok") and snapshot.get("sd_ok") is False else "NOT TESTED",
        "Current SD initialization status is ready; a write/read test is separate."
        if live("sd_ok") and snapshot.get("sd_ok") is True else "Current SD initialization evidence is unavailable or reports a fault.")
    capacity_action = (actions or {}).get("sd_capacity") or {}
    total_bytes, used_bytes = capacity_action.get("total"), capacity_action.get("used")
    direct_free = (total_bytes - used_bytes if capacity_action.get("success") is True and
                   isinstance(total_bytes, int) and not isinstance(total_bytes, bool) and
                   isinstance(used_bytes, int) and not isinstance(used_bytes, bool) and
                   0 <= used_bytes <= total_bytes else None)
    free_bytes = direct_free if direct_free is not None else (
        snapshot.get("sd_free_bytes") if live("sd_free_bytes") else None)
    add("SD free space", "PASS" if isinstance(free_bytes, int) and free_bytes >= 8 * 1024 * 1024 else
        "WARNING" if isinstance(free_bytes, int) and free_bytes >= 0 else "NOT TESTED",
        f"Fresh idle SD capacity reports {free_bytes / (1024 * 1024):.1f} MiB free; 8 MiB is a conservative software threshold, not a duration guarantee."
        if isinstance(free_bytes, int) else
        "SD capacity is unavailable or stale; an idle file listing refreshes the reading."
        + (f" Refresh error: {capacity_action['error']}." if capacity_action.get("error") else ""))
    sd_action = (actions or {}).get("sd_test")
    add("SD write/read/cleanup", "PASS" if sd_action and sd_action.get("http_status") == 200 and sd_action.get("success") is True else
        "FAIL" if sd_action else "NOT TESTED",
        sd_action.get("message", "SD test failed; inspect the Feather response.") if sd_action else
        "Run Full Preflight to verify a dedicated temporary file; never use a flight recording.")
    add("BMP388", "PASS" if live("barometer_ready") and snapshot.get("barometer_ready") is True else
        "FAIL" if live("barometer_ready") and snapshot.get("barometer_ready") is False else "NOT TESTED",
        "Current barometer runtime status is ready." if live("barometer_ready") and snapshot.get("barometer_ready") is True
        else "Current barometer status is unavailable or reports a fault; inspect Diagnostics.")
    baseline = snapshot.get("calibration_baseline_pa") if live("calibration_baseline_pa") else None
    noise = snapshot.get("calibration_noise_m") if live("calibration_noise_m") else None
    calibration_ready = (isinstance(baseline, (int, float)) and not isinstance(baseline, bool) and
                         isinstance(noise, (int, float)) and not isinstance(noise, bool) and
                         math.isfinite(baseline) and math.isfinite(noise) and
                         baseline > 0 and 0 <= noise <= 0.55)
    add("Stationary barometer baseline", "PASS" if calibration_ready else
        "WARNING" if live("recorder_mode") and snapshot.get("recorder_mode") in (0, 1, 4) else "NOT TESTED",
        f"Baseline {baseline:.1f} Pa; altitude noise {noise:.3f} m (firmware limit 0.55 m)."
        if calibration_ready else "Keep the rocket stationary and prepare the recorder; inspect calibration noise and retry if unstable.")
    mode = snapshot.get("recorder_mode") if live("recorder_mode") else None
    add("Autonomous recorder watcher", "PASS" if mode == 1 and calibration_ready and
        live("sd_ok") and snapshot.get("sd_ok") is True else
        "FAIL" if mode == 4 else "WARNING" if mode is not None else "NOT TESTED",
        "V12 recorder is watching for launch with valid baseline and initialized SD."
        if mode == 1 and calibration_ready and live("sd_ok") and snapshot.get("sd_ok") is True else
        "Recorder is not yet confirmed ready to watch; check calibration, SD and mode in Mission Control.")
    imu_result = snapshot.get("imu_test_result") if live("imu_test_result") else None
    imu_message = (snapshot.get("imu_test_message") if live("imu_test_message") else
                   "Run the stationary IMU hardware test in Preflight.")
    if live("imu_test_samples") and live("imu_test_duration_ms"):
        imu_message += (f" {snapshot['imu_test_samples']} valid samples in a "
                        f"{snapshot['imu_test_duration_ms'] / 1000:.0f} s test.")
    if live("imu_test_peak_deviation_g"):
        imu_message += f" Peak acceleration deviation: {snapshot['imu_test_peak_deviation_g']:.3f} g."
    add("LSM6DSO32", imu_result if imu_result in ("PASS", "WARNING", "FAIL") else
        "WARNING" if imu_result else "NOT TESTED",
        imu_message)
    gps_result = snapshot.get("gps_test_result") if live("gps_test_result") else None
    gps_message = (snapshot.get("gps_test_message") if live("gps_test_message") else
                   "Run the GPS communication test; a satellite fix is separate.")
    if live("gps_test_nmea") and live("gps_test_parsed"):
        gps_message += (f" {snapshot['gps_test_nmea']} NMEA sentences received; "
                        f"{snapshot['gps_test_parsed']} parsed.")
    add("PA1010D communication", gps_result if gps_result in ("PASS", "WARNING", "FAIL") else
        "WARNING" if gps_result else "NOT TESTED",
        gps_message)
    sensor_action = (actions or {}).get("sensor_test")
    add("Stationary sensor test request", "WARNING" if sensor_action and sensor_action.get("http_status") == 202 else
        "FAIL" if sensor_action else "NOT TESTED",
        "Feather accepted the sensor tests; refresh after completion to see measured results."
        if sensor_action and sensor_action.get("http_status") == 202 else
        "Run Full Preflight with the rocket stationary." if not sensor_action else
        sensor_action.get("message", "Feather did not accept the sensor tests."))
    fix_live = snapshot.get("gps_status") == "live" and live("latitude") and live("longitude")
    add("GPS fix", "PASS" if fix_live else "WARNING" if snapshot.get("gps_status") == "stale" else "NOT TESTED",
        f"Current valid GPS position; {snapshot.get('satellites', 'unknown')} satellites, fix quality {snapshot.get('gps_fix_quality', 'unknown')}."
        if fix_live else "A current outdoor GPS fix is unavailable; GPS communication can still pass independently.")
    stability_status, stability_message = (gps_position_stability(MANAGER.recent_history(200))
                                          if fix_live and snapshot.get("active_source") == "wifi" else
                                          ("NOT TESTED", "A current Feather Wi-Fi GPS fix is required before checking stationary position scatter."))
    add("Stationary GPS position stability", stability_status, stability_message)
    utc_fields = ("gps_utc_valid", "gps_utc_hour", "gps_utc_minute", "gps_utc_second")
    utc_available = all(live(name) for name in utc_fields)
    utc_valid = utc_available and snapshot.get("gps_utc_valid") is True
    utc_status = "NOT TESTED"
    utc_message = "Recent GPS date and time have not been verified from live Feather Wi-Fi."
    if utc_valid:
        hour, minute, second = (snapshot[name] for name in utc_fields[1:])
        if all(isinstance(value, int) and not isinstance(value, bool) for value in
               (hour, minute, second)) and 0 <= hour <= 23 and 0 <= minute <= 59 and 0 <= second <= 60:
            gps_seconds = hour * 3600 + minute * 60 + second
            now = datetime.now(timezone.utc)
            laptop_seconds = now.hour * 3600 + now.minute * 60 + now.second
            gap = abs(gps_seconds - laptop_seconds)
            gap = min(gap, 86400 - gap)
            utc_status = "PASS" if gap <= 120 else "WARNING"
            utc_message = (f"GPS {hour:02d}:{minute:02d}:{second:02d} UTC versus laptop UTC time of day: "
                           f"{gap} s apart. GPS date and laptop date are not compared.")
        else:
            utc_status = "WARNING"
            utc_message = "Feather marked GPS UTC current but its clock components are invalid."
    elif (live("gps_utc_valid") and
          isinstance(snapshot.get("gps_nmea_sentences"), int) and
          snapshot["gps_nmea_sentences"] > 0):
        utc_status = "WARNING"
        utc_message = "GPS is sending NMEA, but a recent valid date and time are unavailable."
    add("GPS UTC time", utc_status, utc_message)
    battery = snapshot.get("battery_v")
    battery_live = live("battery_v")
    battery_valid = snapshot.get("battery_measurement_valid") is True
    battery_status = (
        "NOT TESTED" if not battery_live or battery is None else
        "FAIL" if not battery_valid or battery < 3.5 else
        "WARNING" if battery < 3.65 else "PASS"
    )
    battery_text = (f"{battery:.3f} V" if isinstance(battery, (int, float)) and
                    not isinstance(battery, bool) else str(battery))
    add("Battery", battery_status,
        "Battery reading is unavailable or stale." if not battery_live or battery is None else
        f"Reported battery: {battery_text}; outside plausible one-cell LiPo range."
        if not battery_valid else
        f"Reported battery: {battery_text}; power classification {snapshot.get('battery_warning', 'unknown')}. "
        "One reading does not verify charge endurance or voltage stability.")
    stability_status, stability_message = battery_stability(
        MANAGER.recent_history(200), snapshot.get("active_source")) if battery_live else (
            "NOT TESTED", "A live hardware battery stream is required for a stability window.")
    add("Stationary battery stability", stability_status, stability_message)
    persistence_ok = MANAGER.writer is not None and MANAGER.writer.directory.is_dir()
    add("Ground persistence", "PASS" if persistence_ok else "FAIL",
        "Append-only session directory is available." if persistence_ok else "Session directory is unavailable.")
    recovery_fresh = all(live(name) for name in
                         ("recovery_status", "recovery_pending", "recovered_logs"))
    pending = snapshot.get("recovery_pending") if recovery_fresh else None
    recovered = snapshot.get("recovered_logs") if recovery_fresh else None
    recovery_text = snapshot.get("recovery_status") if recovery_fresh else ""
    scan_done = (recovery_fresh and isinstance(pending, int) and
                 isinstance(recovered, int) and pending >= 0 and recovered >= 0 and
                 isinstance(recovery_text, str) and
                 (recovery_text == "No interrupted logs detected" or
                  "interrupted log(s) reported" in recovery_text or
                  "recovery item(s) pending inspection" in recovery_text))
    add("Interrupted-file recovery scan", "WARNING" if scan_done and pending else
        "PASS" if scan_done else "NOT TESTED",
        f"Feather boot scan reported {recovered} interrupted log(s) and {pending} pending item(s). "
        "Inspect original BIN and RCV evidence; this does not prove power-loss survival."
        if scan_done else "No current Feather boot-recovery scan result is available.")
    overall = "FAIL" if any(item["status"] == "FAIL" for item in checks) else "WARNING" if any(item["status"] in ("WARNING", "NOT TESTED", "INCONCLUSIVE", "RUNNING") for item in checks) else "PASS"
    return {"overall": overall, "checks": checks, "generated_utc": datetime.now(timezone.utc).isoformat()}


def save_preflight_run(report: dict, run_id: str | None = None) -> dict:
    """Atomically save a derived preflight report without touching flight files."""
    run_id = run_id or uuid.uuid4().hex
    report = {**report, "run_id": run_id}
    with PREFLIGHT_LOCK:
        PREFLIGHT_DIR.mkdir(parents=True, exist_ok=True)
        path = PREFLIGHT_DIR / f"preflight-{run_id}.json"
        temporary = path.with_suffix(".tmp")
        temporary.write_text(json.dumps(report, allow_nan=False, indent=2), encoding="utf-8")
        temporary.replace(path)
    return report


def preflight_runs(limit: int = 30) -> list[dict]:
    if not PREFLIGHT_DIR.is_dir():
        return []
    paths = sorted(PREFLIGHT_DIR.glob("preflight-*.json"), key=lambda p: p.stat().st_mtime, reverse=True)
    reports = []
    for path in paths[:limit]:
        try:
            report = json.loads(path.read_text(encoding="utf-8"))
            reports.append({key: report.get(key) for key in
                            ("run_id", "created_utc", "run_status", "overall", "progress")})
        except (OSError, json.JSONDecodeError):
            continue
    return reports


def refresh_preflight_run() -> dict:
    runs = preflight_runs(1)
    if not runs:
        return {**preflight_report(), "run_status": "NOT RUN", "progress": "Run Full Preflight to begin."}
    run_id = runs[0]["run_id"]
    path = PREFLIGHT_DIR / f"preflight-{run_id}.json"
    report = json.loads(path.read_text(encoding="utf-8"))
    if report.get("run_status") != "RUNNING":
        return report
    refreshed = preflight_report(report.get("actions"))
    snapshot = MANAGER.snapshot()
    def new_terminal_result(name: str) -> bool:
        meta = snapshot.get("field_meta", {}).get(name, {})
        received = meta.get("received_utc")
        return (meta.get("freshness") == "live" and isinstance(received, str) and
                received > report["created_utc"] and
                snapshot.get(name) in ("PASS", "FAIL", "INCONCLUSIVE"))
    sensor_done = all(
        new_terminal_result(name) for name in ("imu_test_result", "gps_test_result")
    )
    started = datetime.fromisoformat(report["created_utc"])
    timed_out = (datetime.now(timezone.utc) - started).total_seconds() >= 45
    complete = sensor_done or timed_out
    refreshed.update(run_id=run_id, created_utc=report["created_utc"],
                     actions=report.get("actions"),
                     run_status="COMPLETE" if complete else "RUNNING",
                     progress="Sensor results received; review each check." if sensor_done else
                     "Sensor results were not confirmed within 45 seconds; review NOT TESTED checks."
                     if timed_out else "SD test finished; waiting for stationary IMU/GPS results.")
    return save_preflight_run(refreshed, run_id)


def diagnostic_test(kind: str) -> tuple[int, dict]:
    """Run only supported Feather tests; classify other checks as read-only."""
    observed = {"bmp": "BMP388", "lipo": "Battery", "wifi": "Feather API",
                "lora": "Ground LoRa module", "usb": "Uno USB port",
                "packets": "LoRa packets", "recorder": "Autonomous recorder watcher",
                "firmware": "Feather firmware version",
                "imu_result": "LSM6DSO32", "gps_result": "PA1010D communication",
                "gps_utc": "GPS UTC time",
                "recovery": "Interrupted-file recovery scan",
                "gps_stability": "Stationary GPS position stability",
                "sd_free": "SD free space"}
    if kind in observed:
        check = next(item for item in preflight_report()["checks"] if item["name"] == observed[kind])
        return 200, {"test": kind, **check, "hardware_test_started": False}
    if kind in ("parser", "checkpoint", "timing", "reconnect"):
        return 200, {"test": kind, **read_only_diagnostic(kind), "hardware_test_started": False}
    paths = {"imu": "/api/hardware-test?sensor=imu",
             "imu_extended": "/api/hardware-test?sensor=imu_extended",
             "gps": "/api/hardware-test?sensor=gps", "sd": "/api/sdtest"}
    if kind not in paths:
        return 400, {"test": kind, "status": "NOT TESTED", "message": "Unknown diagnostic test."}
    snapshot = MANAGER.snapshot()
    if (snapshot.get("recording") or snapshot.get("active_source") != "wifi" or
            snapshot.get("sources", {}).get("wifi", {}).get("status") != "live"):
        return 409, {"test": kind, "status": "NOT TESTED",
                     "message": "A live idle V12 Feather Wi-Fi connection is required; no test was started."}
    try:
        base, current = feather_status()
        if (not isinstance(current, dict) or
                not str(current.get("firmware_version", "")).startswith("V12") or
                current.get("recording") is True or
                current.get("recorder_mode") not in (0, 1, 3, 4)):
            return 409, {"test": kind, "status": "NOT TESTED",
                         "message": "Current V12 Feather identity and idle recorder state are required; no test was started."}
        code, result = feather_post(paths[kind], base_url=base)
        if kind == "sd":
            status = "PASS" if code == 200 and result.get("success") is True else "FAIL"
            message = result.get("message") or result.get("error") or "SD self-test did not return a result."
        else:
            status = "WARNING" if code == 202 and result.get("accepted") is True else "FAIL"
            message = ("Sensor test started; keep the rocket stationary for 30 seconds, then check the latest IMU result."
                       if status == "WARNING" and kind == "imu_extended" else
                       "Sensor test started; check the latest sensor result after it finishes."
                       if status == "WARNING" else result.get("error") or "Sensor test was not accepted.")
        return code, {"test": kind, "status": status, "message": message,
                      "hardware_test_started": code == 202 or kind == "sd" and code in (200, 500)}
    except (OSError, URLError, TimeoutError, ValueError, json.JSONDecodeError) as error:
        return 502, {"test": kind, "status": "NOT TESTED",
                     "message": f"Could not contact the Feather: {error}", "hardware_test_started": False}


def read_only_diagnostic(kind: str) -> dict:
    """Summarize accepted current-session evidence without initiating hardware work."""
    snapshot = MANAGER.snapshot()
    sources = snapshot.get("sources", {})
    hardware_sources = [sources.get(name, {}) for name in ("wifi", "radio")]
    if kind == "parser":
        received = sum(source.get("received", 0) for source in hardware_sources)
        errors = sum(source.get("parse_errors", 0) for source in hardware_sources)
        status = "WARNING" if errors else "PASS" if received else "NOT TESTED"
        return {"status": status, "message": f"{received} accepted Wi-Fi/LoRa packets; {errors} parser errors this session. "
                "This does not test malformed-packet rejection or disconnected hardware."}
    if kind == "checkpoint":
        fields = snapshot.get("field_meta", {})
        fresh = (snapshot.get("active_source") == "wifi" and
                 all(fields.get(key, {}).get("freshness") == "live" for key in
                     ("checkpoint_failures", "checkpoint_committed_records")))
        failures = snapshot.get("checkpoint_failures")
        records = snapshot.get("checkpoint_committed_records")
        if not fresh or not isinstance(failures, int) or not isinstance(records, int):
            return {"status": "NOT TESTED", "message": "Live Feather checkpoint counters are unavailable; connect Feather Wi-Fi."}
        status = "WARNING" if failures else "PASS" if records > 0 else "NOT TESTED"
        return {"status": status, "message": f"{records} committed RKT4 records; {failures} reported checkpoint failures. "
                "This is counter evidence only; interrupted-file recovery and power-loss survival need a separate test."}
    if kind in ("timing", "reconnect"):
        points = [point for point in MANAGER.recent_history(200)
                  if point.get("source") in ("wifi", "radio")]
        if kind == "reconnect":
            sequence = [point.get("source") for point in points]
            transitions = sum(left != right for left, right in zip(sequence, sequence[1:]))
            stage = 0
            for source in sequence:
                if stage == 0 and source == "wifi":
                    stage = 1
                elif stage == 1 and source == "radio":
                    stage = 2
                elif stage == 2 and source == "wifi":
                    stage = 3
                    break
            returned_wifi = stage == 3
            return {"status": "PASS" if returned_wifi else "NOT TESTED",
                    "message": f"{transitions} accepted-source transitions in recent hardware packets. " +
                    ("Wi-Fi to LoRa to Wi-Fi observed; physical link-loss cause is unverified."
                     if returned_wifi else "No Wi-Fi to LoRa to Wi-Fi return observed; physically test reconnection when hardware is available.")}
        uptimes = [point["uptime_ms"] for point in points if
                   isinstance(point.get("uptime_ms"), int) and not isinstance(point.get("uptime_ms"), bool)]
        if len(set(uptimes)) < 3:
            return {"status": "NOT TESTED", "message": "Need three distinct accepted hardware packet uptimes to assess ordering."}
        regressions = sum(later < earlier for earlier, later in zip(uptimes, uptimes[1:]))
        invalid_stamps = []
        if snapshot.get("active_source") == "wifi" and isinstance(snapshot.get("uptime_ms"), int):
            current_uptime = snapshot["uptime_ms"]
            for field in ("pressure_pa", "acceleration_g", "battery_v"):
                meta = snapshot.get("field_meta", {}).get(field, {})
                stamp = meta.get("sample_uptime_ms")
                if (meta.get("freshness") == "live" and meta.get("sample_time_basis") == "sensor_sample" and
                        isinstance(stamp, int) and (stamp > current_uptime + 1000 or
                        current_uptime - stamp > 5000)):
                    invalid_stamps.append(field)
        return {"status": "WARNING" if regressions or invalid_stamps else "PASS",
                "message": f"{len(uptimes)} accepted packet uptimes; {regressions} backwards steps; "
                f"{len(invalid_stamps)} inconsistent or old sensor stamps"
                + (f" ({', '.join(invalid_stamps)})" if invalid_stamps else "") + ". "
                "Delivery reordering, reboot, or counter wrap may explain a step; GPS UTC synchronization is not verified."}
    raise ValueError("unknown read-only diagnostic")


def record_diagnostic_result(result: dict) -> None:
    entry = {"recorded_utc": datetime.now(timezone.utc).isoformat(),
             "test": result.get("test"), "status": result.get("status"),
             "message": result.get("message"),
             "hardware_test_started": result.get("hardware_test_started", False)}
    for key in ("file", "sha256", "summary"):
        if key in result:
            entry[key] = result[key]
    with DIAGNOSTIC_LOCK, DIAGNOSTIC_LOG.open("a", encoding="utf-8", newline="\n") as handle:
        handle.write(json.dumps(entry, allow_nan=False, separators=(",", ":")) + "\n")
        handle.flush()


def recent_diagnostics(limit: int = 50) -> list[dict]:
    if not DIAGNOSTIC_LOG.is_file():
        return []
    with DIAGNOSTIC_LOCK, DIAGNOSTIC_LOG.open(encoding="utf-8") as handle:
        lines = deque(handle, maxlen=limit)
    results = []
    for line in reversed(lines):
        try:
            results.append(json.loads(line))
        except json.JSONDecodeError:
            continue
    return results


def available_ports() -> list[dict]:
    try:
        from serial.tools import list_ports  # type: ignore
    except ImportError:
        return []
    return [
        {"device": item.device, "description": item.description, "hwid": item.hwid}
        for item in list_ports.comports()
    ]


class Handler(SimpleHTTPRequestHandler):
    def translate_path(self, path: str) -> str:
        return str(ROOT / "index.html")

    def do_HEAD(self) -> None:
        self.send_error(405, "HEAD is not supported")

    def do_GET(self) -> None:
        security = getattr(getattr(self, "server", None), "companion_security", None)
        if security and security.handle_get(self):
            return
        parsed = urlparse(self.path)
        path = parsed.path
        if path in ("/api/telemetry", "/api/status"):
            json_response(self, MANAGER.snapshot())
            return
        if path == "/api/history":
            json_response(self, {"points": MANAGER.recent_history()})
            return
        if path == "/api/preflight/history":
            json_response(self, {"reports": preflight_runs()})
            return
        if path == "/api/diagnostics/history":
            json_response(self, {"results": recent_diagnostics()})
            return
        if path == "/api/diagnostics/export":
            bytes_response(self, DIAGNOSTIC_LOG.read_bytes() if DIAGNOSTIC_LOG.is_file() else b"",
                           "application/x-ndjson", "green-ion-diagnostics.jsonl")
            return
        if path == "/api/preflight/export":
            raw_id = parse_qs(parsed.query).get("id", [""])[0]
            try:
                run_id = uuid.UUID(hex=raw_id).hex
                if run_id != raw_id:
                    raise ValueError("noncanonical preflight ID")
                path_on_disk = PREFLIGHT_DIR / f"preflight-{run_id}.json"
                bytes_response(self, path_on_disk.read_bytes(), "application/json",
                               f"preflight-{run_id}.json")
            except (ValueError, OSError):
                json_response(self, {"error": "preflight report not found"}, 404)
            return
        if path == "/api/ports":
            with CONFIG_LOCK:
                config = dict(CONFIG)
            json_response(self, {"ports": available_ports(), "config": config})
            return
        if path == "/api/sessions":
            json_response(self, {"sessions": SESSIONS.list(),
                                 "active_session_name": MANAGER.writer.raw_path.name if MANAGER.writer else None})
            return
        if path == "/api/sessions/trash":
            json_response(self, {"sessions": SESSIONS.list_trash()})
            return
        if path == "/api/library/backup":
            with tempfile.NamedTemporaryFile(dir=DATA_DIR, prefix="backup-", suffix=".part",
                                             delete=False) as output:
                temporary = Path(output.name)
            try:
                create_backup(DATA_DIR, temporary)
            except (OSError, BackupError, ValueError, KeyError, TypeError) as error:
                temporary.unlink(missing_ok=True)
                json_response(self, {"error": f"Library backup unavailable: {error}"}, 409)
                return
            try:
                self.send_response(200)
                self.send_header("Content-Type", "application/zip")
                self.send_header("Cache-Control", "no-store")
                self.send_header("Content-Disposition", 'attachment; filename="green-ion-v12-library.zip"')
                self.send_header("Content-Length", str(temporary.stat().st_size))
                self.end_headers()
                with temporary.open("rb") as handle:
                    while chunk := handle.read(65536):
                        self.wfile.write(chunk)
            finally:
                temporary.unlink(missing_ok=True)
            return
        if path == "/api/flights":
            json_response(self, {"flights": FLIGHTS.list()})
            return
        if path == "/api/flights/trash":
            json_response(self, {"flights": FLIGHTS.list_trash()})
            return
        if path == "/api/flights/compare":
            try:
                ids = parse_qs(parsed.query).get("ids", [""])[0].split(",")
                json_response(self, FLIGHTS.compare(ids))
            except (OSError, FlightImportError, json.JSONDecodeError) as error:
                json_response(self, {"error": str(error)}, 400)
            return
        if path == "/api/feather/files":
            try:
                with CONFIG_LOCK:
                    base = CONFIG["feather_url"]
                with urlopen(Request(base + "/api/files", headers={"Accept": "application/json"}), timeout=2.0) as response:
                    payload = json.loads(response.read(131072))
                json_response(self, payload)
            except (OSError, URLError, TimeoutError, json.JSONDecodeError) as error:
                json_response(self, {"success": False, "error": f"Feather file list unavailable: {error}"}, 502)
            return
        if path.startswith("/api/flight/"):
            parts = path.strip("/").split("/")
            if len(parts) != 4:
                json_response(self, {"error": "invalid flight request"}, 400)
                return
            _, _, identifier, export = parts
            try:
                if export == "timeline":
                    json_response(self, FLIGHTS.analysis_timeline(identifier))
                    return
                metadata, raw = FLIGHTS.get(identifier)
                if export == "raw":
                    file_response(self, raw, "application/octet-stream", metadata["original_name"])
                elif export == "sensor-raw":
                    sensor, path = FLIGHTS.get_sensor(identifier)
                    file_response(self, path, "application/octet-stream", sensor["original_name"])
                elif export == "sensor-csv":
                    sensor, _path = FLIGHTS.get_sensor(identifier)
                    bytes_response(self, FLIGHTS.sensor_csv_export(identifier), "text/csv",
                                   Path(sensor["original_name"]).stem +
                                   ("-valid-prefix.csv" if sensor["inspection"]["status"] != "valid" else ".csv"))
                elif export == "csv":
                    stream_response(self, FLIGHTS.iter_csv_export(identifier, _verified=(metadata, raw)), "text/csv",
                                    Path(metadata["original_name"]).stem + ".csv")
                elif export == "json":
                    stream_response(self, FLIGHTS.iter_json_export(identifier, _verified=(metadata, raw)), "application/json",
                                    Path(metadata["original_name"]).stem + ".json")
                elif export == "report":
                    bytes_response(self, FLIGHTS.report_html(identifier), "text/html")
                else:
                    json_response(self, {"error": "unknown export"}, 404)
            except (OSError, FlightImportError, SensorLogError, json.JSONDecodeError) as error:
                json_response(self, {"error": str(error)}, 404)
            return
        if path.startswith("/api/session/"):
            name = path.rsplit("/", 1)[-1]
            candidate = (DATA_DIR / name).resolve()
            if candidate.parent != DATA_DIR.resolve() or not candidate.is_file() or not name.startswith("session-") or not name.endswith(".jsonl"):
                json_response(self, {"error": "invalid session file"}, 400)
                return
            if name.startswith("session-import-"):
                try:
                    candidate = SESSIONS.imported_file(name)
                except SessionImportError as error:
                    json_response(self, {"error": str(error)}, 409)
                    return
            self.send_response(200)
            self.send_header("Content-Type", "application/x-ndjson")
            self.send_header("Content-Disposition", f'attachment; filename="{name}"')
            self.send_header("Content-Length", str(candidate.stat().st_size))
            self.end_headers()
            with candidate.open("rb") as handle:
                while chunk := handle.read(65536):
                    self.wfile.write(chunk)
            return
        if path in ("/", "/index.html"):
            super().do_GET()
        else:
            json_response(self, {"error": "not found"}, 404)

    def do_POST(self) -> None:
        security = getattr(getattr(self, "server", None), "companion_security", None)
        if security and security.handle_post(self):
            return
        if self.path in ("/api/bench/start", "/api/bench/stop", "/api/bench/verify"):
            try:
                if self.headers.get("Content-Type", "").split(";", 1)[0].strip().lower() != "application/json":
                    raise ValueError("A JSON bench request is required.")
                length = int(self.headers.get("Content-Length", "0"))
                if length <= 0 or length > 512:
                    raise ValueError("Invalid bench request length.")
                payload = json.loads(self.rfile.read(length))
                if not isinstance(payload, dict):
                    raise ValueError("Invalid bench request payload.")
                status, result = (start_bench_recording(payload.get("confirm"))
                                  if self.path == "/api/bench/start" else
                                  stop_bench_recording(payload.get("confirm"), payload.get("file"))
                                  if self.path == "/api/bench/stop" else
                                  verify_bench_recording(payload.get("file")))
                if self.path == "/api/bench/verify" and result.get("file"):
                    try:
                        record_diagnostic_result(result)
                    except OSError as error:
                        result["history_error"] = f"Verification result could not be saved: {error}"
                json_response(self, result, status)
            except (ValueError, json.JSONDecodeError) as error:
                json_response(self, {"success": False, "error": str(error)}, 400)
            return
        if self.path == "/api/diagnostics/test":
            try:
                if self.headers.get("Content-Type", "").split(";", 1)[0].strip().lower() != "application/json":
                    raise ValueError("A JSON test request is required.")
                length = int(self.headers.get("Content-Length", "0"))
                if length <= 0 or length > 256:
                    raise ValueError("Invalid diagnostic request length.")
                payload = json.loads(self.rfile.read(length))
                if not isinstance(payload, dict) or not isinstance(payload.get("test"), str):
                    raise ValueError("Invalid diagnostic test request.")
                status, result = diagnostic_test(payload["test"])
                try:
                    record_diagnostic_result(result)
                except OSError as error:
                    result["history_error"] = f"Diagnostic result could not be saved: {error}"
                json_response(self, result, status)
            except (ValueError, json.JSONDecodeError) as error:
                json_response(self, {"status": "NOT TESTED", "error": str(error)}, 400)
            return
        if self.path == "/api/recorder/prepare":
            status, result = prepare_recorder()
            json_response(self, result, status)
            return
        if self.path == "/api/recorder/stop":
            try:
                if self.headers.get("Content-Type", "").split(";", 1)[0].strip().lower() != "application/json":
                    raise ValueError("A JSON confirmation is required.")
                length = int(self.headers.get("Content-Length", "0"))
                if length <= 0 or length > 512:
                    raise ValueError("Invalid confirmation length.")
                payload = json.loads(self.rfile.read(length))
                if not isinstance(payload, dict):
                    raise ValueError("Invalid confirmation payload.")
                status, result = stop_recorder(payload.get("confirm"), payload.get("file"))
                json_response(self, result, status)
            except (ValueError, json.JSONDecodeError) as error:
                json_response(self, {"success": False, "error": str(error)}, 400)
            return
        if self.path.startswith("/api/flight/") and self.path.endswith("/details"):
            try:
                parts = self.path.strip("/").split("/")
                if len(parts) != 4:
                    raise FlightImportError("invalid flight details request")
                length = int(self.headers.get("Content-Length", "0"))
                if length <= 0 or length > 512:
                    raise FlightImportError("invalid flight details length")
                payload = json.loads(self.rfile.read(length))
                if not isinstance(payload, dict):
                    raise FlightImportError("invalid flight details payload")
                result = FLIGHTS.update_details(parts[2], payload.get("label", ""),
                                                payload.get("rocket_profile", ""),
                                                payload.get("flight_date"))
                json_response(self, {"success": True, "flight": result})
            except (OSError, ValueError, FlightImportError, json.JSONDecodeError) as error:
                json_response(self, {"success": False, "error": str(error)}, 400)
            return
        if self.path.startswith("/api/flight/") and (self.path.endswith("/trash") or self.path.endswith("/restore")):
            try:
                parts = self.path.strip("/").split("/")
                if len(parts) != 4:
                    raise FlightImportError("invalid flight Trash request")
                if self.headers.get("Content-Type", "").split(";", 1)[0].strip().lower() != "application/json":
                    raise FlightImportError("a JSON confirmation is required")
                length = int(self.headers.get("Content-Length", "0"))
                if length <= 0 or length > 256:
                    raise FlightImportError("invalid flight Trash request length")
                payload = json.loads(self.rfile.read(length))
                if not isinstance(payload, dict):
                    raise FlightImportError("invalid flight Trash payload")
                if parts[3] == "trash":
                    if payload.get("confirm") != "TRASH LAPTOP COPY":
                        raise FlightImportError("type TRASH LAPTOP COPY to move only the laptop copy")
                    if MANAGER.snapshot().get("recording") is True:
                        json_response(self, {"success": False, "error": "Cannot move a flight while the recorder reports active."}, 409)
                        return
                    result = FLIGHTS.trash_flight(parts[2])
                else:
                    result = FLIGHTS.restore_trash(parts[2])
                json_response(self, {"success": True, "flight": result})
            except (OSError, ValueError, FlightImportError, json.JSONDecodeError) as error:
                json_response(self, {"success": False, "error": str(error)}, 400)
            return
        if self.path.startswith("/api/session/") and (self.path.endswith("/trash") or self.path.endswith("/restore")):
            try:
                parts = self.path.strip("/").split("/")
                if len(parts) != 4:
                    raise SessionImportError("invalid session Trash request")
                if self.headers.get("Content-Type", "").split(";", 1)[0].strip().lower() != "application/json":
                    raise SessionImportError("a JSON confirmation is required")
                length = int(self.headers.get("Content-Length", "0"))
                if length <= 0 or length > 256:
                    raise SessionImportError("invalid session Trash request length")
                payload = json.loads(self.rfile.read(length))
                if not isinstance(payload, dict):
                    raise SessionImportError("invalid session Trash payload")
                if parts[3] == "trash":
                    if payload.get("confirm") != "TRASH LAPTOP SESSION":
                        raise SessionImportError("type TRASH LAPTOP SESSION to move only the laptop session")
                    result = SESSIONS.trash_session(parts[2], MANAGER.writer.raw_path.name if MANAGER.writer else None)
                else:
                    result = SESSIONS.restore_trash(parts[2])
                json_response(self, {"success": True, "session": result})
            except (OSError, ValueError, SessionImportError, json.JSONDecodeError) as error:
                json_response(self, {"success": False, "error": str(error)}, 400)
            return
        if self.path == "/api/import-bin":
            try:
                length = int(self.headers.get("Content-Length", "0"))
                if length <= 0 or length > 256 * 1024 * 1024:
                    raise FlightImportError("BIN upload must be between 1 byte and 256 MB")
                filename = self.headers.get("X-Green-Ion-Filename", "")
                result = FLIGHTS.import_stream(filename, LimitedReader(self.rfile, length),
                                               expected_bytes=length, source_kind="manual_upload")
                json_response(self, {"success": True, "flight": result}, 200)
            except (OSError, ValueError, FlightImportError) as error:
                json_response(self, {"success": False, "error": str(error)}, 400)
            return
        if self.path == "/api/import-session":
            try:
                length = int(self.headers.get("Content-Length", "0"))
                if length <= 0 or length > MAX_SESSION_BYTES:
                    raise SessionImportError("JSONL upload must be between 1 byte and 32 MiB")
                result = SESSIONS.import_stream(self.headers.get("X-Green-Ion-Filename", ""),
                                                LimitedReader(self.rfile, length), expected_bytes=length)
                json_response(self, {"success": True, "session": result})
            except (OSError, ValueError, SessionImportError) as error:
                json_response(self, {"success": False, "error": str(error)}, 400)
            return
        if self.path == "/api/library/restore":
            try:
                length = int(self.headers.get("Content-Length", "0"))
                if length <= 0 or length > MAX_ARCHIVE_BYTES:
                    raise BackupError("backup upload must be between 1 byte and 512 MiB")
                result = restore_backup(DATA_DIR, LimitedReader(self.rfile, length), expected_bytes=length)
                json_response(self, {"success": True, "restore": result})
            except (OSError, ValueError, BackupError, zipfile.BadZipFile) as error:
                json_response(self, {"success": False, "error": str(error)}, 400)
            return
        if self.path.startswith("/api/import-sen/"):
            try:
                identifier = self.path.rsplit("/", 1)[-1]
                length = int(self.headers.get("Content-Length", "0"))
                if length <= 0 or length > MAX_SENSOR_BYTES:
                    raise FlightImportError("SEN upload must be between 1 byte and 16 MB")
                filename = self.headers.get("X-Green-Ion-Filename", "")
                result = FLIGHTS.import_sensor_stream(identifier, filename,
                                                     LimitedReader(self.rfile, length), expected_bytes=length)
                json_response(self, {"success": True, "sensor": result})
            except (OSError, ValueError, FlightImportError) as error:
                json_response(self, {"success": False, "error": str(error)}, 400)
            return
        if self.path == "/api/feather/download":
            try:
                length = min(int(self.headers.get("Content-Length", "0")), 4096)
                payload = json.loads(self.rfile.read(length))
                filename = str(payload.get("name", ""))
                with CONFIG_LOCK:
                    base = CONFIG["feather_url"]
                url = base + "/api/download?name=" + quote(filename)
                with urlopen(Request(url, headers={"Accept": "application/octet-stream"}), timeout=20.0) as response:
                    expected_header = response.headers.get("X-Green-Ion-Bytes") or response.headers.get("Content-Length")
                    expected = int(expected_header) if expected_header else None
                    result = FLIGHTS.import_stream(filename, response, expected_bytes=expected,
                                                   source_kind="feather_sd")
                json_response(self, {"success": True, "flight": result})
            except (OSError, URLError, TimeoutError, ValueError, json.JSONDecodeError, FlightImportError) as error:
                json_response(self, {"success": False, "error": f"Feather download failed: {error}"}, 502)
            return
        if self.path == "/api/feather/sensor":
            try:
                length = min(int(self.headers.get("Content-Length", "0")), 4096)
                payload = json.loads(self.rfile.read(length))
                metadata, _raw = FLIGHTS.get(str(payload.get("id", "")))
                filename = Path(metadata["original_name"]).stem + ".SEN"
                with CONFIG_LOCK:
                    base = CONFIG["feather_url"]
                url = base + "/api/sensor?name=" + quote(filename)
                with urlopen(Request(url, headers={"Accept": "application/octet-stream"}), timeout=20.0) as response:
                    expected_header = response.headers.get("X-Green-Ion-Bytes") or response.headers.get("Content-Length")
                    expected = int(expected_header) if expected_header else None
                    result = FLIGHTS.import_sensor_stream(metadata["id"], filename, response,
                                                          expected_bytes=expected)
                json_response(self, {"success": True, "sensor": result})
            except (OSError, URLError, TimeoutError, ValueError, json.JSONDecodeError, FlightImportError) as error:
                json_response(self, {"success": False, "error": f"Feather sensor download failed: {error}"}, 502)
            return
        if self.path == "/api/preflight":
            started_utc = datetime.now(timezone.utc).isoformat()
            try:
                if MANAGER.snapshot().get("recording"):
                    json_response(self, {"success": False, "error": "Preflight tests are disabled while the recorder is active."}, 409)
                    return
                sd_status, sd_result = feather_post("/api/sdtest")
                try:
                    capacity_result = feather_capacity()
                except (OSError, URLError, TimeoutError, ValueError, json.JSONDecodeError) as error:
                    capacity_result = {"success": False, "error": str(error)}
                test_status, test_result = feather_post("/api/hardware-test?sensor=all")
                actions = {
                    "sd_test": {"http_status": sd_status, **sd_result},
                    "sd_capacity": capacity_result,
                    "sensor_test": {"http_status": test_status, **test_result},
                }
                report = preflight_report(actions)
                report.update(actions=actions, created_utc=started_utc,
                              run_status="RUNNING" if test_status == 202 else "COMPLETE",
                              progress="SD test finished; waiting for stationary IMU/GPS results."
                              if test_status == 202 else "Sensor test request failed; inspect each check.")
                report["message"] = ("Sensor tests started. Keep the unit stationary; GPS fix may require an outdoor view."
                                     if test_status == 202 else
                                     "Sensor tests were not accepted; inspect the individual results and Feather response.")
                report = save_preflight_run(report)
                json_response(self, {"success": True, "report": report}, 202)
            except (OSError, URLError, TimeoutError, ValueError, json.JSONDecodeError) as error:
                report = preflight_report()
                report.update(created_utc=started_utc, run_status="COMPLETE",
                              progress="Feather contact failed; no hardware test completed.",
                              message=f"Preflight could not contact the Feather: {error}")
                report = save_preflight_run(report)
                json_response(self, {"success": False, "error": report["message"], "report": report}, 502)
            return
        if self.path == "/api/preflight/report":
            json_response(self, {"success": True, "report": refresh_preflight_run()})
            return
        if self.path != "/api/config":
            json_response(self, {"error": "not found"}, 404)
            return
        try:
            length = min(int(self.headers.get("Content-Length", "0")), 4096)
            payload = json.loads(self.rfile.read(length))
            serial_port = str(payload.get("serial_port", "auto")).strip() or "auto"
            feather_url = str(payload.get("feather_url", "http://192.168.4.1")).strip().rstrip("/")
            if not feather_url.startswith(("http://", "https://")):
                raise ValueError("Feather URL must begin with http:// or https://")
            with CONFIG_LOCK:
                CONFIG.update(serial_port=serial_port, feather_url=feather_url)
            json_response(self, {"success": True, "message": "Connections will use the new settings on their next retry."})
        except (ValueError, json.JSONDecodeError) as error:
            json_response(self, {"success": False, "error": str(error)}, 400)

    def log_message(self, format: str, *args: object) -> None:
        return


def choose_port(requested: str) -> str | None:
    if requested.lower() != "auto":
        return requested
    ports = available_ports()
    preferred = [item for item in ports if any(word in (item["description"] + item["hwid"]).lower()
                                               for word in ("arduino", "ch340", "usb serial", "uno"))]
    candidates = preferred or ports
    return candidates[0]["device"] if len(candidates) == 1 else None


def serial_source() -> None:
    try:
        import serial  # type: ignore
    except ImportError:
        MANAGER.note_connection("radio", "pyserial is not installed; run INSTALL_GROUND_STATION.cmd once")
        return
    while not STOP.is_set():
        with CONFIG_LOCK:
            requested = CONFIG["serial_port"]
        port = choose_port(requested)
        if not port:
            MANAGER.note_connection("radio", "No unique Uno serial port detected; choose a COM port in Settings")
            STOP.wait(2.0)
            continue
        try:
            MANAGER.note_connection("radio", f"Opening {port}")
            with serial.Serial(port, 115200, timeout=1) as connection:
                MANAGER.note_connection("radio", None)
                while not STOP.is_set():
                    with CONFIG_LOCK:
                        if CONFIG["serial_port"] != requested:
                            break
                    line = connection.readline().decode("utf-8", errors="replace").strip()
                    if not line:
                        continue
                    try:
                        row = json.loads(line)
                    except json.JSONDecodeError:
                        MANAGER.note_parse_error("radio", "Uno sent a non-JSON line")
                        continue
                    kind = row.get("type")
                    if kind == "telemetry":
                        try:
                            MANAGER.ingest("radio", row)
                        except ValueError as error:
                            MANAGER.note_parse_error("radio", str(error))
                    elif kind == "receiver_status":
                        try:
                            MANAGER.note_receiver_status(row)
                            MANAGER.note_connection("radio", None if row["ok"] else row.get("error", "receiver error"))
                        except ValueError as error:
                            MANAGER.note_parse_error("radio", str(error))
                    elif kind == "receiver_error":
                        MANAGER.note_parse_error("radio", f"receiver rejected packet code {row.get('code')}")
        except (OSError, serial.SerialException) as error:
            MANAGER.note_connection("radio", f"{port}: {error}")
            STOP.wait(2.0)
        finally:
            MANAGER.clear_receiver_status()


def normalize_wifi(row: dict, sequence: int) -> dict:
    return {
        **row,
        "sequence": sequence,
        "uptime_ms": int(row["uptime_ms"]) if "uptime_ms" in row else
                     int(row.get("uptime_s", 0)) * 1000,
        "speed_mps": row.get("gps_speed_mps"),
        "gps_speed_mps": row.get("gps_speed_mps"),
        "gps_status": row.get("gps_status", "missing"),
    }


def wifi_source() -> None:
    sequence = 0
    while not STOP.is_set():
        with CONFIG_LOCK:
            base = CONFIG["feather_url"]
        try:
            request = Request(base + "/api/status", headers={"Accept": "application/json"})
            with urlopen(request, timeout=1.2) as response:
                row = json.loads(response.read(65536))
            MANAGER.ingest("wifi", normalize_wifi(row, sequence))
            sequence = (sequence + 1) & 0xFFFFFFFF
            STOP.wait(0.25)
        except (OSError, URLError, TimeoutError, json.JSONDecodeError, ValueError) as error:
            MANAGER.note_connection("wifi", str(error))
            STOP.wait(1.5)


def sample_source() -> None:
    sequence = 0
    started = time.monotonic()
    while not STOP.wait(0.2):
        elapsed = time.monotonic() - started
        altitude = max(0.0, 310.0 * math.sin(min(elapsed / 35.0, 1.0) * math.pi))
        gps_dropout = 18.0 < elapsed % 45.0 < 26.0
        MANAGER.ingest("simulation", {
            "sequence": sequence, "uptime_ms": round(elapsed * 1000),
            "latitude": 37.4219999 + elapsed * 0.000004,
            "longitude": -122.0840575 + elapsed * 0.000003,
            "altitude_m": round(altitude, 2), "pressure_pa": round(101325 * math.exp(-altitude / 8434.5)),
            "gps_altitude_m": round(altitude + 1.8, 2), "acceleration_g": 1.0,
            "vertical_speed_mps": round(max(0.0, 55.0 * math.sin(min(elapsed / 35.0, 1.0) * math.pi)), 2),
            "battery_v": round(4.12 - elapsed * 0.0008, 3),
            "gps_fix_age_ms": 5000 if gps_dropout else 0,
            "gps_status": "stale" if gps_dropout else "live", "satellites": 0 if gps_dropout else 10,
            "flight_state": 3 if altitude > 5 else 1, "health": 0,
        })
        sequence += 1


def main() -> None:
    parser = argparse.ArgumentParser(description="GREEN ION V12 offline Ground Station")
    parser.add_argument("--sample", action="store_true", help="explicit simulated telemetry; never hardware evidence")
    parser.add_argument("--port", default="auto", help="Uno COM port or auto")
    parser.add_argument("--feather-url", default="http://192.168.4.1")
    parser.add_argument("--no-serial", action="store_true")
    parser.add_argument("--no-wifi", action="store_true")
    parser.add_argument("--listen", default="127.0.0.1")
    parser.add_argument("--http-port", type=int, default=8765)
    parser.add_argument("--https-cert", type=Path, help="trusted local HTTPS certificate PEM for iPhone Companion")
    parser.add_argument("--https-key", type=Path, help="private key PEM; keep it outside the repository")
    parser.add_argument("--no-browser", action="store_true")
    args = parser.parse_args()
    try:
        address = ipaddress.ip_address(args.listen)
    except ValueError:
        parser.error("--listen must be an explicit IPv4 loopback or private LAN address")
    private_lan = any(address in ipaddress.ip_network(block) for block in
                      ("10.0.0.0/8", "172.16.0.0/12", "192.168.0.0/16")) if address.version == 4 else False
    if address.version != 4 or not (address.is_loopback or private_lan):
        parser.error("--listen must be an IPv4 loopback or specific private LAN address")
    companion = None
    if not address.is_loopback:
        if not args.https_cert or not args.https_key:
            parser.error("a private LAN bind requires --https-cert and --https-key")
    if bool(args.https_cert) != bool(args.https_key):
        parser.error("--https-cert and --https-key must be supplied together")
    if args.https_cert:
        password = os.environ.get("GREEN_ION_COMPANION_PASSWORD", "")
        try:
            companion = CompanionSecurity(password)
        except ValueError as error:
            parser.error(str(error))
    tls = None
    if args.https_cert:
        tls = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        tls.minimum_version = ssl.TLSVersion.TLSv1_2
        try:
            tls.load_cert_chain(str(args.https_cert), str(args.https_key))
        except (OSError, ssl.SSLError) as error:
            parser.error(f"cannot load HTTPS certificate/key: {error}")
    with CONFIG_LOCK:
        CONFIG.update(serial_port=args.port, feather_url=args.feather_url.rstrip("/"))
    workers = []
    if args.sample:
        workers.append(threading.Thread(target=sample_source, daemon=True, name="simulation"))
    else:
        if not args.no_serial:
            workers.append(threading.Thread(target=serial_source, daemon=True, name="radio"))
        if not args.no_wifi:
            workers.append(threading.Thread(target=wifi_source, daemon=True, name="wifi"))
    for worker in workers:
        worker.start()
    url = f"{'https' if tls else 'http'}://{args.listen}:{args.http_port}"
    try:
        server = ThreadingHTTPServer((args.listen, args.http_port), Handler)
        server.companion_security = companion
        if tls:
            server.socket = tls.wrap_socket(server.socket, server_side=True)
    except OSError as error:
        STOP.set()
        if getattr(error, "winerror", None) == 10048 or error.errno in (48, 98):
            print(f"Ground Station is already running at {url}")
            if not args.no_browser:
                webbrowser.open(url)
            return
        raise
    print(f"GREEN ION Ground Station: {url}")
    print("Mode: SIMULATION — NOT HARDWARE" if args.sample else "Mode: HARDWARE CONNECTIONS (Wi-Fi + Uno USB)")
    print(f"Session: {MANAGER.writer.raw_path if MANAGER.writer else 'memory only'}")
    if not args.no_browser:
        threading.Timer(0.3, webbrowser.open, args=(url,)).start()
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        STOP.set()
        server.server_close()


if __name__ == "__main__":
    main()
