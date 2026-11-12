import test from "node:test";
import assert from "node:assert/strict";
import {readFileSync} from "node:fs";
import {
 DATA_SCHEMA,TRANSFER_SCHEMA,flightBase,buildFlightPackages,
 loadTransferJournal,recordTransfer,transferEntry,clearFinishedTransfers
} from "../web/v16_data.mjs";

function storage(){
 const map=new Map();
 return {
  getItem:key=>map.has(key)?map.get(key):null,
  setItem:(key,value)=>map.set(key,String(value)),
  removeItem:key=>map.delete(key)
 };
}

test("flightBase strips only supported terminal extensions",()=>{
 assert.equal(flightBase("V12D007.BIN"),"V12D007");
 assert.equal(flightBase("V12D007.SEN"),"V12D007");
 assert.equal(flightBase("archive.BIN.extra"),"archive.BIN.extra");
 assert.equal(flightBase("weirdxBIN"),"weirdxBIN");
 assert.equal(DATA_SCHEMA,"GREEN-ION-V16-DATA-1");
});

test("one local library entry groups BIN/SEN/TXT without requiring companions",()=>{
 const files=[
  {name:"V12D007.BIN",kind:"BIN",checksum:"a".repeat(64),size:100,key:"a.bin"},
  {name:"V12D007.SEN",kind:"SEN",checksum:"b".repeat(64),size:200,key:"b.sen"},
  {name:"V12D007.TXT",kind:"TXT",checksum:"c".repeat(64),size:30,key:"c.txt"},
  {name:"V12D008.BIN",kind:"BIN",checksum:"d".repeat(64),size:90,key:"d.bin"}
 ];
 const packages=buildFlightPackages(files);
 assert.equal(packages.length,2);
 const full=packages.find(x=>x.flight_id==="V12D007");
 assert.equal(full.status,"FULL");
 assert.equal(full.has_bin,true);
 assert.equal(full.full_companions,true);
 assert.equal(full.total_bytes,330);
 const recorded=packages.find(x=>x.flight_id==="V12D008");
 assert.equal(recorded.status,"RECORDED");
 assert.equal(recorded.has_bin,true);
 assert.equal(recorded.full_companions,false);
 assert.deepEqual(recorded.missing,["SEN","TXT"]);
});

test("same flight/kind with different checksum is surfaced as collision",()=>{
 const packages=buildFlightPackages([
  {name:"FLT1.BIN",kind:"BIN",checksum:"1".repeat(64),size:10,key:"1.bin"},
  {name:"flt1.BIN",kind:"BIN",checksum:"2".repeat(64),size:10,key:"2.bin"}
 ]);
 assert.equal(packages.length,1);
 assert.equal(packages[0].status,"COLLISION");
 assert.equal(packages[0].conflicts.length,1);
 assert.equal(packages[0].files.BIN.sha256,"1".repeat(64));
});

test("package status combines optional cloud and PC copies without changing local authority",()=>{
 const checksum="a".repeat(64);
 const packages=buildFlightPackages(
  [{name:"F001.BIN",kind:"BIN",checksum,size:100,key:"a.bin"}],
  {cloudState:{per_file:{[checksum]:{status:"SYNCED"}}},pcChecksums:new Set([checksum])}
 );
 assert.equal(packages[0].cloud_status,"SYNCED");
 assert.equal(packages[0].windows_status,"VERIFIED");
 assert.equal(packages[0].status,"RECORDED");
});

test("transfer journal survives restart and backs off per file",()=>{
 const store=storage();
 assert.equal(loadTransferJournal(store).schema,TRANSFER_SCHEMA);
 recordTransfer("V12D001.BIN",100,"DOWNLOADING",{},store);
 recordTransfer("V12D001.BIN",100,"DEFERRED",{next_retry_at:12345,error:"link lost"},store);
 const restored=transferEntry("V12D001.BIN",100,store);
 assert.equal(restored.status,"DEFERRED");
 assert.equal(restored.attempts,1);
 assert.equal(restored.next_retry_at,12345);
 recordTransfer("V12D001.BIN",100,"DOWNLOADING",{},store);
 const verified=recordTransfer("V12D001.BIN",100,"VERIFIED",{checksum:"f".repeat(64)},store);
 assert.equal(verified.attempts,2);
 assert.equal(verified.checksum,"f".repeat(64));
 clearFinishedTransfers(store);
 assert.equal(transferEntry("V12D001.BIN",100,store),null);
});

test("cloud backup state is explicit and file-granular",()=>{
 const cloud=readFileSync(new URL("../web/v14_cloud.mjs",import.meta.url),"utf8");
 for(const state of ["SIGNED_OUT","UNAVAILABLE","OFFLINE","IDLE","PENDING","SYNCING","RETRYING","SYNCED","RESTORING","ERROR"])
  assert.match(cloud,new RegExp('"'+state+'"'));
 assert.match(cloud,/per_file/);
 assert.match(cloud,/setFileState/);
 assert.match(cloud,/x-upsert":"false"/);
 assert.match(cloud,/on_conflict=user_id,checksum/);
 assert.match(cloud,/Retry-safe at file granularity/);
 assert.match(cloud,/Original files remain stored on this device/);
});

test("cloud restore remains checksum verified and never overwrites originals",()=>{
 const cloud=readFileSync(new URL("../web/v14_cloud.mjs",import.meta.url),"utf8");
 assert.match(cloud,/bytes\.byteLength!==Number\(file\.byte_size\)\|\|await digest\(bytes\)!==file\.checksum/);
 assert.match(cloud,/saveLocalFile\(\{name,bytes,origin:"Supabase restore"\}\)/);
 assert.match(cloud,/No existing recording was overwritten/);
});

test("Feather transfer is persistent, bounded and rejects partial bytes",()=>{
 const app=readFileSync(new URL("../web/v14_app.mjs",import.meta.url),"utf8");
 assert.match(app,/recordTransfer\(file\.name,size,"DOWNLOADING"/);
 assert.match(app,/transferEntry\(file\.name,size\)/);
 assert.match(app,/next_retry_at/);
 assert.match(app,/Partial Feather transfer rejected/);
 assert.match(app,/data\.byteLength!==size/);
 assert.match(app,/recordTransfer\(file\.name,size,"VERIFIED"/);
});

test("Phase 5 UI stays local-first and grouped",()=>{
 const data=readFileSync(new URL("../web/v16_data.mjs",import.meta.url),"utf8");
 const app=readFileSync(new URL("../web/v14_app.mjs",import.meta.url),"utf8");
 const sw=readFileSync(new URL("../web/service-worker.js",import.meta.url),"utf8");
 assert.match(data,/Flight library · local first/);
 assert.match(data,/One entry per flight/);
 assert.match(data,/Verified local originals are the source of truth/);
 assert.match(app,/initV16Data\(\)/);
 assert.match(sw,/v16_data\.mjs/);
 assert.match(sw,/phase7-data-hardening/);
});
