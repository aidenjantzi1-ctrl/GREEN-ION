"""GREEN ION V13: safe, local bridge between Feather Wi-Fi and Uno LoRa.

The Windows browser stays at 127.0.0.1 even when Feather Wi-Fi disappears.
Only the fixed Feather host (192.168.4.1 by default) is contacted. No generic
URL proxy, Internet dependency or commands over LoRa are provided.
"""
from __future__ import annotations

from http.client import HTTPConnection, HTTPException
import json
import math
import threading
import time
from urllib.parse import urlsplit

MAX_CONTROL_UPLOAD = 8 * 1024 * 1024


def _live_number(value):
    return value if isinstance(value, (int, float)) and not isinstance(value, bool) and math.isfinite(value) else None


def _valid_position(lat, lon):
    return (_live_number(lat) is not None and _live_number(lon) is not None
            and -90 <= lat <= 90 and -180 <= lon <= 180)


class FeatherBridge:
    """Poll Wi-Fi independently of USB LoRa; never send flight controls over radio."""

    def __init__(self, host="192.168.4.1", port=80, clock=None):
        self.host = host
        self.port = port
        self.clock = clock or time.monotonic
        self._lock = threading.Lock()
        self._status = None
        self._last_good = None
        self._error = "No Feather status received"

    def _fetch_status(self):
        conn = HTTPConnection(self.host, self.port, timeout=1.0)
        try:
            conn.request("GET", "/api/status", headers={"Connection": "close", "Cache-Control": "no-store"})
            response = conn.getresponse()
            if response.status != 200:
                raise ConnectionError("Feather returned HTTP " + str(response.status))
            data = response.read(128 * 1024 + 1)
            if len(data) > 128 * 1024:
                raise ValueError("Feather status is unexpectedly large")
            status = json.loads(data)
            if not isinstance(status, dict) or not {"uptime_s", "sd_ok", "pressure_pa"} <= status.keys():
                raise ValueError("Response is not a GREEN ION Feather status")
            return status
        finally:
            conn.close()

    def poll_once(self):
        try:
            status = self._fetch_status()
            with self._lock:
                self._status = status
                self._last_good = self.clock()
                self._error = ""
            return True
        except (OSError, TimeoutError, HTTPException, ValueError, json.JSONDecodeError) as error:
            with self._lock:
                self._error = str(error)[:200]
            return False

    def run(self, stop):
        while not stop.is_set():
            self.poll_once()
            stop.wait(0.9)

    def _wifi(self):
        with self._lock:
            status = dict(self._status) if self._status is not None else None
            age = None if self._last_good is None else max(0, self.clock() - self._last_good)
            error = self._error
        state = "missing" if age is None else "live" if age <= 2.5 else "stale"
        return state, status, age, error

    def link_status(self, radio):
        wifi, _, age, error = self._wifi()
        demo = radio.get("source") == "simulation"
        lora = "demo" if demo else radio.get("link_status", "missing")
        source = "wifi" if wifi == "live" else ("demo" if demo and lora == "demo" else
                 "lora" if lora == "live" else "stale" if wifi == "stale" or lora == "stale" else "none")
        return {
            "version": 13,
            "source": source,
            "wifi": {"status": wifi, "age_ms": round(age * 1000) if age is not None else None,
                     "error": error if wifi != "live" else ""},
            "lora": {"status": lora, "packet_age_ms": radio.get("packet_age_ms"),
                     "receiver_state": radio.get("receiver_state", "missing"),
                     "receiver_error": radio.get("receiver_error", ""),
                     "rssi_dbm": radio.get("rssi_dbm"),
                     "packets_received": radio.get("packets_received", 0),
                     "bench_packets": radio.get("bench_packets", 0),
                     "estimated_missing": radio.get("packets_missed_estimated", 0)},
            "last_position": radio.get("last_position"),
            "last_position_age_ms": radio.get("last_position_age_ms"),
            "gps_status": radio.get("gps_status", "missing"),
            "radio_source": radio.get("source", ""),
        }

    def unified_status(self, radio):
        """Return the Feather API shape so the exact same dashboard can render it.

        Wi-Fi wins when reachable. Radio-only mode has NO SD, OTA, preflight,
        control, or full-resolution-recording claims. Those features require
        a fresh Wi-Fi connection and are blocked by the HTTP proxy.
        """
        wifi, status, _, _ = self._wifi()
        if wifi == "live" and status is not None:
            status["v13_source"] = "wifi"
            status["v13_wifi_connected"] = True
            return status
        if radio.get("link_status") != "live":
            return None
        source = "demo" if radio.get("source") == "simulation" else "lora"
        lat, lon = radio.get("latitude"), radio.get("longitude")
        gps = radio.get("gps_status", "missing")
        if not _valid_position(lat, lon) or gps != "live":
            last = radio.get("last_position")
            if isinstance(last, (list, tuple)) and len(last) == 2 and _valid_position(*last):
                lat, lon = last
                gps = "stale"
            else:
                lat, lon = None, None
                gps = "missing"
        data = {
            "v13_source": source,
            "v13_wifi_connected": False,
            "recording": False,  # Unknown over radio; not proof of SD state.
            "recorder_mode": -1,
            "flight_state": radio.get("flight_state"),
            "relative_altitude_m": _live_number(radio.get("altitude_m")),
            "pressure_pa": _live_number(radio.get("pressure_pa")),
            "battery_v": _live_number(radio.get("battery_v")),
            "acceleration_g": _live_number(radio.get("acceleration_g")),
            "acceleration_x_g": _live_number(radio.get("acceleration_x_g")),
            "acceleration_y_g": _live_number(radio.get("acceleration_y_g")),
            "acceleration_z_g": _live_number(radio.get("acceleration_z_g")),
            "rotation_x_dps": _live_number(radio.get("rotation_x_dps")),
            "rotation_y_dps": _live_number(radio.get("rotation_y_dps")),
            "rotation_z_dps": _live_number(radio.get("rotation_z_dps")),
            "vertical_speed_mps": None,  # LoRa speed is GPS ground speed, NOT vertical speed.
            "gps_speed_mps": _live_number(radio.get("speed_mps")),
            "gps_altitude_m": _live_number(radio.get("gps_altitude_m")),
            "gps_status": gps,
            "latitude": lat,
            "longitude": lon,
            "satellites": radio.get("satellites"),
            "gps_fix_age_ms": radio.get("gps_fix_age_ms"),
            "preflight_state": "WIFI_UNAVAILABLE",
            "preflight_flags": 0,
            "hardware_tests_allowed": False,
            "ota_available": False,
            "sd_ok": False,  # Unknown: displayed as unavailable, never as failed hardware.
            "imu_ok": False,
            "barometer_ready": False,
            "selected_rocket": "LoRa telemetry (profile unavailable)",
            "radio_ok": True,
            "firmware_build": "Wi-Fi disconnected",
            "radio_packet_age_ms": radio.get("packet_age_ms"),
            "radio_rssi_dbm": radio.get("rssi_dbm"),
        }
        return data

    def relay(self, handler):
        """Forward one /api request to the fixed Feather; preserve binary responses.

        Browser requests arrive on localhost. No CORS changes, open proxy,
        redirect following or LoRa command fallback are permitted.
        """
        # Never send a command or firmware image to an unverified IP when
        # Windows has left the rocket AP and joined another network.
        if self._wifi()[0] != "live":
            handler.send_error(503, "Feather Wi-Fi is not currently verified")
            return
        path = urlsplit(handler.path)
        if not path.path.startswith("/api/") or path.path.startswith("/api//") or "\\" in path.path:
            handler.send_error(404, "Not a Feather API path")
            return
        size_text = handler.headers.get("Content-Length", "0")
        try:
            size = int(size_text)
        except ValueError:
            handler.send_error(400, "Invalid request length")
            return
        if size < 0 or size > MAX_CONTROL_UPLOAD:
            handler.send_error(413, "Request too large")
            return
        body = handler.rfile.read(size) if size else None
        upstream = path.path + ("?" + path.query if path.query else "")
        conn = HTTPConnection(self.host, self.port, timeout=8.0 if handler.command == "POST" else 3.0)
        headers_sent = False
        try:
            headers = {"Connection": "close"}
            if handler.headers.get("Content-Type"):
                headers["Content-Type"] = handler.headers["Content-Type"]
            conn.request(handler.command, upstream, body=body, headers=headers)
            response = conn.getresponse()
            handler.send_response(response.status)
            for key in ("Content-Type", "Content-Disposition", "Content-Length"):
                value = response.getheader(key)
                if value:
                    handler.send_header(key, value)
            handler.send_header("Cache-Control", "no-store")
            handler.send_header("Connection", "close")
            handler.end_headers()
            headers_sent = True
            while True:
                chunk = response.read(64 * 1024)
                if not chunk:
                    break
                handler.wfile.write(chunk)
        except (OSError, TimeoutError, HTTPException) as error:
            # If headers were sent, the browser sees a network error on this
            # connection; do not attempt to append a second HTTP response.
            if headers_sent:
                handler.close_connection = True
            else:
                handler.send_error(503, "Feather Wi-Fi unavailable: " + str(error)[:120])
        finally:
            conn.close()
