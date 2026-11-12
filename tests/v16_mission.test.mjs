import test from "node:test";
import assert from "node:assert/strict";
import {readFileSync} from "node:fs";
import {
 MISSION_SCHEMA,newMission,reduceMission,restoreMission,missionView
} from "../web/v16_mission.mjs";

function telemetry(phase,{gps="live",file="V12D042.BIN",mode="wifi+lora"}={}){
 return {
  schema:"GREEN-ION-V16-TELEMETRY-1",
  generated_utc:"2026-11-12T03:00:00Z",
  mode,
  primary_source:mode==="lora"?"lora":"wifi",
  identity:{rocket:"Green Eggs",motor:"D12-5",mission_file_id:file},
  readings:{
   flight_state:{value:phase,source:"wifi",freshness:"live",age_ms:100},
   gps_status:{value:gps,source:"wifi",freshness:gps==="live"?"live":"stale",age_ms:100}
  }
 };
}
function started(){
 return reduceMission(newMission(1000),{type:"START_MISSION",detail:{id:"m1",rocket:"Green Eggs",motor:"D12-5"}},2000);
}
function passedPreflight(mission){
 mission=reduceMission(mission,{type:"PREFLIGHT_RESULT",detail:{
  complete:true,fails:0,blocking_unverified:0,manual_unverified:3
 }},3000);
 return reduceMission(mission,{type:"CONFIRM_PHYSICAL"},4000);
}

test("mission starts in PREFLIGHT and ignores flight evidence while IDLE",()=>{
 const idle=newMission(1000,"idle");
 assert.equal(idle.schema,MISSION_SCHEMA);
 assert.equal(reduceMission(idle,{type:"TELEMETRY",detail:telemetry(3)},1100).state,"IDLE");
 assert.equal(reduceMission(idle,{type:"PREFLIGHT_RESULT",detail:{complete:true}},1200).state,"IDLE");
 const mission=started();
 assert.equal(mission.state,"PREFLIGHT");
 assert.equal(mission.id,"m1");
 assert.equal(mission.transition_log.at(-1).from,"IDLE");
});

test("measured failure blocks mission and can retry preflight",()=>{
 let mission=started();
 mission=reduceMission(mission,{type:"PREFLIGHT_RESULT",detail:{
  complete:true,fails:1,blocking_unverified:0,manual_unverified:3
 }},3000);
 assert.equal(mission.state,"ERROR");
 assert.match(missionView(mission).instruction,/blocked/i);
 mission=reduceMission(mission,{type:"RETRY_PREFLIGHT"},4000);
 assert.equal(mission.state,"PREFLIGHT");
 assert.equal(mission.preflight.complete,false);
});

test("receiver evidence cannot be overridden by physical confirmation",()=>{
 let mission=started();
 mission=reduceMission(mission,{type:"PREFLIGHT_RESULT",detail:{
  complete:true,fails:0,blocking_unverified:1,manual_unverified:3
 }},3000);
 assert.equal(mission.state,"PREFLIGHT");
 mission=reduceMission(mission,{type:"CONFIRM_PHYSICAL"},4000);
 assert.equal(mission.state,"PREFLIGHT");
 assert.match(mission.warnings.at(-1).text,/READY blocked/);
});

test("passing measured checks plus physical confirmation reaches READY then PRELAUNCH",()=>{
 let mission=passedPreflight(started());
 assert.equal(mission.state,"READY");
 assert.equal(mission.preflight.physical_confirmed,true);
 mission=reduceMission(mission,{type:"BEGIN_PRELAUNCH"},5000);
 assert.equal(mission.state,"PRELAUNCH");
 assert.match(missionView(mission).instruction,/Feather/);
});

test("Feather telemetry drives ascent descent landing and recovery",()=>{
 let mission=passedPreflight(started());
 mission=reduceMission(mission,{type:"BEGIN_PRELAUNCH"},5000);
 mission=reduceMission(mission,{type:"TELEMETRY",detail:telemetry(3,{gps:"missing"})},6000);
 assert.equal(mission.state,"ASCENT");
 assert.equal(mission.expected_flight_file,"V12D042.BIN");
 assert.match(mission.warnings.at(-1).text,/GPS is not live/);
 mission=reduceMission(mission,{type:"TELEMETRY",detail:telemetry(5)},7000);
 assert.equal(mission.state,"DESCENT");
 mission=reduceMission(mission,{type:"TELEMETRY",detail:telemetry(6)},8000);
 assert.equal(mission.state,"LANDED");
 mission=reduceMission(mission,{type:"BEGIN_RECOVERY"},9000);
 assert.equal(mission.state,"RECOVERY");
});

