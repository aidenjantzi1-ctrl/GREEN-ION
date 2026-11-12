import test from "node:test";
import assert from "node:assert/strict";
import {readFileSync} from "node:fs";
import {visualStateFromTelemetry,V162_VISUAL_LABEL} from "../web/v16_2_dashboard.mjs";

const html=readFileSync(new URL("../web/index.html",import.meta.url),"utf8");
const css=readFileSync(new URL("../web/v14_layout.css",import.meta.url),"utf8");
const app=readFileSync(new URL("../web/v14_app.mjs",import.meta.url),"utf8");
const map=readFileSync(new URL("../web/v16_2_map.mjs",import.meta.url),"utf8");
const sd=readFileSync(new URL("../web/v16_2_sd_manager.mjs",import.meta.url),"utf8");
const testing=readFileSync(new URL("../web/v16_2_testing.mjs",import.meta.url),"utf8");
const exp=readFileSync(new URL("../web/v16_2_experience.mjs",import.meta.url),"utf8");
const sw=readFileSync(new URL("../web/service-worker.js",import.meta.url),"utf8");
const cloud=readFileSync(new URL("../web/v14_cloud.mjs",import.meta.url),"utf8");

test("V16.2 keeps exactly five visible tabs and the original blue background",()=>{
 const block=html.match(/<nav class="tabs v16-tabs"[^>]*>([\s\S]*?)<\/nav>/)?.[1]||"";
 const labels=[...block.matchAll(/<button[^>]*>([\s\S]*?)<\/button>/g)].map(x=>x[1].replace(/&amp;/g,"&").trim());
 assert.deepEqual(labels,["Dashboard","Flight","Flights & Analysis","Testing","Settings & Advanced"]);
 assert.match(html,/--bg:#081420/);assert.match(css,/V16\.2/);
});

test("Dashboard live visualization distinguishes live delayed and illustrative effects",()=>{
 assert.equal(V162_VISUAL_LABEL,"Flight Visualization — Sensor-Based / Estimated");
 const now=10_000;
 const live=visualStateFromTelemetry({
  primary_source:"wifi",links:{wifi:{freshness:"live",age_ms:100}},
  readings:{
   altitude_m:{value:120,source:"wifi",freshness:"live",age_ms:100},
   gps_speed_mps:{value:42,source:"wifi",freshness:"live",age_ms:100},
   flight_state:{value:3,source:"wifi",freshness:"live",age_ms:100},
   battery_v:{value:3.91,source:"wifi",freshness:"live",age_ms:100}
  }},now);
 assert.equal(live.live,true);assert.equal(live.phase,"Ascent");assert.equal(live.flameIllustrative,true);
 const delayed=visualStateFromTelemetry({
  primary_source:"wifi",links:{wifi:{freshness:"stale",age_ms:9000}},
  readings:{altitude_m:{value:120,freshness:"stale",age_ms:9000}}
 },now);
 assert.equal(delayed.live,false);assert.equal(delayed.stale,true);
});

test("V16.2 app uses true WebGL viewer and integrated new modules",()=>{
 for(const module of ["v16_2_dashboard","v16_2_webgl","v16_2_map","v16_2_sd_manager","v16_2_compare","v16_2_testing","v16_2_experience","v16_2_scorecard"])
  assert.ok(app.includes(module),module);
 assert.doesNotMatch(app,/initV16Visualization\(\)/);
});

test("embedded Google Maps is optional and no private key is committed",()=>{
 assert.match(map,/googleMapsBrowserKey/);
 assert.match(map,/mapTypeId:"satellite"/);
 assert.match(map,/terrain/);
 assert.match(map,/confirmed received rocket GPS fixes/i);
 assert.doesNotMatch(map,/AIza[0-9A-Za-z_-]{20,}/);
 assert.doesNotMatch(cloud,/service_role/i);
});

test("SD manager requires completion backup evidence and typed exact filename",()=>{
 assert.match(sd,/recording_complete/);
 assert.match(sd,/Browser copy:/);
 assert.match(sd,/Windows archive:/);
 assert.match(sd,/Cloud backup:/);
 assert.match(sd,/Type the exact filename/);
 assert.match(sd,/v162DeleteConfirm/);
 assert.match(sd,/companions first, BIN last/);
 assert.match(sd,/recheck/i);
});

test("Testing presents one primary full home test and keeps detailed tools expandable",()=>{
 assert.match(testing,/RUN FULL HOME TEST/);
 assert.match(testing,/Detailed tests & manual tools/);
 for(const state of ["PASS","FAIL","PENDING","INCONCLUSIVE","NOT TESTED"])assert.ok(testing.includes(state),state);
});

test("Balanced presentation, profiles, first run and next action are local presentation features",()=>{
 assert.match(exp,/green-ion-v16\.2-active-profile/);
 assert.match(exp,/green-ion-v16\.2-first-run-complete/);
 assert.match(exp,/Sensor nose direction/);
 assert.match(exp,/V16\.2 Feather recorder is fixed at the validated 50 Hz cadence/);
 assert.match(exp,/nextAction/);
 assert.match(css,/data-v162-visibility="simple"/);
 assert.match(sw,/v16_2_webgl\.mjs/);
});
