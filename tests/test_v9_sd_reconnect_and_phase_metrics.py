#!/usr/bin/env python3
"""Regression tests for SD remounting and consistent saved-flight phase metrics."""
import json
from pathlib import Path
import shutil
import subprocess
import unittest

ROOT = Path(__file__).resolve().parents[1]
INO = (ROOT / "Green_Ion_V9/Green_Ion_V9.ino").read_text(encoding="utf-8")
DASH = (ROOT / "Green_Ion_V9/Dashboard.h").read_text(encoding="utf-8")


def section(source, start, end):
    assert source.count(start) == 1, start
    assert source.count(end) == 1, end
    return start + source.split(start, 1)[1].split(end, 1)[0]


class SdReconnectTests(unittest.TestCase):
    def test_idle_retest_remounts_card_before_read_write(self):
        code = section(INO, "bool verifySD() {", "\n// ---------- RKT4 INTEGRITY")
        self.assertIn("SD.end();", code)
        self.assertIn("sdOK = false;", code)
        self.assertIn("if (!initSD()) return false;", code)
        self.assertLess(code.index("SD.end();"), code.index("if (!initSD())"))
        self.assertLess(code.index("if (!initSD())"), code.index("SD.open(path, FILE_WRITE)"))
        self.assertIn("recording || armed || calibrating || groundTestActive", code)
        self.assertNotIn("if (!sdOK && !initSD())", code)


class ConsistentFlightPhaseTests(unittest.TestCase):
    def test_phase_detection_precedes_performance_metrics(self):
        loader = section(DASH, "async function loadSavedGraph(name){",
                         "\n// Split samples at invalid pressure readings")
        self.assertLess(loader.index("computePhases(savedFlightSeries);"),
                        loader.index("renderFlightPerformance(savedFlightSeries);"))
        performance = section(DASH, "function renderFlightPerformance(rows){",
                              "\nfunction drawSavedGraph(){")
        self.assertIn('phaseRow("Launch detected")', performance)
        self.assertIn('phaseRow("Apogee")', performance)
        self.assertIn('phaseRow("Landing detected")', performance)
        self.assertNotIn("pts.find(p=>p[5]>=5)", performance)

    @unittest.skipUnless(shutil.which("node"), "Node.js required for dashboard metric tests")
    def test_single_noise_spike_does_not_invent_launch_duration(self):
        segment = section(DASH, "function validFlightSegments(rows,column){",
                          "\nfunction enrichFlightSeries(rows){")
        phases = section(DASH, "function computePhases(rows){",
                         "\nfunction renderFlightPerformance(rows){")
        performance = section(DASH, "function renderFlightPerformance(rows){",
                              "\nfunction drawSavedGraph(){")
        harness = """
function el(){return {children:[],textContent:"",className:"",
 append(...items){this.children.push(...items)},
 appendChild(item){this.children.push(item)},
 replaceChildren(){this.children=[];this.textContent=""}};}
const elements={flightPhases:el(),flightPerformance:el()};
const $=id=>elements[id];
const document={createElement:()=>el()};
const window={currentFlightSamples:0,currentFlightMissed:0,currentFlightInvalid:0};
let flightPhases=[];
function extreme(vals,minimum){return minimum?Math.min(...vals):Math.max(...vals);}
const row=(i,alt,valid=1)=>[i*.1,101325,20,3.9,valid,alt];
function metrics(rows){
 computePhases(rows);renderFlightPerformance(rows);
 return Object.fromEntries(elements.flightPerformance.children
  .filter(x=>x.className==="row")
  .map(x=>[x.children[0].textContent,x.children[1].textContent]));
}
const spike=metrics([0,0,6,0,0,0,0,0].map((v,i)=>row(i,v)));
const sustained=metrics([0,1,5,6,7,15,20,19,8,3,2,1,0].map((v,i)=>row(i,v)));
console.log(JSON.stringify({spike,sustained}));
"""
        script = segment + phases + performance + harness
        result = subprocess.run(
            ["node", "-"], input=script, text=True, capture_output=True,
            check=True, timeout=15,
        )
        data = json.loads(result.stdout)
        self.assertEqual(data["spike"]["Time to apogee"], "Not determined")
        self.assertEqual(data["spike"]["Ascent duration"], "Not determined")
        self.assertEqual(data["spike"]["Descent duration"], "Not determined")
        self.assertEqual(data["sustained"]["Time to apogee"], "0.40 s")
        self.assertEqual(data["sustained"]["Descent duration"], "0.30 s")


if __name__ == "__main__":
    unittest.main()
