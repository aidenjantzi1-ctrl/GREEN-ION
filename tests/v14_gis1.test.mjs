import test from "node:test";
import assert from "node:assert/strict";
import {crc32,parseGis1,sensorCsv,gpsTrackGeoJson,SENSOR_GRAPH_METRICS} from "../web/analysis/gis1.mjs";
const fixture=()=>{
 const data=new Uint8Array(32+4*68),v=new DataView(data.buffer);
 data.set(new TextEncoder().encode("GIS1"));v.setUint16(4,1,true);
 v.setUint16(6,32,true);v.setUint16(8,68,true);
 v.setUint16(10,50,true);v.setUint16(12,7,true);
 v.setUint32(16,1000,true);v.setUint32(20,0x12345678,true);
 v.setUint32(28,crc32(data.subarray(0,28)),true);
 for(let i=0;i<4;i++){
  const o=32+i*68;
  v.setUint32(o,i*20000,true);v.setUint32(o+4,i,true);
  v.setFloat32(o+8,9.80665+i,true);v.setFloat32(o+12,0,true);v.setFloat32(o+16,0,true);
  v.setFloat32(o+20,0.1,true);v.setFloat32(o+24,0.2,true);v.setFloat32(o+28,0.3,true);
  v.setInt32(o+32,340000000+i*2,true);v.setInt32(o+36,-1170000000-i*2,true);
  v.setFloat32(o+40,300+i,true);v.setFloat32(o+44,15+i,true);
  v.setUint32(o+48,100,true);v.setUint32(o+52,500,true);
  v.setUint16(o+56,7,true);v.setUint8(o+58,10);
  v.setUint8(o+59,14);v.setUint8(o+60,12);v.setUint8(o+61,30);
  v.setUint32(o+64,crc32(data.subarray(o,o+64)),true);
 }
 return data;
};
test("CRC32 agrees with standard golden vector and GIS1 v1 validates",()=>{
 assert.equal(crc32(new TextEncoder().encode("123456789")),0xcbf43926);
 const data=fixture(),original=data.slice();
 const p=parseGis1(data);
 assert.equal(p.decoded,4);assert.equal(p.rateHz,50);assert.equal(p.flightSequence,7);
 assert.equal(p.summary.validImu,4);assert.equal(p.summary.currentGps,4);
 assert.ok(p.summary.peakTotalG>1);
 assert.equal(p.samples[0].latitude,34);
 assert.ok(Math.abs(p.samples[0].gyroXDps-5.72958)<0.001);
 assert.ok(SENSOR_GRAPH_METRICS.gpsAltitudeM);
 assert.deepEqual(data,original,"decoder must not modify raw input");
 assert.match(sensorCsv(p),/gyroXDps,gpsCurrent/);
});
test("GIS1 refuses damaged header; corrupt record only returns validated prefix",()=>{
 const file=fixture();file[3]=0;
 assert.throws(()=>parseGis1(file),/magic/);
 const other=fixture();other[19]^=1;
 assert.throws(()=>parseGis1(other),/CRC32/);
 const damaged=fixture();damaged[32+68+10]^=1;
 const p=parseGis1(damaged);
 assert.equal(p.decoded,1);
 assert.equal(p.invalidIndex,1);
 assert.ok(p.warnings.some(x=>x.includes("CRC32 mismatch")));
});
test("GPS export includes only confirmed current positions; no invented track",()=>{
 const data=fixture(),p=parseGis1(data);
 assert.deepEqual(gpsTrackGeoJson(p).features.length,0,"time window <1 second; no meaningful route yet");
 const extended={samples:[{gpsCurrent:true,timeS:0,latitude:34,longitude:-117,gpsAltitudeM:10},
 {gpsCurrent:false,timeS:1,latitude:34.1,longitude:-117.1,gpsAltitudeM:11},
 {gpsCurrent:true,timeS:2,latitude:34.2,longitude:-117.2,gpsAltitudeM:12}]};
 const geo=gpsTrackGeoJson(extended);
 assert.equal(geo.type,"FeatureCollection");
 assert.equal(geo.features[0].geometry.type,"LineString");
 assert.deepEqual(geo.features[0].geometry.coordinates,[[ -117,34,10],[-117.2,34.2,12]]);
});
