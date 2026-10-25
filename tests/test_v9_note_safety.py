#!/usr/bin/env python3
"""Regressions for BIN-only notes and Storage actions."""
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


def run_node(script):
    p = subprocess.run(["node", "-"], input=script, text=True,
                       capture_output=True, check=True, timeout=15)
    return json.loads(p.stdout)


LISTING = section(DASH, "async function loadFiles() {", '\n$("metric").onclick=')
HELPERS = section(DASH, "function bytes(n) {", "// Keep blank CSV measurements")
DOM = r"""
function element(){
 return {children:[],textContent:"",value:"",title:"",disabled:false,selectedIndex:0,
  replaceChildren(){this.children=[];this.textContent="";},
  append(...v){this.children.push(...v);},appendChild(v){this.children.push(v);}};
}
const ids={};const $=id=>ids[id]||(ids[id]=element());
const document={createElement:()=>element()};
const state={sdVerified:true,recording:false};
let comparisonMeta=null,comparisonRequestId=0,selectedFlight="FLT0001.BIN";
"""


class FirmwareNoteSafety(unittest.TestCase):
    def test_missing_txt_rejected_before_append(self):
        code = section(INO, "void noteAPI() {", "\nvoid diagnosticsAPI() {")
        self.assertIn('if (!SD.exists(summaryPath))', code)
        self.assertIn('TXT summary missing; original BIN unchanged', code)
        self.assertLess(code.index('if (!SD.exists(summaryPath))'),
                        code.index('SD.open(summaryPath, FILE_APPEND)'))
        self.assertNotIn('SD.open("/" + companionFile(n), FILE_APPEND)', code)

    def test_incomplete_note_append_not_reported_as_success(self):
        code = section(INO, "void noteAPI() {", "\nvoid diagnosticsAPI() {")
        self.assertIn('const size_t written = f.print(line);', code)
        self.assertIn('if (written != line.length())', code)
        self.assertIn('sdVerified = false;', code)
        self.assertIn('Note write incomplete; inspect TXT and retest SD', code)
        self.assertLess(code.index('if (written != line.length())'),
                        code.index('respond(true, "Note saved with flight")'))


@unittest.skipUnless(shutil.which("node"), "Node.js required")
class BrowserNoteSafety(unittest.TestCase):
    def test_missing_txt_actions_are_unavailable_but_bin_csv_remain(self):
        script = HELPERS + LISTING + DOM + r"""
const api=async()=>({total:8388608,used:100,files:[
 {name:"FLT0001.BIN",size:64,recordLengthOK:true,hasSummary:false,protected:false},
 {name:"FLT0002.BIN",size:64,recordLengthOK:true,hasSummary:true,protected:false}
]});
(async()=>{
 await loadFiles();
 const rows=$("fileList").children.map(row=>{
  const buttons=row.children[1].children;
  return buttons.map(b=>({text:b.textContent,disabled:b.disabled,title:b.title}));
 });
 console.log(JSON.stringify(rows));
})().catch(e=>{console.error(e);process.exitCode=1;});
"""
        rows = run_node(script)
        missing, complete = rows
        self.assertEqual(missing[0]["text"], "View data")
        self.assertFalse(missing[1]["disabled"])  # BIN
        self.assertFalse(missing[2]["disabled"])  # CSV
        self.assertTrue(missing[3]["disabled"])  # TXT
        self.assertTrue(missing[5]["disabled"])  # Add note
        self.assertIn("original TXT", missing[5]["title"])
        self.assertEqual(complete[0]["text"], "View stats")
        self.assertFalse(complete[3]["disabled"])
        self.assertFalse(complete[5]["disabled"])


    def test_refresh_reenables_notes_only_when_txt_appears(self):
        script = HELPERS + LISTING + DOM + r"""
let refresh=0;
$("fileSort").value="size";
const api=async()=>{
 refresh++;
 return {total:8388608,used:100,files:[
  {name:"FLT0001.BIN",size:64,recordLengthOK:true,
   hasSummary:refresh>1,protected:false},
  {name:"FLT0002.BIN",size:128,recordLengthOK:true,
   hasSummary:true,protected:false}
 ]};
};
function noteDisabled(){
 const row=$("fileList").children.find(row=>row.children[0].textContent.startsWith("FLT0001"));
 return row.children[1].children[5].disabled;
}
(async()=>{
 await loadFiles();const first=noteDisabled();
 await loadFiles();const second=noteDisabled();
 console.log(JSON.stringify({first,second,refresh}));
})().catch(e=>{console.error(e);process.exitCode=1;});
"""
        result = run_node(script)
        self.assertTrue(result["first"])
        self.assertFalse(result["second"])
        self.assertEqual(result["refresh"], 2)


if __name__ == "__main__":
    unittest.main()
