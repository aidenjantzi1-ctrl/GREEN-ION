// GREEN ION V11 browser analysis engine. No network or Feather connection required.
// RKT4 v2: 32-byte little-endian header, followed by 16-byte samples.
const RKT4_HEADER_BYTES = 32;
const RKT4_SAMPLE_BYTES = 16;
const ALTITUDE_EXPONENT = 0.190294957;

export function parseRkt4(input) {
  const bytes = input instanceof Uint8Array ? input : new Uint8Array(input);
  const warnings = [];
  if (bytes.byteLength < RKT4_HEADER_BYTES) throw new Error("RKT4 file is shorter than its header");
  const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
  if (String.fromCharCode(...bytes.slice(0, 4)) !== "RKT4") throw new Error("Invalid RKT4 signature");
  const version = view.getUint16(4, true), headerBytes = view.getUint16(6, true);
  const sampleBytes = view.getUint16(8, true), sampleRateHz = view.getUint16(10, true);
  const baselinePressurePa = view.getFloat32(12, true), batteryMv = view.getUint16(16, true);
  if (version !== 2 || headerBytes !== 32 || sampleBytes !== 16) warnings.push("Unsupported or extended RKT4 layout");
  const payload = bytes.byteLength - RKT4_HEADER_BYTES;
  const completeSamples = Math.floor(payload / RKT4_SAMPLE_BYTES);
  if (payload % RKT4_SAMPLE_BYTES) warnings.push("Trailing partial sample detected");
  const samples = [];
  for (let i = 0; i < completeSamples; i++) {
    const o = RKT4_HEADER_BYTES + i * RKT4_SAMPLE_BYTES;
    const timestampUs = view.getUint32(o, true), pressurePa = view.getFloat32(o + 4, true);
    const temperatureC = view.getFloat32(o + 8, true), batteryMvSample = view.getUint16(o + 12, true);
    const valid = view.getUint16(o + 14, true) === 1;
    if (valid && (!Number.isFinite(pressurePa) || pressurePa <= 0)) warnings.push(`Invalid pressure at sample ${i}`);
    samples.push({ timestampUs, timeS: timestampUs / 1e6, pressurePa, temperatureC, batteryMv: batteryMvSample, valid });
  }
  return { version, sampleRateHz, baselinePressurePa, batteryMv, samples, warnings,
    complete: payload % RKT4_SAMPLE_BYTES === 0 };
}

export function altitudeFromPressure(pressurePa, baselinePressurePa) {
  if (!(pressurePa > 0) || !(baselinePressurePa > 0)) return NaN;
  return 44330 * (1 - Math.pow(pressurePa / baselinePressurePa, ALTITUDE_EXPONENT));
}

export function analyzeFlight(log) {
  const samples = log.samples.map(s => ({ ...s, altitudeM: s.valid ? altitudeFromPressure(s.pressurePa, log.baselinePressurePa) : NaN, velocityMps: NaN }));
  for (let i = 1; i < samples.length; i++) {
    const prev = samples[i - 1], s = samples[i], dt = s.timeS - prev.timeS;
    if (dt > 0 && Number.isFinite(s.altitudeM) && Number.isFinite(prev.altitudeM)) s.velocityMps = (s.altitudeM - prev.altitudeM) / dt;
  }
  let peakAltitudeM = 0;
  for (const s of samples) if (Number.isFinite(s.altitudeM)) peakAltitudeM = Math.max(peakAltitudeM, s.altitudeM);
  return { ...log, samples, peakAltitudeM, estimated: true,
    warnings: [...log.warnings, "Velocity and phases are pressure-derived estimates"] };
}
