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
from urllib.error import URLError
from urllib.request import Request, urlopen
from urllib.parse import parse_qs, quote, urlparse
import webbrowser

from flight_library import FlightImportError, FlightLibrary
from telemetry_manager import SessionWriter, TelemetryManager

ROOT = Path(__file__).resolve().parent
DATA_DIR = ROOT / "data"
MANAGER = TelemetryManager(SessionWriter(DATA_DIR))
STOP = threading.Event()
CONFIG = {"serial_port": "auto", "feather_url": "http://192.168.4.1"}
CONFIG_LOCK = threading.Lock()
FLIGHTS = FlightLibrary(DATA_DIR / "flights")


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


def feather_post(path: str, timeout: float = 4.0) -> tuple[int, dict]:
    with CONFIG_LOCK:
        base = CONFIG["feather_url"]
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


def preflight_report() -> dict:
    snapshot = MANAGER.snapshot()
    wifi = snapshot["sources"]["wifi"]
    radio = snapshot["sources"]["radio"]
    checks = []
    def add(name: str, status: str, message: str) -> None:
        checks.append({"name": name, "status": status, "message": message})
    add("Feather Wi-Fi", "PASS" if wifi["status"] == "live" else "FAIL",
        "Live Feather status is available." if wifi["status"] == "live" else (wifi["error"] or "No live Feather status."))
    add("Uno USB and LoRa", "PASS" if radio["status"] == "live" else "NOT TESTED",
        "A validated LoRa telemetry packet reached the laptop." if radio["status"] == "live" else (radio["error"] or "No validated radio packet received."))
    add("SD recorder", "PASS" if snapshot.get("sd_ok") is True else "FAIL" if snapshot.get("sd_ok") is False else "NOT TESTED",
        "Feather reports the SD recorder ready." if snapshot.get("sd_ok") is True else "No ready SD result is available.")
    add("BMP388", "PASS" if snapshot.get("barometer_ready") is True else "FAIL" if snapshot.get("barometer_ready") is False else "NOT TESTED",
        "Barometer runtime status is ready." if snapshot.get("barometer_ready") is True else "No ready barometer status is available.")
    add("LSM6DSO32", snapshot.get("imu_test_result", "NOT TESTED"), snapshot.get("imu_test_message", "Run the IMU hardware test."))
    gps_result = snapshot.get("gps_test_result", "NOT TESTED")
    add("PA1010D communication", gps_result, snapshot.get("gps_test_message", "Run the GPS hardware test."))
    add("GPS fix", "PASS" if snapshot.get("gps_status") == "live" else "WARNING" if snapshot.get("gps_status") == "stale" else "NOT TESTED",
        "Current valid GPS position." if snapshot.get("gps_status") == "live" else "A current outdoor GPS fix is not available; communication can still pass independently.")
    battery = snapshot.get("battery_v")
    battery_live = snapshot["link_status"] == "live"
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
        if not battery_valid else f"Reported battery: {battery_text}.")
    persistence_ok = MANAGER.writer is not None and MANAGER.writer.directory.is_dir()
    add("Ground persistence", "PASS" if persistence_ok else "FAIL",
        "Append-only session directory is available." if persistence_ok else "Session directory is unavailable.")
    overall = "FAIL" if any(item["status"] == "FAIL" for item in checks) else "WARNING" if any(item["status"] in ("WARNING", "NOT TESTED", "INCONCLUSIVE", "RUNNING") for item in checks) else "PASS"
    return {"overall": overall, "checks": checks, "generated_utc": datetime.now(timezone.utc).isoformat()}


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
            try:
                if MANAGER.snapshot().get("recording"):
                    json_response(self, {"success": False, "error": "Preflight tests are disabled while the recorder is active."}, 409)
                    return
                sd_status, sd_result = feather_post("/api/sdtest")
                test_status, test_result = feather_post("/api/hardware-test?sensor=all")
                report = preflight_report()
                report["actions"] = {
                    "sd_test": {"http_status": sd_status, **sd_result},
                    "sensor_test": {"http_status": test_status, **test_result},
                }
                report["message"] = "Sensor tests started. Keep the unit stationary; GPS fix may require an outdoor view. Refresh the report after the test interval."
                json_response(self, {"success": True, "report": report}, 202)
            except (OSError, URLError, TimeoutError, ValueError, json.JSONDecodeError) as error:
                json_response(self, {"success": False, "error": f"Preflight could not contact the Feather: {error}", "report": preflight_report()}, 502)
            return
        if self.path == "/api/preflight/report":
            json_response(self, {"success": True, "report": preflight_report()})
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
                        MANAGER.note_connection("radio", None if row.get("ok") else row.get("error", "receiver error"))
                    elif kind == "receiver_error":
                        MANAGER.note_parse_error("radio", f"receiver rejected packet code {row.get('code')}")
        except (OSError, serial.SerialException) as error:
            MANAGER.note_connection("radio", f"{port}: {error}")
            STOP.wait(2.0)


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
