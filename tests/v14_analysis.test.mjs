import test from "node:test";
import assert from "node:assert/strict";
import {parseRkt4,analyzeFlight,analysisCsv,flightReport} from "../web/analysis/rkt4.mjs";
test("offline BIN decoder preserves original file and produces analysis",()=>{
 const bytes=new Uint8Array(32+16*16),v=new DataView(bytes.buffer);
 bytes.set(new TextEncoder().encode("RKT4"));
 v.setUint16(4,2,true);v.setUint16(6,32,true);v.setUint16(8,16,true);
 v.setUint16(10,50,true);v.setFloat32(12,101325,true);v.setUint16(16,4100,true);
 for(let i=0;i<16;i++){
   const offset=32+16*i;
   v.setUint32(offset,i*20000,true);
   v.setFloat32(offset+4,101325-i*17,true);
   v.setFloat32(offset+8,20.5,true);
   v.setUint16(offset+12,4100-i,true);
   v.setUint16(offset+14,1,true);
 }
 const unchanged=bytes.slice();
 const analysis=analyzeFlight(parseRkt4(bytes));
 assert.equal(analysis.samples.length,16);
 assert.equal(analysis.rateHz,50);
 assert.ok(analysis.peakAltitudeM>0);
 assert.equal(analysis.samples[0].timeS,0);
 assert.equal(flightReport(analysis,"V12D001.BIN").name,"V12D001.BIN");
 assert.match(analysisCsv(analysis),/altitude_m,velocity_mps/);
 assert.deepEqual(bytes,unchanged);
});
test("invalid headers are rejected without generating an incorrect flight",()=>{
 const bytes=new Uint8Array(32+16);
 assert.throws(()=>parseRkt4(bytes),/Unsupported RKT4/);
});
