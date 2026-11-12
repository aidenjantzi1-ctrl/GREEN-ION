import test from "node:test";
import assert from "node:assert/strict";
import {readFileSync} from "node:fs";
import {analyzeOriginals} from "../web/v15_analysis_core.mjs";
import {searchHelp,TOPICS} from "../web/v15_help.mjs";

function sampleRkt4(n=150){
 const raw=new Uint8Array(32+16*n),v=new DataView(raw.buffer);
 raw.set([82,75,84,52]);
 v.setUint16(4,2,true);v.setUint16(6,32,true);
 v.setUint16(8,16,true);v.setUint16(10,50,true);
 v.setFloat32(12,101325,true);v.setUint16(16,4000,true);
 for(let i=0;i<n;i++){
  const o=32+i*16;
  v.setUint32(o,i*20000,true);
  v.setFloat32(o+4,101325-Math.sin(i/150*Math.PI)*600,true);
  v.setFloat32(o+8,24,true);
  v.setUint16(o+12,4000-Math.floor(i/50),true);
  v.setUint16(o+14,1,true);
 }
 return raw.buffer;
}
test("advanced local report analyzes original v2 recording without cloud",()=>{
 const source=sampleRkt4();
 const copy=source.slice(0);
 const result=analyzeOriginals(source,null,{name:"V12D001.BIN",checksum:"a".repeat(64)});
 assert.equal(result.schema,"GREEN-ION-V15-REPORT-1");
 assert.equal(result.observed.sampleCount,150);
 assert.equal(result.observed.validPressureSamples,150);
 assert.equal(result.observed.timestampGaps,0);
 assert.equal(result.observed.trailingBytes,0);
 assert.ok(result.estimates.apogeeM>0);
 assert.equal(result.observed.startBatteryV,4);
 assert.equal(result.sensors,null);
 assert.equal(result.dataCompletenessScore,100);
 assert.ok(result.limitations.includes("NOT")||result.limitations.includes("not"));
 assert.deepEqual(source,copy,"analysis must not mutate original bytes");
});
test("truncated RKT4 and missing samples reduce completeness and report warnings",()=>{
 const raw=sampleRkt4(10);
 const odd=new Uint8Array(raw.byteLength+1);odd.set(new Uint8Array(raw));
 const report=analyzeOriginals(odd.buffer,null,{name:"TEST.BIN"});
 assert.equal(report.observed.trailingBytes,1);
 assert.ok(report.dataCompletenessScore<100);
 assert.equal(report.evidenceStatus,"REVIEW");
 assert.ok(report.warnings.some(x=>/truncat/i.test(x)));
});
test("offline help answers where to find recovery, preflight and cloud sync",()=>{
 assert.equal(searchHelp("how do I find the rocket")[0].tab,"flight");
 assert.equal(searchHelp("run full preflight")[0].tab,"testing");
 assert.ok(searchHelp("when internet returns cloud backup").some(x=>x.tab==="system"));
 const html=readFileSync(new URL("../web/index.html",import.meta.url),"utf8");
 for(const item of TOPICS){
  assert.ok(html.includes('data-tab="'+item.tab+'"'),item.title);
  assert.ok(html.includes('id="'+item.id+'"'),item.title);
 }
});
test("battery estimate never certifies flight from voltage alone",async()=>{
 globalThis.window={GREEN_ION_TRANSPORT:{}};
 const {estimateBattery}=await import("../web/v15_battery.mjs");
 assert.equal(estimateBattery({voltage:3.4}).state,"NOT READY");
 assert.equal(estimateBattery({voltage:3.9,capacityMah:0,drawMa:0,requiredMinutes:30}).state,"NOT VERIFIED");
 const r=estimateBattery({voltage:4,capacityMah:500,drawMa:150,requiredMinutes:30});
 assert.equal(r.state,"ESTIMATE ONLY");
 assert.ok(r.reason.includes("NOT flight approval"));
});
test("one-button preflight and local sync are explicit, removable modules",()=>{
 const html=readFileSync(new URL("../web/index.html",import.meta.url),"utf8");
 for(const id of ["homeAuto","homeAutoStatus","v15RecoveryTrack","v15LinkCard",
  "v15ReportCard","v15LocalCard","v15BatteryCard","v15HelpCard"]){
  assert.ok(html.includes('id="'+id+'"'),id);
 }
 const app=readFileSync(new URL("../web/v14_app.mjs",import.meta.url),"utf8");
 for(const module of ["v15_recovery.mjs","v15_advanced.mjs","v15_link.mjs",
  "v15_local.mjs","v15_battery.mjs","v15_help.mjs"]){
  assert.ok(app.includes(module),module);
 }
});
