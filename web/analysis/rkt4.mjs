// Offline RKT4 v2 decoder and pressure-derived analysis for GREEN ION V11.
// It never changes the original Uint8Array and requires no Feather or network.
const HEADER = 32, RECORD = 16, EXPONENT = 0.190294957;

export function parseRkt4(input) {
  const raw = input instanceof Uint8Array ? input : new Uint8Array(input);
  if (raw.byteLength < HEADER) throw new Error("RKT4 file is shorter than 32-byte header");
  const d = new DataView(raw.buffer, raw.byteOffset, raw.byteLength);
  const magic = String.fromCharCode(...raw.slice(0, 4));
  const version = d.getUint16(4, true), headerBytes = d.getUint16(6, true), recordBytes = d.getUint16(8, true);
  if (magic !== "RKT4" || version !== 2 || headerBytes !== HEADER || recordBytes !== RECORD) throw new Error("Unsupported RKT4 v2 layout");
  const rateHz = d.getUint16(10, true), baselinePressurePa = d.getFloat32(12, true), startBatteryMv = d.getUint16(16, true);
  const payload = raw.byteLength - HEADER, count = Math.floor(payload / RECORD), trailingBytes = payload % RECORD;
  const samples = [];
  for (let i = 0; i < count; i++) {
    const o = HEADER + i * RECORD;
    samples.push({ timestampUs: d.getUint32(o, true), pressurePa: d.getFloat32(o + 4, true), temperatureC: d.getFloat32(o + 8, true), batteryMv: d.getUint16(o + 12, true), valid: d.getUint16(o + 14, true) === 1 });
  }
  const warnings = [];
  if (![10, 25, 50].includes(rateHz)) warnings.push("Unexpected sample rate");
  if (trailingBytes) warnings.push(`Truncated transfer: ${trailingBytes} trailing byte(s)`);
  return { format: "RKT4", version, rateHz, baselinePressurePa, startBatteryMv, samples, trailingBytes, warnings };
}

export function altitudeM(pressurePa, baselinePressurePa) {
  return pressurePa > 0 && baselinePressurePa > 0 ? 44330 * (1 - Math.pow(pressurePa / baselinePressurePa, EXPONENT)) : NaN;
}

export function analyzeFlight(log, options = {}) {
  const smoothing = Math.min(.8, Math.max(.05, options.smoothing ?? .28));
  const warnings = [...log.warnings]; let previous = null, filtered = NaN, smoothAcceleration = NaN, peakAltitudeM = 0, maxAscentMps = -Infinity, maxDescentMps = Infinity;
  const samples = log.samples.map((raw, index) => {
    const timeS = raw.timestampUs / 1e6, measuredAltitude = raw.valid ? altitudeM(raw.pressurePa, log.baselinePressurePa) : NaN;
    if (Number.isFinite(measuredAltitude)) filtered = Number.isFinite(filtered) ? filtered + smoothing * (measuredAltitude - filtered) : measuredAltitude;
    const altitude = raw.valid ? filtered : NaN;
    let velocityMps = NaN, accelerationMps2 = NaN;
    if (previous && timeS > previous.timeS && Number.isFinite(altitude) && Number.isFinite(previous.altitudeM)) {
      const dt=timeS-previous.timeS;
      if (dt <= Math.max(.5, 4/log.rateHz)) {
        velocityMps = (altitude - previous.altitudeM) / dt;
        if (Math.abs(velocityMps) > 500) { warnings.push(`Rejected motion spike at sample ${index}`); velocityMps=NaN; }
        if (Number.isFinite(previous.velocityMps) && Number.isFinite(velocityMps)) { const rawA=(velocityMps-previous.velocityMps)/dt; smoothAcceleration=Number.isFinite(smoothAcceleration) ? .3*rawA+.7*smoothAcceleration : rawA; accelerationMps2=smoothAcceleration; }
      } else warnings.push(`Timestamp gap at sample ${index}`);
    }
    const sample = { ...raw, index, timeS, altitudeM: altitude, velocityMps, accelerationMps2, estimatedVerticalG: Number.isFinite(accelerationMps2) ? accelerationMps2 / 9.80665 : NaN };
    if (Number.isFinite(altitude)) peakAltitudeM = Math.max(peakAltitudeM, altitude);
    if (Number.isFinite(velocityMps)) { maxAscentMps = Math.max(maxAscentMps, velocityMps); maxDescentMps = Math.min(maxDescentMps, velocityMps); }
    previous = sample; return sample;
  });
  const phases = estimatePhases(samples);
  warnings.push("Altitude, velocity, acceleration, G-force, and phases are pressure-derived estimates.");
  return { ...log, samples, peakAltitudeM, maxAscentMps: Number.isFinite(maxAscentMps) ? maxAscentMps : NaN, maxDescentMps: Number.isFinite(maxDescentMps) ? maxDescentMps : NaN, phases, warnings, analysisVersion: 2 };
}

