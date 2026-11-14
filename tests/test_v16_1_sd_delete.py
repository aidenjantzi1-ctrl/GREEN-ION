from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
FW = (ROOT / "Past_Versions/V14/Green_Ion_V14" / "Green_Ion_V14.ino").read_text(encoding="utf-8")
DASH = (ROOT / "Past_Versions/V14/Green_Ion_V14" / "Dashboard.h").read_text(encoding="utf-8")


class V161SdDeleteTests(unittest.TestCase):
    def test_delete_endpoint_is_narrow_and_explicit(self):
        self.assertIn('dashboardServer.on("/api/delete", HTTP_POST, deleteDashboardFlight);', FW)
        self.assertIn('dashboardServer.on("/api/flightstats", []() { sendDashboardDownload(true); });', FW)
        self.assertIn('dashboardFlightFilename(name, ".BIN")', FW)
        self.assertIn('confirmation != name', FW)
        self.assertIn('Only V12D###.BIN flight packages can be deleted', FW)
        self.assertNotIn('dashboardServer.arg("path")', FW)

    def test_delete_is_blocked_during_critical_sd_operations(self):
        block = FW[FW.index("static bool dashboardSdDeleteBlocked()"):
                   FW.index("static void deleteDashboardFlight()")]
        for token in (
            "state.recording",
            "recorderCore.recording()",
            "logFile",
            "sensorLogFile",
            "RECORDER_RECORDING",
            "RECORDER_HOME_TEST",
            "RECORDER_MAINTENANCE",
            "homeLogActive",
            "otaUploadInProgress",
        ):
            self.assertIn(token, block)
        self.assertGreaterEqual(FW.count("dashboardSdDeleteBlocked()"), 3)

    def test_delete_removes_package_companions_and_bin_last(self):
        start = FW.index("static void deleteDashboardFlight()")
        end = FW.index("static void sendDashboardFiles()", start)
        delete = FW[start:end]
        for ext in (".SEN", ".TXT", ".JRA", ".JRB", ".RCV"):
            self.assertIn(f'"{ext}"', delete)
        self.assertLess(delete.index("for (const char *extension : companions)"),
                        delete.index("SD.remove(binPath)"))
        self.assertIn("retain the BIN for recovery", delete)
        self.assertIn("verified = !SD.exists(binPath)", delete)
        self.assertIn("deleted_files", delete)

    def test_delete_pauses_watcher_and_recalibrates(self):
        start = FW.index("static void deleteDashboardFlight()")
        end = FW.index("static void sendDashboardFiles()", start)
        delete = FW[start:end]
        self.assertIn("RECORDER_WATCHING", delete)
        self.assertIn("flightWatcher.cancel()", delete)
        self.assertIn("resetCalibration(millis())", delete)
        self.assertIn("stationary calibration restarted", delete)

    def test_normal_sd_browse_and_download_are_hardened(self):
        files = FW[FW.index("static void sendDashboardFiles()"):
                   FW.index("static void sendDashboardDownload")]
        download = FW[FW.index("static void sendDashboardDownload"):
                      FW.index("static void runDashboardSdTest")]
        self.assertIn("dashboardSdDeleteBlocked()", files)
        self.assertIn("dashboardSdDeleteBlocked()", download)

    def test_embedded_dashboard_exposes_only_working_sd_delete_controls(self):
        self.assertIn('rm.textContent="Delete from SD"', DASH)
        self.assertIn('&confirm="+encodeURIComponent(f.name)', DASH)
        self.assertIn('id="sdDeleteFeedback"', DASH)
        self.assertNotIn('protect?name=', DASH)
        self.assertNotIn('note?name=', DASH)
        self.assertIn("Browser, Windows, and cloud backups are not deleted", DASH)


if __name__ == "__main__":
    unittest.main()
