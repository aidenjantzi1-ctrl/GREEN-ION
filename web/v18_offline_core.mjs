export const V18_TABS=Object.freeze(["flight","recovery","data","settings"]);
export const FLIGHT_STATES=Object.freeze(["OFFLINE","READY","TESTING","ARMED","FLYING","SAVING","SAVED","ERROR"]);
export const RADIO_PROTOCOL=2;
export const PACKET_STALE_MS=3000;
export const GPS_STALE_MS=15000;

const finite=v=>v!==null&&v!==undefined&&v!==""&&Number.isFinite(Number(v))?Number(v):null;
export function validCoordinate(lat,lon){
 const a=finite(lat),o=finite(lon);
 return a!==null&&o!==null&&a>=-90&&a<=90&&o>=-180&&o<=180&&!(a===0&&o===0);
}
export function normalizeReceiverObject(packet,receivedAt=Date.now()){
 if(!packet||typeof packet!=="object"||Array.isArray(packet))return null;
 const type=String(packet.type||"");
 if(type==="receiver_status"){
  if(packet.protocol!=null&&Number(packet.protocol)!==RADIO_PROTOCOL)return null;
  return {kind:"receiver_status",type,ok:packet.ok===true,error:typeof packet.error==="string"?packet.error:"",
   receiverFirmware:typeof packet.receiver_firmware==="string"?packet.receiver_firmware:null,
   protocol:packet.protocol==null?null:Number(packet.protocol),receivedAt,raw:packet};
 }
 if(!["telemetry","radio_bench"].includes(type))return null;
 if(Number(packet.protocol)!==RADIO_PROTOCOL)return null;
 const altitudeM=finite(packet.altitude_m),batteryV=finite(packet.battery_v);
 if(altitudeM===null||Math.abs(altitudeM)>100000||batteryV===null||batteryV<0||batteryV>8)return null;
 const gpsStatus=["live","stale","missing"].includes(packet.gps_status)?packet.gps_status:"missing";
 const coords=validCoordinate(packet.latitude,packet.longitude);
 return {kind:"packet",type,protocol:RADIO_PROTOCOL,sequence:Number.isInteger(packet.sequence)?packet.sequence:null,
  uptimeMs:finite(packet.uptime_ms),altitudeM,batteryV,pressurePa:finite(packet.pressure_pa),
  gpsStatus:gpsStatus==="live"&&coords?"live":coords?"stale":"missing",
  latitude:coords?Number(packet.latitude):null,longitude:coords?Number(packet.longitude):null,
  gpsFixAgeMs:finite(packet.gps_fix_age_ms),lastPositionAgeMs:finite(packet.last_position_age_ms),
  rssiDbm:finite(packet.rssi_dbm),snrDb:finite(packet.snr_db),flightState:finite(packet.flight_state),
  satellites:finite(packet.satellites),receivedAt,raw:packet};
}
export function parseReceiverLine(line,receivedAt=Date.now()){
 const text=String(line||"").trim();
 if(!text.startsWith("{")||text.length>8192)return null;
 try{return normalizeReceiverObject(JSON.parse(text),receivedAt);}catch{return null;}
}
export function recoveryView(packet,lastKnown=null,now=Date.now()){
 const age=packet?Math.max(0,now-packet.receivedAt):null;
 const signal=!packet?"missing":age>PACKET_STALE_MS?"stale":"live";
 const current=signal==="live"&&packet?.kind==="packet"&&packet.gpsStatus==="live"&&validCoordinate(packet.latitude,packet.longitude);
 const fallback=current?{latitude:packet.latitude,longitude:packet.longitude,at:packet.receivedAt}:lastKnown;
 const positionAge=fallback?Math.max(0,now-fallback.at):null;
 const gps=current?"live":fallback?"stale":"missing";
 return {signal,packetAgeMs:age,gps,position:fallback,positionAgeMs:positionAge,
  rssiDbm:packet?.rssiDbm??null,snrDb:packet?.snrDb??null};
}
export function deriveFlightState(status,{saveVerified=false,connected=true}={}){
 if(!connected||!status)return "OFFLINE";
 if(status.flight_prep_failure||status.sd_ok===false||status.recorder_mode===4)return "ERROR";
 if(status.recording===true||status.recorder_mode===2)return "FLYING";
 if(status.flight_prep_active===true)return "TESTING";
 if(status.armed===true||status.flight_prep_ready===true||status.recorder_mode===1)return "ARMED";
 if(status.recorder_mode===3)return saveVerified?"SAVED":"SAVING";
 return "READY";
}
export function criticalFlightProblem(status){
 if(!status)return "No Feather status";
 if(status.sd_ok===false)return "microSD unavailable";
 if(status.barometer_ready===false)return "Barometer unavailable";
 if(status.imu_ok===false)return "IMU unavailable";
 if(status.flight_prep_failure)return String(status.flight_prep_failure);
 const mv=finite(status.battery_mv);
 if(mv!==null&&mv<3600)return "Battery below current V17 critical threshold";
 return "";
}
export function optionalFlightWarnings(status){
 if(!status)return [];
 const out=[];
 if(status.gps_status!=="live")out.push("GPS is not live; recovery position may be unavailable or stale.");
 if(status.radio_ok!==true)out.push("LoRa is not ready; microSD remains the authoritative flight record.");
 return out;
}
export function canPrepare(status,{saveVerified=true}={}){
 if(!status)return false;
 if(status.recording||status.armed||status.flight_prep_active)return false;
 if(status.recorder_mode===3&&!saveVerified)return false;
 return !criticalFlightProblem(status);
}
export function fileSaveEvidence(files,name){
 if(!name||!Array.isArray(files?.files))return null;
 const file=files.files.find(x=>String(x.name||"").toUpperCase()===String(name).toUpperCase());
 if(!file||file.recording_complete!==true||!Number.isFinite(Number(file.size))||Number(file.size)<=32)return null;
 return {name:file.name,size:Number(file.size),recoveryState:file.recovery_state||null};
}
export function storageMessage(persisted){
 return persisted?
  "Persistent browser storage granted. Keep exported backups anyway; browser storage is not a substitute for the original BIN files.":
  "Persistent browser storage is not guaranteed. Export backups; clearing browser data can remove local history.";
}
