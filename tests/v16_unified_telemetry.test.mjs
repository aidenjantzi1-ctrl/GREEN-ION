import test from "node:test";
import assert from "node:assert/strict";
import {acceptUnifiedTelemetry,conciseTelemetry,unifiedTelemetryState,UNIFIED_SCHEMA} from "../web/v16_telemetry.mjs";

function sample(){
 return {
  schema:UNIFIED_SCHEMA,
  generated_utc:"2026-11-12T02:00:00Z",
  mode:"wifi+lora",
  primary_source:"wifi",
  links:{
   wifi:{connection_state:"connected",freshness:"live",age_ms:250},
   lora:{freshness:"live",age_ms:400}
  },
  identity:{packet_id:"session:0:10"},
  readings:{
   altitude_m:{value:123.4,source:"wifi",age_ms:250,freshness:"live"},
   battery_v:{value:4.02,source:"wifi",age_ms:250,freshness:"live"},
   flight_state:{value:3,source:"wifi",age_ms:250,freshness:"live"},
   gps_status:{value:"live",source:"wifi",age_ms:250,freshness:"live"},
   latitude:{value:34.1,source:"wifi",age_ms:250,freshness:"live"},
   longitude:{value:-117.2,source:"wifi",age_ms:250,freshness:"live"}
  }
 };
}

test("unified telemetry validates and exposes concise mission values",()=>{
 const data=sample();
 assert.equal(acceptUnifiedTelemetry(data),true);
 const result=conciseTelemetry();
 assert.equal(result.mode,"wifi+lora");
 assert.equal(result.primarySource,"wifi");
 assert.equal(result.altitudeM,123.4);
 assert.equal(result.batteryV,4.02);
 assert.equal(result.altitudeSource,"wifi");
 assert.equal(result.packetId,"session:0:10");
});

test("same exact unified document is deduplicated",()=>{
 const data=sample();
 assert.equal(acceptUnifiedTelemetry(data),true);
 assert.equal(acceptUnifiedTelemetry(data),false);
 assert.equal(unifiedTelemetryState.lastPacketId,"session:0:10");
});

test("invalid unified telemetry is rejected",()=>{
 assert.throws(()=>acceptUnifiedTelemetry({schema:"wrong"}),/Invalid V16/);
});

test("missing readings remain null instead of invented zeros",()=>{
 const data=sample();
 data.readings.altitude_m={value:null,source:"none",freshness:"missing",age_ms:null};
 data.readings.latitude={value:null,source:"none",freshness:"missing",age_ms:null};
 acceptUnifiedTelemetry(data);
 const result=conciseTelemetry(data);
 assert.equal(result.altitudeM,null);
 assert.equal(result.latitude,null);
});
