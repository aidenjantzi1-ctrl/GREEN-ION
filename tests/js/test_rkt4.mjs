import assert from "node:assert/strict";
import { analyzeFlight, analysisCsv, flightReport, parseRkt4 } from "../../web/analysis/rkt4.mjs";

function fixture(trailing = 0) {
  const bytes = new Uint8Array(32 + 16 * 4 + trailing), d = new DataView(bytes.buffer);
  for (const [i, c] of [..."RKT4"].entries()) bytes[i] = c.charCodeAt(0);
  d.setUint16(4, 2, true); d.setUint16(6, 32, true); d.setUint16(8, 16, true); d.setUint16(10, 25, true); d.setFloat32(12, 101325, true); d.setUint16(16, 4000, true);
  [[0,101325],[40000,101000],[80000,100700],[120000,100600]].forEach(([t,p], i) => { const o=32+i*16; d.setUint32(o,t,true); d.setFloat32(o+4,p,true); d.setFloat32(o+8,20,true); d.setUint16(o+12,4000,true); d.setUint16(o+14,1,true); });
  return bytes;
}
const original = fixture(), before = original.slice();
const parsed = parseRkt4(original); assert.equal(parsed.samples.length, 4); assert.deepEqual(original, before);
assert.equal(parseRkt4(fixture(3)).trailingBytes, 3);
const analysis = analyzeFlight(parsed); assert.ok(analysis.peakAltitudeM > 0); assert.match(analysis.warnings.at(-1), /estimates/);
assert.match(analysisCsv(analysis), /estimated_vertical_g/); assert.equal(flightReport(analysis).samples, 4);
assert.throws(() => parseRkt4(new Uint8Array(12)), /shorter/);
console.log("RKT4 browser analysis tests passed");
