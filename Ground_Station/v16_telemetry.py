"""GREEN ION V16 unified telemetry model.

This module fuses independently-fresh Feather Wi-Fi and receive-only LoRa
observations for the Windows mission display. It never authorizes commands and
never replaces immutable SD recordings.
"""
from __future__ import annotations

import math
from typing import Any

SCHEMA = "GREEN-ION-V16-TELEMETRY-1"
EXPECTED_RADIO_PROTOCOL = 2
RADIO_LIVE_MS = 3000

_FIELDS = {
    "altitude_m": ("relative_altitude_m", "altitude_m"),
    "pressure_pa": ("pressure_pa", "pressure_pa"),
    "battery_v": ("battery_v", "battery_v"),
    "acceleration_g": ("acceleration_g", "acceleration_g"),
    "acceleration_x_g": ("acceleration_x_g", "acceleration_x_g"),
    "acceleration_y_g": ("acceleration_y_g", "acceleration_y_g"),
    "acceleration_z_g": ("acceleration_z_g", "acceleration_z_g"),
    "rotation_x_dps": ("rotation_x_dps", "rotation_x_dps"),
    "rotation_y_dps": ("rotation_y_dps", "rotation_y_dps"),
    "rotation_z_dps": ("rotation_z_dps", "rotation_z_dps"),
    "gps_altitude_m": ("gps_altitude_m", "gps_altitude_m"),
    "gps_speed_mps": ("gps_speed_mps", "speed_mps"),
    "flight_state": ("flight_state", "flight_state"),
    "health": ("health", "health"),
    "latitude": ("latitude", "latitude"),
    "longitude": ("longitude", "longitude"),
    "satellites": ("satellites", "satellites"),
    "gps_status": ("gps_status", "gps_status"),
}


def _safe(value: Any) -> Any:
    if isinstance(value, bool) or value is None or isinstance(value, str):
        return value
    if isinstance(value, (int, float)):
        return value if math.isfinite(value) else None
    return None


def _reading(value: Any, source: str, age_ms: int | None, freshness: str) -> dict:
    return {
        "value": _safe(value),
        "source": source,
        "age_ms": age_ms,
        "freshness": freshness,
    }


def _pick(wifi: dict | None, radio: dict, wifi_live: bool, radio_live: bool,
          wifi_age: int | None, radio_age: int | None, wifi_key: str, radio_key: str) -> dict:
    wifi_value = _safe((wifi or {}).get(wifi_key))
    radio_value = _safe(radio.get(radio_key))
    if wifi_live and wifi_value is not None:
        return _reading(wifi_value, "wifi", wifi_age, "live")
    if radio_live and radio_value is not None:
        return _reading(radio_value, "lora", radio_age, "live")
    if wifi_value is not None:
        return _reading(wifi_value, "wifi", wifi_age, "stale")
    if radio_value is not None:
        return _reading(radio_value, "lora", radio_age, "stale")
    return _reading(None, "none", None, "missing")


