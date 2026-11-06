import json
from io import BytesIO
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
        current = {"recorder_mode": 1, "recording": False, "flight_state": 1}
        with patch.object(ground_station.MANAGER, "snapshot", return_value=ready), \
                patch.object(ground_station, "feather_status", return_value=("http://192.168.4.1", current)), \
                patch.object(ground_station, "feather_post", return_value=(202, {"success": True})) as post:
            self.assertEqual((202, {"success": True}), ground_station.prepare_recorder())
            post.assert_called_once_with("/api/calibrate", base_url="http://192.168.4.1")
            ready.update({"recorder_mode": 3, "flight_state": 6})
            current.update({"recorder_mode": 3, "flight_state": 6})
            self.assertEqual(202, ground_station.prepare_recorder()[0])
            post.assert_called_with("/api/calibrate", base_url="http://192.168.4.1")
            ready.update({"recorder_mode": 1, "flight_state": 1})
            current.update({"recorder_mode": 1, "flight_state": 1})
            current["recording"] = True
            self.assertEqual(409, ground_station.prepare_recorder()[0])
            current["recording"] = False
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


class RecorderStopTests(unittest.TestCase):
    def test_stop_save_requires_current_file_and_explicit_confirmation(self):
        active = {"active_source": "wifi", "sources": {"wifi": {"status": "live"}},
                  "field_meta": {name: {"freshness": "live"}
                                 for name in ("recording", "recorder_mode", "log_file")},
                  "recording": True, "recorder_mode": 2, "log_file": "/V12D007.BIN"}
        current = {"recording": True, "recorder_mode": 2, "log_file": "/V12D007.BIN"}
        with patch.object(ground_station.MANAGER, "snapshot", return_value=active), \
                patch.object(ground_station, "feather_status", return_value=("http://192.168.4.1", current)), \
                patch.object(ground_station, "feather_post", return_value=(200, {"success": True})) as post:
            self.assertEqual(400, ground_station.stop_recorder("STOP", "/V12D007.BIN")[0])
            self.assertEqual(409, ground_station.stop_recorder("STOP AND SAVE", "/V12D008.BIN")[0])
            post.assert_not_called()
            self.assertEqual((200, {"success": True}),
                             ground_station.stop_recorder("STOP AND SAVE", "/V12D007.BIN"))
            post.assert_called_once_with("/api/stop", base_url="http://192.168.4.1")
            current["log_file"] = "/V12D008.BIN"
            self.assertEqual(409, ground_station.stop_recorder("STOP AND SAVE", "/V12D007.BIN")[0])
            post.assert_called_once()
            current["log_file"] = "/V12D007.BIN"
            for change in ({"recording": False}, {"active_source": "radio"},
                           {"field_meta": {"recording": {"freshness": "stale"}}}):
                active.update(change)
                post.reset_mock()
                self.assertEqual(409, ground_station.stop_recorder("STOP AND SAVE", "/V12D007.BIN")[0])
                post.assert_not_called()
                active.update({"recording": True, "active_source": "wifi",
                               "field_meta": {name: {"freshness": "live"}
                                              for name in ("recording", "recorder_mode", "log_file")}})

    def test_stop_route_rejects_non_json_confirmation(self):
        class Request:
            path = "/api/recorder/stop"
            headers = {"Content-Type": "text/plain", "Content-Length": "2"}
            rfile = BytesIO(b"{}")

        with patch.object(ground_station, "stop_recorder") as stop, \
                patch.object(ground_station, "json_response") as reply:
            ground_station.Handler.do_POST(Request())
            stop.assert_not_called()
            self.assertEqual(400, reply.call_args.args[2])

    def test_bench_recording_requires_live_ready_feather_and_test_file(self):
        ready = {"active_source": "wifi", "sources": {"wifi": {"status": "live"}},
                 "field_meta": {key: {"freshness": "live"} for key in
                                ("firmware_version", "recording", "recorder_mode", "sd_ok", "battery_v")},
                 "firmware_version": "V12-development", "recording": False,
                 "recorder_mode": 1, "sd_ok": True, "battery_v": 3.9}
        current = {"firmware_version": "V12-development", "recording": False,
                   "recorder_mode": 1, "sd_ok": True, "battery_v": 3.9}
        with patch.object(ground_station.MANAGER, "snapshot", return_value=ready), \
                patch.object(ground_station, "feather_status", return_value=("http://192.168.4.1", current)), \
                patch.object(ground_station, "feather_post", return_value=(202, {"success": True,
                    "file": "/TEST000.BIN"})) as post:
            self.assertEqual(400, ground_station.start_bench_recording("START")[0])
            self.assertEqual(202, ground_station.start_bench_recording("START BENCH TEST")[0])
            post.assert_called_once_with("/api/bench/start", base_url="http://192.168.4.1")
            ready["battery_v"] = 3.4
            self.assertEqual(409, ground_station.start_bench_recording("START BENCH TEST")[0])
            post.assert_called_once()
        with patch.object(ground_station, "stop_recorder", return_value=(200, {"success": True})) as stop:
            self.assertEqual(400, ground_station.stop_bench_recording("STOP BENCH TEST", "/V12D001.BIN")[0])
            self.assertEqual(200, ground_station.stop_bench_recording("STOP BENCH TEST", "/TEST001.BIN")[0])
            stop.assert_called_once_with("STOP AND SAVE", "/TEST001.BIN")

    def test_sensor_import_route_bounds_upload_and_keeps_raw_in_library(self):
        class Request:
            path = "/api/import-sen/" + "a" * 64
            headers = {"Content-Length": "4", "X-Green-Ion-Filename": "V12D007.SEN"}
            rfile = BytesIO(b"GIS1")

        with patch.object(ground_station.FLIGHTS, "import_sensor_stream", return_value={
                "sha256": "b" * 64, "inspection": {"status": "valid"}}) as imported, \
                patch.object(ground_station, "json_response") as reply:
            ground_station.Handler.do_POST(Request())
            self.assertEqual(4, imported.call_args.kwargs["expected_bytes"])
            self.assertEqual("V12D007.SEN", imported.call_args.args[1])
            self.assertEqual(200, reply.call_args.args[2] if len(reply.call_args.args) > 2 else 200)
            Request.headers["Content-Length"] = str(16 * 1024 * 1024 + 1)
            ground_station.Handler.do_POST(Request())
            self.assertEqual(1, imported.call_count)
            self.assertEqual(400, reply.call_args.args[2])

    def test_sensor_feather_download_uses_imported_bin_identity(self):
        class Request:
            path = "/api/feather/sensor"
            headers = {"Content-Length": str(len(b'{"id":"' + b"a" * 64 + b'"}'))}
            rfile = BytesIO(b'{"id":"' + b"a" * 64 + b'"}')

        response = BytesIO(b"GIS1")
        response.headers = {"Content-Length": "4"}
        with patch.object(ground_station.FLIGHTS, "get", return_value=(
                {"id": "a" * 64, "original_name": "V12D007.BIN"}, Path("unused"))), \
                patch.object(ground_station, "urlopen", return_value=response) as download, \
                patch.object(ground_station.FLIGHTS, "import_sensor_stream", return_value={
                    "sha256": "b" * 64}) as imported, \
                patch.object(ground_station, "json_response") as reply:
            ground_station.Handler.do_POST(Request())
            self.assertIn("/api/sensor?name=V12D007.SEN", download.call_args.args[0].full_url)
            self.assertEqual("V12D007.SEN", imported.call_args.args[1])
            self.assertEqual(4, imported.call_args.kwargs["expected_bytes"])
            self.assertTrue(reply.call_args.args[1]["success"])

    def test_flight_details_route_accepts_small_json_and_rejects_oversize(self):
        payload = b'{"label":"Flight A","rocket_profile":"Estes Green Eggs","flight_date":"2026-11-05"}'

        class Request:
            path = "/api/flight/" + "a" * 64 + "/details"
            headers = {"Content-Length": str(len(payload))}
            rfile = BytesIO(payload)

        with patch.object(ground_station.FLIGHTS, "update_details", return_value={"label": "Flight A"}) as update, \
                patch.object(ground_station, "json_response") as reply:
            ground_station.Handler.do_POST(Request())
            update.assert_called_once_with("a" * 64, "Flight A", "Estes Green Eggs", "2026-11-05")
            self.assertTrue(reply.call_args.args[1]["success"])
            Request.headers["Content-Length"] = "513"
            ground_station.Handler.do_POST(Request())
            self.assertEqual(1, update.call_count)
            self.assertEqual(400, reply.call_args.args[2])

    def test_bench_verify_requires_completed_exact_file_and_preserves_raw_import(self):
        ready = {"active_source": "wifi", "sources": {"wifi": {"status": "live"}},
                 "field_meta": {name: {"freshness": "live"} for name in
                                ("firmware_version", "recording", "log_file")},
                 "firmware_version": "V12-development", "recording": False,
                 "log_file": "/TEST001.BIN"}
        current = {"firmware_version": "V12-development", "recording": False,
                   "log_file": "/TEST001.BIN"}
        response = BytesIO(b"RKT4")
        response.headers = {"X-Green-Ion-Bytes": "48"}
        summary = {"records": 40, "duration_s": 8.0, "timestamp_gaps": 0,
                   "invalid_records": 0, "trailing_bytes": 0, "warnings": []}
        with patch.object(ground_station.MANAGER, "snapshot", return_value=ready), \
                patch.object(ground_station, "feather_status", return_value=("http://192.168.4.1", current)), \
                patch.object(ground_station, "urlopen", return_value=response) as download, \
                patch.object(ground_station.FLIGHTS, "import_stream", return_value={
                    "summary": summary, "sha256": "a" * 64}) as imported:
            self.assertEqual(400, ground_station.verify_bench_recording("/V12D001.BIN")[0])
            self.assertEqual(409, ground_station.verify_bench_recording("/TEST002.BIN")[0])
            download.assert_not_called()
            status, result = ground_station.verify_bench_recording("/TEST001.BIN")
            self.assertEqual(200, status)
            self.assertEqual("PASS", result["status"])
            self.assertIn("physical Wi-Fi disconnect is not proven", result["message"])
            imported.assert_called_once()
            self.assertEqual("TEST001.BIN", imported.call_args.args[0])
            self.assertEqual(48, imported.call_args.kwargs["expected_bytes"])
            ready["recording"] = True
            self.assertEqual(409, ground_station.verify_bench_recording("/TEST001.BIN")[0])
            self.assertEqual(1, download.call_count)
            ready["recording"] = False
        damaged = BytesIO(b"broken")
        damaged.headers = {"Content-Length": "6"}
        with patch.object(ground_station.MANAGER, "snapshot", return_value=ready), \
                patch.object(ground_station, "feather_status", return_value=("http://192.168.4.1", current)), \
                patch.object(ground_station, "urlopen", return_value=damaged), \
                patch.object(ground_station.FLIGHTS, "import_stream",
                             side_effect=ground_station.FlightImportError("bad header")):
            code, result = ground_station.verify_bench_recording("/TEST001.BIN")
            self.assertEqual(422, code)
            self.assertEqual("FAIL", result["status"])
            self.assertIn("Keep the original SD file", result["message"])


