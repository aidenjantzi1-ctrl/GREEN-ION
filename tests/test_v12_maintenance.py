"""Static safety contracts for V12 phone OTA and maintenance UI.

These checks do not substitute for ESP32 target compilation or power-loss tests.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
FW = (ROOT / "Green_Ion_V12" / "Green_Ion_V12.ino").read_text()
OTA = (ROOT / "Green_Ion_V12" / "Maintenance.h").read_text()
UI = (ROOT / "Green_Ion_V12" / "Dashboard.h").read_text()


class V12MaintenanceContractTests(unittest.TestCase):
    def test_ota_backend_is_connected(self):
        for token in (
            '#include <Update.h>',
            '#include <esp_ota_ops.h>',
            '#include "Maintenance.h"',
            '"/api/update"',
            "finishV12FirmwareUpload",
            "handleV12FirmwareUpload",
            '"/api/rollback"',
            "handleV12Rollback",
        ):
            self.assertIn(token, FW)

    def test_ota_never_overrides_active_recorder(self):
        for token in (
            "!state.recording",
            "!recorderCore.recording()",
            "recorderMode != RECORDER_RECORDING",
            "!imuHardwareTest.running()",
            "!gpsHardwareTest.running()",
            "state.batteryMv >= OTA_MIN_BATTERY_MV",
            "otaMaintenanceHold = true",
            "Update.abort()",
        ):
            self.assertIn(token, OTA)
        loop = FW[FW.index("void loop() {"):]
        self.assertLess(loop.index("if (otaMaintenanceHold)"),
                        loop.index("sampleClock.due(nowUs)"))

    def test_update_validates_image_and_defers_restart(self):
        for token in (
            'name.endsWith(".ino.bin")',
            "upload.buf[0] != 0xE9",
            "Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)",
            "Update.write(upload.buf, upload.currentSize)",
            "otaUploadBytes < 200000",
            "Update.end(true)",
            "otaRebootAtMs = millis() + 1200",
        ):
            self.assertIn(token, OTA)
        self.assertIn("otaRebootPending", FW)

    def test_rollback_requires_valid_other_slot_and_control(self):
        for token in (
            "requireMaintenanceController()",
            "maintenanceSafeToBegin()",
            "esp_ota_get_partition_description",
            "esp_ota_set_boot_partition(previous)",
            "maintenanceControllerToken",
        ):
            self.assertIn(token, OTA)
        self.assertIn('"/api/control"', FW)
        self.assertIn('"/api/release"', FW)
        self.assertIn('"maintenance_controller"', FW.replace('\\', ''))

    def test_removed_simulation_controls_and_enabled_maintenance_ui(self):
        self.assertNotIn('id="simulate"', UI)
        self.assertNotIn('id="simstop"', UI)
        self.assertNotIn('$("simulate").onclick', UI)
        self.assertIn("async function v12TakeControl()", UI)
        self.assertIn("async function v12ReleaseControl()", UI)
        self.assertIn("maintenanceReady", UI)
        self.assertIn("$('installFirmware').onclick=installFirmware", UI)
        self.assertIn("$('rollbackFirmware').onclick=rollbackFirmware", UI)

    def test_console_is_readable_and_raw_diagnostics_remain_opt_in(self):
        self.assertIn("BenchMode benchMode = BENCH_OFF", FW)
        self.assertIn("printConsoleStatus()", FW)
        self.assertIn('!strcmp(serialCommand, "status")', FW)
        self.assertIn("BENCH IMU status=", FW)
        self.assertIn("BENCH GPS status=", FW)
        self.assertIn("[STATUS]", FW)
        self.assertIn("recorderMode != RECORDER_RECORDING", FW)


if __name__ == "__main__":
    unittest.main()
