from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]


class V12StructureTests(unittest.TestCase):
    def test_required_projects_and_docs_exist(self):
        paths = (
            "Green_Ion_V12/Green_Ion_V12.ino", "Green_Ion_V12/Config.h",
            "Green_Ion_V12/Dashboard.h",
            "Ground_Receiver/Ground_Receiver.ino", "Ground_Station/ground_station.py",
            "Ground_Station/index.html", "docs/V12_BUILD_GUIDE.md",
            "docs/V12_WIRING.md", "docs/V12_TESTING.md",
            "docs/V12_DEVELOPMENT_LOG.md", "docs/V12_ROADMAP.md",
            "docs/V12_HARDWARE_BRINGUP.md",
        )
        for path in paths:
            self.assertTrue((ROOT / path).is_file(), path)

    def test_pin_assignments_are_centralized_and_safe(self):
        config = (ROOT / "Green_Ion_V12" / "Config.h").read_text(encoding="utf-8")
        self.assertIn("PIN_SD_CS = 33", config)
        self.assertIn("PIN_LORA_CS = 27", config)
        self.assertIn("PIN_I2C_POWER = 2", config)
        self.assertIn("PIN_SD_CS != PIN_LORA_CS", config)

    def test_both_radios_use_the_shared_configuration(self):
        transmitter = (ROOT / "Green_Ion_V12" / "Config.h").read_text(encoding="utf-8")
        receiver = (ROOT / "Ground_Receiver" / "Ground_Receiver.ino").read_text(encoding="utf-8")
        for name in ("kFrequencyMhz", "kSignalBandwidthHz", "kSpreadingFactor", "kCodingRateDenominator"):
            self.assertIn(name, transmitter)
            self.assertIn(name, receiver)

    def test_no_energetic_device_control_surface(self):
        source = "\n".join(
            path.read_text(encoding="utf-8", errors="ignore")
            for folder in (ROOT / "Green_Ion_V12", ROOT / "Ground_Receiver")
            for path in folder.rglob("*") if path.is_file()
        ).lower()
        for forbidden in ("fire_igniter", "/api/ignition", "deploy_parachute", "stage_separation"):
            self.assertNotIn(forbidden, source)


if __name__ == "__main__":
    unittest.main()

