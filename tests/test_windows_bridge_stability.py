"""Regression tests for Windows bridge responsiveness and command safety."""
import sys
import unittest
from pathlib import Path
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "Ground_Station"))
from v13_bridge import (
    CONTROL_FRESH_S, STATUS_TIMEOUT_S, TELEMETRY_FRESH_S, FeatherBridge
)


class BridgeTests(unittest.TestCase):
    def setUp(self):
        self.now = 100.0
        self.bridge = FeatherBridge(clock=lambda: self.now)
        self.status = {"uptime_s": 12, "sd_ok": True, "pressure_pa": 101300}

    def test_slow_single_poll_does_not_immediately_report_offline(self):
        with patch.object(self.bridge, "_fetch_status", return_value=self.status):
            self.assertTrue(self.bridge.poll_once())
        self.now += STATUS_TIMEOUT_S + 0.2
        self.assertEqual(self.bridge._wifi()[0], "live")
        self.now += TELEMETRY_FRESH_S
        self.assertEqual(self.bridge._wifi()[0], "stale")

    def test_cached_status_is_not_returned_after_expiration(self):
        with patch.object(self.bridge, "_fetch_status", return_value=self.status):
            self.bridge.poll_once()
        self.assertEqual(self.bridge.unified_status({"link_status": "missing"})["v13_source"], "wifi")
        self.now += TELEMETRY_FRESH_S + 0.1
        self.assertIsNone(self.bridge.unified_status({"link_status": "missing"}))

    def test_controls_do_not_gain_extended_verification_window(self):
        with patch.object(self.bridge, "_fetch_status", return_value=self.status):
            self.bridge.poll_once()
        self.now += CONTROL_FRESH_S + 0.1
        self.assertEqual(self.bridge._wifi()[0], "live")
        class FakeHandler:
            def __init__(self):
                self.errors = []
            def send_error(self, code, message):
                self.errors.append((code, message))
        handler = FakeHandler()
        self.bridge.relay(handler)
        self.assertEqual(handler.errors[0][0], 503)
        self.assertIn("not currently verified", handler.errors[0][1])

    def test_status_timeout_is_longer_than_original(self):
        self.assertGreater(STATUS_TIMEOUT_S, 1.0)
        self.assertGreater(TELEMETRY_FRESH_S, STATUS_TIMEOUT_S)
        self.assertLess(CONTROL_FRESH_S, TELEMETRY_FRESH_S)


if __name__ == "__main__":
    unittest.main()
