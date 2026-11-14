import test from "node:test";
import assert from "node:assert/strict";
import {readFileSync} from "node:fs";

const hosted=readFileSync(new URL("../web/index.html",import.meta.url),"utf8");
const embedded=readFileSync(new URL("../Green_Ion_V16_3/Dashboard.h",import.meta.url),"utf8");

for(const [name,html] of [["hosted",hosted],["embedded",embedded]]){
 test(name+" exposes one-press V17 readiness",()=>{
  for(const id of ["v17PrepareCard","v17PrepareFlight","v17PrepProgress","v17PrepSd",
   "v17PrepStorage","v17PrepSensors","v17PrepBattery","v17PrepCalibration",
   "v17PrepGps","v17PrepLora","v17PrepRecorder","v17PrepFiles","v17PrepAdvice"])
   assert.match(html,new RegExp('id="'+id+'"'));
  assert.match(html,/PREPARE FOR FLIGHT/);
  assert.match(html,/ARMED — READY FOR LAUNCH/);
  assert.match(html,/GPS and LoRa are warnings only|GPS\/LoRa can warn/);
  assert.match(html,/microSD recording is authoritative/);
 });

 test(name+" LoRa diagnostic is explicit about antenna limitations",()=>{
  assert.match(html,/RUN PASSIVE LORA DIAGNOSTIC/);
  assert.match(html,/TRANSMIT 3 BENCH PACKETS/);
  assert.match(html,/Antenna attachment is NOT detectable by software|cannot.*detect an antenna/i);
  assert.match(html,/LoRa CS\/NSS 27/);
  assert.match(html,/DIO0 32/);
 });
}
