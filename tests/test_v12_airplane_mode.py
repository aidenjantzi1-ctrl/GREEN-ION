"""Airplane mode source and offline-export safety contracts."""

from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
FIRMWARE = (ROOT / "Green_Ion_V12" / "Green_Ion_V12.ino").read_text(encoding="utf-8")
DASHBOARD = (ROOT / "Green_Ion_V12" / "Dashboard.h").read_text(encoding="utf-8")
CONFIG = (ROOT / "Green_Ion_V12" / "Config.h").read_text(encoding="utf-8")


class AirplaneModeContracts(unittest.TestCase):
    def test_airplane_and_rocket_modes_are_separate(self):
        self.assertIn('openRawLog(nowMs, nowUs, "AIRP")', FIRMWARE)
        self.assertIn('const char *prefix = "V12D"', FIRMWARE)
        self.assertIn('"AIRP"', FIRMWARE)
        self.assertIn('data-tab="airplane"', DASHBOARD)
        self.assertIn("Cabin-pressure-equivalent", DASHBOARD)

    def test_radio_silence_is_persistent_and_blocks_all_transmit_paths(self):
        for token in (
            'putBool("airSilent", enabled)',
            'getBool("airSilent", false)',
            "WiFi.softAPdisconnect(true)",
            "WiFi.mode(WIFI_OFF)",
            "btStop()",
            "digitalWrite(config::PIN_LORA_RST, LOW)",
            "if (airplaneMode.radioSilenceLatched()) return;",
        ):
            self.assertIn(token, FIRMWARE)
        self.assertGreaterEqual(FIRMWARE.count("if (airplaneMode.radioSilenceLatched()) return;"), 3)

    def test_offline_serial_controls_and_bounded_duration(self):
        for command in (
            "airplane prepare", "airplane start", "airplane status",
            "airplane stop", "airplane unlock",
        ):
            self.assertIn(command, FIRMWARE)
            self.assertIn(command, DASHBOARD)
        self.assertIn("AIRPLANE_MAX_RECORDING_MS", CONFIG)
        self.assertIn("automaticStopDue", FIRMWARE)

    def test_recorder_and_download_guards_remain_active(self):
        self.assertIn("Recording is active; downloads are deferred", FIRMWARE)
        self.assertIn("recorderCore.flush()", FIRMWARE)
        self.assertIn("saveCheckpoint(nowMs)", FIRMWARE)
        self.assertIn("recoverInterruptedLog", FIRMWARE)
        self.assertIn("airplaneJournalSlots", FIRMWARE)


if __name__ == "__main__":
    unittest.main()
