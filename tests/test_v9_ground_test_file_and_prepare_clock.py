#!/usr/bin/env python3
"""Source-contract regression checks for ground-test file safety and Prepare clock."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
INO = (ROOT / "Green_Ion_V9/Green_Ion_V9.ino").read_text(encoding="utf-8")


def between(start, end):
    assert INO.count(start) == 1, start
    return INO.split(start, 1)[1].split(end, 1)[0]


class GroundTestFileSafetyTests(unittest.TestCase):
    def test_ground_test_allocates_unused_bounded_fat_name(self):
        start = between("bool startGroundTest() {", "\nvoid finishGroundTest() {")
        self.assertIn("for (unsigned slot = 0; slot < 16; ++slot)", start)
        self.assertIn('snprintf(candidate, sizeof(candidate), "/GND%04u.TMP", slot)', start)
        self.assertIn("if (!SD.exists(candidate))", start)
        self.assertIn("if (!groundTestPath[0])", start)
        self.assertIn("SD.open(groundTestPath, FILE_WRITE)", start)
        self.assertLess(start.index("if (!SD.exists(candidate))"),
                        start.index("SD.open(groundTestPath, FILE_WRITE)"))
        self.assertNotIn('SD.remove("/GNDTEST.TMP")', start)
        self.assertNotIn('SD.open("/GNDTEST.TMP", FILE_WRITE)', start)

    def test_ground_test_cleanup_failure_is_not_a_pass(self):
        finish = between("void finishGroundTest() {", "\nbool startPrepareAll() {")
        self.assertIn("bool cleanupOK = SD.remove(groundTestPath);", finish)
        self.assertIn("bytes == groundTestSamples * 16UL && cleanupOK", finish)
        self.assertIn("sdVerified = false;", finish)
        self.assertIn("Ground test cleanup failed: retest SD before flight", finish)
        self.assertIn("temporary file cleanup failed; retest SD", finish)
        self.assertNotIn('SD.remove("/GNDTEST.TMP")', finish)

    def test_file_path_cleared_before_next_ground_test(self):
        self.assertIn("char groundTestPath[16] = {0};", INO)
        start = between("bool startGroundTest() {", "\nvoid finishGroundTest() {")
        finish = between("void finishGroundTest() {", "\nbool startPrepareAll() {")
        self.assertIn("groundTestPath[0] = '\\0';", start)
        self.assertIn("groundTestPath[0] = '\\0';", finish)


class PrepareClockValidationTests(unittest.TestCase):
    def test_prepare_uses_same_strict_parsers_as_time_route(self):
        time = between('web.on("/api/time", HTTP_POST, []() {',
                       '\n  web.on("/api/calibrate"')
        prepare = between('web.on("/api/prepare", HTTP_POST, []() {',
                          '\n  web.on("/api/arm"')
        for body in (time, prepare):
            self.assertIn('parsePhoneEpoch(web.arg("epoch"), requestedEpoch)', body)
            self.assertIn('parsePhoneUtcOffset(web.arg("offset"), requestedOffset)', body)
        self.assertIn('web.hasArg("epoch") || web.hasArg("offset")', prepare)
        self.assertIn('!web.hasArg("epoch") || !web.hasArg("offset")', prepare)
        self.assertNotIn("strtoull(", prepare)
        self.assertNotIn(".toInt()", prepare)
        self.assertLess(prepare.index("parsePhoneUtcOffset("),
                        prepare.index("if (!startPrepareAll())"))
        self.assertLess(prepare.index("parsePhoneEpoch("),
                        prepare.index("missionKind = requestedKind;"))
        self.assertLess(prepare.index("parsePhoneUtcOffset("),
                        prepare.index("phoneEpochMs = requestedEpoch;"))


if __name__ == "__main__":
    unittest.main()
