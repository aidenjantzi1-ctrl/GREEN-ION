#!/usr/bin/env python3
"""Regression checks for V9 CSV parsing and derived flight metrics."""
from pathlib import Path
import json
import re
import shutil
import subprocess
import unittest

ROOT = Path(__file__).resolve().parents[1]
DASH = (ROOT / "Green_Ion_V9/Dashboard.h").read_text(encoding="utf-8")
INO = (ROOT / "Green_Ion_V9/Green_Ion_V9.ino").read_text(encoding="utf-8")


class CsvReliabilityTests(unittest.TestCase):
    def test_firmware_uses_explicit_first_timestamp_state(self):
        csv = INO.split("void csvAPI() {", 1)[1].split("\nvoid metadataAPI()", 1)[0]
        self.assertIn("bool hasFirstStamp = false;", csv)
        self.assertIn("if (!hasFirstStamp) { firstStamp = stamp; hasFirstStamp = true; }", csv)
        self.assertNotIn("if (!firstStamp) firstStamp = stamp;", csv)

    def test_invalid_sensor_reading_resets_derivative_state(self):
        csv = INO.split("void csvAPI() {", 1)[1].split("\nvoid metadataAPI()", 1)[0]
        self.assertRegex(csv, r"else\s*\{\s*// Never derive velocity or acceleration across an invalid pressure sample\.")
        self.assertIn("previousAltitude = NAN;", csv)
        self.assertIn("previousVelocity = NAN;", csv)
        self.assertIn("dt >= 0.01f && dt <= 2.0f", csv)

    @unittest.skipUnless(shutil.which("node"), "Node.js is required for dashboard parser tests")
    def test_blank_fields_are_not_zero(self):
        match = re.search(r"function parseCSVNumber\(value\)\{[^}]+\}", DASH)
        self.assertIsNotNone(match)
        script = (
            match.group(0) +
            '\nconsole.log(JSON.stringify(["", " ", "0", "-2.5", "3.75", "not-a-number"].map('
            's=>{const v=parseCSVNumber(s);return Number.isNaN(v)?"NaN":v;})));'
        )
        result = subprocess.run(["node", "-e", script], check=True,
                                capture_output=True, text=True, timeout=10)
        self.assertEqual(json.loads(result.stdout),
                         ["NaN", "NaN", 0, -2.5, 3.75, "NaN"])

    def test_csv_loader_rejects_bad_validity_flags(self):
        self.assertIn("line.split(\",\").map(parseCSVNumber)", DASH)
        self.assertIn("(r[4]===0||r[4]===1)", DASH)


if __name__ == "__main__":
    unittest.main()
