"""Offline GREEN ION V12 Windows Ground Station.

The default mode keeps the local dashboard running while it independently
reconnects to an Arduino Uno receiver and the Feather Wi-Fi status endpoint.
Simulation is available only through the explicit ``--sample`` option.
"""

from __future__ import annotations

import argparse
from datetime import datetime, timezone
from http.server import ThreadingHTTPServer, SimpleHTTPRequestHandler
import json
import math
from pathlib import Path
import threading
import time
import uuid
from urllib.error import URLError
from urllib.request import Request, urlopen
from urllib.parse import parse_qs, quote, urlparse
import webbrowser

from flight_library import FlightImportError, FlightLibrary
from telemetry_manager import SessionWriter, TelemetryManager

ROOT = Path(__file__).resolve().parent
GROUND_STATION_VERSION = "V12-development"
DATA_DIR = ROOT / "data"
MANAGER = TelemetryManager(SessionWriter(DATA_DIR))
STOP = threading.Event()
CONFIG = {"serial_port": "auto", "feather_url": "http://192.168.4.1"}
CONFIG_LOCK = threading.Lock()
FLIGHTS = FlightLibrary(DATA_DIR / "flights")
PREFLIGHT_DIR = DATA_DIR / "preflight"
PREFLIGHT_LOCK = threading.Lock()


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


