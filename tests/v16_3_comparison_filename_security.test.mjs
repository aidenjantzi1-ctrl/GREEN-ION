import test from "node:test";
import assert from "node:assert/strict";
import {readFileSync} from "node:fs";
import {makeComparisonHeader} from "../web/v16_2_compare.mjs";

function fakeDocument(){
 return {createElement(tag){return {
  tag, textContent:"", children:[], append(node){this.children.push(node);}
 };}};
}

test("flight comparison treats imported filenames as literal text, never HTML",()=>{
 const injected='</th><img src=x onerror=alert(1)>';
 const row=makeComparisonHeader(["Flight 1.BIN",injected],fakeDocument());
 assert.equal(row.tag,"tr");
 assert.deepEqual(row.children.map(cell=>cell.tag),["th","th","th"]);
 assert.deepEqual(row.children.map(cell=>cell.textContent),
   ["Metric","Flight 1.BIN",injected]);
 assert.ok(row.children.every(cell=>cell.children.length===0));
});

test("comparison renderer invokes the safe DOM header helper",()=>{
 const source=readFileSync(new URL("../web/v16_2_compare.mjs",import.meta.url),"utf8");
 assert.match(source,/makeComparisonHeader\(results\.map\(x=>x\.name\)\)/);
 assert.doesNotMatch(source,/head\.innerHTML/);
});
