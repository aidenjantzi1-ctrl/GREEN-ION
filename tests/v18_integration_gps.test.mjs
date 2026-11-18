import test from "node:test";
import assert from "node:assert/strict";
import {validCoordinate, parseReceiverLine, recoveryView, PACKET_STALE_MS} from "../web/v18_offline_core.mjs";

test("V18 integration rejects null, undefined and empty GPS coordinate fields", () => {
 for (const missing of [null, undefined, ""]) {
  assert.equal(validCoordinate(missing, -118.1), false, "latitude must be present");
  assert.equal(validCoordinate(34.1, missing), false, "longitude must be present");
 }
 assert.equal(validCoordinate(34.1, -118.1), true);
 const bad = {type:"telemetry",protocol:2,altitude_m:5,battery_v:4,gps_status:"live",
              latitude:null,longitude:-118.1};
 const parsed = parseReceiverLine(JSON.stringify(bad), 1000);
 assert.ok(parsed);
 assert.equal(parsed.latitude, null);
 assert.equal(parsed.gpsStatus, "missing");
});

test("V18 integration never labels an expired serial GPS packet LIVE", () => {
 const p = parseReceiverLine(JSON.stringify({
  type:"telemetry",protocol:2,altitude_m:5,battery_v:4,gps_status:"live",
  latitude:34.1,longitude:-118.1
 }), 1000);
 assert.ok(p);
 const view = recoveryView(p,{latitude:34.1,longitude:-118.1,at:1000},
                           1000+PACKET_STALE_MS+1);
 assert.equal(view.signal,"stale");
 assert.notEqual(view.gps,"live");
});
