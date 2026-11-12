export const UNIFIED_SCHEMA="GREEN-ION-V16-TELEMETRY-1";

export const unifiedTelemetryState={
 latest:null,
 receivedAt:0,
 lastPacketId:null
};

function finite(value){
 return typeof value==="number"&&Number.isFinite(value);
}

export function acceptUnifiedTelemetry(data){
 if(!data||data.schema!==UNIFIED_SCHEMA||typeof data.links!=="object"||typeof data.readings!=="object"){
  throw Error("Invalid V16 unified telemetry document");
 }
 const packetId=data.identity?.packet_id??null;
 if(packetId&&packetId===unifiedTelemetryState.lastPacketId&&data.generated_utc===unifiedTelemetryState.latest?.generated_utc){
  return false;
 }
 unifiedTelemetryState.latest=data;
 unifiedTelemetryState.receivedAt=Date.now();
 if(packetId)unifiedTelemetryState.lastPacketId=packetId;
 if(typeof window!=="undefined"){
  window.GREEN_ION_UNIFIED_TELEMETRY=data;
  window.dispatchEvent(new CustomEvent("green-ion-v16-telemetry",{detail:data}));
 }
 return true;
}

export function conciseTelemetry(data=unifiedTelemetryState.latest){
 if(!data)return null;
 const value=name=>data.readings?.[name]?.value??null;
 const source=name=>data.readings?.[name]?.source??"none";
 return {
  mode:data.mode,
  primarySource:data.primary_source,
  altitudeM:finite(value("altitude_m"))?value("altitude_m"):null,
  batteryV:finite(value("battery_v"))?value("battery_v"):null,
  flightState:value("flight_state"),
  gpsStatus:value("gps_status"),
  latitude:finite(value("latitude"))?value("latitude"):null,
  longitude:finite(value("longitude"))?value("longitude"):null,
  altitudeSource:source("altitude_m"),
  batterySource:source("battery_v"),
  packetId:data.identity?.packet_id??null,
  packetAgeMs:data.links?.lora?.age_ms??null,
  wifiState:data.links?.wifi?.connection_state??"disconnected",
  radioState:data.links?.lora?.freshness??"missing"
 };
}
