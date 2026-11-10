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
from dataclasses import dataclass, field
from datetime import datetime, timezone
from http.server import ThreadingHTTPServer, SimpleHTTPRequestHandler
import json
import math
from urllib.parse import urlsplit, unquote
from pathlib import Path
import threading
import time
import webbrowser

ROOT = Path(__file__).resolve().parent
LOG_DIR = ROOT / "logs"


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
    last_seq: int | None = None
    last_uptime: int | None = None
    avg_rssi: float | None = None
    log_path: Path | None = None

    def receiver(self, status: str, detail: str = "", info: dict | None = None) -> None:
        with self.lock:
            self.receiver_state = status
            self.receiver_error = detail[:300]
            if info is not None:
                self.receiver_info = info

    def _observe_packet(self, row: dict) -> None:
        """Call under lock for both telemetry and CRC-valid radio_bench frames."""
        self.received_packets += 1
        seq, uptime = row.get("sequence"), row.get("uptime_ms")
        if isinstance(seq, int) and isinstance(uptime, int):
            if self.last_uptime is not None and uptime < self.last_uptime:
                self.resets += 1
                self.last_seq = None
            if self.last_seq is not None:
                delta = (seq - self.last_seq) & 0xFFFFFFFF
                if delta == 0:
                    self.repeated_packets += 1
                elif 1 < delta < 1000000:
                    self.inferred_missing += delta - 1
                elif delta >= 1000000:
                    self.out_of_order += 1
            self.last_seq, self.last_uptime = seq, uptime
        rssi = row.get("rssi_dbm")
        if isinstance(rssi, (int, float)) and math.isfinite(rssi):
            self.avg_rssi = float(rssi) if self.avg_rssi is None else (
                0.9 * self.avg_rssi + 0.1 * float(rssi)
            )

    def bench(self, row: dict) -> None:
        with self.lock:
            self.bench_packets += 1
            self.last_bench = row
            self._observe_packet(row)  # Avoid falsely counting bench packets as lost.

    def update(self, row: dict) -> None:
        now = time.monotonic()
        with self.lock:
            self.latest = dict(row)
            self.received_monotonic = now
            self.telemetry_packets += 1
            self._observe_packet(row)
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
            row["bench_packets"] = self.bench_packets
            row["last_bench"] = self.last_bench
            row["packets_received"] = self.received_packets
            row["telemetry_packets"] = self.telemetry_packets
            row["packets_missed_estimated"] = self.inferred_missing
            row["packets_repeated"] = self.repeated_packets
            row["packets_out_of_order"] = self.out_of_order
            row["sender_resets"] = self.resets
            total = self.received_packets + self.inferred_missing
            row["loss_pct_estimated"] = round(self.inferred_missing * 100 / total, 2) if total else 0
            row["rssi_average_dbm"] = round(self.avg_rssi, 1) if self.avg_rssi is not None else None
            row["log_download_available"] = bool(self.log_path and self.log_path.is_file())
            return row

STORE = Store()


class Handler(SimpleHTTPRequestHandler):
    def translate_path(self, path: str) -> str:
        relative = unquote(urlsplit(path).path).lstrip("/") or "index.html"
        requested = (ROOT / relative).resolve()
        return str(requested if requested.is_relative_to(ROOT) else ROOT / "index.html")

    def do_GET(self) -> None:
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

    def _json(self, result: dict) -> None:
        body = json.dumps(result, allow_nan=False).encode()
        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.send_header("Cache-Control", "no-store")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def log_message(self, format: str, *args: object) -> None:
        return


def write_log(row: dict, path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    exists = path.exists()
    with path.open("a", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=sorted(row))
        if not exists:
            writer.writeheader()
        writer.writerow(row)


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


def serial_source(stop: threading.Event, port: str, log_path: Path) -> None:
    try:
        import serial  # type: ignore
    except ImportError:
        STORE.receiver("missing", "Install pyserial: py -3 -m pip install pyserial")
        return
    while not stop.is_set():
        try:
            with serial.Serial(port, 115200, timeout=1) as connection:
                STORE.receiver("connected", "Waiting for Uno receiver status")
                print(f"[UNO] Serial connected on {port}; waiting for packets.")
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
                        STORE.bench(row)
                    elif kind == "telemetry":
                        row["source"] = "radio"
                        row["received_utc"] = datetime.now(timezone.utc).isoformat()
                        STORE.update(row)
                        try:
                            write_log(row, log_path)
                        except OSError as error:
                            STORE.receiver("error", f"CSV write failed: {error}")
                            print(f"[LOG] Cannot write telemetry CSV: {error}")
        except (serial.SerialException, OSError) as error:
            STORE.receiver("disconnected", str(error))
            print(f"[UNO] Disconnected: {error}; retrying in 2 seconds.")
        if stop.wait(2):
            break


def main() -> None:
    parser = argparse.ArgumentParser(description="GREEN ION V12 local ground station")
    source = parser.add_mutually_exclusive_group()
    source.add_argument("--sample", action="store_true", help="use simulated rocket telemetry")
    source.add_argument("--port", help="Arduino Uno serial port, for example COM5")
    parser.add_argument("--listen", default="127.0.0.1")
    parser.add_argument("--http-port", type=int, default=8765)
    parser.add_argument("--no-browser", action="store_true", help="do not open the dashboard automatically")
    args = parser.parse_args()

    timestamp = datetime.now().strftime("%Y%m%d-%H%M%S")
    log_path = LOG_DIR / f"telemetry-{timestamp}.csv"
    stop = threading.Event()
    use_sample = args.sample or not args.port
    target = sample_source if use_sample else serial_source
    target_args = (stop, log_path) if use_sample else (stop, args.port, log_path)
    STORE.log_path = log_path
    if not use_sample:
        try:
            import serial  # noqa: F401
        except ImportError as error:
            raise SystemExit("Live Uno mode requires pyserial: py -3 -m pip install pyserial") from error
    worker = threading.Thread(target=target, args=target_args, daemon=True)
    worker.start()
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
    print(f"GREEN ION ground station: {url}")
    print("Mode: SIMULATED DATA — NOT HARDWARE" if use_sample else f"Mode: radio receiver on {args.port}")
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

