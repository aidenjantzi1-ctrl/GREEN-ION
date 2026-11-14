"""Guided home test contracts: firmware isolation, real SD evidence and dashboard behavior."""
from pathlib import Path
import re
import subprocess
import unittest

ROOT = Path(__file__).resolve().parents[1]
FW = (ROOT / "Past_Versions/V12/Green_Ion_V12" / "Green_Ion_V12.ino").read_text()
HOME = (ROOT / "Past_Versions/V12/Green_Ion_V12" / "HomeVerification.h").read_text()
UI = (ROOT / "Past_Versions/V12/Green_Ion_V12" / "Dashboard.h").read_text()
OTA = (ROOT / "Past_Versions/V12/Green_Ion_V12" / "FirmwareMaintenance.h").read_text()


class GuidedHomeVerification(unittest.TestCase):
    def test_reserved_file_does_not_touch_real_flights(self):
        self.assertIn('HOME_TEST_PATH[] = "/GIHOME.BIN"', HOME)
        self.assertIn("SD.remove(HOME_TEST_PATH)", HOME)
        self.assertNotIn('"/V12D', HOME)
        self.assertIn("green_ion::rkt4::encodeHeader", HOME)
        self.assertIn("green_ion::rkt4::decodeHeader", HOME)
        self.assertIn("green_ion::rkt4::encodeRecord", HOME)
        self.assertIn("green_ion::rkt4::decodeRecord", HOME)
        self.assertIn("HOME_TEST_DURATION_MS = 30000UL", HOME)
        self.assertIn("homeSamples >= 1400", HOME)

    def test_no_flight_recorder_or_ota_during_home_test(self):
        for token in ("RECORDER_HOME_TEST", "homeCapture(nowUs, barometerReadingValid)",
                      '#include "HomeVerification.h"', "homeLogActive"):
            self.assertIn(token, FW)
        self.assertIn("flightWatcher.cancel()", HOME)
        self.assertIn("resetCalibration(millis())", HOME)
        self.assertIn("recorderMode == RECORDER_HOME_TEST", OTA)
        self.assertIn('dashboardServer.on("/api/home/finish", HTTP_POST, homeFinish)', FW)
        self.assertIn("homeWifiAbsent", FW + HOME)
        self.assertIn("homeCleanupOk", FW + HOME)

    def test_guided_app_and_manual_receiver_ack(self):
        for token in ('id="homeVerificationCard"', "homeStepTitle", "homeConfirm",
                      "home_record_wifi_absent", "home_record_missed", "home_radio_sent",
                      "uno_confirmed:true", "NOT VERIFIED", "homeReport",
                      "flight_ready:false"):
            self.assertIn(token, UI)

    def test_home_script_javascript_parses(self):
        scripts = re.findall(r"<script[^>]*>(.*?)</script>", UI, re.S)
        guided = [s for s in scripts if "GREEN ION V12 guided home verification" in s]
        self.assertEqual(len(guided), 1)
        result = subprocess.run(["node", "--check", "-"], input=guided[0], text=True,
                                capture_output=True)
        self.assertEqual(result.returncode, 0, result.stderr)


if __name__ == "__main__":
    unittest.main()
