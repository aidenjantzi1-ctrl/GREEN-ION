import test from "node:test";
import assert from "node:assert/strict";
import {readFileSync} from "node:fs";
import {
 V18_TABS,FLIGHT_STATES,RADIO_PROTOCOL,PACKET_STALE_MS,
 deriveFlightState,criticalFlightProblem,optionalFlightWarnings,canPrepare,
 fileSaveEvidence,parseReceiverLine,recoveryView
} from "../web/v18_offline_core.mjs";
import {parseRkt4,analyzeFlight} from "../web/analysis/rkt4.mjs";

const source=p=>readFileSync(new URL("../"+p,import.meta.url),"utf8");
const html=source("web/v18-offline.html");
const app=source("web/v18_offline_app.mjs");
const sw=source("web/v18_service_worker.js");
const manifest=JSON.parse(source("web/v18.webmanifest"));

function rkt4(records=4){
 const bytes=new Uint8Array(32+16*records),d=new DataView(bytes.buffer);
 bytes.set([82,75,84,52],0);d.setUint16(4,2,true);d.setUint16(6,32,true);d.setUint16(8,16,true);d.setUint16(10,50,true);
 d.setFloat32(12,101325,true);d.setUint16(16,4100,true);
 for(let i=0;i<records;i++){const o=32+i*16;d.setUint32(o,i*20000,true);d.setFloat32(o+4,101325-i*10,true);d.setFloat32(o+8,20,true);d.setUint16(o+12,4100-i,true);d.setUint16(o+14,1,true);}
 return bytes;
}