class PreflightHistoryTests(unittest.TestCase):
    def test_saved_report_is_exportable_and_incomplete_run_times_out(self):
        from datetime import datetime, timedelta, timezone

        with tempfile.TemporaryDirectory() as temporary, \
                patch.object(ground_station, "PREFLIGHT_DIR", Path(temporary)), \
                patch.object(ground_station, "available_ports", return_value=[]):
            created = (datetime.now(timezone.utc) - timedelta(seconds=46)).isoformat()
            report = ground_station.save_preflight_run({
                "created_utc": created, "run_status": "RUNNING", "overall": "WARNING",
                "progress": "waiting", "actions": {"sensor_test": {"http_status": 202}}})
            self.assertEqual(report["run_id"], ground_station.preflight_runs()[0]["run_id"])
            finished = ground_station.refresh_preflight_run()
            self.assertEqual("COMPLETE", finished["run_status"])
            self.assertIn("not confirmed", finished["progress"])
            raw = (Path(temporary) / f"preflight-{report['run_id']}.json").read_bytes()
            self.assertEqual("COMPLETE", json.loads(raw)["run_status"])

    def test_individual_diagnostics_distinguish_read_only_and_hardware_tests(self):
        manager = TelemetryManager(clock=Clock())
        with patch.object(ground_station, "MANAGER", manager), \
                patch.object(ground_station, "available_ports", return_value=[]), \
                patch.object(ground_station, "feather_post") as post:
            self.assertEqual("NOT TESTED", ground_station.diagnostic_test("lora")[1]["status"])
            self.assertEqual(409, ground_station.diagnostic_test("sd")[0])
            post.assert_not_called()
            manager.ingest("wifi", {"sequence": 1, "uptime_ms": 100,
                                     "recording": False, "recorder_mode": 1})
            with patch.object(ground_station, "feather_status", return_value=(
                    "http://192.168.4.1", {"firmware_version": "V12-development",
                                         "recording": False, "recorder_mode": 1})):
                post.return_value = (200, {"success": True, "message": "readback passed"})
                self.assertEqual("PASS", ground_station.diagnostic_test("sd")[1]["status"])
                post.assert_called_with("/api/sdtest", base_url="http://192.168.4.1")
                post.return_value = (202, {"accepted": True})
                self.assertEqual("WARNING", ground_station.diagnostic_test("gps")[1]["status"])
                self.assertIn("30 seconds", ground_station.diagnostic_test("imu_extended")[1]["message"])
                post.assert_called_with("/api/hardware-test?sensor=imu_extended", base_url="http://192.168.4.1")

    def test_read_only_diagnostics_require_actual_current_session_evidence(self):
        manager = TelemetryManager(clock=Clock())
        with patch.object(ground_station, "MANAGER", manager), \
                patch.object(ground_station, "available_ports", return_value=[]), \
                patch.object(ground_station, "feather_post") as post:
            for kind in ("parser", "checkpoint", "timing", "reconnect"):
                self.assertEqual("NOT TESTED", ground_station.diagnostic_test(kind)[1]["status"])
            manager.ingest("simulation", {"sequence": 1, "uptime_ms": 100})
            self.assertEqual("NOT TESTED", ground_station.diagnostic_test("parser")[1]["status"])
            manager.ingest("wifi", {"sequence": 1, "uptime_ms": 100,
                                    "checkpoint_committed_records": 10, "checkpoint_failures": 0})
            manager.ingest("radio", {"sequence": 1, "uptime_ms": 120})
            manager.ingest("wifi", {"sequence": 2, "uptime_ms": 140,
                                    "checkpoint_committed_records": 20, "checkpoint_failures": 0})
            for kind in ("parser", "checkpoint", "timing", "reconnect"):
                self.assertEqual("PASS", ground_station.diagnostic_test(kind)[1]["status"])
                self.assertFalse(ground_station.diagnostic_test(kind)[1]["hardware_test_started"])
            manager.note_parse_error("wifi", "bad packet")
            self.assertEqual("WARNING", ground_station.diagnostic_test("parser")[1]["status"])
            manager.ingest("wifi", {"sequence": 3, "uptime_ms": 90,
                                    "checkpoint_committed_records": 20, "checkpoint_failures": 1})
            self.assertEqual("WARNING", ground_station.diagnostic_test("timing")[1]["status"])
            self.assertEqual("WARNING", ground_station.diagnostic_test("checkpoint")[1]["status"])
            manager.ingest("wifi", {"sequence": 4, "uptime_ms": 20000,
                                    "battery_v": 3.9, "battery_sample_uptime_ms": 5000})
            self.assertIn("battery_v", ground_station.diagnostic_test("timing")[1]["message"])
            post.assert_not_called()

    def test_battery_stability_needs_distinct_recent_samples(self):
        from datetime import datetime, timedelta, timezone

        now = datetime.now(timezone.utc)
        points = [{"source": "wifi", "battery_sample_uptime_ms": i * 500,
                   "battery_v": 4.0 + i * 0.005,
                   "received_utc": (now - timedelta(seconds=4 - i)).isoformat()}
                  for i in range(5)]
        self.assertEqual("PASS", ground_station.battery_stability(points, "wifi", now)[0])
        self.assertEqual("NOT TESTED", ground_station.battery_stability(points, "simulation", now)[0])
        duplicates = [{**point, "battery_sample_uptime_ms": 100} for point in points]
        self.assertEqual("NOT TESTED", ground_station.battery_stability(duplicates, "wifi", now)[0])
        points[-1]["battery_v"] = 4.2
        self.assertEqual("WARNING", ground_station.battery_stability(points, "wifi", now)[0])

    def test_gps_stability_uses_distinct_current_fixes_and_geographic_distance(self):
        from datetime import datetime, timedelta, timezone

        now = datetime.now(timezone.utc)
        points = [{"source": "wifi", "gps_status": "live", "gps_fix_age_ms": 0,
                   "gps_fix_uptime_ms": i * 1000, "latitude": 37.0 + i * 0.00001,
                   "longitude": -121.0,
                   "received_utc": (now - timedelta(seconds=4 - i)).isoformat()}
                  for i in range(5)]
        self.assertEqual("PASS", ground_station.gps_position_stability(points, now)[0])
        self.assertIn("does not prove position accuracy", ground_station.gps_position_stability(points, now)[1])
        self.assertEqual("NOT TESTED", ground_station.gps_position_stability(
            [{**point, "gps_fix_uptime_ms": 1000} for point in points], now)[0])
        self.assertEqual("NOT TESTED", ground_station.gps_position_stability(
            [{**point, "source": "simulation"} for point in points], now)[0])
        points[-1]["latitude"] += 0.001
        self.assertEqual("WARNING", ground_station.gps_position_stability(points, now)[0])

    def test_diagnostic_history_appends_and_preserves_test_origin(self):
        with tempfile.TemporaryDirectory() as temporary, \
                patch.object(ground_station, "DIAGNOSTIC_LOG", Path(temporary) / "diagnostics.jsonl"):
            ground_station.record_diagnostic_result({"test": "lora", "status": "NOT TESTED",
                                                     "message": "No receiver status",
                                                     "hardware_test_started": False})
            ground_station.record_diagnostic_result({"test": "sd", "status": "PASS",
                                                     "message": "readback verified",
                                                     "hardware_test_started": True})
            ground_station.record_diagnostic_result({"test": "bench_file_integrity", "status": "PASS",
                                                     "message": "RKT4 file validated", "file": "/TEST001.BIN",
                                                     "sha256": "a" * 64, "summary": {"timestamp_gaps": 0}})
            recent = ground_station.recent_diagnostics()
            self.assertEqual(["bench_file_integrity", "sd", "lora"], [item["test"] for item in recent])
            self.assertEqual([False, True, False], [item["hardware_test_started"] for item in recent])
            self.assertEqual("/TEST001.BIN", recent[0]["file"])
            self.assertEqual(0, recent[0]["summary"]["timestamp_gaps"])
            self.assertEqual(3, len((Path(temporary) / "diagnostics.jsonl").read_text().splitlines()))


