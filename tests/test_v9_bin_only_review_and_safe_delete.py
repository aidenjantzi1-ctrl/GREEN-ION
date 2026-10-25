#!/usr/bin/env python3
"""Regression checks for BIN-only review and non-destructive failed deletion."""
import json
from pathlib import Path
import shutil
import subprocess
import unittest

ROOT = Path(__file__).resolve().parents[1]
DASH = (ROOT / "Green_Ion_V9/Dashboard.h").read_text(encoding="utf-8")
INO = (ROOT / "Green_Ion_V9/Green_Ion_V9.ino").read_text(encoding="utf-8")


def section(source, start, end):
    assert source.count(start) == 1, start
    assert source.count(end) == 1, end
    return start + source.split(start, 1)[1].split(end, 1)[0]


def run_node(script):
    p = subprocess.run(["node", "-"], input=script, text=True,
                       capture_output=True, check=True, timeout=15)
    return json.loads(p.stdout)


SUMMARY = section(DASH, "async function showFlightStats(name,scrollToReport=true) {",
                  "\nfunction safeHTML(v)")
PDF = section(DASH, "function exportFlightPDF(){", "\nfunction compareBarGraph(")
DOM = r"""
function element(){return {textContent:"",children:[],replaceChildren(){this.children=[];},
 scrollIntoView(){this.scrolled=true;},querySelectorAll(){return [];}};}
const elements={};const $=id=>elements[id]||(elements[id]=element());
let selectedFlight="",flightSummaryRequestId=0,graphRequestId=0,
 savedFlightSeries=[],loadedFlightName="",flightMeta={},flightPhases=[];
const window={};const imperial=false;let loaded=[];
function drawSavedGraph(){}
function loadSavedGraph(name){loaded.push(name);}
"""


class FirmwareDeletionChecks(unittest.TestCase):
    def test_failed_bin_delete_does_not_delete_summary(self):
        code = section(INO, "void deleteAPI() {", "\nvoid protectAPI() {")
        self.assertIn('if (!SD.exists(binPath))', code)
        self.assertIn('if (!SD.remove(binPath))', code)
        self.assertIn('TXT summary preserved', code)
        self.assertIn('const bool summaryRemoved = !SD.exists(txtPath) || SD.remove(txtPath);', code)
        self.assertLess(code.index('if (!SD.remove(binPath))'),
                        code.index('SD.remove(txtPath)'))
        self.assertNotIn('SD.remove("/" + companionFile(n))', code)


@unittest.skipUnless(shutil.which("node"), "Node.js required")
class BrowserFallbackChecks(unittest.TestCase):
    def test_missing_txt_still_loads_raw_bin(self):
        script = DOM + SUMMARY + r"""
const fetch=async()=>({ok:false,status:404,text:async()=>"Flight summary unavailable"});
(async()=>{
 await showFlightStats("FLT0001.BIN",false);
 console.log(JSON.stringify({loaded,status:$("flightSummaryStatus").textContent,
  selected:selectedFlight,meta:flightMeta}));
})().catch(e=>{console.error(e);process.exitCode=1;});
"""
        result = run_node(script)
        self.assertEqual(result["loaded"], ["FLT0001.BIN"])
        self.assertIn("TXT summary missing", result["status"])
        self.assertEqual(result["selected"], "FLT0001.BIN")
        self.assertEqual(result["meta"], {})

    def test_busy_sd_does_not_trigger_bin_fallback(self):
        script = DOM + SUMMARY + r"""
const fetch=async()=>({ok:false,status:409,text:async()=>"SD busy"});
(async()=>{
 await showFlightStats("FLT0001.BIN",false);
 console.log(JSON.stringify({loaded,status:$("flightSummaryStatus").textContent}));
})().catch(e=>{console.error(e);process.exitCode=1;});
"""
        result = run_node(script)
        self.assertEqual(result["loaded"], [])
        self.assertIn("SD busy", result["status"])

    def test_late_missing_txt_response_cannot_replace_new_flight(self):
        script = DOM + SUMMARY + r"""
let resolveOld;
const fetch=url=>url.includes("FLT0001")?
 new Promise(resolve=>{resolveOld=resolve;}):
 Promise.resolve({ok:false,status:404,text:async()=>"Missing"});
(async()=>{
 const first=showFlightStats("FLT0001.BIN",false);
 await showFlightStats("FLT0002.BIN",false);
 resolveOld({ok:false,status:404,text:async()=>"Missing"});
 await first;
 console.log(JSON.stringify({loaded,selected:selectedFlight,
  status:$("flightSummaryStatus").textContent}));
})().catch(e=>{console.error(e);process.exitCode=1;});
"""
        result = run_node(script)
        self.assertEqual(result["loaded"], ["FLT0002.BIN"])
        self.assertEqual(result["selected"], "FLT0002.BIN")
        self.assertIn("FLT0002.BIN", result["status"])

    def test_bin_only_pdf_labels_unavailable_metadata(self):
        script = DOM + PDF + r"""
selectedFlight="FLT0001.BIN";loadedFlightName=selectedFlight;
savedFlightSeries=[[0,100000,20,4,1,0,0]];
const alert=()=>{throw Error("Unexpected alert");};
const safeHTML=v=>String(v);
const pdfRows=rows=>JSON.stringify(rows);
const reportGraph=()=>"<graph>";
let report="";
const printableReport=(title,body)=>{report=body;};
exportFlightPDF();
console.log(JSON.stringify({missing:report.includes("TXT summary unavailable"),
 source:report.includes("Source BIN:"),graphs:report.includes("<graph>")}));
"""
        result = run_node(script)
        self.assertTrue(result["missing"])
        self.assertTrue(result["source"])
        self.assertTrue(result["graphs"])


if __name__ == "__main__":
    unittest.main()