def preflight_report(actions: dict | None = None) -> dict:
    snapshot = MANAGER.snapshot()
    wifi = snapshot["sources"]["wifi"]
    radio = snapshot["sources"]["radio"]
    checks = []
    def add(name: str, status: str, message: str) -> None:
        checks.append({"name": name, "status": status, "message": message})
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
    add("LSM6DSO32", imu_result if imu_result in ("PASS", "WARNING", "FAIL") else
        "WARNING" if imu_result else "NOT TESTED",
        snapshot.get("imu_test_message") if live("imu_test_message") else "Run the stationary IMU hardware test in Preflight.")
    gps_result = snapshot.get("gps_test_result") if live("gps_test_result") else None
    add("PA1010D communication", gps_result if gps_result in ("PASS", "WARNING", "FAIL") else
        "WARNING" if gps_result else "NOT TESTED",
        snapshot.get("gps_test_message") if live("gps_test_message") else "Run the GPS communication test; a satellite fix is separate.")
    sensor_action = (actions or {}).get("sensor_test")
    add("Stationary sensor test request", "WARNING" if sensor_action and sensor_action.get("http_status") == 202 else
        "FAIL" if sensor_action else "NOT TESTED",
        "Feather accepted the sensor tests; refresh after completion to see measured results."
        if sensor_action and sensor_action.get("http_status") == 202 else
        "Run Full Preflight with the rocket stationary." if not sensor_action else
        sensor_action.get("message", "Feather did not accept the sensor tests."))
    fix_live = snapshot.get("gps_status") == "live" and live("latitude") and live("longitude")
    add("GPS fix", "PASS" if fix_live else "WARNING" if snapshot.get("gps_status") == "stale" else "NOT TESTED",
        "Current valid GPS position." if fix_live else "A current outdoor GPS fix is unavailable; GPS communication can still pass independently.")
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
        if not battery_valid else f"Reported battery: {battery_text}; one reading does not verify charge endurance or voltage stability.")
    stability_status, stability_message = battery_stability(
        MANAGER.recent_history(200), snapshot.get("active_source")) if battery_live else (
            "NOT TESTED", "A live hardware battery stream is required for a stability window.")
    add("Stationary battery stability", stability_status, stability_message)
    persistence_ok = MANAGER.writer is not None and MANAGER.writer.directory.is_dir()
    add("Ground persistence", "PASS" if persistence_ok else "FAIL",
        "Append-only session directory is available." if persistence_ok else "Session directory is unavailable.")
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
                "lora": "Ground LoRa module"}
    if kind in observed:
        check = next(item for item in preflight_report()["checks"] if item["name"] == observed[kind])
        return 200, {"test": kind, **check, "hardware_test_started": False}
    paths = {"imu": "/api/hardware-test?sensor=imu",
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
            message = ("Sensor test started; keep the rocket stationary and inspect the updated result in Preflight."
                       if status == "WARNING" else result.get("error") or "Sensor test was not accepted.")
        return code, {"test": kind, "status": status, "message": message,
                      "hardware_test_started": code == 202 or kind == "sd" and code in (200, 500)}
    except (OSError, URLError, TimeoutError, ValueError, json.JSONDecodeError) as error:
        return 502, {"test": kind, "status": "NOT TESTED",
                     "message": f"Could not contact the Feather: {error}", "hardware_test_started": False}


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
        relative = path.split("?", 1)[0].lstrip("/") or "index.html"
        candidate = (ROOT / relative).resolve()
        if ROOT not in candidate.parents and candidate != ROOT:
            return str(ROOT / "index.html")
        return str(candidate)

    def do_GET(self) -> None:
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
            sessions = []
            for item in sorted(DATA_DIR.glob("session-*.jsonl"), reverse=True):
                sessions.append({"name": item.name, "bytes": item.stat().st_size,
                                 "modified": datetime.fromtimestamp(item.stat().st_mtime, timezone.utc).isoformat()})
            json_response(self, {"sessions": sessions})
            return
        if path == "/api/flights":
            json_response(self, {"flights": FLIGHTS.list()})
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
                metadata, raw = FLIGHTS.get(identifier)
                if export == "raw":
                    bytes_response(self, raw.read_bytes(), "application/octet-stream", metadata["original_name"])
                elif export == "csv":
                    bytes_response(self, FLIGHTS.csv_export(identifier), "text/csv", metadata["original_name"].replace(".BIN", ".csv"))
                elif export == "json":
                    bytes_response(self, FLIGHTS.json_export(identifier), "application/json", metadata["original_name"].replace(".BIN", ".json"))
                elif export == "report":
                    bytes_response(self, FLIGHTS.report_html(identifier), "text/html")
                else:
                    json_response(self, {"error": "unknown export"}, 404)
            except (OSError, FlightImportError, json.JSONDecodeError) as error:
                json_response(self, {"error": str(error)}, 404)
            return
        if path.startswith("/api/session/"):
            name = path.rsplit("/", 1)[-1]
            candidate = (DATA_DIR / name).resolve()
            if candidate.parent != DATA_DIR.resolve() or not candidate.is_file() or not name.startswith("session-") or not name.endswith(".jsonl"):
                json_response(self, {"error": "invalid session file"}, 400)
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
        super().do_GET()

    def do_POST(self) -> None:
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
        if self.path == "/api/import-bin":
            try:
                length = int(self.headers.get("Content-Length", "0"))
                if length <= 0 or length > 256 * 1024 * 1024:
                    raise FlightImportError("BIN upload must be between 1 byte and 256 MB")
                filename = self.headers.get("X-Green-Ion-Filename", "")
                result = FLIGHTS.import_stream(filename, LimitedReader(self.rfile, length), expected_bytes=length)
                json_response(self, {"success": True, "flight": result}, 200)
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
                    result = FLIGHTS.import_stream(filename, response, expected_bytes=expected)
                json_response(self, {"success": True, "flight": result})
            except (OSError, URLError, TimeoutError, ValueError, json.JSONDecodeError, FlightImportError) as error:
                json_response(self, {"success": False, "error": f"Feather download failed: {error}"}, 502)
            return
        if self.path == "/api/preflight":
            started_utc = datetime.now(timezone.utc).isoformat()
            try:
                if MANAGER.snapshot().get("recording"):
                    json_response(self, {"success": False, "error": "Preflight tests are disabled while the recorder is active."}, 409)
                    return
                sd_status, sd_result = feather_post("/api/sdtest")
                test_status, test_result = feather_post("/api/hardware-test?sensor=all")
                actions = {
                    "sd_test": {"http_status": sd_status, **sd_result},
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
        "uptime_ms": int(row.get("uptime_s", 0)) * 1000,
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
    parser.add_argument("--no-browser", action="store_true")
    args = parser.parse_args()
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
    url = f"http://{args.listen}:{args.http_port}"
    try:
        server = ThreadingHTTPServer((args.listen, args.http_port), Handler)
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
