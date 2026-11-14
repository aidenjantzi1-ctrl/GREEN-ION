/* Existing V16.2 comparison worker: original BIN/SEN data remains immutable. */
import {parseRkt4,analyzeFlight} from "./analysis/rkt4.mjs";
import {parseGis1} from "./analysis/gis1.mjs";
const finite=Number.isFinite;
const rad=v=>v*Math.PI/180;
function distance(a,b){
 const R=6371000,dLat=rad(b.latitude-a.latitude),dLon=rad(b.longitude-a.longitude),p1=rad(a.latitude),p2=rad(b.latitude);
 const h=Math.sin(dLat/2)**2+Math.cos(p1)*Math.cos(p2)*Math.sin(dLon/2)**2;
 return 2*R*Math.atan2(Math.sqrt(h),Math.sqrt(Math.max(0,1-h)));
}
export function peakAngularRate(samples){
 let peak=null;
 for(const s of samples||[]){
  if(!s.imuValid||![s.gyroXDps,s.gyroYDps,s.gyroZDps].every(finite))continue;
  const rate=Math.hypot(s.gyroXDps,s.gyroYDps,s.gyroZDps);
  if(finite(rate))peak=Math.max(peak??0,rate);
 }
 return peak;
}
export function comparisonFromAnalyses(analysis,sensor=null,name="flight"){
 const phases=analysis.phases||[];
 const launch=phases.find(x=>x.name==="launch"),apogee=phases.find(x=>x.name==="apogee"),landing=phases.find(x=>x.name==="landing");
 const start=launch?.timeS??analysis.samples[0]?.timeS??0;
 const end=landing?.timeS??analysis.samples.at(-1)?.timeS??start;
 const senSamples=sensor?.samples||[];
 let gpsDistance=0,lastFix=null,peakMeasuredG=null,valid=0,peakAccel=null,peakSpeed=null,peakEstimatedG=null;
 for(const s of senSamples){
  if(s.gpsCurrent&&finite(s.latitude)&&finite(s.longitude)){
   if(lastFix)gpsDistance+=distance(lastFix,s);
   lastFix=s;
  }
 }
 peakMeasuredG=sensor?.summary?.validImu?sensor.summary.peakTotalG:null;
 for(const s of analysis.samples){
  if(s.valid)valid++;
  if(finite(s.accelerationMps2))peakAccel=Math.max(peakAccel??0,Math.abs(s.accelerationMps2));
  if(finite(s.velocityMps))peakSpeed=Math.max(peakSpeed??0,Math.abs(s.velocityMps));
  if(finite(s.estimatedVerticalG))peakEstimatedG=Math.max(peakEstimatedG??0,Math.abs(s.estimatedVerticalG));
 }
 const total=analysis.samples.length,stride=Math.max(1,Math.ceil(total/1600)),graph=[];
 let nextSen=0;
 for(let i=0;i<total;i+=stride){
  const s=analysis.samples[i];
  while(nextSen+1<senSamples.length&&senSamples[nextSen+1].timeS<=s.timeS)nextSen++;
  const q=senSamples[nextSen],match=q&&q.imuValid&&Math.abs(q.timeS-s.timeS)<.2?q:null;
  const angular=match&&[match.gyroXDps,match.gyroYDps,match.gyroZDps].every(finite)?
   Math.hypot(match.gyroXDps,match.gyroYDps,match.gyroZDps):null;
  graph.push({t:s.timeS-start,altitudeM:finite(s.altitudeM)?s.altitudeM:null,
   velocityMps:finite(s.velocityMps)?s.velocityMps:null,
   accelerationMps2:finite(s.accelerationMps2)?s.accelerationMps2:null,
   rotationDps:angular});
 }
 return {name,metrics:{
  maxAltitudeM:finite(analysis.peakAltitudeM)?analysis.peakAltitudeM:null,
  flightDurationS:Math.max(0,end-start),
  maxVelocityMps:peakSpeed,
  maxAccelerationMps2:peakAccel,
  peakAngularDps:peakAngularRate(senSamples),
  maxG:finite(peakMeasuredG)?peakMeasuredG:peakEstimatedG,
  gSource:finite(peakMeasuredG)?"measured IMU":"estimated vertical",
  ascentDurationS:launch&&apogee?Math.max(0,apogee.timeS-launch.timeS):null,
  descentDurationS:apogee&&landing?Math.max(0,landing.timeS-apogee.timeS):null,
  gpsDistanceM:lastFix?gpsDistance:null,
  gpsTrackAvailable:senSamples.filter(s=>s.gpsCurrent&&finite(s.latitude)&&finite(s.longitude)).length>=2,
  recoveryLocation:lastFix?{latitude:lastFix.latitude,longitude:lastFix.longitude,timeS:lastFix.timeS}:null,
  completenessPct:total?100*valid/total:null,
  trailingBytes:analysis.trailingBytes??0,
  sampleRateHz:analysis.rateHz,
  dataWarnings:[...(analysis.warnings||[]),...(sensor?.warnings||[])].filter(x=>!/are pressure-derived estimates/.test(x))
 },graph,phases:phases.map(x=>({name:x.name,timeS:x.timeS-start,confidence:x.confidence??null}))};
}
function metric(bin,sen,name){
 const analysis=analyzeFlight(parseRkt4(bin)),sensor=sen?parseGis1(sen):null;
 return comparisonFromAnalyses(analysis,sensor,name);
}
if(typeof self!=="undefined")self.onmessage=e=>{try{
 const {id,name,bin,sen}=e.data;self.postMessage({id,result:metric(new Uint8Array(bin),sen?new Uint8Array(sen):null,name)});
}catch(error){self.postMessage({id,error:error.message});}};
