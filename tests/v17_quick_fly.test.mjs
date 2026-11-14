import test from "node:test";
import assert from "node:assert/strict";
import {readFileSync} from "node:fs";
import {
  QUICK_FLY_STATES,
  deriveQuickFlyState,
  nextFlightNumber,
  parseFlightMetadata,
  batteryAdvisory,
  storageAdvisory,
  optionalWarnings
} from "../web/v17_quick_fly.mjs";

const source=path=>readFileSync(new URL("../"+path,import.meta.url),"utf8");
const prep=source("Green_Ion_V16_3/FlightPreparation.h");
const ino=source("Green_Ion_V16_3/Green_Ion_V16_3.ino");
const direct=source("Green_Ion_V16_3/QuickFlyDashboard.h");
const hosted=source("web/quick-fly.html");
const app=source("web/v17_quick_fly.mjs");
const sw=source("web/service-worker.js");
const navigation=source("web/v16_navigation.mjs");
const manifest=JSON.parse(source("web/manifest.webmanifest"));

test("Quick Fly exposes exactly the requested live status vocabulary",()=>{
 assert.deepEqual(QUICK_FLY_STATES,["READY","TESTING","ARMED","FLYING","SAVING","SAVED","ERROR"]);
 assert.equal(deriveQuickFlyState({sd_ok:true,recorder_mode:0}),"READY");
 assert.equal(deriveQuickFlyState({sd_ok:true,flight_prep_active:true,recorder_mode:0}),"TESTING");
 assert.equal(deriveQuickFlyState({sd_ok:true,armed:true,recorder_mode:1}),"ARMED");
 assert.equal(deriveQuickFlyState({sd_ok:true,recording:true,recorder_mode:2}),"FLYING");
 assert.equal(deriveQuickFlyState({sd_ok:true,recorder_mode:3},{verifiedSaved:false}),"SAVING");
 assert.equal(deriveQuickFlyState({sd_ok:true,recorder_mode:3},{verifiedSaved:true}),"SAVED");
 assert.equal(deriveQuickFlyState({sd_ok:false,recorder_mode:0}),"ERROR");
 assert.equal(deriveQuickFlyState({sd_ok:true,flight_prep_failure:"bad",recorder_mode:0}),"ERROR");
 assert.equal(deriveQuickFlyState(null,{connected:false}),"ERROR");
});

test("flight counter uses firmware-reserved next sequence",()=>{
 assert.equal(nextFlightNumber({next_sequence:0,files:[]}),1);
 assert.equal(nextFlightNumber({next_sequence:7,files:[]}),8);
 assert.equal(nextFlightNumber({next_sequence:999,files:[]}),1000);
 assert.equal(nextFlightNumber({files:[{name:"V12D000.BIN"},{name:"V12D002.BIN"}]}),4);
});

test("completion metadata drives saved duration and size evidence",()=>{
 const parsed=parseFlightMetadata(
   "GREEN ION FLIGHT COMPLETE\nfile=V12D007.BIN\nduration_ms=45200\nfile_bytes=3232\nsamples=2000\nmissed_sample_periods=0\n"
 );
 assert.equal(parsed.durationMs,45200);
 assert.equal(parsed.fileBytes,3232);
 assert.equal(parsed.samples,2000);
 assert.equal(parsed.missedSamplePeriods,0);
});

test("battery and storage warnings use conservative launch-day thresholds",()=>{
 assert.equal(batteryAdvisory(3.59).level,"critical");
 assert.equal(batteryAdvisory(3.70).level,"warning");
 assert.equal(batteryAdvisory(4.05).level,"ok");
 assert.equal(storageAdvisory(4*1024*1024,128*1024*1024).level,"warning");
 assert.equal(storageAdvisory(100*1024*1024,128*1024*1024).level,"ok");
});

test("GPS and LoRa are warnings, not substitutes for microSD",()=>{
 const warnings=optionalWarnings({gps_status:"missing",radio_ok:false});
 assert.equal(warnings.length,2);
 assert.match(warnings[0],/recovery aid only/i);
 assert.match(warnings[1],/microSD remains the required flight record/i);
 assert.equal(optionalWarnings({gps_status:"live",radio_ok:true}).length,0);
});

test("firmware owns full and quick preparation truth",()=>{
 for(const token of [
   "flightReadinessQuick",
   "runFlightReadinessSdIntegrityTest",
   "state.batteryMv < 3600U",
   "state.barometerOk",
   "state.imuOk",
   "HARDWARE_TEST_IMU",
   "flightReadinessSimulationVerifiedThisBoot",
   "resetCalibration(nowMs)",
   "FLIGHT_PREP_ARM"
 ]) assert.ok(prep.includes(token),token);
 assert.match(prep,/flightReadinessQuick \? HARDWARE_TEST_IMU : HARDWARE_TEST_ALL/);
 assert.match(prep,/if \(!flightReadinessQuick\) runPassiveLoraDiagnostic\(true\)/);
 assert.match(prep,/flightReadinessQuick && flightReadinessSimulationVerifiedThisBoot/);
 assert.match(prep,/microSD, battery, critical sensors and a fresh altitude baseline/i);
});

test("firmware status and files APIs expose Quick Fly evidence",()=>{
 for(const token of [
   '"/api/preflight/start"',
   "flight_prep_mode",
   "flight_prep_progress",
   "flight_prep_ready",
   "flight_prep_failure",
   "launch_simulation_cached",
   "next_sequence",
   "flightSequenceOccupied"
 ]) assert.ok(ino.includes(token),token);
 assert.ok(ino.includes('dashboardServer.on("/quick-fly"'));
 assert.ok(ino.includes("QUICK_FLY_HTML"));
 assert.match(ino,/addAll\(\['\/','\/quick-fly'\]\)/);
});

test("SAVED is gated on completed Feather file evidence",()=>{
 assert.match(app,/file\.recording_complete!==true/);
 assert.match(app,/await request\("\/api\/metadata\?name="/);
 assert.match(app,/this\.verifiedSaved=true/);
 assert.match(app,/recorder_mode===3\)return verifiedSaved\?"SAVED":"SAVING"/);
 assert.ok(!app.includes('setTimeout(()=>{this.verifiedSaved=true'));
});

test("Quick Fly pages are mobile, offline-oriented, and keep advanced tools separate",()=>{
 for(const token of ["PREPARE &amp; ARM","QUICK FLY","microSD · mandatory","FLY AGAIN"])
   assert.ok(hosted.includes(token),token);
 assert.ok(direct.includes("PREPARE &amp; ARM"));
 assert.ok(direct.includes("Direct Feather"));
 assert.ok(direct.includes('href="/"'));
 assert.ok(hosted.includes("./index.html#history"));
 assert.ok(navigation.includes('window.addEventListener("hashchange",openHash)'));
 assert.ok(navigation.includes("queueMicrotask(openHash)"));
 assert.ok(sw.includes("./quick-fly.html"));
 assert.ok(sw.includes("./v17_quick_fly.mjs"));
 assert.ok(app.includes('serviceWorker.register("./service-worker.js")'));
 assert.equal(manifest.start_url,"./quick-fly.html");
});

test("client prevents accidental duplicate prepare commands",()=>{
 assert.match(app,/now-this\.lastCommandAt<1400/);
 assert.match(direct,/now-lastCommand<1400/);
 assert.match(app,/this\.commandLock/);
});
