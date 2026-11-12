"""Local, dependency-light GREEN ION V12 telemetry ground station.

Run sample data:
    py -3 ground_station.py --sample

Run an Uno receiver (requires pyserial):
    py -3 -m pip install pyserial
    py -3 ground_station.py --port COM5
"""

from __future__ import annotations

import argparse
import csv
from collections import deque
from dataclasses import dataclass, field
from datetime import datetime, timezone
from http.server import ThreadingHTTPServer, SimpleHTTPRequestHandler
import json
import math
import hashlib
import re
from urllib.parse import urlsplit, unquote, parse_qs
from pathlib import Path
import shutil
import os
import platform
import threading
import time
import webbrowser
import uuid
from v13_bridge import FeatherBridge
from v16_telemetry import build_unified_telemetry

ROOT = Path(__file__).resolve().parent
# Never put new logs inside a replaceable downloaded app folder.
LOG_DIR = Path(os.environ.get("GREEN_ION_DATA_DIR", str(Path.home() / "GREEN_ION_Data"))) / "logs"
BRIDGE = FeatherBridge()
# The current blue PWA is served from source checkout or release ZIP.
STATIC_ROOT = ROOT.parent / "web" if (ROOT.parent / "web/index.html").exists() else ROOT.parent / "Website"
DATA_ROOT = Path(os.environ.get("GREEN_ION_DATA_DIR", str(Path.home() / "GREEN_ION_Data")))
FLIGHT_DIR = DATA_ROOT / "flights"
SAFE_NAME = re.compile(r"[A-Za-z0-9_.-]{1,176}\.(?:BIN|SEN|TXT)", re.IGNORECASE)
SAFE_HASH = re.compile(r"[0-9a-f]{64}")
# This is a PUBLISHABLE browser key, not a service role credential.
PUBLIC_CLOUD = {
    "supabaseUrl": os.environ.get("SUPABASE_URL", "https://bqvvtbnjthbiqyljmzfx.supabase.co"),
    "supabaseKey": os.environ.get("SUPABASE_PUBLISHABLE_KEY", "sb_publishable_m8GYtLRB9LKT2bPzxVMESA_yhlgIAZR"),
}
try:
    APP_INFO = json.loads((ROOT / "version.json").read_text(encoding="utf-8"))
except (OSError, ValueError, TypeError):
    APP_INFO = {"schema": "GREEN-ION-V16-WINDOWS-1", "version": "unknown", "channel": "unknown"}
RUNTIME = {
    "started_utc": datetime.now(timezone.utc).isoformat(),
    "receiver_port": None,
    "receiver_requested_port": None,
    "receiver_auto_discovery": True,
    "receiver_last_connected_utc": None,
    "receiver_last_disconnect_utc": None,
    "receiver_last_error": "",
    "receiver_searching_since_utc": None,
    "mode": "starting",
}


# Fixed columns survive optional fields, receiver reconnects and a mixture of
# ordinary telemetry and bench-test packets. Original JSON is retained.
LOG_COLUMNS = (
    "received_utc", "source", "type", "protocol", "sequence", "uptime_ms",
    "rssi_dbm", "snr_db", "altitude_m", "pressure_pa", "gps_altitude_m",
    "acceleration_g", "acceleration_x_g", "acceleration_y_g",
    "acceleration_z_g", "rotation_x_dps", "rotation_y_dps",
    "rotation_z_dps", "speed_mps", "battery_v", "gps_fix_age_ms",
    "gps_status", "satellites", "flight_state", "flags", "health",
    "estimated_missing_packets", "valid_packets", "invalid_packets",
    "bench_received", "sender_restarts", "duplicates", "out_of_order",
    "latitude", "longitude", "last_position_age_ms", "raw_packet_json",
)