test("V18 has exactly four user-facing tabs",()=>{
 assert.deepEqual(V18_TABS,["flight","recovery","data","settings"]);
 for(const name of ["Flight","Recovery","Data","Settings"])assert.match(html,new RegExp(">"+name+"<"));
 assert.equal((html.match(/data-tab="/g)||[]).length,4);
});

test("Flight state stays firmware-owned and SAVED is evidence-gated",()=>{
 assert.deepEqual(FLIGHT_STATES,["OFFLINE","READY","TESTING","ARMED","FLYING","SAVING","SAVED","ERROR"]);
 assert.equal(deriveFlightState(null,{connected:false}),"OFFLINE");
 assert.equal(deriveFlightState({sd_ok:true,recorder_mode:0}),"READY");
 assert.equal(deriveFlightState({sd_ok:true,flight_prep_active:true,recorder_mode:0}),"TESTING");
 assert.equal(deriveFlightState({sd_ok:true,armed:true,recorder_mode:1}),"ARMED");
 assert.equal(deriveFlightState({sd_ok:true,recording:true,recorder_mode:2}),"FLYING");
 assert.equal(deriveFlightState({sd_ok:true,recorder_mode:3},{saveVerified:false}),"SAVING");
 assert.equal(deriveFlightState({sd_ok:true,recorder_mode:3},{saveVerified:true}),"SAVED");
 assert.equal(deriveFlightState({sd_ok:false,recorder_mode:0}),"ERROR");
 assert.equal(fileSaveEvidence({files:[{name:"V12D001.BIN",size:800,recording_complete:false}]},"V12D001.BIN"),null);
 assert.deepEqual(fileSaveEvidence({files:[{name:"V12D001.BIN",size:800,recording_complete:true,recovery_state:"COMPLETE"}]},"V12D001.BIN"),
  {name:"V12D001.BIN",size:800,recoveryState:"COMPLETE"});
 assert.match(app,/\/api\/metadata\?name=/);
 assert.match(app,/fileSaveEvidence/);
});

test("critical vs optional readiness matches V17 contract without inventing sensor truth",()=>{
 assert.match(criticalFlightProblem({sd_ok:false}),/microSD/);
 assert.match(criticalFlightProblem({sd_ok:true,imu_ok:false}),/IMU/);
 assert.match(criticalFlightProblem({sd_ok:true,imu_ok:true,barometer_ready:false}),/Barometer/);
 assert.equal(optionalFlightWarnings({gps_status:"missing",radio_ok:false}).length,2);
 assert.equal(canPrepare({sd_ok:true,imu_ok:true,barometer_ready:true,recorder_mode:0}),true);
 assert.equal(canPrepare({sd_ok:true,imu_ok:true,barometer_ready:true,recording:true,recorder_mode:2}),false);
 assert.equal(canPrepare({sd_ok:true,imu_ok:true,barometer_ready:true,recorder_mode:3},{saveVerified:false}),false);
});

test("Recovery accepts only current protocol-v2 receiver JSON and handles stale GPS",()=>{
 const now=100000;
 const good=parseReceiverLine(JSON.stringify({type:"telemetry",protocol:RADIO_PROTOCOL,sequence:7,uptime_ms:9000,altitude_m:123.4,battery_v:4.02,gps_status:"live",latitude:34.1,longitude:-117.2,rssi_dbm:-71,snr_db:8.5}),now);
 assert.ok(good);assert.equal(good.gpsStatus,"live");assert.equal(good.sequence,7);
 assert.equal(parseReceiverLine(JSON.stringify({type:"telemetry",protocol:99,altitude_m:1,battery_v:4}),now),null);
 assert.equal(parseReceiverLine("{bad json",now),null);
 assert.equal(parseReceiverLine(JSON.stringify({type:"control",protocol:2,altitude_m:1,battery_v:4}),now),null);
 const live=recoveryView(good,null,now+1000);assert.equal(live.signal,"live");assert.equal(live.gps,"live");
 const stale=recoveryView(good,{latitude:34.1,longitude:-117.2,at:now},now+PACKET_STALE_MS+1);assert.equal(stale.signal,"stale");assert.equal(stale.gps,"stale");
 const oldFix=parseReceiverLine(JSON.stringify({type:"telemetry",protocol:2,altitude_m:1,battery_v:4,gps_status:"live",gps_fix_age_ms:16001,latitude:34.1,longitude:-117.2}),now);assert.equal(oldFix.gpsStatus,"stale");
 const noFix=parseReceiverLine(JSON.stringify({type:"telemetry",protocol:2,altitude_m:1,battery_v:4,gps_status:"missing",latitude:null,longitude:null}),now+5000);
 assert.equal(recoveryView(noFix,{latitude:34.1,longitude:-117.2,at:now},now+5000).gps,"stale");
});

test("Web Serial flow covers permission, disconnect and reconnect guidance",()=>{
 for(const token of ["navigator.serial.requestPort","navigator.serial.getPorts","baudRate:115200","addEventListener(\"disconnect\"","addEventListener(\"connect\"","CONNECT RECEIVER","automatic reconnect"])
  assert.ok(app.includes(token)||html.includes(token),token);
 assert.match(app,/parseReceiverLine/);
 assert.match(html,/Chrome\/Edge will show a serial-port permission picker/);
});

test("manual BIN import preserves immutable RKT4 v2 bytes and rejects malformed BIN",()=>{
 const good=parseRkt4(rkt4());assert.equal(good.format,"RKT4");assert.equal(good.samples.length,4);
 const analyzed=analyzeFlight(good);assert.ok(analyzed.warnings.some(x=>/pressure-derived estimates/i.test(x)));
 assert.throws(()=>parseRkt4(new Uint8Array([1,2,3])),/shorter than 32-byte header/);
 const bad=rkt4();bad[0]=0;assert.throws(()=>parseRkt4(bad),/Unsupported RKT4 v2 layout/);
 for(const token of ["file.arrayBuffer()","parseRkt4(bytes)","saveLocalFile","origin:\"manual-v18-import\"","DOWNLOAD ORIGINAL BIN"])
  assert.ok(app.includes(token)||html.includes(token),token);
});

test("multi-flight local history, comparison, CSV JSON and print remain local",()=>{
 for(const token of ["listLocalMetadata","compareFlight","analysisCsv","flightReport","window.print()","exportArchive","restoreArchive"])
  assert.ok(app.includes(token),token);
 assert.ok(!app.includes("supabase"));
 assert.ok(!app.includes("/api/cloud"));
 assert.ok(!app.includes("fetch(\"https://"));
 assert.match(html,/No cloud account, remote telemetry upload, subscription, or internet connection is required/);
});

test("PR #56 regressions are isolated: V18 owns cache version and hash navigation",()=>{
 assert.match(sw,/const VERSION="green-ion-v18-offline-app-v2"/);
 assert.ok(sw.includes('"./v18-offline.html"'));
 assert.ok(sw.includes('"./analysis/rkt4.mjs"'));
 assert.match(sw,/url\.pathname\.startsWith\("\/api\/"\)/);
 assert.equal(manifest.start_url,"./v18-offline.html#flight");
 assert.match(app,/location\.hash=name/);
 assert.match(app,/location\.hash\.slice\(1\)/);
 assert.ok(!app.includes("v16_navigation.mjs"),"V18 does not inherit PR #56 history-navigation assertions");
});

test("offline startup has no cloud dependency and settings are locked during active flight",()=>{
 for(const asset of ["./v18_offline_app.mjs","./v18_offline_core.mjs","./analysis/rkt4.mjs","./v14_store.mjs"])
  assert.ok(sw.includes(asset),asset);
 assert.match(app,/serviceWorker\.register\("\.\/v18_service_worker\.js"\)/);
 assert.match(html,/No cloud account, remote telemetry upload, subscription, or internet connection is required/);
 assert.match(app,/data-lock-during-flight/);
 assert.match(app,/s\?\.recording\|\|s\?\.armed\|\|s\?\.flight_prep_active/);
});


test("Data renders an explicit local estimated phase timeline",()=>{
 assert.match(html,/id="phaseTimeline"/);
 assert.match(app,/function renderPhaseTimeline/);
 assert.match(app,/phase\.confidence/);
 assert.match(app,/m estimated/);
});

test("Settings exposes firmware-owned calibration, profile metadata and diagnostics safely",()=>{
 for(const id of ["restartCalibration","rocketProfile","motorProfile","saveProfile","sampleRate","calibrationBaseline","statusJson"])
  assert.ok(html.includes('id="'+id+'"'),id);
 assert.match(app,/\/api\/calibrate/);
 assert.match(app,/\/api\/profile/);
 assert.match(app,/new URLSearchParams\(\{rocket,motor\}\)/);
 assert.match(app,/selected_rocket/);
 assert.match(app,/sample_rate_hz/);
 assert.match(app,/settingsMutationLocked/);
 assert.ok(!app.includes("sample_rate_hz="),"frontend must not attempt to change firmware recorder rate");
});
