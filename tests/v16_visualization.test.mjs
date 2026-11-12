import test from "node:test";
import assert from "node:assert/strict";
import {crc32} from "../web/analysis/gis1.mjs";
import {integrateGyroOrientation,rotateVector,reconstructFlight,VIS_SCHEMA} from "../web/v16_visualization.mjs";

function makeRkt4(){
 const raw=new Uint8Array(32+6*16),v=new DataView(raw.buffer);
 raw.set([...Buffer.from("RKT4")],0);
 v.setUint16(4,2,true);v.setUint16(6,32,true);v.setUint16(8,16,true);v.setUint16(10,10,true);
 v.setFloat32(12,101325,true);v.setUint16(16,4100,true);
 const pressures=[101325,101200,100900,100500,100900,101200];
 for(let i=0;i<pressures.length;i++){
  const o=32+i*16;v.setUint32(o,i*100000,true);v.setFloat32(o+4,pressures[i],true);
  v.setFloat32(o+8,20,true);v.setUint16(o+12,4050-i*2,true);v.setUint16(o+14,1,true);
 }
 return raw.buffer;
}
function makeGis1(){
 const raw=new Uint8Array(32+2*68),v=new DataView(raw.buffer);
 raw.set([...Buffer.from("GIS1")],0);
 v.setUint16(4,1,true);v.setUint16(6,32,true);v.setUint16(8,68,true);v.setUint16(10,10,true);v.setUint16(12,1,true);
 v.setUint32(16,0,true);v.setUint32(20,0,true);v.setUint32(28,crc32(raw.subarray(0,28)),true);
 const rows=[
  {t:0,seq:1,lat:33.6400000,lon:-117.6000000,alt:100,gyro:0},
  {t:500000,seq:2,lat:33.6400100,lon:-117.5999900,alt:120,gyro:Math.PI/2}
 ];
 rows.forEach((r,i)=>{
  const o=32+i*68;
  v.setUint32(o,r.t,true);v.setUint32(o+4,r.seq,true);
  v.setFloat32(o+8,0,true);v.setFloat32(o+12,0,true);v.setFloat32(o+16,9.80665,true);
  v.setFloat32(o+20,0,true);v.setFloat32(o+24,0,true);v.setFloat32(o+28,r.gyro,true);
  v.setInt32(o+32,Math.round(r.lat*1e7),true);v.setInt32(o+36,Math.round(r.lon*1e7),true);
  v.setFloat32(o+40,r.alt,true);v.setFloat32(o+44,2,true);v.setUint32(o+48,0,true);v.setUint32(o+52,r.t,true);
  v.setUint16(o+56,7,true);v.setUint8(o+58,9);v.setUint8(o+59,12);v.setUint8(o+60,0);v.setUint8(o+61,i);
  v.setUint32(o+64,crc32(raw.subarray(o,o+64)),true);
 });
 return raw.buffer;
}

test("relative gyro integration produces a normalized orientation",()=>{
 const samples=[
  {timeS:0,imuValid:true,gyroXDps:0,gyroYDps:0,gyroZDps:90},
  {timeS:.1,imuValid:true,gyroXDps:0,gyroYDps:0,gyroZDps:90},
  {timeS:.2,imuValid:true,gyroXDps:0,gyroYDps:0,gyroZDps:90}
 ];
 const out=integrateGyroOrientation(samples);
 assert.ok(out.length>=2);
 const q=out.at(-1).q;
 assert.ok(Math.abs(Math.hypot(...q)-1)<1e-9);
 const x=rotateVector(q,[1,0,0]);
 assert.ok(x[1]>0.2);
});

test("pressure-only reconstruction remains available without GIS1",()=>{
 const r=reconstructFlight(makeRkt4());
 assert.equal(r.schema,VIS_SCHEMA);
 assert.equal(r.mode,"pressure-only");
 assert.ok(r.points.length>0);
 assert.match(r.limitations.join(" "),/No GIS1 companion/);
});

test("GPS plus pressure reconstruction creates horizontal 3D evidence",()=>{
 const r=reconstructFlight(makeRkt4(),makeGis1());
 assert.equal(r.mode,"gps+pressure");
 assert.ok(r.points.length>=2);
 assert.ok(r.horizontalTrackM>0);
 assert.ok(r.orientation.length>0);
 assert.match(r.limitations.join(" "),/not absolute attitude/);
 assert.match(r.limitations.join(" "),/not to scale/);
});
