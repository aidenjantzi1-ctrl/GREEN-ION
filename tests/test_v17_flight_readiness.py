import pathlib
import unittest

ROOT=pathlib.Path(__file__).resolve().parents[1]
FW=(ROOT/"Green_Ion_V16_3"/"Green_Ion_V16_3.ino").read_text(encoding="utf-8")
PREP=(ROOT/"Green_Ion_V16_3"/"FlightPreparation.h").read_text(encoding="utf-8")
DASH=(ROOT/"Green_Ion_V16_3"/"Dashboard.h").read_text(encoding="utf-8")
UNO=(ROOT/"Ground_Receiver_V16_3"/"Ground_Receiver_V16_3.ino").read_text(encoding="utf-8")
CFG=(ROOT/"Green_Ion_V16_3"/"Config.h").read_text(encoding="utf-8")

class V17FlightReadiness(unittest.TestCase):
    def test_sd_is_50hz_authoritative_and_rkt4_unchanged(self):
        self.assertIn("SENSOR_RATE_HZ = 50", CFG)
        self.assertIn("RecorderCore<config::PREBUFFER_RECORDS>", FW)
        self.assertIn("green_ion::rkt4::Header", FW)
        self.assertIn("LOG_FLUSH_INTERVAL_MS = 500", CFG)
        self.assertIn("CHECKPOINT_INTERVAL_MS = 5000", CFG)
        self.assertIn("recoverInterruptedLogs();", FW)

    def test_lora_absence_cannot_gate_launch_or_primary_recording(self):
        start=FW.index("static void appendLog")
        end=FW.index("static uint16_t telemetryFlags",start)
        block=FW[start:end]
        self.assertIn("updateLaunchWatcher(nowMs, valid)", block)
        self.assertIn("state.sdOk && openRawLog", block)
        self.assertNotIn("state.radioOk && openRawLog", block)
        self.assertNotIn("gpsPositionCurrent", block)
        self.assertLess(FW.index("recorderCore.flush()"),FW.index("serviceTelemetry(nowMs"))

    def test_one_press_preflight_is_firmware_owned(self):
        self.assertIn('"/api/preflight/start"', FW)
        self.assertIn("serviceFlightReadiness(nowMs)", FW)
        for token in (
            "runFlightReadinessSdIntegrityTest",
            "requestHardwareTest(HARDWARE_TEST_ALL)",
            "runPassiveLoraDiagnostic(true)",
            "simulateLaunchAndLandingDetectors",
            "FLIGHT_PREP_ARM",
            "STATE_ARMED",
        ):
            self.assertIn(token, PREP+FW)
        self.assertIn("RecorderCore<8> simulatedRecorder", PREP)
        self.assertIn("validateHeader(simulatedSink.data()", PREP)
        self.assertIn("PREPARE FOR FLIGHT", DASH)
        self.assertIn("ARMED — READY FOR LAUNCH", DASH)

    def test_sd_integrity_temp_file_is_not_real_flight_data(self):
        self.assertIn('"/GIPREFLT.BIN"', PREP)
        self.assertIn("recordCount = 100", PREP)
        self.assertIn("validateHeader", PREP)
        self.assertIn("SD.remove(path)", PREP)
        self.assertNotIn('"/V12D', PREP)

    def test_optional_gps_lora_warn_but_do_not_fail(self):
        self.assertIn("WARNING_GPS", (ROOT/"libraries/GreenIonRecorder/src/GreenIonFlightReadiness.h").read_text())
        self.assertIn("WARNING_LORA", (ROOT/"libraries/GreenIonRecorder/src/GreenIonFlightReadiness.h").read_text())
        self.assertIn('(state.radioOk && radioTransmitAuthorized) ? "READY" : "WARNING"', PREP)
        self.assertIn('gpsPositionCurrent(nowMs) ? "READY" : "WARNING"', PREP)

    def test_lora_diagnostic_reports_pins_and_never_guesses_antenna(self):
        expected = {
            "PIN_SPI_SCK": 5, "PIN_SPI_MOSI": 19, "PIN_SPI_MISO": 21,
            "PIN_SD_CS": 33, "PIN_LORA_CS": 27, "PIN_LORA_RST": 15,
            "PIN_LORA_DIO0": 32,
        }
        for name, value in expected.items():
            self.assertIn(f"{name} = {value};", CFG)
        self.assertIn("RegVersion", PREP)
        self.assertIn("antenna_detectable", PREP)
        self.assertIn("Transmission refused", PREP)
        self.assertIn("radioTransmitAuthorized = true", PREP)
        self.assertIn("!radioTransmitAuthorized", FW)
        self.assertIn("radio_tx_authorized", FW)

    def test_flight_phase_and_apogee_tracking(self):
        self.assertIn("STATE_COAST", FW)
        self.assertIn("STATE_DESCENT", FW)
        self.assertIn("apogeeDetected", FW)
        self.assertIn("apogee_estimate_m", FW)
        self.assertIn("finalizeRecording(nowMs)", FW)

    def test_uno_version_and_diagnostics_are_v17(self):
        self.assertIn('RECEIVER_FIRMWARE_VERSION[] = "17.0.1"', UNO)
        self.assertNotIn('"receiver_firmware":"16.3.0"', UNO)
        self.assertIn("readRadioVersionRegister", UNO)
        self.assertIn("receiver_diagnostic", UNO)
        self.assertIn("receive_only", UNO)

if __name__=="__main__":
    unittest.main()
