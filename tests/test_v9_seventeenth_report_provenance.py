#!/usr/bin/env python3
"""Regression checks for saved-report provenance and Storage focus."""
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


SUMMARY = section(DASH, "async function showFlightStats(name,scrollToReport=true) {", "\nfunction safeHTML(v)")
PERFORMANCE = section(DASH, "function renderFlightPerformance(rows){", "\nfunction drawSavedGraph(){")
LISTING = section(DASH, "async function loadFiles() {", '\n$("metric").onclick=')
HELPERS = section(DASH, "function bytes(n) {", "// Keep blank CSV measurements")


def node_result(script):
    p = subprocess.run(["node", "-"], input=script, text=True,
                       capture_output=True, check=True, timeout=15)
    return json.loads(p.stdout)


DOM = r"""
function element(){return {children:[],textContent:"",value:"",disabled:false,selectedIndex:0,
 replaceChildren(){this.children=[];this.textContent="";},
 append(...v){this.children.push(...v);},
 appendChild(v){this.children.push(v);},
 querySelectorAll(){return [];},
 scrollIntoView(){},
 get childElementCount(){return this.children.length;}};}
const ids={};const $=id=>ids[id]||(ids[id]=element());
const document={createElement:()=>element()};
const window={};
"""


class FirmwareSafety(unittest.TestCase):
    def test_firmware_rejects_zero_numbered_recording(self):
        code = section(INO, "bool validFile(String n) {", "\nString companionFile(String n)")
        self.assertIn('n.substring(3, 7) != "0000"', code)
        self.assertIn("(?!0000)", DASH)

    def test_protect_does_not_append_to_existing_lock(self):
        code = section(INO, "void protectAPI() {", "\n// Limit metadata line lengths")
        self.assertLess(code.index("if (SD.exists(lock)) ok = true;"),
                        code.index("SD.open(lock, FILE_WRITE)"))


@unittest.skipUnless(shutil.which("node"), "Node.js required")
class ReportProvenance(unittest.TestCase):
    def test_missing_txt_clears_previous_sample_counts(self):
        script = DOM + SUMMARY + r"""
let selectedFlight="",flightSummaryRequestId=0,graphRequestId=0,
 savedFlightSeries=[],loadedFlightName="",flightMeta={},flightPhases=[];
window.currentFlightSamples="100";window.currentFlightMissed="0";
window.currentFlightInvalid="0";window.currentFlightSampleRate="25";
const imperial=false;function drawSavedGraph(){}
const loaded=[];function loadSavedGraph(n){loaded.push(n);}
const fetch=async()=>({ok:false,status:404,text:async()=>"Missing"});
(async()=>{
 await showFlightStats("FLT0002.BIN",false);
 console.log(JSON.stringify({counts:[window.currentFlightSamples,
 window.currentFlightMissed,window.currentFlightInvalid,window.currentFlightSampleRate],
 loaded,selected:selectedFlight}));
})().catch(e=>{console.error(e);process.exitCode=1;});
"""
        result = node_result(script)
        self.assertEqual(result["counts"], [None, None, None, None])
        self.assertEqual(result["loaded"], ["FLT0002.BIN"])

    def test_completeness_requires_valid_saved_counts(self):
        script = DOM + PERFORMANCE + r"""
let flightPhases=[];
const rows=Array.from({length:8},(_,i)=>[i,100000,20,4,1,i,0]);
function validFlightSegments(rs){return [rs];}
function extreme(v,minimum){return minimum?Math.min(...v):Math.max(...v);}
function completeness(){const target=$("flightPerformance");target.replaceChildren();
 renderFlightPerformance(rows);
 return target.children.find(r=>r.children?.[0]?.textContent==="Recording completeness")?.children[1].textContent;}
const missing=completeness();
window.currentFlightSamples="10";window.currentFlightMissed="2";window.currentFlightInvalid="1";
const valid=completeness();
window.currentFlightInvalid="99";const impossible=completeness();
window.currentFlightInvalid="";const blank=completeness();
window.currentFlightInvalid="-1";const negative=completeness();
console.log(JSON.stringify({missing,valid,impossible,blank,negative}));
"""
        result = node_result(script)
        self.assertEqual(result["missing"], "Not determined")
        self.assertEqual(result["valid"], "75.0 %")
        self.assertEqual(result["impossible"], "Not determined")
        self.assertEqual(result["blank"], "Not determined")
        self.assertEqual(result["negative"], "Not determined")

    def test_delete_of_other_recording_preserves_current_report(self):
        script = DOM + HELPERS + LISTING + r"""
let selectedFlight="FLT0001.BIN",loadedFlightName=selectedFlight,
 savedFlightSeries=[[0]],flightPhases=[["Launch detected",1,5]],
 flightMeta={label:"KEEP"},graphRequestId=0,flightSummaryRequestId=0,
 comparisonMeta=null,comparisonRequestId=0;
const state={sdVerified:true,recording:false};
const confirm=()=>true,alert=()=>{throw Error("unexpected alert");};
const poll=async()=>{};
const api=async()=>({total:8388608,used:1024,files:[
 {name:"FLT0001.BIN",size:64,hasSummary:true,protected:false},
 {name:"FLT0002.BIN",size:64,hasSummary:true,protected:false}]});
(async()=>{
 await loadFiles();
 const row=$("fileList").children.find(r=>r.children[0].textContent.startsWith("FLT0002"));
 await row.children[1].children[6].onclick();
 console.log(JSON.stringify({selected:selectedFlight,meta:flightMeta,
 phases:flightPhases,graph:loadedFlightName}));
})().catch(e=>{console.error(e);process.exitCode=1;});
"""
        result = node_result(script)
        self.assertEqual(result["selected"], "FLT0001.BIN")
        self.assertEqual(result["meta"], {"label": "KEEP"})
        self.assertEqual(result["graph"], "FLT0001.BIN")

    def test_note_double_click_does_not_duplicate_request(self):
        script = DOM + HELPERS + LISTING + r"""
let selectedFlight="",comparisonMeta=null,comparisonRequestId=0;
const state={sdVerified:true,recording:false};
const prompt=()=>"single note",alert=()=>{throw Error("unexpected alert");};
const showFlightStats=async()=>{};
let noteRequests=0,finish;
const api=async(path)=>{
 if(path.startsWith("note?")){noteRequests++;return new Promise(r=>{finish=r;});}
 return {total:1000,used:10,files:[{name:"FLT0001.BIN",size:64,
  hasSummary:true,protected:false}]};
};
(async()=>{
 await loadFiles();
 const note=$("fileList").children[0].children[1].children[5];
 const first=note.onclick(),second=note.onclick();
 finish({success:true});await Promise.all([first,second]);
 console.log(JSON.stringify({requests:noteRequests,disabled:note.disabled}));
})().catch(e=>{console.error(e);process.exitCode=1;});
"""
        result = node_result(script)
        self.assertEqual(result["requests"], 1)
        self.assertFalse(result["disabled"])


if __name__ == "__main__":
    unittest.main()
