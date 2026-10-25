#!/usr/bin/env python3
"""Regression tests for saved-flight comparison and storage display behavior."""
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


def run_node(script):
    result = subprocess.run(
        ["node", "-"], input=script, text=True, capture_output=True,
        check=True, timeout=15,
    )
    return json.loads(result.stdout)


class FirmwareAndMarkupTests(unittest.TestCase):
    def test_file_listing_avoids_uint32_size_cast(self):
        files = section(INO, "void filesAPI() {", "\nvoid downloadAPI() {")
        self.assertIn("u64Text((uint64_t)f.size())", files)
        self.assertNotIn("(uint32_t)f.size()", files)

    def test_comparison_is_visible_and_has_select_labels(self):
        self.assertIn('id="flightComparisonCard"', DASH)
        self.assertNotIn('style="display:none"><h2>Compare flights', DASH)
        self.assertIn('<label for="compareA">First recording</label>', DASH)
        self.assertIn('<label for="compareB">Second recording</label>', DASH)


@unittest.skipUnless(shutil.which("node"), "Node.js is needed for dashboard behavior tests")
class DashboardBehaviorTests(unittest.TestCase):
    def test_units_and_missing_metadata(self):
        helpers = section(DASH, "function bytes(n) {", "// Keep blank CSV measurements")
        script = helpers + r"""
const results = {
 units: [bytes(0), bytes(1024), bytes(1048576), bytes(8589934592), bytes(-1)],
 missing: [summaryValue(""), summaryValue("   "), summaryValue(null), summaryValue("NaN")],
 zero: summaryValue("0"),
 numericZero: comparisonNumber("0"),
 blankIsMissing: Number.isNaN(comparisonNumber("")),
 nullIsMissing: Number.isNaN(comparisonNumber(null)),
 textIsMissing: Number.isNaN(comparisonNumber("invalid"))
};
console.log(JSON.stringify(results));
"""
        result = run_node(script)
        self.assertEqual(result["units"], ["0 bytes", "1.0 KiB", "1.00 MiB", "8.00 GiB", "Unknown"])
        self.assertEqual(result["missing"], ["—"] * 4)
        self.assertEqual(result["zero"], "0")
        self.assertEqual(result["numericZero"], 0)
        self.assertTrue(result["blankIsMissing"])
        self.assertTrue(result["nullIsMissing"])
        self.assertTrue(result["textIsMissing"])

    def test_comparison_preserves_real_zero_and_shows_missing(self):
        helpers = section(DASH, "function bytes(n) {", "// Keep blank CSV measurements")
        compare = section(DASH, "async function compareFlights(){",
                          "\nasync function showFlightStats(name,scrollToReport=true)")
        harness = r"""
function element(){return {children:[],value:"",textContent:"",
 replaceChildren(){this.children=[];this.textContent="";},
 append(...v){this.children.push(...v);},appendChild(v){this.children.push(v);}};}
const ids={compareA:element(),compareB:element(),comparisonResults:element()};
const $=id=>ids[id]||(ids[id]=element());
const document={createElement:()=>element()};
let comparisonRequestId=0,comparisonMeta=null;
const readFlightMetadata=async name=>name==="FLT0001.BIN"
 ? {samples:"0",missed_samples:"",duration_s:"0",peak_altitude_m:""}
 : {samples:"2",missed_samples:"0",duration_s:"3",peak_altitude_m:"10"};
(async()=>{
 $("compareA").value="FLT0001.BIN";$("compareB").value="FLT0002.BIN";
 await compareFlights();
 const rows=$("comparisonResults").children.map(r=>({
   title:r.children[0].textContent,value:r.children[1].textContent
 }));
 console.log(JSON.stringify({rows,metadata:!!comparisonMeta}));
})().catch(e=>{console.error(e);process.exitCode=1;});
"""
        result = run_node(helpers + compare + harness)
        rows = {r["title"]: r["value"] for r in result["rows"]}
        self.assertTrue(result["metadata"])
        self.assertEqual(rows["Samples"], "0 / 2")
        self.assertEqual(rows["Missed samples"], "— / 0")
        self.assertEqual(rows["Duration (s)"], "0 / 3")
        self.assertEqual(rows["Peak altitude (m)"], "— / 10")

    def test_comparison_graph_refuses_blank_numeric_data(self):
        helpers = section(DASH, "function bytes(n) {", "// Keep blank CSV measurements")
        graph = section(DASH, "function compareBarGraph(a,b,key,title,unit){",
                        "\nfunction exportComparePDF(){")
        script = helpers + graph + r"""
const safeHTML=s=>String(s);
const document={createElement:()=>{throw Error("No canvas for missing values");}};
console.log(JSON.stringify({
 blank:compareBarGraph({samples:""},{samples:"0"},"samples","Samples","samples"),
 absent:compareBarGraph({},{samples:"0"},"samples","Samples","samples")
}));
"""
        result = run_node(script)
        self.assertIn("not enough data", result["blank"])
        self.assertIn("not enough data", result["absent"])

    def test_empty_storage_clears_comparison_and_bad_capacity(self):
        helpers = section(DASH, "function bytes(n) {", "// Keep blank CSV measurements")
        listing = section(DASH, "async function loadFiles() {", '\n$("metric").onclick=')
        script = helpers + listing + r"""
function element(){return {children:[],value:"",textContent:"",disabled:false,
 replaceChildren(){this.children=[];this.textContent="";}};}
const ids={};const $=id=>ids[id]||(ids[id]=element());
const state={sdVerified:true};let comparisonMeta={old:true},comparisonRequestId=3;
const api=async()=>({files:[],total:100,used:200});
(async()=>{
 await loadFiles();
 console.log(JSON.stringify({
 used:$("storageUsed").textContent,free:$("sdFreeSpace").textContent,
 disabled:$("compareFlights").disabled,meta:comparisonMeta,
 requestId:comparisonRequestId,
 message:$("fileList").textContent
 }));
})().catch(e=>{console.error(e);process.exitCode=1;});
"""
        result = run_node(script)
        self.assertEqual(result["used"], "Capacity unavailable")
        self.assertIn("Unknown", result["free"])
        self.assertTrue(result["disabled"])
        self.assertIsNone(result["meta"])
        self.assertEqual(result["requestId"], 4)
        self.assertIn("No flight files", result["message"])


if __name__ == "__main__":
    unittest.main()