class GroundStationManagerTests(unittest.TestCase):
    def test_wifi_uptime_prefers_precise_milliseconds_with_legacy_fallback(self):
        self.assertEqual(1534, ground_station.normalize_wifi(
            {"uptime_s": 1, "uptime_ms": 1534}, 7)["uptime_ms"])
        self.assertEqual(1000, ground_station.normalize_wifi(
            {"uptime_s": 1}, 7)["uptime_ms"])

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
        row.update(latitude=37.1, longitude=-121.9, gps_status="live", gps_fix_age_ms=0,
                   flight_state=5, gps_fix_quality=1, satellites=7)
        self.manager.ingest("radio", row)
        self.clock.advance(20)
        snapshot = self.manager.snapshot()
        self.assertEqual([37.1, -121.9], snapshot["last_position"])
        self.assertEqual("stale", snapshot["link_status"])
        self.assertEqual(20000, snapshot["last_position_age_ms"])
        self.assertEqual("stale", snapshot["gps_status"])
        self.assertEqual("stale", snapshot["field_meta"]["latitude"]["freshness"])
        self.assertEqual((5, 1, 7), (snapshot["last_position_phase"],
                                     snapshot["last_position_fix_quality"],
                                     snapshot["last_position_satellites"]))
        self.manager.ingest("radio", {**self.row(2), "flight_state": 6,
                                      "gps_status": "missing", "latitude": None, "longitude": None})
        self.assertEqual(5, self.manager.snapshot()["last_position_phase"])

    def test_recent_history_retains_fields_needed_to_restore_mission_trail(self):
        row = {**self.row(7), "gps_status": "live", "gps_fix_age_ms": 0,
               "gps_fix_uptime_ms": 700,
               "latitude": 37.1, "longitude": -121.9, "gps_altitude_m": 125.0,
               "battery_v": 3.9, "pressure_pa": 101000,
               "acceleration_x_g": 0.1, "rotation_z_dps": 2.5,
               "flight_state": 3}
        self.manager.ingest("wifi", row)
        point = self.manager.recent_history()[-1]
        self.assertEqual(("wifi", 7, 125.0, 3.9, 101000),
                         (point["source"], point["sequence"], point["gps_altitude_m"],
                          point["battery_v"], point["pressure_pa"]))
        self.assertEqual((37.1, -121.9, 0),
                         (point["latitude"], point["longitude"], point["gps_fix_age_ms"]))
        self.assertEqual(700, point["gps_fix_uptime_ms"])
        self.assertEqual((0.1, 2.5), (point["acceleration_x_g"], point["rotation_z_dps"]))
        self.assertEqual(3, point["flight_state"])

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

    def test_gps_utc_requires_live_valid_time_and_flags_clock_disagreement(self):
        from datetime import datetime, timezone

        now = datetime.now(timezone.utc)
        row = {"sequence": 1, "uptime_ms": 100, "gps_utc_valid": True,
               "gps_utc_hour": now.hour, "gps_utc_minute": now.minute,
               "gps_utc_second": now.second, "gps_nmea_sentences": 4}
        with patch.object(ground_station, "MANAGER", self.manager), \
                patch.object(ground_station, "available_ports", return_value=[]):
            self.manager.ingest("wifi", row)
            check = next(item for item in ground_station.preflight_report()["checks"]
                         if item["name"] == "GPS UTC time")
            self.assertEqual("PASS", check["status"])
            self.assertIn("GPS date and laptop date are not compared", check["message"])
            self.manager.ingest("wifi", {**row, "sequence": 2,
                                          "gps_utc_minute": (now.minute + 10) % 60})
            check = next(item for item in ground_station.preflight_report()["checks"]
                         if item["name"] == "GPS UTC time")
            self.assertEqual("WARNING", check["status"])
            self.manager.ingest("wifi", {**row, "sequence": 3, "gps_utc_valid": False})
            check = next(item for item in ground_station.preflight_report()["checks"]
                         if item["name"] == "GPS UTC time")
            self.assertEqual("WARNING", check["status"])
            self.assertIn("recent valid date", check["message"])
        simulated = TelemetryManager(clock=Clock())
        simulated.ingest("simulation", row)
        with patch.object(ground_station, "MANAGER", simulated), \
                patch.object(ground_station, "available_ports", return_value=[]):
            check = next(item for item in ground_station.preflight_report()["checks"]
                         if item["name"] == "GPS UTC time")
            self.assertEqual("NOT TESTED", check["status"])

    def test_recovery_scan_does_not_pass_when_sd_was_unavailable(self):
        def check():
            with patch.object(ground_station, "MANAGER", self.manager), \
                    patch.object(ground_station, "available_ports", return_value=[]):
                return next(item for item in ground_station.preflight_report()["checks"]
                            if item["name"] == "Interrupted-file recovery scan")

        self.manager.ingest("wifi", {"sequence": 1, "uptime_ms": 100,
                                     "recovery_status": "SD unavailable; recovery not scanned",
                                     "recovery_pending": 0, "recovered_logs": 0})
        self.assertEqual("NOT TESTED", check()["status"])
        self.manager.ingest("wifi", {"sequence": 2, "uptime_ms": 200,
                                     "recovery_status": "1 recovery item(s) pending inspection",
                                     "recovery_pending": 1, "recovered_logs": 0})
        self.assertEqual("WARNING", check()["status"])
        self.assertIn("preserve the SD card", check()["correction"])
        self.manager.ingest("wifi", {"sequence": 3, "uptime_ms": 300,
                                     "recovery_status": "No interrupted logs detected",
                                     "recovery_pending": 0, "recovered_logs": 0})
        self.assertEqual("PASS", check()["status"])

    def test_preflight_requires_fresh_sensor_evidence_and_separate_sd_readback(self):
        with patch.object(ground_station, "MANAGER", self.manager), \
                patch.object(ground_station, "available_ports", return_value=[]):
            self.manager.ingest("wifi", {**self.row(1), "sd_ok": True,
                                         "barometer_ready": True, "imu_test_result": "PASS",
                                         "imu_test_samples": 1400,
                                         "imu_test_duration_ms": 30000,
                                         "imu_test_peak_deviation_g": 0.012})
            checks = {item["name"]: item for item in ground_station.preflight_report()["checks"]}
            self.assertEqual("PASS", checks["SD initialization"]["status"])
            self.assertIn("1400 valid samples in a 30 s test", checks["LSM6DSO32"]["message"])
            self.assertIsNone(checks["SD initialization"]["correction"])
            self.assertEqual("NOT TESTED", checks["SD write/read/cleanup"]["status"])
            self.assertIn("temporary-file cleanup", checks["SD write/read/cleanup"]["correction"])
            self.assertEqual("NOT TESTED", checks["Uno USB port"]["status"])
            self.assertIn("COM port", checks["Uno USB port"]["correction"])
            self.assertEqual("NOT TESTED", checks["LoRa packets"]["status"])
            with patch.object(ground_station, "available_ports", return_value=[
                    {"device": "COM4", "description": "USB Serial", "hwid": "generic"}]):
                generic = {item["name"]: item for item in ground_station.preflight_report()["checks"]}
                self.assertEqual("WARNING", generic["Uno USB port"]["status"])
            self.clock.advance(3)
            checks = {item["name"]: item for item in ground_station.preflight_report()["checks"]}
            self.assertNotEqual("PASS", checks["SD initialization"]["status"])
            self.assertNotEqual("PASS", checks["LSM6DSO32"]["status"])
            actions = {"sd_test": {"http_status": 200, "success": True,
                                   "message": "temporary file verified and removed"}}
            checks = {item["name"]: item for item in ground_station.preflight_report(actions)["checks"]}
            self.assertEqual("PASS", checks["SD write/read/cleanup"]["status"])

    def test_preflight_sd_capacity_needs_fresh_idle_evidence(self):
        with patch.object(ground_station, "MANAGER", self.manager), \
                patch.object(ground_station, "available_ports", return_value=[]):
            self.manager.ingest("wifi", {**self.row(1), "sd_ok": True,
                                         "sd_capacity_known": True, "sd_free_bytes": 16 * 1024 * 1024,
                                         "sd_capacity_age_ms": 0, "recording": False})
            check = next(item for item in ground_station.preflight_report()["checks"]
                         if item["name"] == "SD free space")
            self.assertEqual("PASS", check["status"])
            self.manager.ingest("wifi", {**self.row(2), "sd_ok": True,
                                         "sd_capacity_known": True, "sd_free_bytes": 1024,
                                         "sd_capacity_age_ms": 0, "recording": False})
            check = next(item for item in ground_station.preflight_report()["checks"]
                         if item["name"] == "SD free space")
            self.assertEqual("WARNING", check["status"])
            self.manager.ingest("wifi", {**self.row(3), "sd_ok": True,
                                         "sd_capacity_known": True, "sd_free_bytes": 16 * 1024 * 1024,
                                         "sd_capacity_age_ms": 30000, "recording": False})
            check = next(item for item in ground_station.preflight_report()["checks"]
                         if item["name"] == "SD free space")
            self.assertEqual("NOT TESTED", check["status"])
            actions = {"sd_capacity": {"success": True, "total": 32 * 1024 * 1024,
                                       "used": 8 * 1024 * 1024}}
            check = next(item for item in ground_station.preflight_report(actions)["checks"]
                         if item["name"] == "SD free space")
            self.assertEqual("PASS", check["status"])
            actions["sd_capacity"]["used"] = 64 * 1024 * 1024
            check = next(item for item in ground_station.preflight_report(actions)["checks"]
                         if item["name"] == "SD free space")
            self.assertEqual("NOT TESTED", check["status"])

    def test_uno_boot_status_does_not_claim_lora_packets(self):
        self.manager.note_receiver_status({"type": "receiver_status", "ok": False,
                                           "firmware_version": "V12-development",
                                           "protocol": 2, "error": "radio_init"})
        with patch.object(ground_station, "MANAGER", self.manager), \
                patch.object(ground_station, "available_ports", return_value=[]):
            checks = {item["name"]: item for item in ground_station.preflight_report()["checks"]}
            self.assertEqual("PASS", checks["Uno USB port"]["status"])
            self.assertEqual("PASS", checks["Uno receiver protocol"]["status"])
            self.assertEqual("FAIL", checks["Ground LoRa module"]["status"])
            self.assertEqual("NOT TESTED", checks["LoRa packets"]["status"])
        self.manager.clear_receiver_status()
        self.assertIsNone(self.manager.snapshot()["receiver_status"])

    def test_simulation_does_not_pass_hardware_preflight(self):
        self.manager.ingest("simulation", {"sequence": 1, "uptime_ms": 100,
                                           "battery_v": 4.0, "gps_status": "live",
                                           "gps_fix_age_ms": 0, "latitude": 37.0,
                                           "longitude": -122.0, "sd_ok": True,
                                           "barometer_ready": True})
        with patch.object(ground_station, "MANAGER", self.manager), \
                patch.object(ground_station, "available_ports", return_value=[]):
            checks = {item["name"]: item for item in ground_station.preflight_report()["checks"]}
        for name in ("Battery", "GPS fix", "SD initialization", "BMP388"):
            self.assertNotEqual("PASS", checks[name]["status"])
        self.assertEqual("WARNING", checks["Telemetry source freshness"]["status"])

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
        self.assertEqual(6, len(re.findall(r'<button\b[^>]*\bdata-view=', html)))
        self.assertIn('id="preflight"', html)
        self.assertIn('id="analysis"', html)
        self.assertIn("const finite=v=>v!==null", html)
        self.assertNotIn("https://cdn", html)
        self.assertNotIn("SIMULATED DATA — NOT HARDWARE", html)


