"""V13 unified link, failover and installation regressions."""
from __future__ import annotations
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "Ground_Station"))
from v13_bridge import FeatherBridge  # noqa: E402


class FakeBridge(FeatherBridge):
    def __init__(self):
        self.now = 100.0
        self.result = {"uptime_s": 42, "sd_ok": True, "pressure_pa": 100100, "battery_v": 4.0}
        super().__init__(host="192.168.4.1", clock=lambda: self.now)

    def _fetch_status(self):
        if self.result is None:
            raise ConnectionError("Feather out of range")
        return dict(self.result)


def radio(**changes):
    values = {
        "type": "telemetry", "source": "radio", "link_status": "live",
        "packet_age_ms": 750, "altitude_m": 123.4, "pressure_pa": 99400,
        "battery_v": 3.9, "acceleration_g": 5.1,
        "rotation_x_dps": 3, "rotation_y_dps": 2, "rotation_z_dps": 1,
        "speed_mps": 45, "gps_status": "live", "latitude": 34.22, "longitude": -116.87,
        "last_position": (34.22, -116.87), "last_position_age_ms": 300,
        "satellites": 10, "flight_state": 3, "rssi_dbm": -75,
    }
    values.update(changes)
    return values


class UnifiedBridgeTests(unittest.TestCase):
    def test_wifi_wins_with_full_status_while_radio_also_receives(self):
        bridge = FakeBridge()
        self.assertTrue(bridge.poll_once())
        status = bridge.unified_status(radio())
        self.assertEqual(status["v13_source"], "wifi")
        self.assertTrue(status["sd_ok"])
        self.assertEqual(status["pressure_pa"], 100100)
        link = bridge.link_status(radio())
        self.assertEqual(link["wifi"]["status"], "live")
        self.assertEqual(link["lora"]["status"], "live")

    def test_flight_wifi_loss_automatically_falls_back_to_lora(self):
        bridge = FakeBridge()
        bridge.poll_once()
        bridge.now += 5
        bridge.result = None
        self.assertFalse(bridge.poll_once())
        status = bridge.unified_status(radio())
        self.assertEqual(status["v13_source"], "lora")
        self.assertFalse(status["v13_wifi_connected"])
        self.assertAlmostEqual(status["relative_altitude_m"], 123.4)
        self.assertIsNone(status["vertical_speed_mps"])  # GPS speed != vertical speed.
        self.assertFalse(status["ota_available"])
        self.assertFalse(status["hardware_tests_allowed"])
        self.assertEqual(status["recorder_mode"], -1)
        self.assertEqual(bridge.link_status(radio())["source"], "lora")

    def test_both_links_lost_do_not_fabricate_fresh_sensor_status(self):
        bridge = FakeBridge()
        bridge.poll_once()
        bridge.now += 10
        no_radio = radio(link_status="stale", packet_age_ms=9000)
        self.assertIsNone(bridge.unified_status(no_radio))
        self.assertEqual(bridge.link_status(no_radio)["source"], "stale")

    def test_radio_gps_stale_does_not_pretend_live_coordinates(self):
        bridge = FakeBridge()
        status = bridge.unified_status(radio(gps_status="stale", latitude=1.0, longitude=2.0))
        self.assertEqual(status["gps_status"], "stale")
        self.assertEqual((status["latitude"], status["longitude"]), (34.22, -116.87))
        status = bridge.unified_status(radio(gps_status="missing", last_position=None))
        self.assertEqual(status["gps_status"], "missing")
        self.assertIsNone(status["latitude"])

    def test_reconnected_wifi_returns_full_dashboard_data(self):
        bridge = FakeBridge()
        bridge.poll_once()
        bridge.now += 10
        self.assertEqual(bridge.unified_status(radio())["v13_source"], "lora")
        bridge.result = {"uptime_s": 70, "sd_ok": True, "pressure_pa": 100002}
        bridge.poll_once()
        self.assertEqual(bridge.unified_status(radio())["v13_source"], "wifi")

    def test_proxy_denies_firmware_or_commands_when_wifi_is_unverified(self):
        class Handler:
            path = "/api/update?token=secret"
            calls = []
            def send_error(self, code, message):
                self.calls.append((code, message))
        bridge = FakeBridge()
        request = Handler()
        bridge.relay(request)
        self.assertEqual(request.calls[0][0], 503)
        self.assertIn("not currently verified", request.calls[0][1])

    def test_demo_never_masquerades_as_hardware(self):
        bridge = FakeBridge()
        self.assertEqual(bridge.link_status(radio(source="simulation"))["source"], "demo")
        self.assertEqual(bridge.unified_status(radio(source="simulation"))["v13_source"], "demo")


class UiAndPackageTests(unittest.TestCase):
    def test_windows_root_is_real_feather_dashboard_with_unified_controls(self):
        page = (ROOT / "Ground_Station/index.html").read_text(encoding="utf-8")
        self.assertIn('id="homeVerificationCard"', page)
        self.assertIn('id="rocketPreset"', page)
        self.assertIn('id="fileList"', page)
        self.assertIn("/v13_unified.js", page)
        self.assertIn("LORA TRACKING", page)
        self.assertIn("V13 WI-FI ONLINE", page)
        js = (ROOT / "Ground_Station/v13_unified.js").read_text(encoding="utf-8")
        self.assertIn("v13LinkPanel", js)
        self.assertIn("v13-wifi-unavailable", js)
        self.assertIn('addEventListener("click"', js)

    def test_firmware_keeps_old_flight_file_format_for_sd_compatibility(self):
        sketch = (ROOT / "Green_Ion_V13/Green_Ion_V13.ino").read_text(encoding="utf-8")
        self.assertIn('"/V12D%03u.BIN"', sketch)
        self.assertIn('"Dashboard.h"', sketch)
        self.assertIn('GREEN ION V13', sketch)

    def test_download_workflow_and_launcher_exist(self):
        workflow = (ROOT / ".github/workflows/build-v13.yml").read_text(encoding="utf-8")
        self.assertIn("GREEN_ION_V13_COMPLETE.zip", workflow)
        self.assertIn("upload-artifact", workflow)
        script = (ROOT / "Ground_Station/OPEN_GROUND_STATION.cmd").read_text(encoding="utf-8")
        self.assertIn('cd /d "%~dp0"', script)
        self.assertIn("ground_station.py", script)


if __name__ == "__main__":
    unittest.main()
