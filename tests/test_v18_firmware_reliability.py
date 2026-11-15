import pathlib
import unittest

ROOT=pathlib.Path(__file__).resolve().parents[1]
FW=(ROOT/"Green_Ion_V16_3"/"Green_Ion_V16_3.ino").read_text(encoding="utf-8")
HOME=(ROOT/"Green_Ion_V16_3"/"HomeWifi.h").read_text(encoding="utf-8")
CFG=(ROOT/"Green_Ion_V16_3"/"Config.h").read_text(encoding="utf-8")
PROTO=(ROOT/"libraries/GreenIonProtocol/src/GreenIonProtocol.h").read_text(encoding="utf-8")
RKT4=(ROOT/"libraries/GreenIonRecorder/src/GreenIonRkt4.h").read_text(encoding="utf-8")
VERIFY=(ROOT/"libraries/GreenIonRecorder/src/GreenIonSaveVerification.h").read_text(encoding="utf-8")
PREP=(ROOT/"Green_Ion_V16_3"/"FlightPreparation.h").read_text(encoding="utf-8")
NAMING=(ROOT/"libraries/GreenIonRecorder/src/GreenIonFileNaming.h").read_text(encoding="utf-8")

class V18FirmwareReliability(unittest.TestCase):
    def test_wifi_off_is_requested_only_after_sd_recording_starts(self):
        start=FW.index("static void appendLog")
        end=FW.index("static uint16_t telemetryFlags", start)
        block=FW[start:end]
        self.assertLess(block.index("state.sdOk && openRawLog"),
                        block.index("recorderMode = RECORDER_RECORDING"))
        self.assertLess(block.index("recorderMode = RECORDER_RECORDING"),
                        block.index("requestFlightWifiSuppression(nowMs)"))
        self.assertNotIn("state.radioOk && openRawLog", block)
        self.assertNotIn("gpsPositionCurrent(nowMs) && openRawLog", block)

    def test_wifi_power_transition_is_subordinate_to_primary_sd(self):
        loop=FW[FW.index("void loop()"):]
        self.assertLess(loop.index("recorderCore.flush()"),
                        loop.index("serviceFlightWifiPower(nowMs, micros())"))
        self.assertLess(loop.index("serviceFlightWifiPower(nowMs, micros())"),
                        loop.index("serviceTelemetry(nowMs, micros())"))
        self.assertIn("WIFI_FLIGHT_OFF_SETTLE_MS = 1000", CFG)
        self.assertIn("WIFI_TRANSITION_GUARD_US = 12000", CFG)
        self.assertIn("hasDeadlineMargin", HOME)
        self.assertIn("WiFi.mode(WIFI_OFF)", HOME)

    def test_wifi_restore_requires_independent_save_readback(self):
        start=FW.index("static bool finalizeRecording")
        end=FW.index("static bool openRawLog", start)
        block=FW[start:end]
        self.assertLess(block.index("writeCompletionMetadata"),
                        block.index("verifyCompletedFlightPackage"))
        self.assertLess(block.index("verifyCompletedFlightPackage"),
                        block.index("SD.exists(checkpointFilenameA)"))
        self.assertLess(block.index("SD.exists(checkpointFilenameB)"),
                        block.index("requestPostFlightWifiRestore()"))
        self.assertIn("validateHeader", VERIFY)
        self.assertIn("trailingByteCount", VERIFY)
        self.assertIn("GREEN-ION-COMPLETION-1", VERIFY)
        self.assertIn("file_bytes=", VERIFY)

    def test_failed_save_cannot_wake_wifi_or_delete_recovery_evidence(self):
        block=FW[FW.index("static bool finalizeRecording"):FW.index("static bool openRawLog")]
        verify_failure=block[block.index("if (!verifyCompletedFlightPackage"):
                             block.index("if (SD.exists(checkpointFilenameA)")]
        self.assertIn("RECORDER_FAULT", verify_failure)
        self.assertIn("return false", verify_failure)
        self.assertNotIn("requestPostFlightWifiRestore", verify_failure)
        self.assertIn("homeWifiFlightSuppressed || homeWifiFlightSuppressionRequested", HOME)

    def test_short_flight_cancels_pending_wifi_suppression(self):
        fn=HOME[HOME.index("static void requestPostFlightWifiRestore"):
                HOME.index("static void restoreWifiAfterVerifiedSave")]
        self.assertIn("if (!homeWifiFlightSuppressed)", fn)
        self.assertIn("homeWifiFlightSuppressionRequested = false", fn)
        self.assertIn("homeWifiPostSaveRestoreRequested = false", fn)
        self.assertIn('homeWifiLastResult = "post_flight_wifi_already_on"', fn)

    def test_repeated_flight_cycles_clear_wifi_transition_state(self):
        init=HOME[HOME.index("static void initializeHomeWifi"):
                  HOME.index("static void requestFlightWifiSuppression")]
        for token in (
            "homeWifiFlightSuppressed = false",
            "homeWifiFlightSuppressionRequested = false",
            "homeWifiPostSaveRestoreRequested = false",
            "homeWifiFlightSuppressionRequestedMs = 0",
        ):
            self.assertIn(token, init)
        restore=HOME[HOME.index("static void restoreWifiAfterVerifiedSave"):
                     HOME.index("static void serviceFlightWifiPower")]
        for token in (
            "homeWifiFlightSuppressed = false",
            "homeWifiFlightSuppressionRequested = false",
            "homeWifiPostSaveRestoreRequested = false",
            "homeWifiFlightSuppressionRequestedMs = 0",
        ):
            self.assertIn(token, restore)

    def test_manual_and_automatic_recording_share_wifi_policy(self):
        self.assertGreaterEqual(FW.count("requestFlightWifiSuppression(nowMs);"), 2)
        self.assertIn("startManualFlightRecording", FW)

    def test_failed_finalization_blocks_new_prepare_until_recovery(self):
        result=PREP[PREP.index("static green_ion::readiness::Result currentFlightReadinessResult"):
                    PREP.index("static void beginFlightReadiness")]
        self.assertIn("recoveryPending == 0 && finalizationFailures == 0", result)

        begin=PREP[PREP.index("static void beginFlightReadiness"):
                   PREP.index("static void serviceFlightReadiness")]
        self.assertLess(begin.index("finalizationFailures != 0"),
                        begin.index("flightWatcher.cancel()"))
        self.assertIn("previous flight finalization failed", begin)

        service=PREP[PREP.index("static void serviceFlightReadiness"):]
        critical=service[service.index("case FLIGHT_PREP_CRITICAL_CHECK"):
                         service.index("case FLIGHT_PREP_SD_INTEGRITY")]
        self.assertIn("if (finalizationFailures)", critical)
        self.assertIn("recovery evidence needs review", critical)

    def test_two_flights_without_reboot_get_fresh_prepare_and_filename(self):
        # COMPLETE is intentionally not a busy state for PREPARE & ARM.
        busy=PREP[PREP.index("static bool flightReadinessContextBusy"):
                  PREP.index("static void updateSdCapacityCache")]
        self.assertNotIn("RECORDER_COMPLETE", busy)
        begin=PREP[PREP.index("static void beginFlightReadiness"):
                   PREP.index("static void serviceFlightReadiness")]
        self.assertIn("recorderCore.resetBuffer()", begin)
        self.assertIn("resetCalibration(flightReadinessStartedMs)", begin)

        open_log=FW[FW.index("static bool openRawLog"):
                    FW.index("static void startManualFlightRecording")]
        self.assertIn("for (uint16_t sequence = 0; sequence < 1000; ++sequence)", open_log)
        self.assertIn("flightSequenceOccupied", open_log)
        self.assertIn('snprintf(candidate, sizeof(candidate), "/V12D%03u.BIN"', open_log)
        self.assertNotIn("SD.remove(candidate)", open_log)
        self.assertIn("flightSequenceOccupied", NAMING)

    def test_power_loss_recovery_runs_before_wifi_startup(self):
        setup=FW[FW.index("void setup()"):FW.index("void loop()")]
        self.assertLess(setup.index("initializeStorage();"),
                        setup.index("recoverInterruptedLogs();"))
        self.assertLess(setup.index("recoverInterruptedLogs();"),
                        setup.index("initializeDashboard();"))
        self.assertIn("recoverInterruptedLogs();", setup)

    def test_primary_formats_and_optional_link_contract_are_unchanged(self):
        self.assertIn("kProtocolVersion = 2", PROTO)
        self.assertIn("sizeof(TelemetryPacket) == 70", PROTO)
        self.assertIn("kVersion = 2", RKT4)
        self.assertIn("kHeaderBytes = 32", RKT4)
        self.assertIn("kRecordBytes = 16", RKT4)
        self.assertIn("SENSOR_RATE_HZ = 50", CFG)

if __name__=="__main__":
    unittest.main()
