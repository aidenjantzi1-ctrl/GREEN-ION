#!/usr/bin/env python3
"""V8.8 CSV and preflight regression checks; no ESP32 required."""
from pathlib import Path
import re
import shutil
import subprocess
import unittest

ROOT = Path(__file__).resolve().parents[1]
INO = (ROOT / "Green_Ion_V8_8/Green_Ion_V8_8.ino").read_text(encoding="utf-8")
DASH = (ROOT / "Green_Ion_V8_8/Dashboard.h").read_text(encoding="utf-8")
WORKFLOW = (ROOT / ".github/workflows/build-firmware.yml").read_text(encoding="utf-8")


class V88ReliabilityTests(unittest.TestCase):
    def test_csv_parser_keeps_missing_values_distinct_from_zero(self):
        match = re.search(r"function parseCSVNumber\(value\)\{[^}]+\}", DASH)
        self.assertIsNotNone(match, "CSV parser helper not found")
        self.assertIn(".map(parseCSVNumber)", DASH)
        self.assertNotIn('.map(line=>line.split(",").map(Number))', DASH)
        if shutil.which("node") is None:
            self.skipTest("Node.js is not installed")
        js = match.group(0) + """
const inputs = ["", "  ", "0", "-2.5", "null", "bad", "3.7"];
const values = inputs.map(parseCSVNumber);
if (!Number.isNaN(values[0]) || !Number.isNaN(values[1])) process.exit(1);
if (values[2] !== 0 || values[3] !== -2.5 || values[6] !== 3.7) process.exit(2);
if (!Number.isNaN(values[4]) || !Number.isNaN(values[5])) process.exit(3);
"""
        subprocess.run(["node", "-e", js], check=True, capture_output=True, text=True)

    def test_first_zero_timestamp_is_not_reassigned(self):
        self.assertIn("bool hasFirstStamp = false;", INO)
        self.assertIn("if (!hasFirstStamp) { firstStamp = stamp; hasFirstStamp = true; }", INO)
        self.assertNotIn("if (!firstStamp) firstStamp = stamp;", INO)

    def test_invalid_sensor_sample_breaks_derivative_chain(self):
        start = INO.index("void csvAPI()")
        end = INO.index("void metadataAPI()", start)
        csv = INO[start:end]
        self.assertIn("if (dt >= 0.01f && dt <= 2.0f)", csv)
        self.assertRegex(csv, r"else\s*\{\s*// Do not bridge invalid samples[\s\S]*?previousAltitude = NAN;")
        self.assertIn("previousVelocity = NAN;", csv)

    def test_sd_retest_unmounts_old_card_before_reinitializing(self):
        start = INO.index("bool verifySD()")
        end = INO.index("// ---------- RKT4 INTEGRITY", start)
        sd = INO[start:end]
        self.assertLess(sd.index("if (sdOK) SD.end();"), sd.index("if (!initSD())"))
        self.assertIn("sdVerified = false;", sd)
        self.assertIn("if (recording || armed || calibrating ||", sd)

    def test_arm_reason_is_visible_and_covers_common_blockers(self):
        self.assertIn('id="armReason"', DASH)
        self.assertIn('"armReason").textContent=armReadinessReason(d,busy)', DASH)
        for reason in ("Wait for calibration", "Tap Take Control", "Detect & Test SD Card",
                       "Battery critically low", "Stop simulation"):
            self.assertIn(reason, DASH)

    def test_pull_request_build_is_enabled(self):
        self.assertIn("  pull_request:", WORKFLOW)
        self.assertIn('      - "Green_Ion_V8_8/**"', WORKFLOW)
        self.assertIn("python3 -m unittest discover", WORKFLOW)


if __name__ == "__main__":
    unittest.main()
