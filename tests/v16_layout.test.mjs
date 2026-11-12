import test from "node:test";
import assert from "node:assert/strict";
import {readFileSync} from "node:fs";

const html=readFileSync(new URL("../web/index.html",import.meta.url),"utf8");
const app=readFileSync(new URL("../web/v14_app.mjs",import.meta.url),"utf8");
const layout=readFileSync(new URL("../web/v16_layout.mjs",import.meta.url),"utf8");
const help=readFileSync(new URL("../web/v15_help.mjs",import.meta.url),"utf8");
const sw=readFileSync(new URL("../web/service-worker.js",import.meta.url),"utf8");

test("Dashboard keeps mission-critical summary and preserves detailed telemetry",()=>{
 for(const id of ["v16DashMission","v16DashRocket","v16DashLinks","v16DashGps",
  "v16DashRecorder","v16DashboardPrepare","altitude","batteryPercent"]){
  assert.ok(html.includes('id="'+id+'"'),id);
 }
 for(const id of ["v14SensorLabCard","graphOptionsCard","altGraphCard","pressureGraphCard","tempGraphCard"])
  assert.ok(layout.includes('"'+id+'"'),id);
});

test("Flight and analysis tabs retain existing operational features",()=>{
 for(const id of ["v16MissionCard","flightProfileCard","prepareRecordCard","recorderHealthCard","v15RecoveryCard",
  "flightSummaryCard","v14OfflineAnalysisCard","v14SensorArchiveCard","v15ReportCard","v15LocalCard"]){
  assert.ok(html.includes('id="'+id+'"'),id);
 }
 assert.match(layout,/moveChildren\(\$\("recovery"\),target\)/);
 assert.match(layout,/moveChildren\(\$\("storage"\),target/);
});

test("advanced features stay present but grouped under Settings & Advanced",()=>{
 for(const id of ["v15BatteryCard","v15LinkCard","v14PreferencesCard","v15HelpCard","v14CloudAccount",
  "v14DownloadUpdate","installFirmware"]){
  assert.ok(html.includes('id="'+id+'"'),id);
 }
 for(const source of ["utilities","connections","cloud","updates","help"])
  assert.ok(layout.includes('["'+source+'"'),source);
});

test("internal shortcuts route through visible V16 destinations",()=>{
 assert.match(help,/Find the rocket after landing[\s\S]*?tab:"flight"/);
 assert.match(help,/Connect Arduino Uno LoRa[\s\S]*?tab:"system"/);
 assert.match(help,/Cloud backup after internet returns[\s\S]*?tab:"system"/);
 assert.match(app,/initLayout\(\)/);
 assert.match(app,/initV16Navigation\(\)/);
});

test("offline cache retains both Phase 4 layout and navigation across later phases",()=>{
 assert.match(sw,/v16_layout\.mjs/);
 assert.match(sw,/v16_navigation\.mjs/);
 assert.match(sw,/green-ion-v16-/);
});
