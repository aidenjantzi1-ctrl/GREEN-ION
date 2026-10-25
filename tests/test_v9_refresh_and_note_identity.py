"""Storage refresh ordering and safe note-target identity checks."""
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


HELPERS = section(DASH, "function bytes(n) {", "// Keep blank CSV measurements")
LISTING = section(DASH, "async function loadFiles() {", '\n$("metric").onclick=')
DOM = r"""
function element(){
 return {children:[],textContent:"",value:"",title:"",disabled:false,selectedIndex:0,
  replaceChildren(){this.children=[];this.textContent="";},
  append(...v){this.children.push(...v);},
  appendChild(v){this.children.push(v);}};
}
const ids={};const $=id=>ids[id]||(ids[id]=element());
const document={createElement:()=>element()};
const state={sdVerified:true,recording:false};
let comparisonMeta=null,comparisonRequestId=0,selectedFlight="FLT0001.BIN";
$("fileSort").value="new";
const pending=[];
const api=()=>new Promise((resolve,reject)=>pending.push({resolve,reject}));
const file=(name)=>({name,size:64,recordLengthOK:true,hasSummary:true,protected:false});
const result=(files)=>({total:8388608,used:1024,files});
"""


def run_node(script):
    result = subprocess.run(
        ["node", "-"], input=script, text=True, capture_output=True,
        check=True, timeout=15
    )
    return json.loads(result.stdout)


class FirmwareNoteIdentity(unittest.TestCase):
    def test_existing_summary_must_have_header_and_matching_bin(self):
        code = section(INO, "void noteAPI() {", "\nvoid diagnosticsAPI() {")
        self.assertIn('SD.open(summaryPath, FILE_READ)', code)
        self.assertIn('header != "GREEN ION V9 FLIGHT METADATA"', code)
        self.assertIn('entry.startsWith("file=")', code)
        self.assertIn('matchingFile = entry.substring(5) == n;', code)
        self.assertIn("TXT summary incomplete or mismatched", code)
        self.assertLess(code.index("existing.close();"), code.index("SD.open(summaryPath, FILE_APPEND)"))

    def test_note_save_does_not_jump_to_report(self):
        self.assertIn("await showFlightStats(f.name,false);", DASH)

    def test_user_instructions_explain_note_refusal(self):
        readme = (ROOT / "README.md").read_text(encoding="utf-8")
        self.assertIn("existing TXT summary identifies the selected BIN", readme)
        self.assertIn("recorder refuses to append", readme)


@unittest.skipUnless(shutil.which("node"), "Node.js required")
class BrowserStorageOrdering(unittest.TestCase):
    def test_older_success_cannot_replace_newer_success(self):
        script = HELPERS + LISTING + DOM + r"""
(async()=>{
 const old=loadFiles(),latest=loadFiles();
 pending[1].resolve(result([file("FLT0001.BIN"),file("FLT0002.BIN")]));
 await latest;
 const fresh={count:$("fileCount").textContent,rows:$("fileList").children.length};
 pending[0].resolve(result([file("FLT0001.BIN")]));
 await old;
 console.log(JSON.stringify({fresh,final:{count:$("fileCount").textContent,
  rows:$("fileList").children.length}}));
})().catch(e=>{console.error(e);process.exitCode=1;});
"""
        data = run_node(script)
        self.assertEqual(data["fresh"]["rows"], 2)
        self.assertEqual(data["final"], data["fresh"])

    def test_older_error_cannot_clear_newer_success(self):
        script = HELPERS + LISTING + DOM + r"""
(async()=>{
 const old=loadFiles(),latest=loadFiles();
 pending[1].resolve(result([file("FLT0001.BIN")]));
 await latest;
 pending[0].reject(Error("stale Wi-Fi failure"));
 await old;
 console.log(JSON.stringify({count:$("fileCount").textContent,
  rows:$("fileList").children.length,storage:$("storageUsed").textContent}));
})().catch(e=>{console.error(e);process.exitCode=1;});
"""
        data = run_node(script)
        self.assertEqual(data["count"], 1)
        self.assertEqual(data["rows"], 1)
        self.assertIn("KiB", data["storage"])

    def test_older_success_cannot_hide_newer_failure(self):
        script = HELPERS + LISTING + DOM + r"""
(async()=>{
 const old=loadFiles(),latest=loadFiles();
 pending[1].reject(Error("SD disconnected"));
 await latest;
 pending[0].resolve(result([file("FLT0001.BIN")]));
 await old;
 console.log(JSON.stringify({count:$("fileCount").textContent,
  error:$("fileList").textContent}));
})().catch(e=>{console.error(e);process.exitCode=1;});
"""
        data = run_node(script)
        self.assertEqual(data["count"], "—")
        self.assertIn("SD disconnected", data["error"])


if __name__ == "__main__":
    unittest.main()
