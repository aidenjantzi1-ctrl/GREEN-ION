import json
from pathlib import Path
import re
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "Ground_Station"))

from telemetry_manager import (MEASUREMENT_FIELDS, SessionWriter, TelemetryManager,
                               battery_estimate_percent, is_newer_sequence, valid_coordinate)
import ground_station


class Clock:
    def __init__(self):
        self.value = 100.0

    def __call__(self):
        return self.value

    def advance(self, seconds):
        self.value += seconds


class RecorderPrepareTests(unittest.TestCase):
    def test_only_stationary_live_feather_status_can_restart_calibration(self):
        ready = {"active_source": "wifi", "sources": {"wifi": {"status": "live"}},
                 "field_meta": {"recorder_mode": {"freshness": "live"}},
                 "recorder_mode": 1, "recording": False, "flight_state": 1}
        with patch.object(ground_station.MANAGER, "snapshot", return_value=ready), \
                patch.object(ground_station, "feather_post", return_value=(202, {"success": True})) as post:
            self.assertEqual((202, {"success": True}), ground_station.prepare_recorder())
            post.assert_called_once_with("/api/calibrate")
            ready.update({"recorder_mode": 3, "flight_state": 6})
            self.assertEqual(202, ground_station.prepare_recorder()[0])
            post.assert_called_with("/api/calibrate")
            ready.update({"recorder_mode": 1, "flight_state": 1})
            for change in ({"active_source": "simulation"}, {"recording": True},
                           {"flight_state": 3}, {"recorder_mode": None},
                           {"field_meta": {"recorder_mode": {"freshness": "stale"}}}):
                post.reset_mock()
                ready.update(change)
                status, result = ground_station.prepare_recorder()
                self.assertEqual(409, status)
                self.assertFalse(result["success"])
                post.assert_not_called()
                ready.update({"active_source": "wifi", "recording": False,
                              "flight_state": 1, "recorder_mode": 1,
                              "field_meta": {"recorder_mode": {"freshness": "live"}}})


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
        self.manager.ingest("radio", {**self.row(1), "gps_altitude_m": 0.0, "speed_mps": 0.0})
        snapshot = self.manager.snapshot()
        self.assertIsNone(snapshot["gps_altitude_m"])
        self.assertFalse(snapshot["field_meta"]["gps_altitude_m"]["valid"])
        self.assertIsNone(snapshot["gps_speed_mps"])

    def test_radio_ground_speed_does_not_become_vertical_speed(self):
        row = self.row(1)
        row.update(latitude=37.1, longitude=-121.9, gps_status="live",
                   gps_fix_age_ms=100, speed_mps=12.5, gps_altitude_m=152.0)
        self.manager.ingest("radio", row)
        snapshot = self.manager.snapshot()
        self.assertEqual(12.5, snapshot["gps_speed_mps"])
        self.assertEqual("radio", snapshot["field_meta"]["gps_speed_mps"]["source"])
        self.assertNotIn("vertical_speed_mps", snapshot)

    def test_battery_estimate_boundaries_and_missing_value(self):
        self.assertIsNone(battery_estimate_percent(None))
        self.assertIsNone(battery_estimate_percent(4.4))
        self.assertEqual(0, battery_estimate_percent(3.3))
        self.assertEqual(100, battery_estimate_percent(4.2))
        self.manager.ingest("wifi", {**self.row(1), "battery_v": 3.49})
        snapshot = self.manager.snapshot()
        self.assertEqual("critical", snapshot["battery_warning"])
        self.assertTrue(snapshot["field_meta"]["battery_estimate_pct"]["valid"])
        self.clock.advance(3)
        self.assertEqual("stale", self.manager.snapshot()["field_meta"]["battery_estimate_pct"]["freshness"])

    def test_radio_sensor_failure_flags_hide_zero_placeholders(self):
        row = {**self.row(1), "flags": 0, "pressure_pa": 0,
               "acceleration_g": 0.0, "rotation_x_dps": 0.0}
        self.manager.ingest("radio", row)
        snapshot = self.manager.snapshot()
        for name in ("pressure_pa", "altitude_m", "acceleration_g", "rotation_x_dps"):
            self.assertIsNone(snapshot[name])
            self.assertFalse(snapshot["field_meta"][name]["valid"])
        self.assertFalse(snapshot["barometer_ready"])
        self.assertFalse(snapshot["imu_ok"])
        self.assertFalse(snapshot["sd_ok"])
        self.assertFalse(snapshot["recording"])

    def test_radio_valid_zero_readings_remain_valid(self):
        flags = (1 << 2) | (1 << 3) | (1 << 4) | (1 << 5)
        self.manager.ingest("radio", {**self.row(1, 0), "flags": flags,
                                      "pressure_pa": 101325, "acceleration_g": 0.0})
        snapshot = self.manager.snapshot()
        self.assertEqual(0, snapshot["altitude_m"])
        self.assertEqual(0.0, snapshot["acceleration_g"])
        self.assertTrue(snapshot["field_meta"]["altitude_m"]["valid"])
        self.assertTrue(snapshot["field_meta"]["acceleration_g"]["valid"])

    def test_gps_altitude_validity_is_independent_of_position_fix(self):
        row = self.row(1)
        row.update(latitude=37.1, longitude=-121.9, gps_status="live",
                   gps_fix_age_ms=100, gps_altitude_m=151.0,
                   gps_altitude_valid=False, gps_fix_quality=0,
                   gps_communication="ok", gps_nmea_sentences=12,
                   gps_parsed_sentences=11)
        self.manager.ingest("wifi", row)
        snapshot = self.manager.snapshot()
        self.assertEqual("live", snapshot["gps_status"])
        self.assertFalse(snapshot["gps_altitude_valid"])
        self.assertEqual("stale", snapshot["field_meta"]["gps_altitude_m"]["freshness"])
        self.assertEqual(0, snapshot["gps_fix_quality"])
        self.assertEqual(12, snapshot["gps_nmea_sentences"])

    def test_recorder_diagnostics_are_present_only_when_reported(self):
        self.manager.ingest("wifi", {**self.row(1), "log_file": "V12D001.BIN",
                                     "recorder_mode": 2, "checkpoint_failures": 0,
                                     "sensor_log_dropped": 3, "recovery_pending": 0,
                                     "recorder_written_records": 45,
                                     "recorder_written_bytes": 752,
                                     "checkpoint_committed_records": 42})
        snapshot = self.manager.snapshot()
        self.assertEqual("V12D001.BIN", snapshot["log_file"])
        self.assertTrue(snapshot["field_meta"]["checkpoint_failures"]["valid"])
        self.assertTrue(snapshot["field_meta"]["recovery_pending"]["valid"])
        self.assertEqual(45, snapshot["recorder_written_records"])
        self.assertEqual(42, snapshot["checkpoint_committed_records"])
        self.clock.advance(3)
        self.manager.ingest("radio", {**self.row(2), "flags": 1 << 4})
        snapshot = self.manager.snapshot()
        self.assertEqual("radio", snapshot["active_source"])
        self.assertFalse(snapshot["field_meta"]["checkpoint_failures"]["valid"])
        self.assertFalse(snapshot["field_meta"]["log_file"]["valid"])
        self.assertFalse(snapshot["field_meta"]["recorder_written_records"]["valid"])

    def test_sd_capacity_snapshot_is_labeled_stale_during_recording(self):
        row = {**self.row(1), "sd_capacity_known": True,
               "sd_total_bytes": 8_000_000, "sd_used_bytes": 2_000_000,
               "sd_free_bytes": 6_000_000, "sd_capacity_age_ms": 100,
               "sample_rate_hz": 50, "checkpoint_committed_records": 42,
               "recording": False}
        self.manager.ingest("wifi", row)
        snapshot = self.manager.snapshot()
        self.assertEqual("live", snapshot["field_meta"]["sd_free_bytes"]["freshness"])
        self.assertEqual(42, snapshot["checkpoint_committed_records"])
        row.update(sequence=2, uptime_ms=200, recording=True, sd_capacity_age_ms=200)
        self.manager.ingest("wifi", row)
        snapshot = self.manager.snapshot()
        self.assertEqual("stale", snapshot["field_meta"]["sd_free_bytes"]["freshness"])
        row.update(sequence=3, uptime_ms=300, recording=False, sd_capacity_age_ms=12000)
        self.manager.ingest("wifi", row)
        self.assertEqual("stale", self.manager.snapshot()["field_meta"]["sd_free_bytes"]["freshness"])

    def test_missing_sd_and_lost_link_do_not_report_live_capacity(self):
        self.manager.ingest("wifi", {**self.row(1), "sd_ok": False,
                                     "sd_capacity_known": False,
                                     "sd_free_bytes": None})
        snapshot = self.manager.snapshot()
        self.assertEqual("missing", snapshot["field_meta"]["sd_free_bytes"]["freshness"])
        self.manager.ingest("wifi", {**self.row(2), "sd_ok": True,
                                     "sd_capacity_known": True,
                                     "sd_free_bytes": 6000,
                                     "sd_capacity_age_ms": 100})
        self.clock.advance(3)
        snapshot = self.manager.snapshot()
        self.assertEqual("stale", snapshot["field_meta"]["sd_free_bytes"]["freshness"])

    def test_sensor_sample_times_are_distinct_from_packet_receive_time(self):
        row = {**self.row(1), "pressure_pa": 101325, "acceleration_g": 1.0,
               "battery_v": 4.0, "gps_status": "live", "latitude": 37.1,
               "longitude": -121.9, "gps_fix_age_ms": 100,
               "gps_altitude_m": 150.0,
               "barometer_sample_uptime_ms": 91,
               "imu_sample_uptime_ms": 92,
               "battery_sample_uptime_ms": 93,
               "gps_fix_uptime_ms": 80,
               "gps_altitude_uptime_ms": 79}
        self.manager.ingest("wifi", row)
        fields = self.manager.snapshot()["field_meta"]
        self.assertEqual(91, fields["pressure_pa"]["sample_uptime_ms"])
        self.assertEqual(92, fields["acceleration_g"]["sample_uptime_ms"])
        self.assertEqual(93, fields["battery_v"]["sample_uptime_ms"])
        self.assertEqual(80, fields["latitude"]["sample_uptime_ms"])
        self.assertEqual(79, fields["gps_altitude_m"]["sample_uptime_ms"])
        self.assertEqual("sensor_sample", fields["gps_altitude_m"]["sample_time_basis"])
        self.assertIsNotNone(fields["pressure_pa"]["received_utc"])

    def test_radio_sample_time_is_packet_uptime_approximation(self):
        self.manager.ingest("radio", {**self.row(1), "flags": 1 << 4,
                                      "pressure_pa": 101325})
        meta = self.manager.snapshot()["field_meta"]["pressure_pa"]
        self.assertEqual(100, meta["sample_uptime_ms"])
        self.assertEqual("packet_uptime", meta["sample_time_basis"])

    def test_preflight_battery_does_not_pass_stale_or_invalid_voltage(self):
        def battery_check():
            with patch.object(ground_station, "MANAGER", self.manager):
                return next(item for item in ground_station.preflight_report()["checks"]
                            if item["name"] == "Battery")
        self.manager.ingest("wifi", {**self.row(1), "battery_v": 4.0})
        self.assertEqual("PASS", battery_check()["status"])
        self.clock.advance(3)
        self.assertEqual("NOT TESTED", battery_check()["status"])
        self.manager.ingest("wifi", {**self.row(2), "battery_v": 5.0})
        self.assertEqual("FAIL", battery_check()["status"])
        self.manager.ingest("wifi", {**self.row(3), "battery_v": 3.49})
        self.assertEqual("FAIL", battery_check()["status"])

    def test_feather_status_fields_are_accepted_by_ground_station(self):
        firmware = (ROOT / "Green_Ion_V12" / "Green_Ion_V12.ino").read_text(encoding="utf-8")
        status = firmware.split("static void sendDashboardStatus()")[1].split("static bool dashboardFlightFilename")[0]
        keys = set(re.findall(r'\\"([a-zA-Z][a-zA-Z0-9_]*)\\"', status))
        # uptime_s is normalized to uptime_ms by the Wi-Fi source adapter.
        self.assertEqual(set(), keys - set(MEASUREMENT_FIELDS) - {"uptime_s"})

    def test_gps_loss_and_recovery_preserve_separate_altitude_sources(self):
        first = {**self.row(1, 120.0), "latitude": 37.1,
                 "longitude": -121.9, "gps_status": "live",
                 "gps_fix_age_ms": 100, "gps_altitude_m": 900.0,
                 "gps_altitude_valid": True}
        self.manager.ingest("wifi", first)
        snapshot = self.manager.snapshot()
        self.assertEqual(120.0, snapshot["altitude_m"])
        self.assertEqual(900.0, snapshot["gps_altitude_m"])
        self.assertNotIn("combined_altitude_m", snapshot)
        self.clock.advance(1)
        lost = {**first, "sequence": 2, "uptime_ms": 200,
                "altitude_m": 130.0, "gps_status": "stale",
                "gps_fix_age_ms": 5000, "gps_altitude_valid": False}
        self.manager.ingest("wifi", lost)
        snapshot = self.manager.snapshot()
        self.assertEqual("stale", snapshot["field_meta"]["gps_altitude_m"]["freshness"])
        self.assertEqual(130.0, snapshot["altitude_m"])
        self.assertEqual([37.1, -121.9], snapshot["last_position"])
        self.clock.advance(1)
        recovered = {**first, "sequence": 3, "uptime_ms": 300,
                     "altitude_m": 140.0, "latitude": 37.2,
                     "gps_altitude_m": 905.0}
        self.manager.ingest("wifi", recovered)
        snapshot = self.manager.snapshot()
        self.assertEqual("live", snapshot["gps_status"])
        self.assertEqual("live", snapshot["field_meta"]["gps_altitude_m"]["freshness"])
        self.assertEqual([37.2, -121.9], snapshot["last_position"])
        self.assertEqual(140.0, snapshot["altitude_m"])
        self.assertEqual(905.0, snapshot["gps_altitude_m"])

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
