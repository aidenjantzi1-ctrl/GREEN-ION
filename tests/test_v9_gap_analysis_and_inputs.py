#!/usr/bin/env python3
"""Host-side regression checks for V9 strict settings and gap-aware analysis."""
import json
from pathlib import Path
import shutil
import subprocess
import unittest

ROOT = Path(__file__).resolve().parents[1]
INO = (ROOT / "Green_Ion_V9/Green_Ion_V9.ino").read_text(encoding="utf-8")
DASH = (ROOT / "Green_Ion_V9/Dashboard.h").read_text(encoding="utf-8")


def js_between(start, end):
    assert DASH.count(start) == 1, start
    return start + DASH.split(start, 1)[1].split(end, 1)[0]


SEGMENTS = js_between("function validFlightSegments(rows,column){", "\nfunction enrichFlightSeries(rows){")
ENRICH = js_between("function enrichFlightSeries(rows){", "\nfunction computePhases(rows){")
SAVED_GRAPH = js_between("function drawSavedGraph(){", "\nasync function readFlightMetadata(")


def run_node(source):
    result = subprocess.run(
        ["node", "-"], input=source, capture_output=True,
        text=True, check=True, timeout=15,
    )
    return json.loads(result.stdout)


class StrictSettingTests(unittest.TestCase):
    def test_firmware_rejects_numeric_suffixes_and_integer_wraps(self):
        helper = INO.split("bool parseChoiceNumber(", 1)[1].split("\nbool flightSettingsLocked()", 1)[0]
        self.assertIn("c < '0' || c > '9'", helper)
        self.assertIn("parsed > 65535UL", helper)
        self.assertIn('parseChoiceNumber(web.arg("rate"), requested)', INO)
        self.assertIn('parseChoiceNumber(web.arg("limit"), requestedLimitNumber)', INO)
        self.assertIn('parseChoiceNumber(web.arg("rate"), parsedRate)', INO)
        self.assertIn('parseChoiceNumber(web.arg("limit"), parsedLimit)', INO)
        self.assertNotIn('(uint8_t)web.arg("limit").toInt()', INO)
        self.assertNotIn('(uint16_t)web.arg("rate").toInt()', INO)


@unittest.skipUnless(shutil.which("node"), "Node.js is required for dashboard tests")
class GapAwareAnalysisTests(unittest.TestCase):
    def test_invalid_and_nonmonotonic_samples_split_segments(self):
        source = SEGMENTS + """
const sample=(t,valid=1)=>[t,101325,20,3.9,valid,t];
const rows=[sample(0),sample(.1),sample(.2,0),sample(.3),sample(3.0),sample(2.9)];
console.log(JSON.stringify(validFlightSegments(rows,5).map(seg=>seg.map(r=>r[0]))));
"""
        self.assertEqual(run_node(source), [[0, 0.1], [0.3], [3.0], [2.9]])

    def test_acceleration_never_bridges_invalid_sensor_row(self):
        source = SEGMENTS + ENRICH + """
const elements={flightExtraMetrics:{replaceChildren(){},appendChild(x){if(x.textContent)globalThis.note=x.textContent;}}};
const $=name=>elements[name];
const document={createElement:()=>({className:"",textContent:"",append(){},appendChild(){}})};
const extreme=(a,minimum)=>minimum?Math.min(...a):Math.max(...a);
const rows=Array.from({length:20},(_,i)=>[i*.1,101325,20,3.9,i===10?0:1,i<10?i:1000+i]);
enrichFlightSeries(rows);
console.log(JSON.stringify({
 first:rows[5][7],second:rows[15][7],gap:Number.isFinite(rows[10][7]),
 note:globalThis.note
}));
"""
        result = run_node(source)
        self.assertAlmostEqual(result["first"], 0, places=5)
        self.assertAlmostEqual(result["second"], 0, places=5)
        self.assertFalse(result["gap"])
        self.assertIn("Sensor gaps", result["note"])

    def test_saved_canvas_graph_breaks_at_invalid_row(self):
        source = SEGMENTS + SAVED_GRAPH + """
const operations=[];
const ctx={setTransform(){},clearRect(){},strokeRect(){},fillText(){},
 beginPath(){},moveTo(){operations.push("M")},lineTo(){operations.push("L")},
 stroke(){},save(){},setLineDash(){},restore(){}};
const canvas={getBoundingClientRect:()=>({width:500}),getContext:()=>ctx};
const $=name=>name==="savedGraphMetric"?{value:"5"}:canvas;
const devicePixelRatio=1,sunlight=false,flightPhases=[];
const savedFlightSeries=[0,1,2,3,4,5,6].map(i=>[i*.1,101325,20,3.9,i===3?0:1,i]);
drawSavedGraph();
console.log(JSON.stringify(operations));
"""
        self.assertEqual(run_node(source), ["M", "L", "L", "M", "L", "L"])


if __name__ == "__main__":
    unittest.main()
