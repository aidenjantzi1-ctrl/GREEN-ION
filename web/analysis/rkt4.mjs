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

export function analyzeFlight(log) {
  const warnings = [...log.warnings]; let previous = null, peakAltitudeM = 0, maxAscentMps = -Infinity, maxDescentMps = Infinity;
  const samples = log.samples.map((raw, index) => {
    const timeS = raw.timestampUs / 1e6, altitude = raw.valid ? altitudeM(raw.pressurePa, log.baselinePressurePa) : NaN;
    let velocityMps = NaN, accelerationMps2 = NaN;
    if (previous && timeS > previous.timeS && Number.isFinite(altitude) && Number.isFinite(previous.altitudeM)) {
      velocityMps = (altitude - previous.altitudeM) / (timeS - previous.timeS);
      if (Number.isFinite(previous.velocityMps)) accelerationMps2 = (velocityMps - previous.velocityMps) / (timeS - previous.timeS);
    }
    const sample = { ...raw, index, timeS, altitudeM: altitude, velocityMps, accelerationMps2, estimatedVerticalG: Number.isFinite(accelerationMps2) ? accelerationMps2 / 9.80665 : NaN };
    if (Number.isFinite(altitude)) peakAltitudeM = Math.max(peakAltitudeM, altitude);
    if (Number.isFinite(velocityMps)) { maxAscentMps = Math.max(maxAscentMps, velocityMps); maxDescentMps = Math.min(maxDescentMps, velocityMps); }
    previous = sample; return sample;
  });
  const phases = estimatePhases(samples);
  warnings.push("Altitude, velocity, acceleration, G-force, and phases are pressure-derived estimates.");
  return { ...log, samples, peakAltitudeM, maxAscentMps: Number.isFinite(maxAscentMps) ? maxAscentMps : NaN, maxDescentMps: Number.isFinite(maxDescentMps) ? maxDescentMps : NaN, phases, warnings, analysisVersion: 1 };
}

export function estimatePhases(samples) {
  const phases = []; let launch = null, apogee = null;
  for (const s of samples) {
    if (!launch && s.altitudeM > 3 && s.velocityMps > 2) { launch = s; phases.push({ name: "launch", timeS: s.timeS, confidence: "low" }); }
    if (launch && !apogee && s.velocityMps <= 0 && s.altitudeM > 5) { apogee = s; phases.push({ name: "apogee", timeS: s.timeS, confidence: "low" }); }
  }
  if (launch && apogee) phases.push({ name: "descent", timeS: apogee.timeS, confidence: "low" });
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
