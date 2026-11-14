"""GREEN ION V16 Phase 2 unified telemetry regressions."""
from __future__ import annotations
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "Ground_Station"))

from ground_station import Store  # noqa: E402
from v16_telemetry import build_unified_telemetry, SCHEMA  # noqa: E402


def wifi_source(status=None, freshness="missing", age_ms=None, connection="disconnected"):
    return {
        "status": status,
        "freshness": freshness,
        "age_ms": age_ms,
        "connection_state": connection,
        "controls_verified": freshness == "live" and (age_ms or 0) < 2500,
        "error": "",
    }


def radio(**changes):
    data = {
        "type": "telemetry",
        "protocol": 2,
        "source": "radio",
        "link_status": "live",
        "packet_age_ms": 250,
        "received_utc": "2026-11-12T02:00:00+00:00",
        "receiver_session_id": "receiver-a",
        "sender_session_index": 0,
        "packet_id": "receiver-a:0:10",
        "sequence": 10,
        "uptime_ms": 10000,
        "altitude_m": 99.0,
        "pressure_pa": 100100,
        "battery_v": 4.0,
        "acceleration_g": 1.2,
        "flight_state": 3,
        "gps_status": "live",
        "latitude": 34.1,
        "longitude": -117.2,
        "satellites": 9,
        "rssi_dbm": -72,
        "snr_db": 8,
        "packets_received": 10,
        "telemetry_packets": 10,
        "accepted_telemetry_packets": 10,
        "packets_repeated": 0,
        "packets_out_of_order": 0,
        "packets_missed_estimated": 1,
        "sender_resets": 0,
        "receiver_reconnects": 0,
    }
    data.update(changes)
    return data


class FusionTests(unittest.TestCase):
    def test_both_links_keep_independent_freshness_and_wifi_priority(self):
        wifi = {
            "relative_altitude_m": 101.5,
            "pressure_pa": 100050,
            "battery_v": 4.1,
            "flight_state": 3,
            "gps_status": "live",
            "latitude": 34.1001,
            "longitude": -117.2001,
            "selected_rocket": "Green Eggs",
            "selected_motor": "D12-5",
        }
        result = build_unified_telemetry(
            wifi_source(wifi, "live", 400, "connected"),
            radio(),
            [],
            "2026-11-12T02:00:01+00:00",
        )
        self.assertEqual(result["schema"], SCHEMA)
        self.assertEqual(result["mode"], "wifi+lora")
        self.assertEqual(result["primary_source"], "wifi")
        self.assertEqual(result["readings"]["altitude_m"]["value"], 101.5)
        self.assertEqual(result["readings"]["altitude_m"]["source"], "wifi")
        # A field absent from Wi-Fi may be filled from a live LoRa frame.
        self.assertEqual(result["readings"]["acceleration_g"]["source"], "lora")
        self.assertEqual(result["links"]["wifi"]["freshness"], "live")
        self.assertEqual(result["links"]["lora"]["freshness"], "live")

    def test_lora_only_is_receive_only_primary_without_fabricated_wifi(self):
        result = build_unified_telemetry(
            wifi_source(),
            radio(),
            [],
            "2026-11-12T02:00:01+00:00",
        )
        self.assertEqual(result["mode"], "lora")
        self.assertEqual(result["primary_source"], "lora")
        self.assertEqual(result["readings"]["battery_v"]["source"], "lora")
        self.assertFalse(result["links"]["wifi"]["controls_verified"])
        self.assertIsNone(result["identity"]["mission_file_id"])

    def test_stale_wifi_remains_visible_as_stale_not_live(self):
        result = build_unified_telemetry(
            wifi_source({"relative_altitude_m": 44.0, "battery_v": 3.9}, "stale", 9000, "delayed"),
            radio(link_status="stale", packet_age_ms=9000),
            [],
            "2026-11-12T02:00:01+00:00",
        )
        self.assertEqual(result["mode"], "stale")
        self.assertEqual(result["primary_source"], "wifi_stale")
        self.assertEqual(result["readings"]["altitude_m"]["freshness"], "stale")
        self.assertFalse(result["links"]["wifi"]["controls_verified"])

    def test_radio_protocol_mismatch_is_explicit(self):
        result = build_unified_telemetry(
            wifi_source(),
            radio(protocol=99),
            [],
            "2026-11-12T02:00:01+00:00",
        )
        self.assertFalse(result["integrity"]["radio_protocol_compatible"])
        self.assertEqual(result["integrity"]["expected_radio_protocol"], 2)
        self.assertEqual(result["integrity"]["radio_protocol"], 99)
        self.assertEqual(result["primary_source"], "none")
        self.assertEqual(result["links"]["lora"]["freshness"], "stale")


