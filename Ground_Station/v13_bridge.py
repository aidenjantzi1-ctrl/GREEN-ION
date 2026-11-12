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
# The ESP32's HTTP handler occasionally needs more than one second.
STATUS_TIMEOUT_S = 3.0
POLL_INTERVAL_S = 1.5
TELEMETRY_FRESH_S = 5.5
CONTROL_FRESH_S = 2.5
STATUS_LOCK_WAIT_S = 0.05
DELAYED_S = 12.0
DISCONNECTED_S = 30.0


class UpstreamBusy(RuntimeError):
    """The Feather is serving another bounded request; this is not link failure."""


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
        self._upstream_lock = threading.Lock()  # Serialize requests to ESP32
        self._status = None
        self._last_good = None
        self._last_attempt = None
        self._last_failure = None
        self._error = "No Feather status received"
        self._consecutive_failures = 0
        self._started = self.clock()
        self._active_operation = ""
        self._active_since = None

    def _set_active(self, operation):
        with self._lock:
            self._active_operation = operation
            self._active_since = self.clock() if operation else None

    def _record_failure(self, error):
        with self._lock:
            self._last_attempt = self.clock()
            self._last_failure = self._last_attempt
            self._consecutive_failures += 1
            self._error = str(error)[:200]

    def _fetch_status(self):
        # ESP32 WebServer is effectively single-client. Do not let a background
        # status poll queue behind a long file transfer: defer it and retry on
        # the next interval instead of manufacturing an OFFLINE event.
        if not self._upstream_lock.acquire(timeout=STATUS_LOCK_WAIT_S):
            raise UpstreamBusy("Feather is busy serving another request")
        self._set_active("status")
        conn = HTTPConnection(self.host, self.port, timeout=STATUS_TIMEOUT_S)
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
            self._set_active("")
            self._upstream_lock.release()

    def poll_once(self):
        with self._lock:
            self._last_attempt = self.clock()
        try:
            status = self._fetch_status()
            with self._lock:
                self._status = status
                self._last_good = self.clock()
                self._last_failure = None
                self._consecutive_failures = 0
                self._error = ""
            return True
        except UpstreamBusy:
            # Intentional local contention is not a failed Wi-Fi probe.
            return None
        except (OSError, TimeoutError, HTTPException, ValueError, json.JSONDecodeError, ConnectionError) as error:
            self._record_failure(error)
            return False

    def run(self, stop):
        while not stop.is_set():
            self.poll_once()
            stop.wait(POLL_INTERVAL_S)

    def _wifi(self):
        with self._lock:
            status = dict(self._status) if self._status is not None else None
            age = None if self._last_good is None else max(0, self.clock() - self._last_good)
            error = self._error
            failures = self._consecutive_failures
            active = self._active_operation
            active_age = None if self._active_since is None else max(0, self.clock() - self._active_since)
            since_start = max(0, self.clock() - self._started)
        live = age is not None and age <= TELEMETRY_FRESH_S and not error
        state = "live" if live else "missing" if age is None else "stale"
        if live:
            connection = "connected"
        elif age is not None and age <= DELAYED_S:
            connection = "delayed"
        elif age is not None and age <= DISCONNECTED_S:
            connection = "reconnecting"
        elif age is None and since_start <= DELAYED_S:
            connection = "reconnecting"
        else:
            connection = "disconnected"
        return state, status, age, error, connection, failures, active, active_age

    def link_status(self, radio):
        wifi, _, age, error, connection, failures, active, active_age = self._wifi()
        demo = radio.get("source") == "simulation"
        lora = "demo" if demo else radio.get("link_status", "missing")
        source = "wifi" if wifi == "live" else ("demo" if demo and lora == "demo" else
                 "lora" if lora == "live" else "stale" if wifi == "stale" or lora == "stale" else "none")
        return {
            "version": 16,
            "source": source,
            "wifi": {
                "status": wifi,
                "connection_state": connection,
                "age_ms": round(age * 1000) if age is not None else None,
                "error": error if wifi != "live" else "",
                "consecutive_failures": failures,
                "active_operation": active or None,
                "active_operation_age_ms": round(active_age * 1000) if active_age is not None else None,
                "controls_verified": wifi == "live" and age is not None and age <= CONTROL_FRESH_S,
            },
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
        wifi, status, age, error, connection, _, _, _ = self._wifi()
        if wifi == "live" and status is not None:
            status["v13_source"] = "wifi"
            status["v13_wifi_connected"] = True
            status["v16_connection_state"] = connection
            status["v16_status_age_ms"] = round(age * 1000) if age is not None else None
            status["v16_status_stale"] = False
            return status
        if radio.get("link_status") != "live":
            if status is None:
                return None
            # Keep last-known Feather measurements visible through interruptions,
            # but never authorize commands or label them as live.
            status["v13_source"] = "wifi_stale"
            status["v13_wifi_connected"] = False
            status["v16_connection_state"] = connection
            status["v16_status_age_ms"] = round(age * 1000) if age is not None else None
            status["v16_status_stale"] = True
            status["v16_link_error"] = error
            return status
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
            "v16_connection_state": connection,
            "v16_status_age_ms": round(age * 1000) if age is not None else None,
            "v16_status_stale": False,
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
        state, _, age, error, _, _, _, _ = self._wifi()
        if state != "live" or error or age is None or age > CONTROL_FRESH_S:
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
            # Reject excessive dashboard bursts instead of queuing ESP32 requests.
            if not self._upstream_lock.acquire(timeout=0.25):
                handler.send_error(503, "Feather busy; retry shortly")
                return
            operation = "file-transfer" if path.path in ("/api/download", "/api/sensors", "/api/metadata") else "api-request"
            self._set_active(operation)
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
            finally:
                self._set_active("")
                self._upstream_lock.release()
        except (OSError, TimeoutError, HTTPException) as error:
            self._record_failure(error)
            # If headers were sent, the browser sees a network error on this
            # connection; do not attempt to append a second HTTP response.
            if headers_sent:
                handler.close_connection = True
            else:
                handler.send_error(503, "Feather Wi-Fi unavailable: " + str(error)[:120])
        finally:
            conn.close()