@dataclass
class Store:
    """Thread-safe link, GPS freshness and receiver-side packet evidence."""
    latest: dict | None = None
    received_monotonic: float = 0.0
    last_position: tuple[float, float] | None = None
    last_position_monotonic: float = 0.0
    lock: threading.Lock = field(default_factory=threading.Lock)
    receiver_state: str = "missing"
    receiver_error: str = ""
    receiver_info: dict = field(default_factory=dict)
    bench_packets: int = 0
    last_bench: dict | None = None
    received_packets: int = 0
    telemetry_packets: int = 0
    inferred_missing: int = 0
    repeated_packets: int = 0
    out_of_order: int = 0
    resets: int = 0
    sender_session_index: int = 0
    last_seq: int | None = None
    last_uptime: int | None = None
    avg_rssi: float | None = None
    avg_snr: float | None = None
    accepted_telemetry_packets: int = 0
    receiver_reconnects: int = 0
    receiver_connected_once: bool = False
    receiver_session_id: str = field(default_factory=lambda: uuid.uuid4().hex)
    radio_history: deque = field(default_factory=lambda: deque(maxlen=240))
    log_path: Path | None = None

    def receiver(self, status: str, detail: str = "", info: dict | None = None) -> None:
        with self.lock:
            old_ready = self.receiver_state in ("connected", "ready")
            new_ready = status in ("connected", "ready")
            if new_ready and self.receiver_connected_once and not old_ready:
                self.receiver_reconnects += 1
            if new_ready:
                self.receiver_connected_once = True
            self.receiver_state = status
            self.receiver_error = detail[:300]
            if info is not None:
                self.receiver_info = info

    def _observe_packet(self, row: dict) -> str:
        """Classify one CRC-valid frame; caller holds lock."""
        self.received_packets += 1
        classification = "new"
        seq, uptime = row.get("sequence"), row.get("uptime_ms")
        if isinstance(seq, int) and isinstance(uptime, int):
            restarted = (
                self.last_uptime is not None
                and self.last_seq is not None
                and uptime < self.last_uptime
                and self.last_uptime - uptime >= 1000
                and seq <= 5
                and seq < self.last_seq
            )
            if restarted:
                self.resets += 1
                self.sender_session_index += 1
                self.last_seq = None
                self.last_uptime = None
                classification = "sender_restart"
            advance = True
            if self.last_seq is not None:
                delta = (seq - self.last_seq) & 0xFFFFFFFF
                if delta == 0:
                    self.repeated_packets += 1
                    classification = "duplicate"
                    advance = False
                elif 1 < delta < 1000000:
                    self.inferred_missing += delta - 1
                    classification = "gap"
                elif delta >= 1000000:
                    self.out_of_order += 1
                    classification = "out_of_order"
                    advance = False
            if advance:
                self.last_seq, self.last_uptime = seq, uptime
        rssi = row.get("rssi_dbm")
        if isinstance(rssi, (int, float)) and math.isfinite(rssi):
            self.avg_rssi = float(rssi) if self.avg_rssi is None else (
                0.9 * self.avg_rssi + 0.1 * float(rssi)
            )
        snr = row.get("snr_db")
        if isinstance(snr, (int, float)) and math.isfinite(snr):
            self.avg_snr = float(snr) if self.avg_snr is None else (
                0.9 * self.avg_snr + 0.1 * float(snr)
            )
        return classification

    def _annotate(self, row: dict, classification: str) -> dict:
        item = dict(row)
        item["receiver_session_id"] = self.receiver_session_id
        item["sender_session_index"] = self.sender_session_index
        item["packet_classification"] = classification
        seq = row.get("sequence")
        item["packet_id"] = (
            f"{self.receiver_session_id}:{self.sender_session_index}:{seq}"
            if isinstance(seq, int) else None
        )
        return item

    def bench(self, row: dict) -> None:
        with self.lock:
            self.bench_packets += 1
            classification = self._observe_packet(row)
            self.last_bench = self._annotate(row, classification)

    def update(self, row: dict) -> None:
        now = time.monotonic()
        with self.lock:
            self.telemetry_packets += 1
            classification = self._observe_packet(row)
            item = self._annotate(row, classification)
            accepted = classification not in ("duplicate", "out_of_order")
            item["accepted_for_live_display"] = accepted
            self.radio_history.append({
                key: item.get(key) for key in (
                    "received_utc", "packet_id", "sender_session_index", "sequence",
                    "uptime_ms", "packet_classification", "accepted_for_live_display",
                    "altitude_m", "battery_v", "flight_state", "gps_status",
                    "latitude", "longitude", "rssi_dbm", "snr_db"
                )
            })
            if not accepted:
                return
            self.latest = item
            self.received_monotonic = now
            self.accepted_telemetry_packets += 1
            lat, lon = row.get("latitude"), row.get("longitude")
            # Received a stale last-known position != a fresh GPS acquisition.
            if (isinstance(lat, (int, float)) and isinstance(lon, (int, float))
                    and math.isfinite(lat) and math.isfinite(lon)
                    and -90 <= lat <= 90 and -180 <= lon <= 180
                    and row.get("gps_status") == "live"):
                self.last_position = (float(lat), float(lon))
                self.last_position_monotonic = now

    def snapshot(self) -> dict:
        now = time.monotonic()
        with self.lock:
            row = dict(self.latest or {})
            age = round((now - self.received_monotonic) * 1000) if self.latest else None
            row["packet_age_ms"] = age
            row["link_status"] = "missing" if age is None else (
                "stale" if age > 3000 else "live"
            )
            # Never label GPS live when the radio link itself is stale.
            if row.get("link_status") != "live" and row.get("gps_status") == "live":
                row["gps_status"] = "stale"
            row["last_position"] = self.last_position
            row["last_position_age_ms"] = (
                round((now - self.last_position_monotonic) * 1000)
                if self.last_position else None
            )
            row["receiver_state"] = self.receiver_state
            row["receiver_error"] = self.receiver_error
            row["receiver_info"] = dict(self.receiver_info)
            row["receiver_session_id"] = self.receiver_session_id
            row["receiver_reconnects"] = self.receiver_reconnects
            row["sender_session_index"] = self.sender_session_index
            row["bench_packets"] = self.bench_packets
            row["last_bench"] = self.last_bench
            row["packets_received"] = self.received_packets
            row["telemetry_packets"] = self.telemetry_packets
            row["accepted_telemetry_packets"] = self.accepted_telemetry_packets
            row["packets_missed_estimated"] = self.inferred_missing
            row["packets_repeated"] = self.repeated_packets
            row["packets_out_of_order"] = self.out_of_order
            row["sender_resets"] = self.resets
            total = self.received_packets + self.inferred_missing
            row["loss_pct_estimated"] = round(self.inferred_missing * 100 / total, 2) if total else 0
            row["rssi_average_dbm"] = round(self.avg_rssi, 1) if self.avg_rssi is not None else None
            row["snr_average_db"] = round(self.avg_snr, 1) if self.avg_snr is not None else None
            row["log_download_available"] = bool(self.log_path and self.log_path.is_file())
            return row

    def history_snapshot(self, limit: int = 120) -> list[dict]:
        with self.lock:
            if limit <= 0:
                return []
            return list(self.radio_history)[-min(limit, self.radio_history.maxlen):]

