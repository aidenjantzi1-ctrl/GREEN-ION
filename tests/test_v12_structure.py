from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]


class V12StructureTests(unittest.TestCase):
    def test_required_projects_and_docs_exist(self):
        paths = (
            "Green_Ion_V12/Green_Ion_V12.ino", "Green_Ion_V12/Config.h",
            "Green_Ion_V12/Dashboard.h",
            "Ground_Receiver/Ground_Receiver.ino", "Ground_Station/ground_station.py",
            "Ground_Station/index.html", "Ground_Station/OPEN_GROUND_STATION.cmd",
            "libraries/GreenIonRecorder/src/GreenIonRecorderCore.h",
            "libraries/GreenIonRecorder/src/GreenIonFlightWatcher.h",
            "libraries/GreenIonRecorder/src/GreenIonCalibration.h",
            "libraries/GreenIonRecorder/src/GreenIonCheckpoint.h",
            "tests/cpp/test_v12_recorder_core.cpp",
            "tests/cpp/test_v12_flight_watcher.cpp",
            "tests/cpp/test_v12_calibration.cpp",
            "tests/cpp/test_v12_checkpoint.cpp",
            "docs/V12_BUILD_GUIDE.md",
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

    def test_ground_station_opens_without_hardware_arguments(self):
        source = (ROOT / "Ground_Station" / "ground_station.py").read_text(encoding="utf-8")
        self.assertIn('use_sample = args.sample or not args.port', source)
        self.assertIn('webbrowser.open', source)
        self.assertNotIn('add_mutually_exclusive_group(required=True)', source)

    def test_firmware_uses_bounded_recorder_core(self):
        config = (ROOT / "Green_Ion_V12" / "Config.h").read_text(encoding="utf-8")
        firmware = (ROOT / "Green_Ion_V12" / "Green_Ion_V12.ino").read_text(encoding="utf-8")
        self.assertIn("PREBUFFER_SECONDS = 3", config)
        self.assertIn("PREBUFFER_RECORDS == 150", config)
        self.assertIn("RecorderCore<config::PREBUFFER_RECORDS>", firmware)
        self.assertIn("class SdFileSink", firmware)
        self.assertIn("recorderCore.observe(record)", firmware)
        self.assertIn("RECORDER_CALIBRATING", firmware)
        self.assertIn("RECORDER_WATCHING", firmware)
        self.assertIn("processCalibration(nowMs, valid)", firmware)
        self.assertIn("updateLaunchWatcher(nowMs, valid)", firmware)
        self.assertIn("saveCheckpoint(nowMs)", firmware)
        self.assertIn("CHECKPOINT_INTERVAL_MS = 5000", config)
        self.assertIn('replace(".BIN", ".JRA")', firmware)
        self.assertIn('replace(".BIN", ".JRB")', firmware)
        self.assertIn("else if (nowMs - lastCheckpointMs", firmware)
        self.assertIn("recoverInterruptedLogs();", firmware)
        self.assertIn("selectNewestCheckpoint(", firmware)
        self.assertIn("commitRecovery(", firmware)
        self.assertIn("class SdRecoveryCommitter", firmware)
        self.assertIn('reportPath.replace(".BIN", ".RCV")', firmware)
        self.assertIn("trailing_partial_bytes", firmware)
        dashboard = (ROOT / "Green_Ion_V12" / "Dashboard.h").read_text(encoding="utf-8")
        self.assertIn('id="recovery"', dashboard)
        self.assertIn("d.recovery_status", dashboard)
        self.assertIn("SD.open(binPath, FILE_READ)", firmware)
        self.assertNotIn("SD.remove(binPath)", firmware)
        self.assertNotIn("log.truncate", firmware)
        self.assertNotIn("state.sdOk && !recorderCore.recording()) openRawLog()", firmware)

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

