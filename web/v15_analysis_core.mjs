/* Local advanced flight analysis. All motion estimates remain explicitly
 * pressure-derived; the original RKT4/GIS1 files are never modified. */
import {parseRkt4,analyzeFlight} from "./analysis/rkt4.mjs";
import {parseGis1} from "./analysis/gis1.mjs";
import {buildFlightSummary} from "./v16_1_analysis_core.mjs";
const finite=v=>Number.isFinite(v);
function finiteMax(values){let max=null;for(const v of values)if(finite(v))max=max===null?v:Math.max(max,v);return max;}
export function analyzeOriginals(binBytes,senBytes=null,{name="flight",checksum=null}={}){
 const parsed=parseRkt4(binBytes),analysis=analyzeFlight(parsed);
 const rows=analysis.samples,valid=rows.filter(r=>r.valid&&finite(r.pressurePa)&&r.pressurePa>0);
 let gaps=0,nonMonotonic=0,previous=null;
 const expected=1/parsed.rateHz;
 for(const row of rows){
  if(previous!==null){
   const dt=row.timeS-previous;
   if(dt<=0)nonMonotonic++;
   else if(dt>Math.max(0.5,4*expected))gaps++;
  }
  previous=row.timeS;
 }
 const count=rows.length,validFraction=count?valid.length/count:0;
 const quality=Math.round(Math.max(0,Math.min(100,
  validFraction*75+(gaps===0?15:Math.max(0,15-3*gaps))+
  (parsed.trailingBytes===0?10:0)-Math.min(nonMonotonic*3,25))));
 const batteries=rows.map(r=>r.batteryMv).filter(v=>finite(v)&&v>0);
 const minBattery=batteries.length?Math.min(...batteries)/1000:null;
 const startBattery=batteries.length?batteries[0]/1000:null;
 const endBattery=batteries.length?batteries.at(-1)/1000:null;
 const maxPressureAccel=finiteMax(rows.map(x=>Math.abs(x.accelerationMps2)));
 const warnings=[...new Set(analysis.warnings)].slice(0,60);
 if(!count)warnings.unshift("Recording contains no complete samples");
 if(!valid.length)warnings.unshift("No valid pressure samples");
 if(gaps)warnings.push(gaps+" timestamp gaps exceed the accepted continuity interval");
 if(nonMonotonic)warnings.push(nonMonotonic+" non-increasing timestamps");
 let sensor=null,sensorReport=null;
 if(senBytes){
  const sen=parseGis1(senBytes);sensorReport=sen;
  let lastFix=null;
  for(let i=sen.samples.length-1;i>=0;i--){if(sen.samples[i].gpsCurrent&&sen.samples[i].latitude!==null){lastFix=sen.samples[i];break;}}
  sensor={
   format:"GIS1",fileVerifiedPrefixRecords:sen.decoded,totalRecords:sen.count,
   crc32ValidatedPrefix:sen.invalidIndex===null,
   imuSamples:sen.summary.validImu,
   freshGpsSamples:sen.summary.currentGps,
   maxMeasuredTotalG:sen.summary.validImu?sen.summary.peakTotalG:null,
   maxMeasuredGpsSpeedMps:sen.summary.currentGps?sen.summary.maxSpeedMps:null,
   maxSatellites:sen.summary.maxSatellites,
   lastObservedGpsFix:lastFix?{latitude:lastFix.latitude,longitude:lastFix.longitude,gpsAltitudeM:lastFix.gpsAltitudeM,timeS:lastFix.timeS}:null,
   warnings:sen.warnings.slice(0,20)
  };
  if(sen.invalidIndex!==null)warnings.push("Sensor CRC32 failed; only the verified prefix was decoded");
 }else warnings.push("No companion GIS1 sensor recording available for this analysis");
 const enhanced=buildFlightSummary(analysis,sensorReport);
 if(enhanced.gpsPressureComparison.status==="DIVERGENT")
  warnings.push("GPS altitude change and pressure altitude diverge substantially; review both sources.");
 return {
  schema:"GREEN-ION-V15-REPORT-1",analysisVersion:16.1,name,checksum,createdAt:new Date().toISOString(),
  observed:{sampleCount:count,validPressureSamples:valid.length,rateHz:parsed.rateHz,
   durationS:count>1?Math.max(0,rows.at(-1).timeS-rows[0].timeS):0,
   minBatteryV:minBattery,startBatteryV:startBattery,endBatteryV:endBattery,
   timestampGaps:gaps,nonMonotonicTimestamps:nonMonotonic,trailingBytes:parsed.trailingBytes},
  estimates:{apogeeM:finite(analysis.peakAltitudeM)?analysis.peakAltitudeM:null,
   maxAscentMps:finite(analysis.maxAscentMps)?analysis.maxAscentMps:null,
   maxDescentMps:finite(analysis.maxDescentMps)?analysis.maxDescentMps:null,
   peakPressureDerivedAccelerationMps2:maxPressureAccel,
   detectedPhases:analysis.phases},
  sensors:sensor,enhanced,dataCompletenessScore:quality,
  evidenceStatus:parsed.trailingBytes||nonMonotonic||sensor?.crc32ValidatedPrefix===false||
    enhanced.gpsPressureComparison.status==="DIVERGENT"?"REVIEW":"ANALYZED",
  warnings,
  limitations:"Completeness score evaluates file integrity and samples, NOT sensor accuracy or launch safety. Pressure-derived speed, acceleration and phases are estimates. GIS1 measured acceleration is not automatically earth-frame motion. GPS gives observed fixes only. Burnout is only a labeled estimate when IMU evidence supports it."
 };
}