class RadioStoreTests(unittest.TestCase):
    def setUp(self):
        self.store = Store(receiver_session_id="receiver-test")

    @staticmethod
    def packet(sequence, uptime, received, **changes):
        item = {
            "type": "telemetry", "protocol": 2, "sequence": sequence,
            "uptime_ms": uptime, "received_utc": received,
            "altitude_m": 10 + sequence, "battery_v": 4.0,
            "gps_status": "missing", "rssi_dbm": -70, "snr_db": 7,
        }
        item.update(changes)
        return item

    def test_duplicate_and_out_of_order_do_not_replace_live_sample(self):
        self.store.update(self.packet(10, 10000, "2026-11-12T02:00:00+00:00"))
        first = self.store.snapshot()
        self.store.update(self.packet(10, 10000, "2026-11-12T02:00:01+00:00"))
        self.store.update(self.packet(9, 9500, "2026-11-12T02:00:02+00:00"))
        latest = self.store.snapshot()
        self.assertEqual(latest["sequence"], first["sequence"])
        self.assertEqual(latest["accepted_telemetry_packets"], 1)
        self.assertEqual(latest["telemetry_packets"], 3)
        self.assertEqual(latest["packets_repeated"], 1)
        self.assertEqual(latest["packets_out_of_order"], 1)
        history = self.store.history_snapshot()
        self.assertEqual(history[-2]["packet_classification"], "duplicate")
        self.assertFalse(history[-2]["accepted_for_live_display"])
        self.assertEqual(history[-1]["packet_classification"], "out_of_order")

    def test_gap_and_sender_restart_have_stable_session_identity(self):
        self.store.update(self.packet(10, 10000, "2026-11-12T02:00:00+00:00"))
        self.store.update(self.packet(12, 12000, "2026-11-12T02:00:01+00:00"))
        self.assertEqual(self.store.snapshot()["packets_missed_estimated"], 1)
        self.store.update(self.packet(0, 100, "2026-11-12T02:00:02+00:00"))
        latest = self.store.snapshot()
        self.assertEqual(latest["sender_resets"], 1)
        self.assertEqual(latest["sender_session_index"], 1)
        self.assertEqual(latest["packet_classification"], "sender_restart")
        self.assertEqual(latest["packet_id"], "receiver-test:1:0")

    def test_receiver_reconnects_and_history_are_bounded(self):
        self.store.receiver("connected")
        self.store.receiver("ready")
        self.store.receiver("disconnected", "USB removed")
        self.store.receiver("connected")
        self.assertEqual(self.store.snapshot()["receiver_reconnects"], 1)
        for n in range(300):
            self.store.update(self.packet(n, n * 1000, f"2026-11-12T02:{n//60:02d}:{n%60:02d}+00:00"))
        self.assertEqual(len(self.store.radio_history), 240)
        self.assertEqual(len(self.store.history_snapshot()), 120)


class Phase2SourceContracts(unittest.TestCase):
    def test_local_server_exposes_fused_endpoint_and_safe_discovery(self):
        source = (ROOT / "Ground_Station" / "ground_station.py").read_text(encoding="utf-8")
        self.assertIn('"/api/v16/telemetry"', source)
        self.assertIn("discover_serial_port", source)
        self.assertIn("GREEN_ION_UNO_PORT", source)
        launcher = (ROOT / "Ground_Station" / "OPEN_GROUND_STATION.cmd").read_text(encoding="utf-8")
        self.assertNotIn("set /p", launcher)
        self.assertIn("--port COM5", launcher)
        self.assertIn("--wifi-only", launcher)

    def test_uno_receiver_exports_protocol_rssi_and_snr_without_packet_format_change(self):
        receiver = (ROOT / "Past_Versions/V12/Ground_Receiver" / "Ground_Receiver.ino").read_text(encoding="utf-8")
        protocol = (ROOT / "libraries" / "GreenIonProtocol" / "src" / "GreenIonProtocol.h").read_text(encoding="utf-8")
        self.assertIn("lastSNR()", receiver)
        self.assertIn('snr_db', receiver)
        self.assertIn('protocol', receiver)
        self.assertIn('sizeof(TelemetryPacket) == 70', protocol)


if __name__ == "__main__":
    unittest.main()
