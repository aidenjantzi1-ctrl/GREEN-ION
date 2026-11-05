import json
from pathlib import Path
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "Ground_Station"))

from telemetry_manager import SessionWriter, TelemetryManager, is_newer_sequence, valid_coordinate


class Clock:
    def __init__(self):
        self.value = 100.0

    def __call__(self):
        return self.value

    def advance(self, seconds):
        self.value += seconds


class GroundStationManagerTests(unittest.TestCase):
    def setUp(self):
        self.clock = Clock()
        self.manager = TelemetryManager(clock=self.clock)

    def row(self, sequence, altitude=10.0):
        return {"sequence": sequence, "uptime_ms": sequence * 100,
                "altitude_m": altitude, "gps_status": "missing"}

    def test_wifi_is_preferred_then_radio_takes_over_when_wifi_is_stale(self):
        self.manager.ingest("radio", self.row(1, 11))
        self.manager.ingest("wifi", self.row(1, 12))
        self.assertEqual("wifi", self.manager.snapshot()["active_source"])
        self.clock.advance(3)
        self.manager.ingest("radio", self.row(2, 13))
        snapshot = self.manager.snapshot()
        self.assertEqual("radio", snapshot["active_source"])
        self.assertEqual(13, snapshot["altitude_m"])

    def test_duplicate_and_out_of_order_packets_are_rejected(self):
        self.assertTrue(self.manager.ingest("radio", self.row(10)))
        self.assertFalse(self.manager.ingest("radio", self.row(10)))
        self.assertFalse(self.manager.ingest("radio", self.row(9)))
        source = self.manager.snapshot()["sources"]["radio"]
        self.assertEqual(1, source["duplicates"])
        self.assertEqual(1, source["out_of_order"])

    def test_sequence_gap_and_rollover_are_counted_correctly(self):
        self.manager.ingest("radio", self.row(1))
        self.manager.ingest("radio", self.row(4))
        self.assertEqual(2, self.manager.snapshot()["sources"]["radio"]["missing_packets"])
        self.assertTrue(is_newer_sequence(0xFFFFFFFF, 0))

    def test_last_valid_position_survives_source_loss(self):
        row = self.row(1)
        row.update(latitude=37.1, longitude=-121.9, gps_status="live", gps_fix_age_ms=0)
        self.manager.ingest("radio", row)
        self.clock.advance(20)
        snapshot = self.manager.snapshot()
        self.assertEqual([37.1, -121.9], snapshot["last_position"])
        self.assertEqual("stale", snapshot["link_status"])
        self.assertEqual(20000, snapshot["last_position_age_ms"])
        self.assertEqual("stale", snapshot["gps_status"])
        self.assertEqual("stale", snapshot["field_meta"]["latitude"]["freshness"])

    def test_field_contract_distinguishes_missing_zero_and_source_fallback(self):
        self.manager.ingest("wifi", {**self.row(1, 0.0), "pressure_pa": 101325})
        snapshot = self.manager.snapshot()
        altitude = snapshot["field_meta"]["altitude_m"]
        self.assertEqual((True, "live", "wifi", "m", "calculated"),
                         (altitude["valid"], altitude["freshness"], altitude["source"],
                          altitude["unit"], altitude["origin"]))
        self.assertEqual("missing", snapshot["field_meta"]["battery_v"]["freshness"])
        self.assertIsNone(snapshot["field_meta"]["battery_v"]["received_utc"])
        self.clock.advance(3)
        self.manager.ingest("radio", {**self.row(2, 18.0), "pressure_pa": 100500})
        snapshot = self.manager.snapshot()
        self.assertEqual("radio", snapshot["field_meta"]["pressure_pa"]["source"])
        self.assertEqual("live", snapshot["field_meta"]["pressure_pa"]["freshness"])

    def test_old_gps_fix_is_never_marked_fresh(self):
        row = self.row(1)
        row.update(latitude=37.1, longitude=-121.9, gps_status="live", gps_fix_age_ms=4000)
        self.manager.ingest("radio", row)
        snapshot = self.manager.snapshot()
        self.assertEqual("stale", snapshot["gps_status"])
        self.assertEqual("stale", snapshot["field_meta"]["longitude"]["freshness"])
        self.assertIsNone(snapshot["last_position"])

    def test_fixed_width_radio_zero_is_not_a_gps_altitude_without_position(self):
        self.manager.ingest("radio", {**self.row(1), "gps_altitude_m": 0.0})
        snapshot = self.manager.snapshot()
        self.assertIsNone(snapshot["gps_altitude_m"])
        self.assertFalse(snapshot["field_meta"]["gps_altitude_m"]["valid"])

    def test_bad_coordinates_are_not_saved(self):
        self.assertFalse(valid_coordinate(91, 0))
        row = self.row(1)
        row.update(latitude=91, longitude=2, gps_status="live")
        self.manager.ingest("radio", row)
        self.assertIsNone(self.manager.snapshot()["last_position"])

    def test_append_only_session_and_restart_snapshot_are_written(self):
        with tempfile.TemporaryDirectory() as directory:
            writer = SessionWriter(Path(directory), "test")
            manager = TelemetryManager(writer=writer, clock=self.clock)
            manager.ingest("wifi", self.row(1))
            manager.ingest("wifi", self.row(2))
            events = [json.loads(line) for line in writer.raw_path.read_text().splitlines()]
            self.assertEqual([1, 2], [event["data"]["sequence"] for event in events])
            snapshot = json.loads(writer.snapshot_path.read_text())
            self.assertEqual("test", snapshot["session_id"])
            self.assertEqual(2, snapshot["snapshot"]["sequence"])

    def test_ground_station_ui_has_six_workflow_destinations_and_no_cdn(self):
        html = (ROOT / "Ground_Station" / "index.html").read_text(encoding="utf-8")
        for destination in ("Preflight", "Mission Control", "Recovery", "Analysis", "Diagnostics", "Settings"):
            self.assertIn(destination, html)
        self.assertEqual(6, html.count("data-view="))
        self.assertIn('id="preflight"', html)
        self.assertIn('id="analysis"', html)
        self.assertIn("const finite=v=>v!==null", html)
        self.assertNotIn("https://cdn", html)
        self.assertNotIn("SIMULATED DATA — NOT HARDWARE", html)


if __name__ == "__main__":
    unittest.main()