STORE = Store()


class Handler(SimpleHTTPRequestHandler):
    def _safe_request(self) -> bool:
        # Prevent DNS-rebinding attacks; only loopback browser pages use this server.
        host = (self.headers.get("Host") or "").split(":")[0].lower()
        origin = self.headers.get("Origin")
        port = self.server.server_address[1]
        allowed = (f"http://127.0.0.1:{port}", f"http://localhost:{port}")
        if host not in ("127.0.0.1", "localhost") or (origin and origin not in allowed):
            self.send_error(403, "Only same-origin local browser access is allowed")
            return False
        return True

    def _local_file_info(self):
        query = parse_qs(urlsplit(self.path).query)
        name = query.get("name", [""])[0]
        return name if SAFE_NAME.fullmatch(name) and "/" not in name and "\\" not in name else None

    def _local_save(self):
        if not self._safe_request():
            return
        name = self._local_file_info()
        checksum = (self.headers.get("X-Green-Ion-SHA256") or "").lower()
        try:
            length = int(self.headers.get("Content-Length", "-1"))
        except ValueError:
            length = -1
        if name is None or SAFE_HASH.fullmatch(checksum) is None or not (0 < length <= 52428800):
            self.send_error(400, "Invalid flight file, SHA-256 or byte length")
            return
        body = self.rfile.read(length)
        if len(body) != length or hashlib.sha256(body).hexdigest() != checksum:
            self.send_error(422, "Flight bytes or SHA-256 do not match")
            return
        FLIGHT_DIR.mkdir(parents=True, exist_ok=True)
        kind = name.rsplit(".", 1)[1].lower()
        target = FLIGHT_DIR / (checksum + "." + kind)
        duplicate = target.exists()
        if not duplicate:
            try:
                # Exclusive create; never overwrite or erase an original.
                with target.open("xb") as stream:
                    stream.write(body)
                    stream.flush()
                    os.fsync(stream.fileno())
            except FileExistsError:
                duplicate = True
            except OSError:
                target.unlink(missing_ok=True)
                raise
        meta = FLIGHT_DIR / (checksum + "." + kind + ".json")
        if not meta.exists():
            try:
                with meta.open("x", encoding="utf-8") as stream:
                    json.dump({"name": name, "sha256": checksum, "size": length, "kind": kind.upper()}, stream)
            except FileExistsError:
                pass
        self._json({"success": True, "name": name, "sha256": checksum, "duplicate": duplicate})

    def _local_files(self):
        out = []
        if FLIGHT_DIR.exists():
            for meta in sorted(FLIGHT_DIR.glob("*.json"))[:10000]:
                try:
                    entry = json.loads(meta.read_text(encoding="utf-8"))
                    checksum = entry.get("sha256", "")
                    name = entry.get("name", "")
                    kind = entry.get("kind", "")
                    path = FLIGHT_DIR / (checksum + "." + kind.lower())
                    if (SAFE_HASH.fullmatch(checksum) and SAFE_NAME.fullmatch(name) and
                            kind in ("BIN", "SEN", "TXT") and path.is_file() and
                            path.stat().st_size == entry.get("size")):
                        out.append(entry)
                except (OSError, ValueError, TypeError):
                    continue
        self._json({"files": out})

    def _local_read(self):
        query = parse_qs(urlsplit(self.path).query)
        sha = query.get("sha256", [""])[0].lower()
        kind = query.get("kind", [""])[0].lower()
        if not SAFE_HASH.fullmatch(sha) or kind not in ("bin", "sen", "txt"):
            self.send_error(400, "Invalid content-addressed file")
            return
        file = FLIGHT_DIR / (sha + "." + kind)
        if not file.is_file() or file.stat().st_size > 52428800:
            self.send_error(404, "Local flight not found")
            return
        body = file.read_bytes()
        if hashlib.sha256(body).hexdigest() != sha:
            self.send_error(409, "Stored flight checksum mismatch")
            return
        self.send_response(200)
        self.send_header("Content-Type", "application/octet-stream")
        self.send_header("Cache-Control", "no-store")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_POST(self) -> None:
        if not self._safe_request():
            return
        if urlsplit(self.path).path == "/api/local/save":
            try:
                self._local_save()
            except OSError:
                self.send_error(507, "Local flight archive write failed")
        elif self.path.startswith("/api/"):
            BRIDGE.relay(self)
        else:
            self.send_error(404)

    def translate_path(self, path: str) -> str:
        relative = unquote(urlsplit(path).path).lstrip("/") or "index.html"
        if relative == "radio.html":
            return str(ROOT / "radio.html")
        requested = (STATIC_ROOT / relative).resolve()
        return str(requested if requested.is_relative_to(STATIC_ROOT.resolve()) else STATIC_ROOT / "index.html")

    def do_GET(self) -> None:
        if not self._safe_request():
            return
        path = urlsplit(self.path).path
        if path == "/config.js":
            body = ("window.GREEN_ION_CONFIG = Object.freeze(" +
                    json.dumps(PUBLIC_CLOUD) + ");\n").encode()
            self.send_response(200)
            self.send_header("Content-Type", "application/javascript; charset=utf-8")
            self.send_header("Cache-Control", "no-store")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)
            return
        if path == "/api/app/health":
            self._json({
                "schema": "GREEN-ION-V16-WINDOWS-HEALTH-2",
                "version": APP_INFO.get("version"),
                "channel": APP_INFO.get("channel"),
                "python": platform.python_version(),
                "platform": platform.platform(),
                "data_root": str(DATA_ROOT),
                "flight_archive": str(FLIGHT_DIR),
                "static_root": str(STATIC_ROOT),
                "receiver_port": RUNTIME.get("receiver_port"),
                "receiver_requested_port": RUNTIME.get("receiver_requested_port"),
                "receiver_auto_discovery": RUNTIME.get("receiver_auto_discovery"),
                "receiver_last_connected_utc": RUNTIME.get("receiver_last_connected_utc"),
                "receiver_last_disconnect_utc": RUNTIME.get("receiver_last_disconnect_utc"),
                "receiver_last_error": RUNTIME.get("receiver_last_error"),
                "receiver_searching_since_utc": RUNTIME.get("receiver_searching_since_utc"),
                "mode": RUNTIME.get("mode"),
                "started_utc": RUNTIME.get("started_utc"),
                "update_script_available": (ROOT / "Update-GreenIon.ps1").is_file(),
                "rollback_script_available": (ROOT / "Rollback-GreenIon.ps1").is_file(),
                "user_data_outside_app": True,
            })
            return
        if path == "/api/app/diagnostics":
            radio = STORE.snapshot()
            try:
                disk_root = DATA_ROOT if DATA_ROOT.exists() else Path.home()
                disk = shutil.disk_usage(disk_root)
                disk_info = {
                    "free_bytes": disk.free,
                    "used_bytes": disk.used,
                    "total_bytes": disk.total,
                }
            except OSError:
                disk_info = {"free_bytes": None, "used_bytes": None, "total_bytes": None}
            archive_count = 0
            try:
                if FLIGHT_DIR.exists():
                    archive_count = sum(1 for _ in FLIGHT_DIR.glob("*.json"))
            except OSError:
                pass
            self._json({
                "schema": "GREEN-ION-V16-DIAGNOSTICS-1",
                "captured_utc": datetime.now(timezone.utc).isoformat(),
                "app": {
                    "version": APP_INFO.get("version"),
                    "channel": APP_INFO.get("channel"),
                    "python": platform.python_version(),
                    "platform": platform.platform(),
                    "started_utc": RUNTIME.get("started_utc"),
                    "mode": RUNTIME.get("mode"),
                    "data_root": str(DATA_ROOT),
                    "archive_count": archive_count,
                    "disk": disk_info,
                },
                "receiver": {
                    "state": radio.get("receiver_state"),
                    "error": radio.get("receiver_error"),
                    "port": RUNTIME.get("receiver_port"),
                    "requested_port": RUNTIME.get("receiver_requested_port"),
                    "auto_discovery": RUNTIME.get("receiver_auto_discovery"),
                    "last_connected_utc": RUNTIME.get("receiver_last_connected_utc"),
                    "last_disconnect_utc": RUNTIME.get("receiver_last_disconnect_utc"),
                    "last_error": RUNTIME.get("receiver_last_error"),
                    "reconnects": radio.get("receiver_reconnects"),
                    "packets_received": radio.get("packets_received"),
                    "estimated_missing": radio.get("packets_missed_estimated"),
                    "duplicates": radio.get("packets_repeated"),
                    "out_of_order": radio.get("packets_out_of_order"),
                    "sender_resets": radio.get("sender_resets"),
                    "rssi_average_dbm": radio.get("rssi_average_dbm"),
                    "snr_average_db": radio.get("snr_average_db"),
                },
                "feather": BRIDGE.link_status(radio),
                "note": "Software diagnostics only; this does not certify RF range, sensor accuracy, SD integrity or flight readiness.",
            })
            return
        if path == "/api/local/files":
            self._local_files()
            return
        if path == "/api/local/file":
            self._local_read()
            return
        if self.path == "/api/status":
            data = BRIDGE.unified_status(STORE.snapshot())
            if data is None:
                self._json({"success": False, "message": "No live Wi-Fi or LoRa telemetry"}, 503)
            else:
                self._json(data)
            return
        if self.path == "/api/v16/telemetry":
            radio = STORE.snapshot()
            self._json(build_unified_telemetry(
                BRIDGE.telemetry_source(),
                radio,
                STORE.history_snapshot(),
                datetime.now(timezone.utc).isoformat(),
            ))
            return
        if self.path == "/api/ground-link":
            self._json(BRIDGE.link_status(STORE.snapshot()))
            return
        if self.path.startswith("/api/") and self.path not in ("/api/telemetry", "/api/export"):
            BRIDGE.relay(self)
            return
        if self.path == "/api/telemetry":
            self._json(STORE.snapshot())
            return
        if self.path == "/api/export":
            with STORE.lock:
                logfile = STORE.log_path
            if not logfile or not logfile.is_file():
                self.send_error(404, "No telemetry CSV yet")
                return
            body = logfile.read_bytes()
            self.send_response(200)
            self.send_header("Content-Type", "text/csv; charset=utf-8")
            self.send_header("Content-Disposition", 'attachment; filename="green-ion-telemetry.csv"')
            self.send_header("Cache-Control", "no-store")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)
            return
        super().do_GET()

    def _json(self, result: dict, status: int = 200) -> None:
        body = json.dumps(result, allow_nan=False).encode()
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Cache-Control", "no-store")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def log_message(self, format: str, *args: object) -> None:
        return


