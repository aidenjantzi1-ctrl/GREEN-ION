#!/usr/bin/env python3
"""Synthetic RKT4 recovery and firmware source contract tests (no ESP32 required)."""
from pathlib import Path
import struct
import unittest

ROOT = Path(__file__).resolve().parents[1]
INO = (ROOT / "Green_Ion_V8_7/Green_Ion_V8_7.ino").read_text()
DASH = (ROOT / "Green_Ion_V8_7/Dashboard.h").read_text()


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
        self.assertIn('if (saved && !sdFault && checkpointFilename.length()) SD.remove(checkpointFilename);', INO)

    def test_dashboard_recovery_display(self):
        for token in ('id="integrityStatus"', 'id="recoveredFlights"',
                      'id="recoveredSamples"', 'id="resetReason"'):
            self.assertIn(token, DASH)


if __name__ == "__main__":
    unittest.main()
