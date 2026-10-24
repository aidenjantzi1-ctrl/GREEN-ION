#!/usr/bin/env python3
"""Regression checks for exported CSV values and firmware conversion guards."""
from pathlib import Path
import re
import shutil
import subprocess
import unittest

ROOT = Path(__file__).resolve().parents[1]
DASH = (ROOT / "Green_Ion_V8_7/Dashboard.h").read_text()
INO = (ROOT / "Green_Ion_V8_7/Green_Ion_V8_7.ino").read_text()


class CSVRegressionTests(unittest.TestCase):
    def test_dashboard_preserves_missing_measurements(self):
        self.assertIn("map(parseCSVNumber)", DASH)
        self.assertNotIn("line.split(\",\").map(Number)", DASH)
        self.assertIn("r[4]===0||r[4]===1", DASH)

    @unittest.skipUnless(shutil.which("node"), "Node.js is needed to execute the dashboard parser")
    def test_dashboard_csv_number_parser_in_javascript(self):
        match = re.search(r"function parseCSVNumber\(value\)\{[^\n]+\}", DASH)
        self.assertIsNotNone(match, "Dashboard CSV parser function not found")
        script = match.group(0) + """
const values = ["", " ", "0", "1.25", "-3.5", "NaN", "not a number"];
const actual = values.map(parseCSVNumber);
if (!Number.isNaN(actual[0]) || !Number.isNaN(actual[1]) ||
    actual[2] !== 0 || actual[3] !== 1.25 || actual[4] !== -3.5 ||
    !Number.isNaN(actual[5]) || !Number.isNaN(actual[6])) {
  throw new Error("CSV number parsing regression: " + JSON.stringify(actual));
}
"""
        subprocess.run(["node", "-e", script], check=True, capture_output=True, text=True)

    def test_firmware_uses_explicit_first_timestamp_flag(self):
        self.assertIn("bool hasFirstStamp = false;", INO)
        self.assertIn("if (!hasFirstStamp) { firstStamp = stamp; hasFirstStamp = true; }", INO)
        self.assertNotIn("if (!firstStamp) firstStamp = stamp;", INO)

    def test_firmware_does_not_bridge_invalid_samples(self):
        self.assertIn("Never estimate derivatives across missing or invalid sensor samples.", INO)
        self.assertIn("if (dt >= 0.01f && dt <= 2.0f)", INO)
        self.assertRegex(INO, r"previousAltitude = NAN;\s*previousStamp = 0;\s*previousVelocity = NAN;\s*previousVelocityStamp = 0;")


if __name__ == "__main__":
    unittest.main()
