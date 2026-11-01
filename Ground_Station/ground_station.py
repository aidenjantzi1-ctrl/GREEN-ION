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
from dataclasses import dataclass, asdict
from datetime import datetime, timezone
from http.server import ThreadingHTTPServer, SimpleHTTPRequestHandler
import json
import math
from pathlib import Path
import threading
import time

ROOT = Path(__file__).resolve().parent
LOG_DIR = ROOT / "logs"


@dataclass
class Store:
    latest: dict | None = None
    received_monotonic: float = 0.0
    last_position: tuple[float, float] | None = None
    last_position_monotonic: float = 0.0
    lock: threading.Lock = threading.Lock()

    def update(self, row: dict) -> None:
        now = time.monotonic()
        with self.lock:
            self.latest = row
            self.received_monotonic = now
            if row.get("latitude") is not None and row.get("longitude") is not None:
                self.last_position = (float(row["latitude"]), float(row["longitude"]))
                self.last_position_monotonic = now

    def snapshot(self) -> dict:
        now = time.monotonic()
        with self.lock:
            row = dict(self.latest or {})
            row["packet_age_ms"] = round((now - self.received_monotonic) * 1000) if self.latest else None
            row["last_position"] = self.last_position
            row["last_position_age_ms"] = (
                round((now - self.last_position_monotonic) * 1000) if self.last_position else None
            )
            if row.get("packet_age_ms") is not None and row["packet_age_ms"] > 3000:
                row["link_status"] = "stale"
            else:
                row["link_status"] = "live" if self.latest else "missing"
            return row


STORE = Store()


class Handler(SimpleHTTPRequestHandler):
    def translate_path(self, path: str) -> str:
        relative = path.split("?", 1)[0].lstrip("/") or "index.html"
        return str(ROOT / relative)

    def do_GET(self) -> None:
        if self.path == "/api/telemetry":
            body = json.dumps(STORE.snapshot(), allow_nan=False).encode()
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Cache-Control", "no-store")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)
            return
        super().do_GET()

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
    except ImportError as error:
        raise SystemExit("pyserial is required for --port; run: py -3 -m pip install pyserial") from error
    with serial.Serial(port, 115200, timeout=1) as connection:
        while not stop.is_set():
            line = connection.readline().decode("utf-8", errors="replace").strip()
            if not line:
                continue
            try:
                row = json.loads(line)
            except json.JSONDecodeError:
                continue
            if row.get("type") != "telemetry":
                continue
            row["source"] = "radio"
            row["received_utc"] = datetime.now(timezone.utc).isoformat()
            STORE.update(row)
            write_log(row, log_path)


def main() -> None:
    parser = argparse.ArgumentParser(description="GREEN ION V12 local ground station")
    source = parser.add_mutually_exclusive_group(required=True)
    source.add_argument("--sample", action="store_true", help="use simulated rocket telemetry")
    source.add_argument("--port", help="Arduino Uno serial port, for example COM5")
    parser.add_argument("--listen", default="127.0.0.1")
    parser.add_argument("--http-port", type=int, default=8765)
    args = parser.parse_args()

    timestamp = datetime.now().strftime("%Y%m%d-%H%M%S")
    log_path = LOG_DIR / f"telemetry-{timestamp}.csv"
    stop = threading.Event()
    target = sample_source if args.sample else serial_source
    target_args = (stop, log_path) if args.sample else (stop, args.port, log_path)
    worker = threading.Thread(target=target, args=target_args, daemon=True)
    worker.start()
    server = ThreadingHTTPServer((args.listen, args.http_port), Handler)
    print(f"GREEN ION ground station: http://{args.listen}:{args.http_port}")
    print(f"Telemetry log: {log_path}")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        stop.set()
        server.server_close()


if __name__ == "__main__":
    main()

