"""Source-level contract checks for the restored V11 dashboard on V12."""

from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]
V12_DASHBOARD = (ROOT / "Past_Versions/V12/Green_Ion_V12" / "Dashboard.h").read_text(encoding="utf-8")
V11_DASHBOARD = (ROOT / "Past_Versions" / "Green_Ion_V11" / "Dashboard.h").read_text(encoding="utf-8")
V12_FIRMWARE = (ROOT / "Past_Versions/V12/Green_Ion_V12" / "Green_Ion_V12.ino").read_text(encoding="utf-8")


class V12DashboardRestoreTests(unittest.TestCase):
    def test_uses_v11_blue_visual_foundation_and_tabs(self):
        for token in (
            "--bg:#081420",
            "--card:#172b40",
            "--blue:#7bdbff",
            'data-tab="status"',
            'data-tab="flight"',
            'data-tab="testing"',
            'data-tab="storage"',
            'id="savedFlightGraph"',
            "@media(min-width:900px)",
        ):
            self.assertIn(token, V11_DASHBOARD)
            self.assertIn(token, V12_DASHBOARD)

    def test_v12_hardware_data_is_presented_in_restored_ui(self):
        for token in (
            "Test IMU",
            "Test GPS",
            "Test All Sensors",
            "acceleration_x_g",
            "rotation_x_dps",
            "gps_fix_age_ms",
            "gps_nmea_sentences",
            "imu_range_32g",
            "hardware_tests_allowed",
        ):
            self.assertIn(token, V12_DASHBOARD + V12_FIRMWARE)

    def test_real_storage_and_safe_recorder_endpoints_exist(self):
        for route in (
            '"/api/files"',
            '"/api/download"',
            '"/api/metadata"',
            '"/api/sdtest"',
            '"/api/calibrate"',
            '"/api/stop"',
        ):
            self.assertIn(route, V12_FIRMWARE)
        self.assertIn("if (state.recording)", V12_FIRMWARE)
        self.assertIn("finalizeRecording(millis())", V12_FIRMWARE)
        self.assertIn("Recording is active; SD browsing is deferred", V12_FIRMWARE)

    def test_unsupported_v11_controls_are_explicitly_disabled(self):
        self.assertIn("v12Unsupported", V12_DASHBOARD)
        self.assertIn("OTA and rollback work only in explicit bench maintenance mode", V12_DASHBOARD)
        self.assertIn("node.disabled=true", V12_DASHBOARD)

    def test_browser_analysis_and_offline_shell_are_retained(self):
        for token in (
            "decodeRkt4",
            "downloadRowsCsv",
            "exportFlightPDF",
            'href="/manifest.webmanifest"',
            "serviceWorker.register('/service-worker.js')",
        ):
            self.assertIn(token, V12_DASHBOARD)
        self.assertIn('"/service-worker.js"', V12_FIRMWARE)
        self.assertIn('"/manifest.webmanifest"', V12_FIRMWARE)


if __name__ == "__main__":
    unittest.main()
