#!/usr/bin/env python3
"""Regression checks for SD retesting and clear preflight guidance."""
from pathlib import Path
import re
import shutil
import subprocess
import unittest

ROOT = Path(__file__).resolve().parents[1]
DASH = (ROOT / "Green_Ion_V8_7/Dashboard.h").read_text()
INO = (ROOT / "Green_Ion_V8_7/Green_Ion_V8_7.ino").read_text()


class PreflightGuidanceTests(unittest.TestCase):
    def test_sd_retest_only_after_idle_guard(self):
        section = INO.split("bool verifySD() {", 1)[1].split("// ---------- RKT4", 1)[0]
        self.assertLess(section.index("if (recording || armed || calibrating ||"), section.index("SD.end()"))
        self.assertLess(section.index("SD.end()"), section.index("initSD()"))
        self.assertIn("SD mount failed: check card seating", section)
        self.assertIn("if (sdVerified) sdFault = false;", section)

    def test_arm_explanation_has_visible_status(self):
        self.assertIn('id="armReason"', DASH)
        self.assertIn('role="status" aria-live="polite"', DASH)
        self.assertIn('$("armReason").textContent=armReadinessReason(d,busy);', DASH)

    @unittest.skipUnless(shutil.which("node"), "Node.js is required")
    def test_arm_reason_for_common_states(self):
        match = re.search(r"function armReadinessReason\(d,isBusy\) \{[\s\S]*?\n\}", DASH)
        self.assertIsNotNone(match, "armReadinessReason not found")
        script = match.group(0) + """
const ready = {controller:true, sensor:true, reading:true, calibrated:true,
               sdVerified:true, sdFault:false, batteryCritical:false,
               armed:false, recording:false, simulation:false, calibrating:false,
               shutdownPending:false, updateInProgress:false};
const reason = (overrides={}, busy=false) =>
  armReadinessReason({...ready,...overrides},busy);
const tests = [
  [{},false,"Ready to arm"],
  [{controller:false},false,"Take Control"],
  [{sensor:false},false,"BMP388"],
  [{calibrated:false},false,"Calibrate"],
  [{sdVerified:false},false,"Storage"],
  [{sdFault:true},false,"Storage"],
  [{batteryCritical:true},false,"Battery"],
  [{recording:true},false,"Recording"],
  [{armed:true},false,"armed"],
  [{simulation:true},false,"simulation"],
  [{},true,"current action"]
];
for (const [state,busy,expected] of tests) {
  const actual = reason(state,busy);
  if (!actual.includes(expected)) throw Error(expected + " not found in " + actual);
}
"""
        subprocess.run(["node", "-e", script], check=True, capture_output=True, text=True)


if __name__ == "__main__":
    unittest.main()