def write_log(row: dict, path: Path) -> None:
    """Append a stable CSV row without discarding any packet fields.

    Radio bench packets and telemetry have different key sets; deriving
    a new column order per row would corrupt the exported CSV schema.
    """
    path.parent.mkdir(parents=True, exist_ok=True)
    needs_header = not path.exists() or path.stat().st_size == 0
    record = {column: row.get(column, "") for column in LOG_COLUMNS}
    record["raw_packet_json"] = json.dumps(row, sort_keys=True, allow_nan=False)
    with path.open("a", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=LOG_COLUMNS)
        if needs_header:
            writer.writeheader()
        writer.writerow(record)


def sample_source(stop: threading.Event, log_path: Path) -> None:
    sequence = 0
    started = time.monotonic()
    while not stop.wait(0.2):
        elapsed = time.monotonic() - started
        altitude = max(0.0, 310.0 * math.sin(min(elapsed / 35.0, 1.0) * math.pi))
        gps_dropout = 18.0 < elapsed % 45.0 < 26.0
        row = {
            "type": "telemetry",
            "sequence": sequence,
            "uptime_ms": round(elapsed * 1000),
            "latitude": 37.4219999 + elapsed * 0.000004,
            "longitude": -122.0840575 + elapsed * 0.000003,
            "altitude_m": round(altitude, 2),
            "pressure_pa": round(101325 * math.exp(-altitude / 8434.5)),
            "gps_altitude_m": round(altitude + 1.8, 2),
            "acceleration_g": round(1.0 + 4.5 * math.exp(-((elapsed - 5.0) / 2.2) ** 2), 3),
            "acceleration_x_g": round(0.08 * math.sin(elapsed), 3),
            "acceleration_y_g": round(0.06 * math.cos(elapsed * 0.7), 3),
            "acceleration_z_g": round(1.0 + 4.5 * math.exp(-((elapsed - 5.0) / 2.2) ** 2), 3),
            "rotation_x_dps": round(18 * math.sin(elapsed * 0.9), 1),
            "rotation_y_dps": round(12 * math.cos(elapsed * 0.6), 1),
            "rotation_z_dps": round(7 * math.sin(elapsed * 0.4), 1),
            "battery_v": round(4.12 - elapsed * 0.0008, 3),
            "speed_mps": round(max(0.0, 55.0 * math.sin(min(elapsed / 35.0, 1.0) * math.pi)), 2),
            "gps_fix_age_ms": round((elapsed % 45.0 - 18.0) * 1000) if gps_dropout else 0,
            "gps_status": "stale" if gps_dropout else "live",
            "satellites": 0 if gps_dropout else 10,
            "flight_state": 3 if altitude > 5 else 1,
            "rssi_dbm": -68 - round(12 * abs(math.sin(elapsed / 10))),
            "health": 0,
            "source": "simulation",
            "received_utc": datetime.now(timezone.utc).isoformat(),
        }
        STORE.update(row)
        write_log(row, log_path)
        sequence += 1


