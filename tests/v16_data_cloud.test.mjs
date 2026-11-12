import test from "node:test";
import assert from "node:assert/strict";
import {readFileSync} from "node:fs";
import {buildFlightPackages,flightBase,DATA_SCHEMA} from "../web/v16_data.mjs";

test("V16 groups immutable siblings into one flight package",()=>{
 const files=[
  {name:"V12D007.BIN",kind:"BIN",checksum:"a".repeat(64),size:100,key:"a.bin"},
  {name:"V12D007.SEN",kind:"SEN",checksum:"b".repeat(64),size:200,key:"b.sen"},
  {name:"V12D007.TXT",kind:"TXT",checksum:"c".repeat(64),size:30,key:"c.txt"},
  {name:"V12D008.BIN",kind:"BIN",checksum:"d".repeat(64),size:90,key:"d.bin"}
 ];
 const packages=buildFlightPackages(files);
 assert.equal(DATA_SCHEMA,"GREEN-ION-V16-DATA-1");
 assert.equal(flightBase("V12D007.BIN"),"V12D007");
 assert.equal(packages.length,2);
 const complete=packages.find(x=>x.flight_id==="V12D007");
 assert.equal(complete.complete,true);
 assert.equal(complete.status,"COMPLETE");
 assert.equal(complete.total_bytes,330);
 const partial=packages.find(x=>x.flight_id==="V12D008");
 assert.equal(partial.complete,false);
 assert.equal(partial.status,"RECORDED_PARTIAL");
 assert.deepEqual(partial.missing,["SEN","TXT"]);
});

test("conflicting siblings are surfaced instead of silently replaced",()=>{
 const packages=buildFlightPackages([
  {name:"FLIGHT1.BIN",kind:"BIN",checksum:"1".repeat(64),size:10,key:"1.bin"},
  {name:"flight1.BIN",kind:"BIN",checksum:"2".repeat(64),size:10,key:"2.bin"}
 ]);
 assert.equal(packages[0].status,"COLLISION");
 assert.equal(packages[0].collision,true);
 assert.equal(packages[0].files.BIN.sha256,"1".repeat(64));
});

test("cloud module persists explicit V16 sync states",()=>{
 const cloud=readFileSync(new URL("../web/v14_cloud.mjs",import.meta.url),"utf8");
 for(const state of ["SIGNED_OUT","OFFLINE","PENDING","RETRYING","SYNCED","ERROR","UNAVAILABLE"])
  assert.match(cloud,new RegExp('"'+state+'"'));
 assert.match(cloud,/green-ion-v16-cloud-state/);
 assert.match(cloud,/Original files remain stored on this device/);
});

test("data card never claims cloud is authoritative",()=>{
 const src=readFileSync(new URL("../web/v16_data.mjs",import.meta.url),"utf8");
 assert.match(src,/local immutable originals/);
 assert.match(src,/Cloud is optional backup/);
 assert.match(src,/not sensor accuracy/);
});
