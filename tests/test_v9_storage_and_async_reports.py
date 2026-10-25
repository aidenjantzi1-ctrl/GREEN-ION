#!/usr/bin/env python3
"""Regression checks for large SD cards, empty CSV files and stale browser responses."""
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


def node_result(script):
    result = subprocess.run(
        ["node", "-"], input=script, text=True, capture_output=True,
        check=True, timeout=15,
    )
    return json.loads(result.stdout)


class SdAndCsvTests(unittest.TestCase):
    def test_storage_totals_are_not_truncated_to_32_bits(self):
        listing = section(INO, "void filesAPI() {", "\nvoid downloadAPI() {")
        self.assertIn("u64Text(SD.totalBytes())", listing)
        self.assertIn("u64Text(SD.usedBytes())", listing)
        self.assertNotIn("(uint32_t)SD.totalBytes()", listing)
        self.assertNotIn("(uint32_t)SD.usedBytes()", listing)

    def test_csv_refuses_header_only_and_partial_first_record(self):
        csv = section(INO, "void csvAPI() {", "\nvoid metadataAPI() {")
        self.assertIn("f.size() < sizeof(header) + 16U", csv)
        self.assertIn('web.send(422, "text/plain", "RKT4 recording has no complete samples")', csv)
        self.assertLess(csv.index("f.size() < sizeof(header) + 16U"),
                        csv.index('web.send(200, "text/csv", "")'))


@unittest.skipUnless(shutil.which("node"), "Node.js required for asynchronous browser tests")
class BrowserRaceTests(unittest.TestCase):
    def test_older_summary_cannot_replace_newer_selected_flight(self):
        summary = section(DASH, "async function showFlightStats(name,scrollToReport=true) {",
                          "\nfunction safeHTML(v)")
        harness = r"""
function element(){return {children:[],textContent:"",
 replaceChildren(){this.children=[];this.textContent="";},
 appendChild(v){this.children.push(v);},
 append(...v){this.children.push(...v);},
 scrollIntoView(){},
 get childElementCount(){return this.children.length;}};}
const elements={};
const $=id=>elements[id]||(elements[id]=element());
const document={createElement:()=>element()};
const window={};
let selectedFlight="",savedFlightSeries=[],loadedFlightName="",flightMeta={},flightPhases=[];
let graphRequestId=0,flightSummaryRequestId=0,imperial=false;
function drawSavedGraph(){}
const loaded=[];
function loadSavedGraph(name){loaded.push(name);}
const pending={};
const fetch=url=>new Promise(resolve=>{pending[url]=resolve;});
(async()=>{
 const old=showFlightStats("FLT0001.BIN",false);
 const newer=showFlightStats("FLT0002.BIN",false);
 pending["/api/flightstats?name=FLT0002.BIN"]({
  ok:true,text:async()=> "label=NEW FLIGHT\nsamples=10\ninvalid_samples=0\n"});
 await newer;
 pending["/api/flightstats?name=FLT0001.BIN"]({
  ok:true,text:async()=> "label=OLD FLIGHT\nsamples=5\ninvalid_samples=0\n"});
 await old;
 console.log(JSON.stringify({
  selected:selectedFlight,label:flightMeta.label,loaded,
  status:$("flightSummaryStatus").textContent
 }));
})().catch(e=>{console.error(e);process.exitCode=1;});
"""
        data = node_result(summary + harness)
        self.assertEqual(data["selected"], "FLT0002.BIN")
        self.assertEqual(data["label"], "NEW FLIGHT")
        self.assertEqual(data["loaded"], ["FLT0002.BIN"])
        self.assertIn("FLT0002.BIN", data["status"])

    def test_older_comparison_cannot_replace_newer_pair(self):
        compare = section(DASH, "async function compareFlights(){",
                          "\nasync function showFlightStats(name,scrollToReport=true)")
        harness = r"""
function element(){return {children:[],textContent:"",value:"",
 replaceChildren(){this.children=[];this.textContent="";},
 appendChild(v){this.children.push(v);},append(...v){this.children.push(...v);}};}
const elements={compareA:element(),compareB:element(),comparisonResults:element()};
const $=id=>elements[id];
const document={createElement:()=>element()};
let comparisonRequestId=0,comparisonMeta=null;
const pending={};
const readFlightMetadata=name=>new Promise(resolve=>{pending[name]=resolve;});
(async()=>{
 $("compareA").value="FLT0001.BIN";$("compareB").value="FLT0002.BIN";
 const old=compareFlights();
 $("compareA").value="FLT0003.BIN";$("compareB").value="FLT0004.BIN";
 const newer=compareFlights();
 pending["FLT0003.BIN"]({motor:"NEW A",samples:"20"});
 pending["FLT0004.BIN"]({motor:"NEW B",samples:"21"});
 await newer;
 pending["FLT0001.BIN"]({motor:"OLD A",samples:"10"});
 pending["FLT0002.BIN"]({motor:"OLD B",samples:"11"});
 await old;
 console.log(JSON.stringify({a:comparisonMeta.a,b:comparisonMeta.b,
  results:$("comparisonResults").children.length}));
})().catch(e=>{console.error(e);process.exitCode=1;});
"""
        helpers = section(DASH, "function bytes(n) {", "// Keep blank CSV measurements")
        data = node_result(helpers + compare + harness)
        self.assertEqual(data["a"], "FLT0003.BIN")
        self.assertEqual(data["b"], "FLT0004.BIN")
        self.assertGreater(data["results"], 0)


if __name__ == "__main__":
    unittest.main()