def discover_serial_port() -> str | None:
    """Safely prefer a recognizable Arduino Uno; never guess among unknown ports."""
    requested = os.environ.get("GREEN_ION_UNO_PORT", "").strip()
    try:
        from serial.tools import list_ports  # type: ignore
    except ImportError:
        return None
    ports = list(list_ports.comports())
    if requested:
        for item in ports:
            if item.device.lower() == requested.lower():
                return item.device
        return None
    scored = []
    for item in ports:
        text = " ".join(str(value or "") for value in (
            item.description, item.manufacturer, item.product
        )).lower()
        score = 0
        if "arduino uno" in text:
            score += 100
        elif "arduino" in text:
            score += 70
        if getattr(item, "vid", None) in (0x2341, 0x2A03):
            score += 40
        if "usb serial" in text or "usb-serial" in text:
            score += 10
        scored.append((score, item.device))
    scored.sort(reverse=True)
    if scored and scored[0][0] >= 40:
        # If two equally likely Arduino ports exist, require explicit selection.
        if len(scored) > 1 and scored[1][0] == scored[0][0]:
            return None
        return scored[0][1]
    return None


def serial_source(stop: threading.Event, requested_port: str | None, log_path: Path,
                  auto_discover: bool = True) -> None:
    """Continuously reconnect the Uno, including when USB COM numbers change."""
    try:
        import serial  # type: ignore
    except ImportError:
        STORE.receiver("missing", "Install pyserial: py -3 -m pip install pyserial")
        RUNTIME["receiver_last_error"] = "pyserial unavailable"
        RUNTIME["mode"] = "wifi-only"
        return
    active_port = requested_port
    while not stop.is_set():
        if auto_discover and not active_port:
            active_port = discover_serial_port()
        if not active_port:
            if not RUNTIME.get("receiver_searching_since_utc"):
                RUNTIME["receiver_searching_since_utc"] = datetime.now(timezone.utc).isoformat()
            RUNTIME["receiver_port"] = None
            RUNTIME["mode"] = "wifi+lora-searching"
            STORE.receiver("searching", "Waiting for Arduino Uno USB receiver")
            if stop.wait(2):
                break
            continue
        try:
            RUNTIME["receiver_port"] = active_port
            RUNTIME["mode"] = "wifi+lora-connecting"
            with serial.Serial(active_port, 115200, timeout=1) as connection:
                RUNTIME["receiver_last_connected_utc"] = datetime.now(timezone.utc).isoformat()
                RUNTIME["receiver_last_error"] = ""
                RUNTIME["receiver_searching_since_utc"] = None
                RUNTIME["mode"] = "wifi+lora"
                STORE.receiver("connected", "Waiting for Uno receiver status")
                print(f"[UNO] Serial connected on {active_port}; waiting for packets.")
                while not stop.is_set():
                    line = connection.readline().decode("utf-8", errors="replace").strip()
                    if not line or not line.startswith("{"):
                        continue
                    try:
                        row = json.loads(line, parse_constant=lambda _value: None)
                    except json.JSONDecodeError:
                        continue
                    kind = row.get("type")
                    if kind == "receiver_status":
                        STORE.receiver("ready" if row.get("ok") else "error",
                                       "" if row.get("ok") else str(row.get("error", "Radio initialization failed")),
                                       row)
                    elif kind == "receiver_error":
                        STORE.receiver("error", "Uno reported an invalid/CRC packet")
                    elif kind == "radio_bench":
                        row["source"] = "radio"
                        row["received_utc"] = datetime.now(timezone.utc).isoformat()
                        STORE.bench(row)
                        try:
                            write_log(row, log_path)
                        except (OSError, ValueError) as error:
                            STORE.receiver("error", f"CSV write failed: {error}")
                            print(f"[LOG] Cannot write radio bench CSV: {error}")
                    elif kind == "telemetry":
                        row["source"] = "radio"
                        row["received_utc"] = datetime.now(timezone.utc).isoformat()
                        STORE.update(row)
                        try:
                            write_log(row, log_path)
                        except (OSError, ValueError) as error:
                            STORE.receiver("error", f"CSV write failed: {error}")
                            print(f"[LOG] Cannot write telemetry CSV: {error}")
        except (serial.SerialException, OSError) as error:
            message = str(error)
            RUNTIME["receiver_last_disconnect_utc"] = datetime.now(timezone.utc).isoformat()
            RUNTIME["receiver_last_error"] = message[:300]
            RUNTIME["receiver_port"] = None
            RUNTIME["mode"] = "wifi+lora-searching" if auto_discover else "wifi+lora-reconnecting"
            STORE.receiver("disconnected", message)
            print(f"[UNO] Disconnected from {active_port}: {error}; retrying automatically.")
            if auto_discover:
                # USB reconnects can come back on a different COM number.
                active_port = None
        if stop.wait(2):
            break


