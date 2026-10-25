#!/usr/bin/env python3
"""Source and Node regression checks for clock, SD test and phase safeguards."""
import json
from pathlib import Path
import shutil
import subprocess
import unittest

ROOT = Path(__file__).resolve().parents[1]
INO = (ROOT / "Green_Ion_V9/Green_Ion_V9.ino").read_text(encoding="utf-8")
DASH = (ROOT / "Green_Ion_V9/Dashboard.h").read_text(encoding="utf-8")


def between(text, start, end):
    assert text.count(start) == 1, start
    return start + text.split(start, 1)[1].split(end, 1)[0]


class ClockAndSDTests(unittest.TestCase):
    def test_phone_clock_rejects_bad_inputs_before_mutating_state(self):
        route = between(INO, 'web.on("/api/time", HTTP_POST, []() {',
                        '\n  web.on("/api/calibrate"')
        self.assertIn("flightSettingsLocked()", route)
        self.assertIn('parsePhoneEpoch(web.arg("epoch"), requestedEpoch)', route)
        self.assertIn('parsePhoneUtcOffset(web.arg("offset"), requestedOffset)', route)
        self.assertLess(route.index("parsePhoneEpoch("), route.index("phoneEpochMs ="))
        self.assertLess(route.index("parsePhoneUtcOffset("), route.index("phoneUtcOffsetMin ="))
        self.assertNotIn("strtoull(", route)
        self.assertNotIn(".toInt()", route)
        helper = between(INO, "bool parsePhoneEpoch(", "\nbool parsePhoneUtcOffset(")
        self.assertIn("raw.length() != 13", helper)
        self.assertIn("parsed >= 4102444800000ULL", helper)
        offset = between(INO, "bool parsePhoneUtcOffset(", "\n// ---------- BMP388 ----------")
        self.assertIn("minutes < -840 || minutes > 720", offset)

    def test_sd_test_never_deletes_preexisting_file(self):
        verify = between(INO, "bool verifySD() {", "\n// ----------")
        self.assertIn("if (!SD.exists(path))", verify)
        self.assertIn("if (!freeSlot)", verify)
        self.assertNotIn('const char *path = "/GITEST.TMP"', verify)
        self.assertNotIn("SD.exists(path) && !SD.remove(path)", verify)
        self.assertIn('snprintf(path, sizeof(path), "/GIT%04u.TMP"', verify)


@unittest.skipUnless(shutil.which("node"), "Node.js required for dashboard checks")
class PhaseTests(unittest.TestCase):
    def test_invalid_sample_cannot_complete_launch_or_landing(self):
        segment = between(DASH, "function validFlightSegments(rows,column){",
                          "\nfunction enrichFlightSeries(rows){")
        phases = between(DASH, "function computePhases(rows){", "\nfunction drawSavedGraph(){")
        # Extract only computePhases, excluding any functions between it and the graph.
        phases = phases.split("\nfunction ", 1)[0]
        script = segment + phases + """
const out={textContent:"",replaceChildren(){this.textContent=""},appendChild(){}};
const $=()=>out;
const document={createElement:()=>({className:"",textContent:"",append(){}})};
let flightPhases=[];
const row=(i,alt,valid=1)=>[i*.1,101325,20,3.9,valid,alt];
const badLaunch=[row(0,0),row(1,7),row(2,7,0),row(3,7),row(4,7),row(5,0),row(6,0),row(7,0)];
computePhases(badLaunch);
const noLaunch=flightPhases.length===0&&out.textContent.includes("No sustained");
const base=[0,6,7,8,12,20,19,3,2,2,1,8,0].map((a,i)=>row(i,a,i===8?0:1));
computePhases(base);
const noLanding=!flightPhases.some(p=>p[0]==="Landing detected");
base[11][5]=0;
computePhases(base);
const yesLanding=flightPhases.some(p=>p[0]==="Landing detected");
console.log(JSON.stringify({noLaunch,noLanding,yesLanding}));
"""
        result = subprocess.run(["node", "-"], input=script, text=True,
                                capture_output=True, timeout=15, check=True)
        self.assertEqual(json.loads(result.stdout),
                         {"noLaunch": True, "noLanding": True, "yesLanding": True})


if __name__ == "__main__":
    unittest.main()
