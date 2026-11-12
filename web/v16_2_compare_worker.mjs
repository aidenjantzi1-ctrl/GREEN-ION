import {parseRkt4,analyzeFlight} from "./analysis/rkt4.mjs";
import {parseGis1} from "./analysis/gis1.mjs";
const finite=Number.isFinite;
function rad(v){return v*Math.PI/180;}
function distance(a,b){
 const R=6371000,dLat=rad(b.latitude-a.latitude),dLon=rad(b.longitude-a.longitude),p1=rad(a.latitude),p2=rad(b.latitude);
 const h=Math.sin(dLat/2)**2+Math.cos(p1)*Math.cos(p2)*Math.sin(dLon/2)**2;
 return 2*R*Math.atan2(Math.sqrt(h),Math.sqrt(Math.max(0,1-h)));
}
function metric(bin,sen,name){
 const analysis=analyzeFlight(parseRkt4(bin)),phases=analysis.phases||[];
 const launch=phases.find(x=>x.name==="launch"),apogee=phases.find(x=>x.name==="apogee"),descent=phases.find(x=>x.name==="descent"),landing=phases.find(x=>x.name==="landing");
 const start=launch?.timeS??analysis.samples[0]?.timeS??0,end=landing?.timeS??analysis.samples.at(-1)?.timeS??start;
 let sensor=null,gpsDistance=0,lastFix=null,peakMeasuredG=null;
 if(sen){
  sensor=parseGis1(sen);const fixes=sensor.samples.filter(x=>x.gpsCurrent&&finite(x.latitude)&&finite(x.longitude));
  for(let i=1;i<fixes.length;i++)gpsDistance+=distance(fixes[i-1],fixes[i]);
  lastFix=fixes.at(-1)||null;peakMeasuredG=sensor.summary.validImu?sensor.summary.peakTotalG:null;
 }
 const estimatedGs=analysis.samples.map(x=>Math.abs(x.estimatedVerticalG)).filter(finite);
 const accels=analysis.samples.map(x=>Math.abs(x.accelerationMps2)).filter(finite);
 const stride=Math.max(1,Math.ceil(analysis.samples.length/1600)),graph=[];
 for(let i=0;i<analysis.samples.length;i+=stride){const x=analysis.samples[i];if(finite(x.altitudeM))graph.push({t:x.timeS-start,altitudeM:x.altitudeM});}
 const valid=analysis.samples.filter(x=>x.valid).length,total=analysis.samples.length;
 return {
  name,metrics:{
   maxAltitudeM:finite(analysis.peakAltitudeM)?analysis.peakAltitudeM:null,
   flightDurationS:Math.max(0,end-start),
   maxVelocityMps:finite(analysis.maxAscentMps)?analysis.maxAscentMps:null,
   maxAccelerationMps2:accels.length?Math.max(...accels):null,
   maxG:peakMeasuredG??(estimatedGs.length?Math.max(...estimatedGs):null),
   gSource:peakMeasuredG!==null?"measured IMU":"estimated vertical",
   ascentDurationS:launch&&apogee?Math.max(0,apogee.timeS-launch.timeS):null,
   descentDurationS:apogee&&landing?Math.max(0,landing.timeS-apogee.timeS):null,
   gpsDistanceM:sensor&&sensor.summary.currentGps?gpsDistance:null,
   recoveryLocation:lastFix?{latitude:lastFix.latitude,longitude:lastFix.longitude,timeS:lastFix.timeS}:null,
   completenessPct:total?100*valid/total:null,trailingBytes:analysis.trailingBytes,
   sampleRateHz:analysis.rateHz
  },graph,phases:phases.map(x=>({name:x.name,timeS:x.timeS-start,confidence:x.confidence??null}))
 };
}
self.onmessage=e=>{try{
 const {id,name,bin,sen}=e.data;self.postMessage({id,result:metric(new Uint8Array(bin),sen?new Uint8Array(sen):null,name)});
}catch(error){self.postMessage({id,error:error.message});}};
