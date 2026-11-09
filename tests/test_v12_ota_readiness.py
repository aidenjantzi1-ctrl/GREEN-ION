"""V12 OTA, serial, dashboard and documentation integration contracts."""
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
FIRMWARE = (ROOT / "Green_Ion_V12" / "Green_Ion_V12.ino").read_text(encoding="utf-8")
MAINTENANCE = (ROOT / "Green_Ion_V12" / "FirmwareMaintenance.h").read_text(encoding="utf-8")
DASHBOARD = (ROOT / "Green_Ion_V12" / "Dashboard.h").read_text(encoding="utf-8")
RECEIVER = (ROOT / "Ground_Receiver" / "Ground_Receiver.ino").read_text(encoding="utf-8")
LAUNCHER = (ROOT / "Ground_Station" / "OPEN_GROUND_STATION.cmd").read_text(encoding="utf-8")
README = (ROOT / "README.md").read_text(encoding="utf-8")


class V12OtaReadinessTests(unittest.TestCase):
    def test_ota_routes_and_update_validation(self):
        for route in ("/api/maintenance", "/api/maintenance/exit", "/api/update", "/api/rollback"):
            self.assertIn(route, FIRMWARE)
        for token in (
            "otaMaintenanceReady()", "otaTokenValid()", "otaTargetPartition()",
            "Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)",
            "upload.buf[0] != 0xE9",
            "Update.write(upload.buf, upload.currentSize)",
            "Update.end(true)", "Update.abort()",
            "esp_ota_set_boot_partition(other)",
            "esp_ota_get_partition_description(other, &description)",
            "otaUploadedBytes > target->size - upload.currentSize",
        ):
            self.assertIn(token, MAINTENANCE)

    def test_flight_lockouts_and_explicit_rearming(self):
        self.assertIn("RECORDER_MAINTENANCE", FIRMWARE)
        self.assertIn("flightWatcher.cancel()", MAINTENANCE)
        self.assertIn("recorderMode = RECORDER_MAINTENANCE", MAINTENANCE)
        self.assertIn("resetCalibration(millis())", MAINTENANCE)
        self.assertIn("state.recording || recorderCore.recording()", MAINTENANCE)
        self.assertIn("state.batteryMv >= 3600", MAINTENANCE)
        self.assertIn("recorderMode == RECORDER_MAINTENANCE) return;", FIRMWARE)
        self.assertIn("hardwareTestsAllowed()", FIRMWARE)

    def test_app_displays_real_v12_state_and_has_no_simulation_buttons(self):
        self.assertNotIn("START SIMULATION", DASHBOARD)
        self.assertNotIn("STOP SIMULATION", DASHBOARD)
        self.assertNotIn("airplane", DASHBOARD.lower())
        self.assertIn("v12InstallFirmware", DASHBOARD)
        self.assertIn("v12RollbackFirmware", DASHBOARD)
        self.assertIn("v12ExitMaintenance", DASHBOARD)
        self.assertIn("sample_rate_hz", FIRMWARE)
        self.assertIn("d.sample_rate_hz||50", DASHBOARD)
        self.assertIn("RECORDER_MAINTENANCE", FIRMWARE)
        self.assertIn('"/api/sensors"', FIRMWARE)
        self.assertIn("sen.textContent='SEN'", DASHBOARD)

    def test_uno_retains_machine_protocol_and_human_console(self):
        self.assertIn("human on", RECEIVER)
        self.assertIn("human off", RECEIVER)
        self.assertIn("estimated_missing_packets", RECEIVER)
        self.assertIn('F("{\\"type\\":\\"telemetry\\",\\"protocol\\":")', RECEIVER)
        self.assertIn("handleSerialCommands();", RECEIVER)
        self.assertIn("UNO_PORT", LAUNCHER)
        self.assertIn('--port "%UNO_PORT%"', LAUNCHER)

    def test_complete_readme_and_hardware_limitations(self):
        for token in (
            "SOLDERING AND WIRING", "BMP388", "LSM6DSO32", "PA1010D",
            "D33", "D27", "D15", "D32", "D14", "D13", "D11", "D12",
            "OPEN_GROUND_STATION.cmd", "COM5", "OTA firmware update",
            "ROLL BACK", "hardware validation pending",
        ):
            self.assertIn(token.lower(), README.lower())

    @unittest.skipUnless(shutil.which("node"), "Node.js not installed")
    def test_dashboard_embedded_javascript_syntax(self):
        scripts = re.findall(r"<script[^>]*>(.*?)</script>", DASHBOARD, re.DOTALL)
        self.assertGreaterEqual(len(scripts), 2)
        with tempfile.TemporaryDirectory() as folder:
            for i, script in enumerate(scripts):
                path = Path(folder) / f"dashboard-{i}.js"
                path.write_text(script, encoding="utf-8")
                result = subprocess.run(
                    ["node", "--check", str(path)], capture_output=True,
                    text=True, timeout=20, check=False,
                )
                self.assertEqual(0, result.returncode, f"Script {i}: {result.stderr}")


if __name__ == "__main__":
    unittest.main()