def main() -> None:
    parser = argparse.ArgumentParser(description="GREEN ION local-first blue dashboard, Feather Wi-Fi and Uno LoRa")
    source = parser.add_mutually_exclusive_group()
    source.add_argument("--sample", action="store_true", help="use simulated rocket telemetry")
    source.add_argument("--port", help="Arduino Uno serial port, for example COM5")
    source.add_argument("--wifi-only", action="store_true", help="disable automatic Uno USB discovery")
    parser.add_argument("--listen", default="127.0.0.1")
    parser.add_argument("--http-port", type=int, default=8765)
    parser.add_argument("--no-browser", action="store_true", help="do not open the dashboard automatically")
    args = parser.parse_args()
    if args.listen not in ("127.0.0.1", "localhost"):
        raise SystemExit("For security, the ground station may listen only on loopback")
    if not (STATIC_ROOT / "index.html").is_file():
        raise SystemExit("Current blue dashboard not found next to Ground_Station (web/ or Website/)")

    timestamp = datetime.now().strftime("%Y%m%d-%H%M%S")
    log_path = LOG_DIR / f"telemetry-{timestamp}.csv"
    stop = threading.Event()
    use_sample = args.sample
    selected_port = args.port
    auto_receiver = not use_sample and not args.wifi_only and not bool(args.port)
    if auto_receiver:
        selected_port = discover_serial_port()
        if selected_port:
            print(f"[UNO] Auto-discovered receiver on {selected_port}.")
        else:
            print("[UNO] No Uno detected yet. The app will keep watching USB and connect automatically when one appears.")
    target = sample_source if use_sample else serial_source
    target_args = (stop, log_path) if use_sample else (stop, selected_port, log_path, auto_receiver)
    STORE.log_path = log_path
    RUNTIME["receiver_port"] = selected_port
    RUNTIME["receiver_requested_port"] = args.port
    RUNTIME["receiver_auto_discovery"] = auto_receiver
    RUNTIME["mode"] = "simulation" if use_sample else (
        "wifi-only" if args.wifi_only else ("wifi+lora" if selected_port else "wifi+lora-searching")
    )
    # Preserve any older Ground_Station/logs after an in-place update.
    for older in (ROOT / "logs").glob("*.csv"):
        try:
            LOG_DIR.mkdir(parents=True, exist_ok=True)
            destination = LOG_DIR / older.name
            if not destination.exists():
                shutil.copy2(older, destination)
        except OSError as error:
            print(f"[LOG] Cannot preserve older log {older.name}: {error}")
    if not use_sample and not args.wifi_only:
        try:
            import serial  # noqa: F401
        except ImportError as error:
            raise SystemExit("Live Uno mode requires pyserial: py -3 -m pip install pyserial") from error
    if use_sample or not args.wifi_only:
        threading.Thread(target=target, args=target_args, daemon=True).start()
    threading.Thread(target=BRIDGE.run, args=(stop,), daemon=True).start()
    url = f"http://{args.listen}:{args.http_port}"
    try:
        server = ThreadingHTTPServer((args.listen, args.http_port), Handler)
    except OSError as error:
        stop.set()
        if getattr(error, "winerror", None) == 10048 or error.errno in (48, 98):
            print(f"Ground station is already running at {url}")
            if not args.no_browser:
                webbrowser.open(url)
            return
        raise
    print(f"GREEN ION V16 Windows Ground Station {APP_INFO.get('version', 'unknown')}: {url}")
    print(f"Automatic computer flight archive: {FLIGHT_DIR}")
    print("Mode: SIMULATED DATA — NOT HARDWARE" if use_sample else
          f"Mode: Feather Wi-Fi + {('Uno radio on ' + selected_port) if selected_port else ('watching USB for Uno' if auto_receiver else 'Wi-Fi only')}")
    print("Feather Wi-Fi: http://192.168.4.1 (join GREEN-ION-V14 or compatible AP)")
    print(f"Telemetry log: {log_path}")
    print("Keep this window open while using the website. Press Ctrl+C to stop.")
    if not args.no_browser:
        threading.Timer(0.25, webbrowser.open, args=(url,)).start()
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        stop.set()
        server.server_close()


if __name__ == "__main__":
    main()

