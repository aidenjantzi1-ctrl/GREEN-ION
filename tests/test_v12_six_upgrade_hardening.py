"""V12 hardening regressions: stable ground CSV and fresh home evidence."""
from __future__ import annotations

import csv
import json
from pathlib import Path
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "Ground_Station"))
from ground_station import LOG_COLUMNS, write_log  # noqa: E402


class GroundCsvEvidenceTests(unittest.TestCase):
    def test_telemetry_and_bench_events_share_one_stable_header(self):
        telemetry = {
            "type": "telemetry", "sequence": 1, "altitude_m": 12.34,
            "received_utc": "2026-11-10T11:00:00+00:00",
            "firmware_future_field": {"custom": "preserved"},
        }
        bench = {
            "type": "radio_bench", "sequence": 2, "rssi_dbm": -76,
            "bench_received": 1, "source": "radio",
        }
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "test.csv"
            write_log(telemetry, path)
            write_log(bench, path)
            with path.open(newline="", encoding="utf-8") as handle:
                reader = csv.DictReader(handle)
                self.assertEqual(tuple(reader.fieldnames), LOG_COLUMNS)
                rows = list(reader)
            self.assertEqual(len(rows), 2)
            self.assertEqual(rows[0]["type"], "telemetry")
            self.assertEqual(rows[1]["type"], "radio_bench")
            self.assertEqual(rows[1]["rssi_dbm"], "-76")
            self.assertEqual(rows[1]["altitude_m"], "")
            self.assertEqual(json.loads(rows[0]["raw_packet_json"]), telemetry)
            self.assertEqual(json.loads(rows[1]["raw_packet_json"]), bench)

    def test_empty_existing_file_gets_header(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "empty.csv"
            path.touch()
            write_log({"type": "telemetry", "sequence": 5}, path)
            with path.open(newline="", encoding="utf-8") as handle:
                rows = list(csv.DictReader(handle))
            self.assertEqual(len(rows), 1)
            self.assertEqual(rows[0]["sequence"], "5")


class FreshHomeSessionTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.fw = (ROOT / "Green_Ion_V12/Green_Ion_V12.ino").read_text()
        cls.home = (ROOT / "Green_Ion_V12/HomeVerification.h").read_text()
        cls.station = (ROOT / "Ground_Station/ground_station.py").read_text()

    def test_imu_and_gps_results_must_be_fresh(self):
        for name in ("homeImuTestStarted", "homeGpsTestStarted"):
            self.assertIn("static bool " + name + " = false;", self.fw)
            self.assertIn(name + " = false;", self.home)
            self.assertIn("!" + name + " ||", self.home)
        self.assertIn("if (recorderMode == RECORDER_HOME_TEST) homeImuTestStarted = true;", self.fw)
        self.assertIn("if (recorderMode == RECORDER_HOME_TEST) homeGpsTestStarted = true;", self.fw)
        self.assertIn("imuHardwareTest.running()", self.home)
        self.assertIn("gpsHardwareTest.running()", self.home)

    def test_launch_watch_cancelled_before_nvs_and_sd_work(self):
        start = self.home.split("static void homeStart() {", 1)[1].split(
            "static void homeStartLog()", 1
        )[0]
        self.assertLess(start.index("flightWatcher.cancel()"),
                        start.index("SD.exists(HOME_TEST_PATH)"))
        self.assertLess(start.index("flightWatcher.cancel()"),
                        start.index('prefs.begin("giohome", false)'))
        self.assertIn("resetCalibration(millis())", start)

    def test_receiver_side_bench_packets_exported(self):
        source = self.station.split('elif kind == "radio_bench":', 1)[1].split(
            'elif kind == "telemetry":', 1
        )[0]
        self.assertIn("STORE.bench(row)", source)
        self.assertIn("write_log(row, log_path)", source)
        self.assertIn('row["received_utc"]', source)


if __name__ == "__main__":
    unittest.main()
