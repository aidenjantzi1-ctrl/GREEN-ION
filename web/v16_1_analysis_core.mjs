/* GREEN ION V16.1 postflight evidence helpers.
 * These functions add confidence/corroboration without changing original files. */
export const ANALYSIS_SCHEMA="GREEN-ION-V16.1-ANALYSIS-1";
const finite=Number.isFinite;

function label(score){
 const value=Math.max(0,Math.min(100,Math.round(score)));
 return {score:value,label:value>=80?"HIGH":value>=55?"MEDIUM":"LOW"};
}
function sustained(samples,start,predicate,count){
 let run=0;
 for(let i=Math.max(0,start);i<samples.length;i++){
  if(predicate(samples[i],i)){run++;if(run>=count)return i-run+1;}
  else run=0;
 }
 return -1;
}
function nearestByTime(samples,timeS,maxDelta=.6){
 if(!samples?.length||!finite(timeS))return null;
 let lo=0,hi=samples.length-1;
 while(lo<hi){
  const mid=Math.floor((lo+hi)/2);
  if(samples[mid].timeS<timeS)lo=mid+1;else hi=mid;
 }
 const choices=[samples[lo],samples[Math.max(0,lo-1)]].filter(Boolean)
  .sort((a,b)=>Math.abs(a.timeS-timeS)-Math.abs(b.timeS-timeS));
 return choices.length&&Math.abs(choices[0].timeS-timeS)<=maxDelta?choices[0]:null;
}
export function comparePressureGps(pressureSamples,sensorSamples){
 const gps=(sensorSamples||[]).filter(s=>s.gpsCurrent&&finite(s.timeS)&&finite(s.gpsAltitudeM));
 const pressure=(pressureSamples||[]).filter(s=>finite(s.timeS)&&finite(s.altitudeM));
 if(gps.length<2||pressure.length<2)return {
  matched:0,rmseM:null,meanDeltaM:null,maxAbsDeltaM:null,status:"INSUFFICIENT",
  note:"Need at least two current GPS altitude fixes matched to valid pressure altitude."
 };
 const baseline=gps[0].gpsAltitudeM;
 const deltas=[];
 for(const fix of gps){
  const p=nearestByTime(pressure,fix.timeS);
  if(!p)continue;
  const gpsRelative=fix.gpsAltitudeM-baseline;
  deltas.push(gpsRelative-p.altitudeM);
 }
 if(deltas.length<2)return {
  matched:deltas.length,rmseM:null,meanDeltaM:null,maxAbsDeltaM:null,status:"INSUFFICIENT",
  note:"GPS and pressure timestamps did not overlap closely enough."
 };
 const mean=deltas.reduce((a,b)=>a+b,0)/deltas.length;
 const rmse=Math.sqrt(deltas.reduce((a,b)=>a+b*b,0)/deltas.length);
 const max=Math.max(...deltas.map(Math.abs));
 return {
  matched:deltas.length,rmseM:rmse,meanDeltaM:mean,maxAbsDeltaM:max,
  status:rmse<=8?"CONSISTENT":rmse<=20?"REVIEW":"DIVERGENT",
  note:"GPS altitude is compared as change from the first current fix; pressure altitude remains the high-rate relative source."
 };
}
export function deriveFlightEvents(pressureAnalysis,sensorReport=null){
 const pressureEvents=(pressureAnalysis?.phases||[]).map(p=>({
  name:p.name,timeS:p.timeS,altitudeM:p.altitudeM??null,
  confidence:p.confidenceScore??(p.confidence==="high"?85:p.confidence==="medium"?65:45),
  confidenceLabel:String(p.confidence||"low").toUpperCase(),
  source:"pressure",method:p.method||"pressure-derived"
 }));
 const sensors=sensorReport?.samples||[];
 const imu=sensors.filter(s=>s.imuValid&&finite(s.timeS)&&finite(s.accelerationG));
 const gps=sensors.filter(s=>s.gpsCurrent&&finite(s.timeS)&&finite(s.gpsSpeedMps));
 const launch=pressureEvents.find(e=>e.name==="launch");
 if(launch&&imu.length){
  const near=imu.filter(s=>Math.abs(s.timeS-launch.timeS)<=1.0);
  const peak=near.reduce((m,s)=>Math.max(m,s.accelerationG),0);
  if(peak>=1.25){
   launch.confidence=Math.min(98,launch.confidence+12);
   launch.confidenceLabel=label(launch.confidence).label;
   launch.corroboration="IMU acceleration near launch";
  }
 }
 if(launch&&gps.length){
  const near=gps.filter(s=>s.timeS>=launch.timeS&&s.timeS<=launch.timeS+3);
  if(near.some(s=>s.gpsSpeedMps>=3)){
   launch.confidence=Math.min(98,launch.confidence+8);
   launch.confidenceLabel=label(launch.confidence).label;
   launch.corroboration=(launch.corroboration?launch.corroboration+" + ":"")+"GPS speed increase";
  }
 }

 // Burnout cannot be measured directly without motor data. Use only a clearly
 // labeled IMU estimate after a high-acceleration launch pulse.
 let burnout=null;
 if(launch&&imu.length){
  const start=imu.findIndex(s=>s.timeS>=launch.timeS);
  const strong=sustained(imu,start,s=>s.accelerationG>=1.35,3);
  if(strong>=0){
   const settle=sustained(imu,strong+3,s=>s.accelerationG<=1.18,5);
   if(settle>=0&&imu[settle].timeS-launch.timeS<=12){
    burnout={
     name:"likely_burnout",timeS:imu[settle].timeS,altitudeM:null,
     confidence:60,confidenceLabel:"MEDIUM",source:"imu",
     method:"acceleration pulse followed by sustained near-1 g",
     note:"Estimated transition only; no motor ignition/burn sensor is present."
    };
   }
  }
 }
 const events=[...pressureEvents];
 if(burnout)events.push(burnout);
 events.sort((a,b)=>a.timeS-b.timeS);
 return events;
}
export function buildFlightSummary(pressureAnalysis,sensorReport=null){
 const samples=pressureAnalysis?.samples||[];
 const valid=samples.filter(s=>s.valid&&finite(s.altitudeM));
 const events=deriveFlightEvents(pressureAnalysis,sensorReport);
 const launch=events.find(e=>e.name==="launch");
 const apogee=events.find(e=>e.name==="apogee");
 const landing=events.find(e=>e.name==="landing");
 const duration=valid.length>1?valid.at(-1).timeS-valid[0].timeS:0;
 const flightDuration=launch&&landing?Math.max(0,landing.timeS-launch.timeS):null;
 const confidenceEvents=events.filter(e=>finite(e.confidence));
 const averageConfidence=confidenceEvents.length?
  confidenceEvents.reduce((sum,e)=>sum+e.confidence,0)/confidenceEvents.length:null;
 const gpsCompare=comparePressureGps(samples,sensorReport?.samples||[]);
 return {
  schema:ANALYSIS_SCHEMA,
  apogeeM:finite(pressureAnalysis?.peakAltitudeM)?pressureAnalysis.peakAltitudeM:null,
  maxAscentMps:finite(pressureAnalysis?.maxAscentMps)?pressureAnalysis.maxAscentMps:null,
  maxDescentMps:finite(pressureAnalysis?.maxDescentMps)?Math.abs(pressureAnalysis.maxDescentMps):null,
  recordingDurationS:duration,
  estimatedFlightDurationS:flightDuration,
  peakMeasuredTotalG:sensorReport?.summary?.validImu?sensorReport.summary.peakTotalG:null,
  maxMeasuredGpsSpeedMps:sensorReport?.summary?.currentGps?sensorReport.summary.maxSpeedMps:null,
  averageEventConfidence:averageConfidence,
  eventConfidence:label(averageConfidence??0),
  events,
  gpsPressureComparison:gpsCompare,
  note:"Pressure-derived motion is an estimate. Measured IMU magnitude is not automatically vertical acceleration. Burnout is only a labeled estimate when IMU evidence supports it."
 };
}