function confidence(score){
  const value=Math.max(0,Math.min(100,Math.round(score)));
  return {confidenceScore:value,confidence:value>=80?"high":value>=55?"medium":"low"};
}
function sustained(samples,start,predicate,count){
  let run=0;
  for(let i=start;i<samples.length;i++){
    if(predicate(samples[i],i)){run++;if(run>=count)return i-run+1;}
    else run=0;
  }
  return -1;
}
export function estimatePhases(samples) {
  const valid=samples.filter(s=>Number.isFinite(s.timeS)&&Number.isFinite(s.altitudeM));
  if(valid.length<6)return [];
  const phases=[];
  const launchIndex=sustained(valid,0,s=>s.altitudeM>3&&Number.isFinite(s.velocityMps)&&s.velocityMps>2,3);
  if(launchIndex<0)return phases;
  const launch=valid[launchIndex];
  const launchEvidence=valid.slice(launchIndex,Math.min(valid.length,launchIndex+8))
    .filter(s=>Number.isFinite(s.velocityMps)&&s.velocityMps>2).length;
  phases.push({name:"launch",timeS:launch.timeS,altitudeM:launch.altitudeM,
    ...confidence(55+Math.min(35,launchEvidence*5)),method:"sustained pressure ascent"});

  let peakIndex=launchIndex;
  for(let i=launchIndex+1;i<valid.length;i++)if(valid[i].altitudeM>valid[peakIndex].altitudeM)peakIndex=i;
  const peak=valid[peakIndex];
  const before=valid.slice(Math.max(launchIndex,peakIndex-8),peakIndex)
    .filter(s=>Number.isFinite(s.velocityMps)&&s.velocityMps>0.5).length;
  const after=valid.slice(peakIndex+1,Math.min(valid.length,peakIndex+9))
    .filter(s=>Number.isFinite(s.velocityMps)&&s.velocityMps<-0.5).length;
  if(peak.altitudeM>5){
    phases.push({name:"apogee",timeS:peak.timeS,altitudeM:peak.altitudeM,
      ...confidence(45+Math.min(25,before*4)+Math.min(25,after*4)),method:"maximum filtered pressure altitude"});
  }

  const descentIndex=sustained(valid,peakIndex+1,s=>Number.isFinite(s.velocityMps)&&s.velocityMps<-1,3);
  if(descentIndex>=0){
    const d=valid[descentIndex];
    phases.push({name:"descent",timeS:d.timeS,altitudeM:d.altitudeM,
      ...confidence(65+Math.min(25,after*4)),method:"sustained negative pressure velocity"});
  }

  // Require a sustained near-ground, low-speed period after the peak. This
  // avoids declaring landing from one noisy barometer sample.
  const landingIndex=sustained(valid,Math.max(peakIndex+1,descentIndex+1),
    s=>Math.abs(s.altitudeM)<=5&&Number.isFinite(s.velocityMps)&&Math.abs(s.velocityMps)<=3,5);
  if(landingIndex>=0){
    const l=valid[landingIndex];
    phases.push({name:"landing",timeS:l.timeS,altitudeM:l.altitudeM,
      ...confidence(78),method:"sustained near-ground low vertical speed"});
  }
  return phases;
}

export function analysisCsv(analysis) {
  const head = "time_s,pressure_pa,temperature_c,battery_v,valid,altitude_m,velocity_mps,estimated_acceleration_mps2,estimated_vertical_g";
  const rows = analysis.samples.map(s => [s.timeS,s.pressurePa,s.temperatureC,s.batteryMv,s.valid ? 1 : 0,s.altitudeM,s.velocityMps,s.accelerationMps2,s.estimatedVerticalG].map(v => Number.isFinite(v) ? v : "").join(","));
  return [head, ...rows].join("\r\n");
}

export function flightReport(analysis, name = "flight") {
  return { name, analysisVersion: analysis.analysisVersion, sampleRateHz: analysis.rateHz,
    samples: analysis.samples.length, peakAltitudeM: analysis.peakAltitudeM,
    maxAscentMps: analysis.maxAscentMps, maxDescentMps: analysis.maxDescentMps,
    phases: analysis.phases, warnings: analysis.warnings,
    disclaimer: "All motion, acceleration, G-force and phase values are pressure-derived estimates." };
}
