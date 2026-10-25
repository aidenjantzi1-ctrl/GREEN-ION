#!/usr/bin/env python3
"""Source and browser regressions for non-destructive SD file checks."""
import json
from pathlib import Path
import shutil
import subprocess
import unittest

ROOT = Path(__file__).resolve().parents[1]
DASH = (ROOT / "Green_Ion_V9/Dashboard.h").read_text(encoding="utf-8")
INO = (ROOT / "Green_Ion_V9/Green_Ion_V9.ino").read_text(encoding="utf-8")


def section(text, start, end):
    assert text.count(start) == 1, start
    assert text.count(end) == 1, end
    return start + text.split(start, 1)[1].split(end, 1)[0]


def node(script):
    result = subprocess.run(
        ["node", "-"], input=script, text=True, capture_output=True,
        check=True, timeout=15,
    )
    return json.loads(result.stdout)


HELPERS = section(DASH, "function bytes(n) {", "// Keep blank CSV measurements")
LISTING = section(DASH, "async function loadFiles() {", '\n$("metric").onclick=')
COMPARE = section(
    DASH, "async function compareFlights(){",
    "\nasync function showFlightStats(name,scrollToReport=true)"
)

# No browser dependencies beyond these minimal DOM stand-ins.
DOM = r"""
function element(){
 return {children:[],textContent:"",value:"",disabled:false,selectedIndex:0,
  replaceChildren(){this.children=[];this.textContent="";},
  append(...children){this.children.push(...children);},
  appendChild(child){this.children.push(child);}};
}
const ids={};
const $=id=>ids[id]||(ids[id]=element());
const document={createElement:()=>element()};
const state={sdVerified:true,recording:false};
let comparisonMeta=null,comparisonRequestId=0,selectedFlight="FLT0001.BIN";
"""


class FirmwareChecks(unittest.TestCase):
    def test_listing_checks_rkt4_length_without_touching_file(self):
        listing = section(INO, "void filesAPI() {", "\nvoid downloadAPI() {")
        self.assertIn("const uint64_t binBytes = (uint64_t)f.size();", listing)
        self.assertIn("binBytes >= 48", listing)
        self.assertIn("((binBytes - 32) % 16) == 0", listing)
        self.assertIn('SD.exists("/" + companionFile(n))', listing)
        self.assertIn('",\\"recordLengthOK\\":" + jb(recordLengthOK)', listing)
        self.assertIn('",\\"hasSummary\\":" + jb(hasSummary)', listing)
        self.assertNotIn("SD.remove(", listing)
        self.assertNotIn("FILE_WRITE", listing)

    def test_printed_flight_identifies_original_and_estimated_values(self):
        report = section(DASH, "function exportFlightPDF(){", "\nfunction compareBarGraph(")
        self.assertIn("<strong>Source BIN:</strong>", report)
        self.assertIn("not ignition", report)
        self.assertIn("barometric estimates", report)


@unittest.skipUnless(shutil.which("node"), "Node.js required for dashboard tests")
class DashboardChecks(unittest.TestCase):
    def test_file_row_warnings_and_stale_comparison(self):
        script = HELPERS + LISTING + DOM + r"""
comparisonMeta={a:"FLT0001.BIN",b:"FLT0002.BIN"};
comparisonRequestId=7;
const api=async()=>({total:8589934592,used:1048576,files:[
 {name:"FLT0001.BIN",size:49,recordLengthOK:false,hasSummary:false,protected:false},
 {name:"FLT0003.BIN",size:64,recordLengthOK:true,hasSummary:true,protected:true}
]});
(async()=>{
 await loadFiles();
 const labels=$("fileList").children.map(row=>row.children[0].textContent);
 console.log(JSON.stringify({
  labels,meta:comparisonMeta,requestId:comparisonRequestId,
  result:$("comparisonResults").textContent,free:$("sdFreeSpace").textContent
 }));
})().catch(e=>{console.error(e);process.exitCode=1;});
"""
        result = node(script)
        labels = {label.split(" · ", 1)[0]: label for label in result["labels"]}
        self.assertIn("Check BIN length", labels["FLT0001.BIN"])
        self.assertIn("TXT missing", labels["FLT0001.BIN"])
        self.assertNotIn("Check BIN length", labels["FLT0003.BIN"])
        self.assertNotIn("TXT missing", labels["FLT0003.BIN"])
        self.assertIsNone(result["meta"])
        self.assertEqual(result["requestId"], 8)
        self.assertIn("no longer", result["result"])
        self.assertIn("GiB", result["free"])

    def test_failed_refresh_clears_old_capacity_and_comparison(self):
        script = HELPERS + LISTING + DOM + r"""
comparisonMeta={a:"FLT0001.BIN",b:"FLT0002.BIN"};
comparisonRequestId=4;
const api=async()=>{throw Error("card removed");};
(async()=>{
 await loadFiles();
 console.log(JSON.stringify({
  used:$("storageUsed").textContent,free:$("sdFreeSpace").textContent,
  count:$("fileCount").textContent,disabled:$("compareFlights").disabled,
  comparison:comparisonMeta,requestId:comparisonRequestId,
  error:$("fileList").textContent
 }));
})().catch(e=>{console.error(e);process.exitCode=1;});
"""
        result = node(script)
        self.assertEqual(result["used"], "Capacity unavailable")
        self.assertIn("Unknown", result["free"])
        self.assertEqual(result["count"], "—")
        self.assertTrue(result["disabled"])
        self.assertIsNone(result["comparison"])
        self.assertEqual(result["requestId"], 5)
        self.assertIn("card removed", result["error"])

    def test_comparison_shows_both_filenames_without_changing_table(self):
        script = HELPERS + COMPARE + DOM + r"""
const readFlightMetadata=async name=>({samples:name==="FLT0001.BIN"?"0":"2"});
(async()=>{
 $("compareA").value="FLT0001.BIN";$("compareB").value="FLT0003.BIN";
 await compareFlights();
 console.log(JSON.stringify({
  source:$("comparisonSources").textContent,
  rows:$("comparisonResults").children.length,
  first:$("comparisonResults").children[0].children[1].textContent
 }));
})().catch(e=>{console.error(e);process.exitCode=1;});
"""
        result = node(script)
        self.assertIn("First: FLT0001.BIN", result["source"])
        self.assertIn("Second: FLT0003.BIN", result["source"])
        self.assertEqual(result["rows"], 1)
        self.assertEqual(result["first"], "0 / 2")


if __name__ == "__main__":
    unittest.main()
