import test from "node:test";
import assert from "node:assert/strict";
import {readFileSync} from "node:fs";

const html=readFileSync(new URL("../web/index.html",import.meta.url),"utf8");
const nav=readFileSync(new URL("../web/v16_navigation.mjs",import.meta.url),"utf8");

test("V16 exposes exactly five top-level tabs",()=>{
 const block=html.match(/<nav class="tabs"[^>]*>([\s\S]*?)<\/nav>/)?.[1]||"";
 const labels=[...block.matchAll(/<button[^>]*>([^<]+)<\/button>/g)].map(m=>m[1].trim());
 assert.deepEqual(labels,["Mission","Live","Recovery","Data","System"]);
});

test("legacy pages remain available behind contextual navigation",()=>{
 for(const page of ["flight","testing","field","storage","cloud","utilities","updates","help"]){
  assert.match(html,new RegExp('data-tab="'+page+'"'));
  assert.match(nav,new RegExp('\\["'+page+'"'));
 }
});

test("old pages map into five primary groups",()=>{
 for(const pair of [
  ["flight","status"],["testing","status"],["field","connections"],
  ["storage","history"],["cloud","history"],["updates","system"],["help","system"]
 ]){
  assert.match(html,new RegExp(pair[0]+':"'+pair[1]+'"'));
 }
 assert.match(nav,/Alt\+1|altKey/);
});
