#!/usr/bin/env python3
"""Synthetic RKT4 recovery and firmware source contract tests (no ESP32 required)."""
from pathlib import Path
import struct
import unittest

ROOT = Path(__file__).resolve().parents[1]
INO = (ROOT / "Green_Ion_V11/Green_Ion_V11.ino").read_text(encoding="utf-8")
DASH = (ROOT / "Green_Ion_V11/Dashboard.h").read_text(encoding="utf-8")


def inspect_rkt4(data):
    if len(data) < 32:
        return False, 0, 0
    magic, version, header, record, rate = struct.unpack_from("<4sHHHH", data)
    valid = magic == b"RKT4" and version == 2 and header == 32 and record == 16 and rate in (10, 25, 50)
    if not valid:
        return False, 0, 0
    count, partial = divmod(len(data) - 32, 16)
    return True, count, partial


def make_log(n=10, rate=25):
    h = bytearray(32)
    struct.pack_into("<4sHHHHfH", h, 0, b"RKT4", 2, 32, 16, rate, 101325.0, 3950)
    return bytes(h) + b"".join(struct.pack("<IffHH", i * 40000, 101325.0, 21.5, 3900, 1) for i in range(n))


class RecoveryTests(unittest.TestCase):
    def test_valid_flights(self):
        for rate in (10, 25, 50):
            for n in (0, 1, 25, 1000):
                self.assertEqual(inspect_rkt4(make_log(n, rate)), (True, n, 0))

    def test_torn_record(self):
        log = make_log(12)
        for tail in range(1, 16):
            self.assertEqual(inspect_rkt4(log + b"X" * tail), (True, 12, tail))

    def test_invalid_header(self):
        self.assertEqual(inspect_rkt4(b"bad"), (False, 0, 0))
        log = bytearray(make_log())
        log[0:4] = b"NOPE"
        self.assertEqual(inspect_rkt4(log), (False, 0, 0))
        log = bytearray(make_log())
        log[8:10] = struct.pack("<H", 32)
        self.assertEqual(inspect_rkt4(log), (False, 0, 0))

    def test_firmware_recovery_hooks(self):
        for token in ("recoverInterruptedFlights();", "validRKT4Header", "saveCheckpoint();",
                      "CHECKPOINT_MS", "recovered_interrupted", "trailing_partial_bytes",
                      "integrityStatus", "resetReasonText"):
            self.assertIn(token, INO)
        self.assertIn('if (flushLog()) saveCheckpoint();', INO)
        self.assertIn('SD.remove(checkpointFilename);', INO)
        self.assertIn('SD.remove(checkpointFilenameB);', INO)

    def test_dashboard_recovery_display(self):
        for token in ('id="integrityStatus"', 'id="recoveredFlights"',
                      'id="recoveredSamples"', 'id="resetReason"'):
            self.assertIn(token, DASH)

    def test_v88_field_confidence_contract(self):
        for token in (
            'FIRMWARE_VERSION = "10.0"', "GREEN_ION_GIT_SHA", "startGroundTest",
            "finishGroundTest", "sensorFrozen", "flightChecksum", "bin_fnv1a32",
            "confidence_score", "maximum_voltage_drop_v", "maximum_flush_us",
            'web.on("/api/time"', 'web.on("/api/protect"', 'web.on("/api/note"',
            'web.on("/api/diagnostics"', "recordingLimitMin",
        ):
            self.assertIn(token, INO)

    def test_v88_dashboard_controls(self):
        for token in (
            'id="groundtest"', 'id="recordingLimit"', 'id="sensorFrozen"',
            'id="maximumFlush"', 'id="flushHeartbeat"', "Add note", "Unprotect",
            "/api/diagnostics",
        ):
            self.assertIn(token, DASH)
        self.assertNotIn("Launch assistant", DASH)
        self.assertNotIn("Compare flights", DASH)
        self.assertNotIn("comparisonMeta", DASH)

    def test_v9_automatic_flight_contract(self):
        for token in (
            "PREBUFFER_CAPACITY", "capturePrelaunch", "writePrebuffer",
            "startLaunchWatch", "wifiLaunchDisconnectPending", "activePhase",
            "phaseParachuteMs", "landingStableSinceMs", "GREENION.CFG",
            "peakAscentSpeedMps", "fastestDescentMps",
            "nextCardSequence", 'web.on("/api/prepare"',
            'web.on("/api/launchwatch"', 'web.on("/api/cancel"',
        ):
            self.assertIn(token, INO)

        for token in (
            'data-tab="status"', 'data-tab="flight"', 'data-tab="utilities"',
            'id="prepareAll"', 'id="cancelWatch"', 'id="flightPhase"',
            "selectedFlight=\"\";loadedFlightName=\"\";savedFlightSeries=[]",
        ):
            self.assertIn(token, DASH)

    def test_v91_altitude_accuracy_contract(self):
        for token in (
            "CAL_WARMUP_SAMPLES", "CAL_TARGET_SAMPLES", "calSamples",
            "sortCalibrationSamples", "beginCalibrationVerification",
            "calNoiseM", "calDriftPaPerSec", "medianPressure3",
            "flightAltM", "displayAlpha", "flightAlpha",
            "raw_pressure_logged;median3_pressure;separate_display_and_flight_filters",
        ):
            self.assertIn(token, INO)
        for token in ('id="calNoise"', 'id="calDrift"', "d.calProgress"):
            self.assertIn(token, DASH)

    def test_v92_resilience_and_two_tap_contract(self):
        for token in (
            "EVENT_PRESSURE_ANOMALY", "batteryRateReduced", "effectiveSampleRateHz",
            "calibrationFreshnessReason", "writePreflightReport", "confidence_event_",
            "attemptSensorRecovery", "sensorOutageActive", 'web.on("/api/armwatch"',
            'web.on("/api/preflight"', "minimum_effective_sample_rate_hz",
        ):
            self.assertIn(token, INO)
        for token in (
            'id="armWatch"', 'id="downloadPreflight"',
            'id="flightConfidenceTimeline"', "api(\"armwatch\",\"POST\")",
        ):
            self.assertIn(token, DASH)
        self.assertNotIn('<label for="missionKind">Recording type</label>', DASH)

    def test_v10_field_resilience_contract(self):
        for token in (
            "EMERGENCY_BUFFER_BYTES", "emergencyBuffer", "checkpointFilenameB",
            "writeHealthSnapshot", "writePostflightReport", "rebuildFlightManifest",
            "sdSafeRateHz", "environmentalWarning", "configurationBackupAPI",
            "configurationRestoreAPI", "integrityScanAPI", "rollbackAPI",
            "enableLoopWDT", "feedLoopWDT", "previousPowerEvidence",
        ):
            self.assertIn(token, INO)
        for token in (
            'id="field"', 'id="audibleCues"', 'id="downloadEverything"',
            'id="scanIntegrity"', 'id="rollbackFirmware"', 'id="restoreConfig"',
        ):
            self.assertIn(token, DASH)


if __name__ == "__main__":
    unittest.main()

