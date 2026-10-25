"""Regression checks for validated Storage listings and bounded flight-note metadata."""
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


HELPERS = section(DASH, "function bytes(n) {", "// Keep blank CSV measurements")
LISTING = section(DASH, "async function loadFiles() {", '\n$("metric").onclick=')
DOM = r"""
function element(){
 return {children:[],textContent:"",value:"",disabled:false,selectedIndex:0,
  replaceChildren(){this.children=[];this.textContent="";},
  append(...v){this.children.push(...v);},
  appendChild(v){this.children.push(v);}};
}
const ids={};const $=id=>ids[id]||(ids[id]=element());
const document={createElement:()=>element()};
const state={sdVerified:true,recording:false};
let comparisonMeta=null,comparisonRequestId=0,selectedFlight="FLT0001.BIN";
$("fileSort").value="size";
let response;
const api=async()=>response;
const good=name=>({name,size:64,recordLengthOK:true,hasSummary:true,protected:false});
"""


def run_node(script):
    proc = subprocess.run(["node", "-"], input=script, text=True,
                          capture_output=True, check=True, timeout=15)
    return json.loads(proc.stdout)


class FirmwareNoteBoundaries(unittest.TestCase):
    def test_line_reader_has_hard_limit_and_no_unbounded_read(self):
        helper = section(INO, "bool readLimitedMetadataLine(File &file, String &line) {",
                         "\nvoid noteAPI() {")
        note = section(INO, "void noteAPI() {", "\nvoid diagnosticsAPI() {")
        self.assertIn("line.length() >= 160", helper)
        self.assertIn("readLimitedMetadataLine(existing, header)", note)
        self.assertIn("readLimitedMetadataLine(existing, entry)", note)
        self.assertIn("if (!readable || header !=", note)
        self.assertNotIn("readStringUntil", note)
        self.assertLess(note.index("existing.close();"), note.index("SD.open(summaryPath, FILE_APPEND)"))

    def test_pdf_metadata_wraps_instead_of_overflowing(self):
        self.assertIn("overflow-wrap:anywhere;word-break:break-word", DASH)


@unittest.skipUnless(shutil.which("node"), "Node.js required")
class BrowserFileValidation(unittest.TestCase):
    def test_non_array_file_list_fails_safely(self):
        script = HELPERS + LISTING + DOM + r"""
(async()=>{
 response={total:1000,used:100,files:{name:"FLT0001.BIN"}};
 await loadFiles();
 console.log(JSON.stringify({error:$("fileList").textContent,
  count:$("fileCount").textContent,rows:$("fileList").children.length}));
})().catch(e=>{console.error(e);process.exitCode=1;});
"""
        data = run_node(script)
        self.assertIn("Invalid file list response", data["error"])
        self.assertEqual(data["count"], "—")
        self.assertEqual(data["rows"], 0)

    def test_duplicate_or_invalid_names_offer_no_actions(self):
        script = HELPERS + LISTING + DOM + r"""
(async()=>{
 response={total:1000,used:100,files:[good("FLT0001.BIN"),good("FLT0001.BIN")]};
 await loadFiles();const duplicate=$("fileList").textContent;
 response={total:1000,used:100,files:[good("../FLT0001.BIN")]};
 await loadFiles();
 console.log(JSON.stringify({duplicate,invalid:$("fileList").textContent,
  rows:$("fileList").children.length}));
})().catch(e=>{console.error(e);process.exitCode=1;});
"""
        data = run_node(script)
        self.assertIn("Duplicate recording names", data["duplicate"])
        self.assertIn("Invalid recording name", data["invalid"])
        self.assertEqual(data["rows"], 0)

    def test_bad_size_is_labeled_unknown_without_losing_download(self):
        script = HELPERS + LISTING + DOM + r"""
(async()=>{
 response={total:1000,used:100,files:[{...good("FLT0001.BIN"),size:-5}]};
 await loadFiles();
 const row=$("fileList").children[0];
 console.log(JSON.stringify({label:row.children[0].textContent,
  actions:row.children[1].children.length,count:$("fileCount").textContent}));
})().catch(e=>{console.error(e);process.exitCode=1;});
"""
        data = run_node(script)
        self.assertIn("Size unavailable", data["label"])
        self.assertEqual(data["actions"], 7)
        self.assertEqual(data["count"], 1)

    def test_mutation_buttons_disable_during_network_request(self):
        listing = LISTING
        self.assertIn("rm.disabled=true;protect.disabled=true;note.disabled=true;", listing)
        self.assertIn("protect.disabled=true;rm.disabled=true;note.disabled=true;", listing)
        self.assertGreaterEqual(listing.count("++globalThis.fileListRequestId;"), 2)


if __name__ == "__main__":
    unittest.main()
