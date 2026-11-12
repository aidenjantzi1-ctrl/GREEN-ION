"""Regression tests for Windows bridge responsiveness and command safety."""
import sys
import unittest
from pathlib import Path
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "Ground_Station"))
from v13_bridge import (
    CONTROL_FRESH_S, DELAYED_S, DISCONNECTED_S, STATUS_LOCK_WAIT_S,
    STATUS_TIMEOUT_S, TELEMETRY_FRESH_S, FeatherBridge
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

    def test_cached_status_after_expiration_is_retained_only_as_stale(self):
        with patch.object(self.bridge, "_fetch_status", return_value=self.status):
            self.bridge.poll_once()
        self.assertEqual(self.bridge.unified_status({"link_status": "missing"})["v13_source"], "wifi")
        self.now += TELEMETRY_FRESH_S + 0.1
        cached = self.bridge.unified_status({"link_status": "missing"})
        self.assertEqual(cached["v13_source"], "wifi_stale")
        self.assertFalse(cached["v13_wifi_connected"])
        self.assertTrue(cached["v16_status_stale"])

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


    def test_transient_failure_is_delayed_and_keeps_last_known_status_stale(self):
        with patch.object(self.bridge, "_fetch_status", return_value=self.status):
            self.assertTrue(self.bridge.poll_once())
        self.now += 1.0
        with patch.object(self.bridge, "_fetch_status", side_effect=ConnectionError("short dropout")):
            self.assertFalse(self.bridge.poll_once())
        link = self.bridge.link_status({"link_status": "missing"})
        self.assertEqual(link["wifi"]["connection_state"], "delayed")
        self.assertEqual(link["wifi"]["status"], "stale")
        cached = self.bridge.unified_status({"link_status": "missing"})
        self.assertEqual(cached["v13_source"], "wifi_stale")
        self.assertFalse(cached["v13_wifi_connected"])
        self.assertTrue(cached["v16_status_stale"])
        self.assertEqual(cached["pressure_pa"], self.status["pressure_pa"])

    def test_prolonged_loss_progresses_to_reconnecting_then_disconnected(self):
        with patch.object(self.bridge, "_fetch_status", return_value=self.status):
            self.bridge.poll_once()
        self.now += DELAYED_S + 0.1
        with patch.object(self.bridge, "_fetch_status", side_effect=ConnectionError("out of range")):
            self.bridge.poll_once()
        self.assertEqual(
            self.bridge.link_status({"link_status": "missing"})["wifi"]["connection_state"],
            "reconnecting",
        )
        self.now += DISCONNECTED_S
        self.assertEqual(
            self.bridge.link_status({"link_status": "missing"})["wifi"]["connection_state"],
            "disconnected",
        )

    def test_reconnect_clears_failure_counter_and_returns_connected(self):
        with patch.object(self.bridge, "_fetch_status", return_value=self.status):
            self.bridge.poll_once()
        with patch.object(self.bridge, "_fetch_status", side_effect=ConnectionError("drop")):
            self.bridge.poll_once()
        self.assertEqual(
            self.bridge.link_status({"link_status": "missing"})["wifi"]["consecutive_failures"], 1
        )
        self.now += 0.5
        with patch.object(self.bridge, "_fetch_status", return_value={**self.status, "uptime_s": 1}):
            self.assertTrue(self.bridge.poll_once())
        link = self.bridge.link_status({"link_status": "missing"})
        self.assertEqual(link["wifi"]["connection_state"], "connected")
        self.assertEqual(link["wifi"]["consecutive_failures"], 0)
        self.assertTrue(link["wifi"]["controls_verified"])

    def test_busy_file_transfer_defers_poll_without_false_failure(self):
        with patch.object(self.bridge, "_fetch_status", return_value=self.status):
            self.bridge.poll_once()
        self.bridge._upstream_lock.acquire()
        try:
            result = self.bridge.poll_once()
        finally:
            self.bridge._upstream_lock.release()
        self.assertIsNone(result)
        link = self.bridge.link_status({"link_status": "missing"})
        self.assertEqual(link["wifi"]["connection_state"], "connected")
        self.assertEqual(link["wifi"]["consecutive_failures"], 0)
        self.assertEqual(link["wifi"]["error"], "")
        self.assertLessEqual(STATUS_LOCK_WAIT_S, 0.1)

    def test_simulated_hour_of_polling_stays_bounded_and_connected(self):
        with patch.object(self.bridge, "_fetch_status", return_value=self.status):
            for _ in range(int(3600 / 1.5)):
                self.assertTrue(self.bridge.poll_once())
                self.now += 1.5
        link = self.bridge.link_status({"link_status": "missing"})
        self.assertEqual(link["wifi"]["connection_state"], "connected")
        self.assertEqual(link["wifi"]["consecutive_failures"], 0)
        self.assertEqual(self.bridge._status, self.status)


if __name__ == "__main__":
    unittest.main()