test("matching verified flight and report complete postflight workflow",()=>{
 let mission=passedPreflight(started());
 mission=reduceMission(mission,{type:"BEGIN_PRELAUNCH"},5000);
 mission=reduceMission(mission,{type:"TELEMETRY",detail:telemetry(7)},6000);
 assert.equal(mission.state,"RECOVERY");
 mission=reduceMission(mission,{type:"DOWNLOAD_START",detail:{name:"V12D042.BIN"}},7000);
 assert.equal(mission.state,"DOWNLOADING");
 mission=reduceMission(mission,{type:"FLIGHT_SAVED",detail:{name:"V12D042.BIN",checksum:"a".repeat(64)}},8000);
 assert.equal(mission.state,"ANALYZING");
 mission=reduceMission(mission,{type:"REPORT_COMPLETE",detail:{name:"V12D042.BIN",quality:98}},9000);
 assert.equal(mission.state,"COMPLETE");
 assert.equal(mission.report_quality,98);
});

test("unrelated archived flight cannot hijack the active mission",()=>{
 let mission=passedPreflight(started());
 mission=reduceMission(mission,{type:"BEGIN_PRELAUNCH"},5000);
 mission=reduceMission(mission,{type:"TELEMETRY",detail:telemetry(7)},6000);
 mission=reduceMission(mission,{type:"DOWNLOAD_START",detail:{name:"V12D001.BIN"}},7000);
 assert.equal(mission.state,"RECOVERY");
 assert.match(mission.warnings.at(-1).text,/unrelated archived flight/);
 mission=reduceMission(mission,{type:"FLIGHT_SAVED",detail:{name:"V12D001.BIN"}},8000);
 assert.equal(mission.state,"RECOVERY");
});

test("browser restart marks active workflow interrupted and fresh telemetry restores real phase",()=>{
 let mission=passedPreflight(started());
 mission=reduceMission(mission,{type:"BEGIN_PRELAUNCH"},5000);
 mission=reduceMission(mission,{type:"TELEMETRY",detail:telemetry(3)},6000);
 const restored=restoreMission(JSON.stringify(mission),7000);
 assert.equal(restored.state,"INTERRUPTED");
 assert.equal(restored.interrupted_from,"ASCENT");
 const resumed=reduceMission(restored,{type:"TELEMETRY",detail:telemetry(5)},8000);
 assert.equal(resumed.state,"DESCENT");
});

test("mission history is bounded",()=>{
 let mission=started();
 for(let i=0;i<120;i++){
  mission.state=i%2?"ERROR":"PREFLIGHT";
  mission=reduceMission(mission,{type:i%2?"RETRY_PREFLIGHT":"PREFLIGHT_RESULT",detail:{
   complete:false,error:"test"
  }},10000+i);
 }
 assert.ok(mission.transition_log.length<=80);
});

test("Phase 3 UI is wired to real preflight, download and postflight events",()=>{
 const html=readFileSync(new URL("../web/index.html",import.meta.url),"utf8");
 const app=readFileSync(new URL("../web/v14_app.mjs",import.meta.url),"utf8");
 const worker=readFileSync(new URL("../web/v15_advanced.mjs",import.meta.url),"utf8");
 const sw=readFileSync(new URL("../web/service-worker.js",import.meta.url),"utf8");
 for(const id of ["v16MissionCard","v16MissionState","v16MissionPrimary","v16MissionPhysical",
  "v16MissionEvidence","v16MissionTimeline","v16MissionExport","v16MissionReset"]){
  assert.ok(html.includes('id="'+id+'"'),id);
 }
 assert.match(html,/green-ion-v16-preflight/);
 assert.match(app,/green-ion-v16-download-start/);
 assert.match(app,/initMission/);
 assert.match(worker,/green-ion-postflight-report/);
 assert.match(sw,/v16_mission\.mjs/);
});