def build_unified_telemetry(wifi_source: dict, radio: dict, radio_history: list[dict],
                            generated_utc: str) -> dict:
    """Return one stable source-aware telemetry document.

    Wi-Fi has priority when live because it exposes the full Feather state.
    LoRa fills gaps or becomes primary when Wi-Fi is unavailable. Every chosen
    value carries provenance and age so stale values cannot masquerade as live.
    """
    wifi = wifi_source.get("status") if isinstance(wifi_source.get("status"), dict) else None
    wifi_age = wifi_source.get("age_ms")
    wifi_freshness = wifi_source.get("freshness", "missing")
    wifi_live = wifi_freshness == "live"

    radio_age = radio.get("packet_age_ms")
    radio_live = radio.get("link_status") == "live" and isinstance(radio_age, (int, float)) and radio_age <= RADIO_LIVE_MS
    radio_protocol = radio.get("protocol")
    if radio_protocol is None and isinstance(radio.get("receiver_info"), dict):
        radio_protocol = radio["receiver_info"].get("protocol")
    protocol_ok = radio_protocol is None or radio_protocol == EXPECTED_RADIO_PROTOCOL
    radio_has_observation = (
        radio.get("type") in ("telemetry", "radio_bench")
        or radio.get("packet_age_ms") is not None
    )

    if wifi_live and radio_live:
        mode = "wifi+lora"
    elif wifi_live:
        mode = "wifi"
    elif radio_live:
        mode = "lora"
    elif wifi is not None or radio_has_observation:
        mode = "stale"
    else:
        mode = "none"

    primary = "wifi" if wifi_live else "lora" if radio_live else "wifi_stale" if wifi is not None else "none"
    readings = {
        name: _pick(wifi, radio, wifi_live, radio_live, wifi_age, radio_age, wk, rk)
        for name, (wk, rk) in _FIELDS.items()
    }

    receiver_session = radio.get("receiver_session_id")
    sender_session = radio.get("sender_session_index")
    sequence = radio.get("sequence")
    packet_id = radio.get("packet_id")
    mission_id = None
    if wifi:
        # Existing firmware has no dedicated mission UUID. A completed/active
        # log filename is safe to expose as a file identity, not a fabricated ID.
        mission_id = wifi.get("log_file") or None

    return {
        "schema": SCHEMA,
        "generated_utc": generated_utc,
        "mode": mode,
        "primary_source": primary,
        "links": {
            "wifi": {
                "freshness": wifi_freshness,
                "connection_state": wifi_source.get("connection_state", "disconnected"),
                "age_ms": wifi_age,
                "controls_verified": bool(wifi_source.get("controls_verified")),
                "error": wifi_source.get("error", ""),
            },
            "lora": {
                "freshness": "live" if radio_live else radio.get("link_status", "missing"),
                "age_ms": radio_age,
                "receiver_state": radio.get("receiver_state", "missing"),
                "receiver_error": radio.get("receiver_error", ""),
                "receiver_reconnects": radio.get("receiver_reconnects", 0),
                "rssi_dbm": radio.get("rssi_dbm"),
                "rssi_average_dbm": radio.get("rssi_average_dbm"),
                "snr_db": radio.get("snr_db"),
                "snr_average_db": radio.get("snr_average_db"),
            },
        },
        "identity": {
            "receiver_session_id": receiver_session,
            "sender_session_index": sender_session,
            "packet_id": packet_id,
            "sequence": sequence,
            "sender_uptime_ms": radio.get("uptime_ms"),
            "mission_file_id": mission_id,
            "rocket": (wifi or {}).get("selected_rocket"),
            "motor": (wifi or {}).get("selected_motor"),
        },
        "integrity": {
            "expected_radio_protocol": EXPECTED_RADIO_PROTOCOL,
            "radio_protocol": radio_protocol,
            "radio_protocol_compatible": protocol_ok,
            "last_packet_classification": radio.get("packet_classification"),
            "packets_received": radio.get("packets_received", 0),
            "telemetry_packets": radio.get("telemetry_packets", 0),
            "accepted_telemetry_packets": radio.get("accepted_telemetry_packets", 0),
            "duplicates": radio.get("packets_repeated", 0),
            "out_of_order": radio.get("packets_out_of_order", 0),
            "estimated_missing": radio.get("packets_missed_estimated", 0),
            "sender_resets": radio.get("sender_resets", 0),
            "loss_pct_estimated": radio.get("loss_pct_estimated", 0),
            "received_utc": radio.get("received_utc"),
        },
        "readings": readings,
        "sources": {
            "radio_latest": {
                key: radio.get(key) for key in (
                    "type", "protocol", "sequence", "uptime_ms", "packet_id",
                    "packet_classification", "received_utc", "packet_age_ms",
                    "altitude_m", "pressure_pa", "gps_altitude_m", "speed_mps",
                    "acceleration_g", "acceleration_x_g", "acceleration_y_g",
                    "acceleration_z_g", "rotation_x_dps", "rotation_y_dps",
                    "rotation_z_dps", "battery_v", "gps_fix_age_ms",
                    "gps_status", "satellites", "flight_state", "health",
                    "latitude", "longitude", "rssi_dbm", "snr_db"
                )
            }
        },
        "radio_history": list(radio_history),
        "detail_policy": {
            "live_display": "critical fused values only",
            "full_resolution": "immutable RKT4 BIN and GIS1 SEN on SD/local history",
            "lora_role": "compact receive-only backup telemetry",
        },
    }
