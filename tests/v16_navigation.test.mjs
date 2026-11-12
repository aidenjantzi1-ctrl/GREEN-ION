import test from "node:test";
import assert from "node:assert/strict";
import {readFileSync} from "node:fs";
import {PRIMARY_TABS,primaryTabForLegacy} from "../web/v16_navigation.mjs";
import {V16_TABS,v16TabFor} from "../web/v16_layout.mjs";

const html=readFileSync(new URL("../web/index.html",import.meta.url),"utf8");
const nav=readFileSync(new URL("../web/v16_navigation.mjs",import.meta.url),"utf8");
const layout=readFileSync(new URL("../web/v16_layout.mjs",import.meta.url),"utf8");

test("V16 exposes exactly the approved five top-level tabs",()=>{
 const block=html.match(/<nav class="tabs v16-tabs"[^>]*>([\s\S]*?)<\/nav>/)?.[1]||"";
 const entries=[...block.matchAll(/<button[^>]*data-tab="([^"]+)"[^>]*>([\s\S]*?)<\/button>/g)]
  .map(m=>[m[1],m[2].replace(/&amp;/g,"&").trim()]);
 assert.deepEqual(entries,[
  ["status","Dashboard"],
  ["flight","Flight"],
  ["history","Flights & Analysis"],
  ["testing","Testing"],
  ["system","Settings & Advanced"]
 ]);
 assert.deepEqual(PRIMARY_TABS,entries);
 assert.deepEqual(V16_TABS,entries);
});

test("legacy destinations map into the five visible tabs",()=>{
 const expected={
  status:"status",
  flight:"flight",field:"flight",recovery:"flight",
  storage:"history",history:"history",
  testing:"testing",
  utilities:"system",system:"system",connections:"system",
  cloud:"system",updates:"system",help:"system"
 };
 for(const [from,to] of Object.entries(expected)){
  assert.equal(primaryTabForLegacy(from),to,from);
  assert.equal(v16TabFor(from),to,from);
 }
});

test("legacy tab aliases remain hidden for compatibility only",()=>{
 const hidden=html.match(/<nav id="v16LegacyTabs" hidden[^>]*>([\s\S]*?)<\/nav>/)?.[1]||"";
 for(const page of ["field","storage","connections","recovery","cloud","updates","help"]){
  assert.match(hidden,new RegExp('data-tab="'+page+'"'));
 }
 assert.match(nav,/Alt\+1|altKey/);
});

test("five-tab layout keeps normal view concise and moves advanced cards",()=>{
 assert.match(html,/id="v16DashboardSummary"/);
 assert.match(html,/id="v16DashboardPrepare"/);
 assert.match(layout,/v16DashboardDetails/);
 assert.match(layout,/v16FieldControls/);
 assert.match(layout,/Profiles, sensors & recording/);
 assert.match(layout,/Connections, radio & diagnostics/);
 assert.match(layout,/Cloud backup/);
 assert.match(layout,/Firmware & app updates/);
 assert.match(layout,/Help & troubleshooting/);
});
