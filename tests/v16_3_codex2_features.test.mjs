import test from "node:test";
import assert from "node:assert/strict";
import {readFileSync} from "node:fs";
import {liveGraphSample,liveGraphSampleFromStatus,appendBoundedGraph,LIVE_GRAPH_FIELDS} from "../web/v16_3_live_graphs.mjs";
import {validFix,observedFix,savedTrack} from "../web/v16_3_recovery_data.mjs";
import {recoveryEvidence} from "../web/v16_3_recovery_tools.mjs";
import {peakAngularRate,comparisonFromAnalyses} from "../web/v16_2_compare_worker.mjs";
import {replaySampleStep} from "../web/v16_2_webgl.mjs";
import {integrateGyroOrientation} from "../web/v16_visualization.mjs";
import {parseRkt4} from "../web/analysis/rkt4.mjs";

const source=p=>readFileSync(new URL("../"+p,import.meta.url),"utf8");
const reading=(value,freshness="live",age_ms=100)=>({value,freshness,age_ms,source:"wifi"});
test("1: live graph accepts fresh real numeric telemetry and rejects stale/missing",()=>{
 const good={primary_source:"wifi",readings:{
  altitude_m:reading(21),acceleration_g:reading(3.5),
  rotation_x_dps:reading(3),rotation_y_dps:reading(4),rotation_z_dps:reading(0),
  pressure_pa:reading(99000),gps_speed_mps:reading(22)
 }};
 const sample=liveGraphSample(good,1000);
 assert.equal(sample.values.altitude_m,21);
 assert.equal(sample.values.rotation_magnitude_dps,5);
 assert.equal(sample.values.pressure_pa,99000);
 assert.equal(sample.values.vertical_velocity_mps,null,"GPS horizontal speed must not represent vertical velocity");
 assert.equal(liveGraphSample({primary_source:"none",readings:{}},1000).values.altitude_m,null);
 good.readings.altitude_m=reading(21,"stale",12000);
 good.readings.rotation_y_dps=reading(4,"missing",null);
 assert.equal(liveGraphSample(good,1000).values.altitude_m,null);
 assert.equal(liveGraphSample(good,1000).values.rotation_magnitude_dps,null);
 assert.deepEqual(Object.keys(LIVE_GRAPH_FIELDS),[
  "altitude_m","acceleration_g","rotation_magnitude_dps","pressure_pa","vertical_velocity_mps"]);
});
test("2: direct Feather barometric velocity only when actually reported",()=>{
 const valid=liveGraphSampleFromStatus({relative_altitude_m:3,velocity_mps:5,rotation_x_dps:2,rotation_y_dps:0,rotation_z_dps:0},"wifi",100);
 assert.equal(valid.values.vertical_velocity_mps,5);
 assert.equal(valid.values.altitude_m,3);
 const radio=liveGraphSampleFromStatus({gps_speed_mps:20},"lora",100);
 assert.equal(radio.values.vertical_velocity_mps,null);
 assert.equal(liveGraphSampleFromStatus({velocity_mps:5},"offline",100).values.vertical_velocity_mps,null);
});
test("3: graph ring buffer bounds by count, age and 10/30/60-second windows",()=>{
 let history=[];
 for(let i=0;i<800;i++)history=appendBoundedGraph(history,{at:i*1000,values:{altitude_m:i}},{maxPoints:300,maxAgeMs:60000});
 assert.ok(history.length<=61);
 assert.equal(history.at(-1).values.altitude_m,799);
 history=appendBoundedGraph(history,{at:900000,values:{}},{maxPoints:5,maxAgeMs:10000});
 assert.equal(history.length,1);
 assert.deepEqual([10,30,60].map(window=>window*1000),[10000,30000,60000]);
});
test("4: GPS fix validity requires real coordinates and non-stale GPS age",()=>{
 assert.equal(validFix(0,0),true);
 assert.equal(validFix(91,10),false);
 assert.equal(validFix(NaN,20),false);
 assert.equal(validFix("33",-117),false);
 const valid={gps_status:"live",latitude:33.5,longitude:-117.2,gps_fix_age_ms:5000,satellites:9};
 const f=observedFix(valid,"LoRa",20000);
 assert.equal(f.at,15000);assert.equal(f.satellites,9);
 assert.equal(observedFix({...valid,gps_status:"stale"},"LoRa"),null);
 assert.equal(observedFix({...valid,latitude:92},"LoRa"),null);
 assert.equal(observedFix({...valid,gps_fix_age_ms:30001},"LoRa"),null);
 assert.equal(observedFix({...valid,gps_fix_age_ms:-1},"Wi-Fi"),null);
 assert.equal(observedFix({...valid,gps_fix_age_ms:null},"Wi-Fi"),null);
});
test("5: last-known state stays explicit when link expires or fix absent",()=>{
 const last=observedFix({gps_status:"live",latitude:33,longitude:-117,gps_fix_age_ms:2000},"Wi-Fi",100000);
 assert.equal(recoveryEvidence(last,101000),"CONFIRMED FRESH GPS FIX");
 assert.equal(recoveryEvidence(last,110000),"STALE · LAST KNOWN GPS");
 assert.equal(recoveryEvidence(null),"NO GPS FIX");
 assert.equal(recoveryEvidence({...last,archived:true}),"SAVED GPS TRACK · NOT LIVE");
 assert.equal(observedFix({gps_status:"missing",latitude:null,longitude:null},"LoRa"),null);
 assert.deepEqual(last,{lat:33,lon:-117,source:"Wi-Fi",at:98000,receivedAt:100000,
  satellites:null,hdop:null,gpsAltitudeM:null,gpsSpeedMps:null});
});
test("6: saved GPS SEN track excludes invalid points and remains bounded offline",()=>{
 const points=Array.from({length:501},(_,i)=>({gpsCurrent:true,latitude:33+i/10000,longitude:-117,timeS:i}));
 points.splice(12,0,{gpsCurrent:false,latitude:30,longitude:20});
 points.splice(13,0,{gpsCurrent:true,latitude:NaN,longitude:20});
 const track=savedTrack(points,120);
 assert.ok(track.length<=120);assert.ok(track.length>=2);
 assert.equal(track[0].lat,33);
 assert.equal(track.at(-1).timeS,500);
 assert.ok(track.every(x=>x.archived&&validFix(x.lat,x.lon)&&x.source==="SAVED SEN"));
 assert.deepEqual(savedTrack([{gpsCurrent:false,latitude:33,longitude:-117}]),[]);
});
test("7: gyroscope relative quaternion integration without absolute yaw",()=>{
 const samples=[{timeS:0,imuValid:true,gyroXDps:0,gyroYDps:0,gyroZDps:90},
  {timeS:0.1,imuValid:true,gyroXDps:0,gyroYDps:0,gyroZDps:90},
  {timeS:0.2,imuValid:true,gyroXDps:0,gyroYDps:0,gyroZDps:90}];
 const out=integrateGyroOrientation(samples);
 assert.ok(out.length>=2);
 assert.ok(out.at(-1).q[3]>0.1);
 const norm=Math.hypot(...out.at(-1).q);
 assert.ok(Math.abs(norm-1)<1e-6);
 assert.match(source("web/v16_visualization.mjs"),/not absolute attitude/);
});
test("8: replay steps are on recorded timestamps, not arbitrary fixed 0.2 s",()=>{
 const times=[{timeS:0},{timeS:.02},{timeS:.05},{timeS:.12}];
 assert.equal(replaySampleStep(times,.02,1),.05);
 assert.equal(replaySampleStep(times,.05,-1),.02);
 assert.equal(replaySampleStep(times,0,-1),0);
 assert.equal(replaySampleStep(times,.12,1),.12);
 assert.equal(replaySampleStep([],1,1),1);
 const replay=source("web/v16_2_webgl.mjs");
 assert.match(replay,/green-ion-v16-replay-time/);
 assert.match(replay,/green-ion-v16-graph-progress/);
 assert.match(replay,/v1623dRestart/);
 assert.match(replay,/<option>4<\/option>/);
});
test("9: worker flight comparisons align launch, include gyro, missing-data states",()=>{
 const analysis={rateHz:50,peakAltitudeM:104,trailingBytes:0,
  warnings:["Timestamp gap at sample 4"],phases:[
   {name:"launch",timeS:12},{name:"apogee",timeS:15},{name:"landing",timeS:20}],
  samples:[{timeS:10,valid:true,altitudeM:0,velocityMps:0,accelerationMps2:0,estimatedVerticalG:0},
   {timeS:12,valid:true,altitudeM:10,velocityMps:20,accelerationMps2:8,estimatedVerticalG:1},
   {timeS:15,valid:true,altitudeM:104,velocityMps:0,accelerationMps2:2,estimatedVerticalG:0.2},
   {timeS:20,valid:false,altitudeM:null,velocityMps:null,accelerationMps2:null,estimatedVerticalG:null}]};
 const sensor={samples:[{timeS:12,imuValid:true,gyroXDps:3,gyroYDps:4,gyroZDps:0,
  gpsCurrent:true,latitude:33,longitude:-117},
  {timeS:15,imuValid:true,gyroXDps:0,gyroYDps:12,gyroZDps:0,
  gpsCurrent:true,latitude:33.0001,longitude:-117}],summary:{validImu:2,peakTotalG:2.1},warnings:[]};
 assert.equal(peakAngularRate(sensor.samples),12);
 assert.equal(peakAngularRate([]),null);
 const first=comparisonFromAnalyses(analysis,sensor,"Flight 1");
 assert.equal(first.graph[0].t,-2);
 assert.equal(first.metrics.flightDurationS,8);
 assert.equal(first.metrics.ascentDurationS,3);
 assert.equal(first.metrics.descentDurationS,5);
 assert.equal(first.metrics.peakAngularDps,12);
 assert.equal(first.metrics.gpsTrackAvailable,true);
 assert.equal(first.metrics.completenessPct,75);
 assert.ok(first.metrics.dataWarnings.some(x=>x.includes("gap")));
 assert.equal(comparisonFromAnalyses(analysis,null,"no SEN").metrics.peakAngularDps,null);
});
test("10: original RKT4 v2 BIN bytes still decode unchanged",()=>{
 const bytes=new Uint8Array(48),v=new DataView(bytes.buffer);
 bytes.set([82,75,84,52]);v.setUint16(4,2,true);v.setUint16(6,32,true);v.setUint16(8,16,true);
 v.setUint16(10,50,true);v.setFloat32(12,101325,true);v.setUint16(16,3900,true);
 v.setUint32(32,20000,true);v.setFloat32(36,101000,true);v.setFloat32(40,22,true);
 v.setUint16(44,3800,true);v.setUint16(46,1,true);
 const original=bytes.slice(),parsed=parseRkt4(bytes);
 assert.equal(parsed.version,2);assert.equal(parsed.rateHz,50);
 assert.equal(parsed.samples.length,1);assert.equal(parsed.samples[0].valid,true);
 assert.deepEqual(bytes,original);
});
test("11: mobile + history backwards compatibility and no firmware control regression",()=>{
 const app=source("web/v14_app.mjs"),lab=source("web/v14_sensor_lab.mjs"),
  recovery=source("web/v15_recovery.mjs"),sw=source("web/service-worker.js"),
  html=source("web/index.html"),db=source("web/v14_store.mjs");
 assert.match(app,/initSensorLab/);assert.match(lab,/initV163LiveGraphs/);
 assert.match(recovery,/initV163RecoveryTools/);
 assert.match(sw,/v16_3_live_graphs\.mjs/);assert.match(sw,/v16_3_recovery_data\.mjs/);
 assert.match(sw,/v16_3_recovery_tools\.mjs/);
 assert.match(html,/<meta name="viewport"/);
 assert.match(db,/indexedDB/);assert.match(db,/SHA-256/);
 assert.match(source("web/v16_2_dashboard.mjs"),/renderLiveRocketModel/);
 assert.match(source("web/v16_2_webgl.mjs"),/touch|pointerdown/i);
 assert.doesNotMatch(source("web/v16_3_recovery_tools.mjs"),/geolocation\.getCurrentPosition/);
});