class FlightExportFilenameTests(unittest.TestCase):
    def test_download_helpers_stream_originals_and_close_derived_exports(self):
        class Output(BytesIO):
            largest_write = 0

            def write(self, data):
                self.largest_write = max(self.largest_write, len(data))
                return super().write(data)

        class Response:
            def __init__(self):
                self.headers = {}
                self.wfile = Output()
                self.close_connection = False

            def send_response(self, status):
                self.status = status

            def send_header(self, key, value):
                self.headers[key] = value

            def end_headers(self):
                pass

        with tempfile.TemporaryDirectory() as directory:
            original = Path(directory) / "original.bin"
            original.write_bytes(b"x" * 150000)
            response = Response()
            ground_station.file_response(response, original, "application/octet-stream", "original.bin")
            self.assertEqual(150000, int(response.headers["Content-Length"]))
            self.assertLessEqual(response.wfile.largest_write, 65536)
            self.assertEqual(original.read_bytes(), response.wfile.getvalue())
        response = Response()
        ground_station.stream_response(response, iter([b"first", b"second"]), "text/csv", "flight.csv")
        self.assertEqual(b"firstsecond", response.wfile.getvalue())
        self.assertEqual("close", response.headers["Connection"])
        self.assertTrue(response.close_connection)
        self.assertNotIn("Content-Length", response.headers)

    def test_lowercase_bin_name_gets_csv_and_json_extensions(self):
        class Request:
            path = "/api/flight/" + "a" * 64 + "/csv"

        metadata = {"original_name": "v12d007.bin"}
        with patch.object(ground_station.FLIGHTS, "get", return_value=(metadata, Path("unused"))) as get, \
                patch.object(ground_station.FLIGHTS, "iter_csv_export", return_value=iter([b"csv"])) as csv_export, \
                patch.object(ground_station.FLIGHTS, "iter_json_export", return_value=iter([b"{}"])) as json_export, \
                patch.object(ground_station, "stream_response") as response:
            ground_station.Handler.do_GET(Request())
            self.assertEqual("v12d007.csv", response.call_args.args[3])
            Request.path = "/api/flight/" + "a" * 64 + "/json"
            ground_station.Handler.do_GET(Request())
            self.assertEqual("v12d007.json", response.call_args.args[3])
            self.assertEqual(2, get.call_count)
            self.assertEqual((metadata, Path("unused")), csv_export.call_args.kwargs["_verified"])
            self.assertEqual((metadata, Path("unused")), json_export.call_args.kwargs["_verified"])

    def test_jsonl_import_route_rejects_oversize_before_reading(self):
        raw = b'{"received_utc":"2026-11-06T01:30:00Z","source":"wifi","data":{}}\n'

        class Request:
            path = "/api/import-session"
            headers = {"Content-Length": str(len(raw)), "X-Green-Ion-Filename": "session.jsonl"}
            rfile = BytesIO(raw)

        with patch.object(ground_station.SESSIONS, "import_stream", return_value={"sha256": "a" * 64}) as imported, \
                patch.object(ground_station, "json_response") as response:
            ground_station.Handler.do_POST(Request())
            self.assertEqual("session.jsonl", imported.call_args.args[0])
            self.assertEqual(len(raw), imported.call_args.kwargs["expected_bytes"])
            self.assertTrue(response.call_args.args[1]["success"])
            Request.headers["Content-Length"] = str(32 * 1024 * 1024 + 1)
            ground_station.Handler.do_POST(Request())
            self.assertEqual(1, imported.call_count)
            self.assertEqual(400, response.call_args.args[2])

    def test_backup_restore_route_rejects_oversize_before_touching_library(self):
        class Request:
            path = "/api/library/restore"
            headers = {"Content-Length": "4"}
            rfile = BytesIO(b"PK!!")

        with patch.object(ground_station, "restore_backup", return_value={"files_added": 1}) as restore, \
                patch.object(ground_station, "json_response") as response:
            ground_station.Handler.do_POST(Request())
            self.assertEqual(4, restore.call_args.kwargs["expected_bytes"])
            self.assertTrue(response.call_args.args[1]["success"])
            Request.headers["Content-Length"] = str(512 * 1024 * 1024 + 1)
            ground_station.Handler.do_POST(Request())
            self.assertEqual(1, restore.call_count)
            self.assertEqual(400, response.call_args.args[2])

    def test_flight_trash_route_requires_phrase_and_idle_recorder(self):
        raw = b'{"confirm":"TRASH LAPTOP COPY"}'

        class Request:
            path = "/api/flight/" + "a" * 64 + "/trash"
            headers = {"Content-Type": "application/json", "Content-Length": str(len(raw))}
            rfile = BytesIO(raw)

        with patch.object(ground_station.FLIGHTS, "trash_flight", return_value={"id": "a" * 64}) as moved, \
                patch.object(ground_station.MANAGER, "snapshot", return_value={"recording": True}) as state, \
                patch.object(ground_station, "json_response") as reply:
            ground_station.Handler.do_POST(Request())
            self.assertEqual(409, reply.call_args.args[2])
            moved.assert_not_called()
            state.return_value = {"recording": False}
            Request.rfile = BytesIO(b'{}')
            Request.headers["Content-Length"] = "2"
            ground_station.Handler.do_POST(Request())
            self.assertEqual(400, reply.call_args.args[2])
            moved.assert_not_called()
            Request.rfile = BytesIO(raw)
            Request.headers["Content-Length"] = str(len(raw))
            ground_station.Handler.do_POST(Request())
            moved.assert_called_once_with("a" * 64)
            self.assertTrue(reply.call_args.args[1]["success"])

    def test_session_trash_route_refuses_current_writable_session(self):
        active = ground_station.MANAGER.writer.raw_path.name
        raw = b'{"confirm":"TRASH LAPTOP SESSION"}'

        class Request:
            path = "/api/session/" + active + "/trash"
            headers = {"Content-Type": "application/json", "Content-Length": str(len(raw))}
            rfile = BytesIO(raw)

        with patch.object(ground_station.SESSIONS, "trash_session",
                          side_effect=ground_station.SessionImportError("current writable laptop session")) as moved, \
                patch.object(ground_station, "json_response") as reply:
            ground_station.Handler.do_POST(Request())
            moved.assert_called_once_with(active, active)
            self.assertEqual(400, reply.call_args.args[2])
            Request.rfile = BytesIO(b'{}')
            Request.headers["Content-Length"] = "2"
            ground_station.Handler.do_POST(Request())
            self.assertEqual(1, moved.call_count)


if __name__ == "__main__":
    unittest.main()
