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
 const gpsFixAgeMs=finite(packet.gps_fix_age_ms);
 const gpsFresh=gpsStatus==="live"&&coords&&(gpsFixAgeMs===null||gpsFixAgeMs<=GPS_STALE_MS);
 return {kind:"packet",type,protocol:RADIO_PROTOCOL,sequence:Number.isInteger(packet.sequence)?packet.sequence:null,
  uptimeMs:finite(packet.uptime_ms),altitudeM,batteryV,pressurePa:finite(packet.pressure_pa),
  gpsStatus:gpsFresh?"live":coords?"stale":"missing",
  latitude:coords?Number(packet.latitude):null,longitude:coords?Number(packet.longitude):null,
  gpsFixAgeMs,lastPositionAgeMs:finite(packet.last_position_age_ms),
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
 const linkFresh=packet?.kind==="packet"&&age!==null&&age<=PACKET_STALE_MS;
 const current=linkFresh&&packet.gpsStatus==="live"&&validCoordinate(packet.latitude,packet.longitude);
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


export const DEVTEST_STATUS_FRESH_MS=3000;
export const DEVTEST_RESULTS=Object.freeze(["PASS","FAIL","NOT TESTED"]);
const devResult=(id,label,state,evidence,next)=>({id,label,state,evidence,next});
const devBool=(id,label,value,fresh,passEvidence,failEvidence,next)=>{
 if(!fresh)return devResult(id,label,"NOT TESTED","No fresh Feather status is available.",next);
 if(value===true)return devResult(id,label,"PASS",passEvidence,next);
 if(value===false)return devResult(id,label,"FAIL",failEvidence,next);
 return devResult(id,label,"NOT TESTED","Firmware did not provide this status field.",next);
};

export function developmentStatusChecks(status,{connected=false,lastStatusAt=0,now=Date.now()}={}){
 const age=lastStatusAt>0?Math.max(0,now-lastStatusAt):null;
 const fresh=!!(connected&&status&&age!==null&&age<=DEVTEST_STATUS_FRESH_MS);
 const items=[
  devResult("bridge","Windows bridge / Feather status",fresh?"PASS":"FAIL",
   fresh?"Fresh /api/status received "+age+" ms ago.":connected&&status?"Last /api/status is stale.":"No current Feather status through the local Windows bridge.",
   fresh?"Keep the Windows bridge open for live checks.":"Start the Windows ground station, connect the Feather, then retry."),
  devBool("sd","microSD reported ready",status?.sd_ok,fresh,
   "Firmware currently reports microSD available. This does not perform a write/delete stress test.",
   "Firmware currently reports microSD unavailable.",
   "Check card seating/formatting; physical write/read/temp-delete remains a separate bench test."),
  devBool("imu","IMU reported ready",status?.imu_ok,fresh,
   "Firmware currently reports the IMU available.",
   "Firmware currently reports the IMU unavailable.",
   "Check LSM6DSO32/STEMMA QT wiring and use the explicit bench diagnostic."),
  devBool("barometer","Barometer reported ready",status?.barometer_ready,fresh,
   "Firmware currently reports the barometer ready.",
   "Firmware currently reports the barometer unavailable.",
   "Check BMP388/STEMMA QT wiring and rerun while stationary.")
 ];
 let mv=finite(status?.battery_mv);
 if(mv===null){const v=finite(status?.battery_v);if(v!==null)mv=Math.round(v*1000);}
 if(!fresh)items.push(devResult("battery","Battery measurement","NOT TESTED","No fresh Feather status is available.","Connect the Feather and retry."));
 else if(mv===null)items.push(devResult("battery","Battery measurement","NOT TESTED","Firmware did not provide a valid battery reading.","Check battery wiring/status."));
 else if(mv>=3600)items.push(devResult("battery","Battery measurement","PASS",(mv/1000).toFixed(3)+" V; at or above the current V17 3.60 V critical threshold.","Loaded endurance/reserve still requires real hardware testing."));
 else items.push(devResult("battery","Battery measurement","FAIL",(mv/1000).toFixed(3)+" V; below the current V17 3.60 V critical threshold.","Charge or replace the battery before preparation."));
 return items;
}

export function developmentReceiverChecks(packet,lastKnown=null,{portConnected=false,now=Date.now()}={}){
 if(!portConnected)return [
  devResult("receiver","Uno receiver valid fresh packet","NOT TESTED","No Uno serial port is connected.","Recovery → CONNECT RECEIVER and select the physical Uno."),
  devResult("receiver_gps","Receiver GPS freshness","NOT TESTED","No live receiver session is available.","Connect the Uno and wait for a current outdoor GPS fix.")
 ];
 if(!packet)return [
  devResult("receiver","Uno receiver valid fresh packet","FAIL","Serial is open but no valid receiver JSON has been accepted.","Check 115200 baud, newline framing, and protocol-v2 JSON."),
  devResult("receiver_gps","Receiver GPS freshness","NOT TESTED","No valid telemetry packet has been accepted.","Wait for protocol-v2 telemetry.")
 ];
 const age=Math.max(0,now-packet.receivedAt),fresh=age<=PACKET_STALE_MS,protocolOk=packet.protocol===RADIO_PROTOCOL;
 const receiver=fresh&&protocolOk?
  devResult("receiver","Uno receiver valid fresh packet","PASS","Fresh accepted protocol-v2 receiver data; packet age "+age+" ms.","Unplug/reconnect once to exercise reconnect handling."):
  devResult("receiver","Uno receiver valid fresh packet","FAIL",
   !fresh?"Last accepted receiver packet is stale ("+age+" ms).":"Current receiver evidence does not yet prove protocol v2.",
   !fresh?"Check USB/Uno data flow; stale data is never shown as live.":"Wait for protocol-v2 telemetry or radio_bench data.");
 const view=recoveryView(packet,lastKnown,now);
 const gps=view.gps==="live"?
  devResult("receiver_gps","Receiver GPS freshness","PASS","Current packet contains a fresh valid non-zero GPS position.","This does not prove RF range or recovery performance."):
  devResult("receiver_gps","Receiver GPS freshness","NOT TESTED",
   view.gps==="stale"?"Only a stale/last-known position is available.":"No current valid GPS position is available.",
   "Move outdoors and wait for a current fix; never treat stale coordinates as live.");
 return [receiver,gps];
}

export function developmentFirmwareDiagnostic(status,{requested=false}={}){
 if(!requested)return devResult("firmware_diag","Firmware IMU + GPS diagnostic","NOT TESTED",
  "The explicit firmware diagnostic has not been requested in this browser session.",
  "On a stationary bench, use RUN FIRMWARE IMU + GPS DIAGNOSTIC.");
 if(!status)return devResult("firmware_diag","Firmware IMU + GPS diagnostic","NOT TESTED","No current Feather status is available.","Reconnect the Feather and retry.");
 const imu=String(status.imu_test_result||"IDLE").toUpperCase();
 const gps=String(status.gps_test_result||"IDLE").toUpperCase();
 const detail="IMU "+imu+": "+String(status.imu_test_message||"no message")+" · GPS "+gps+": "+String(status.gps_test_message||"no message");
 if(imu==="FAIL"||gps==="FAIL")return devResult("firmware_diag","Firmware IMU + GPS diagnostic","FAIL",detail,"Follow the firmware message, correct the hardware issue, and rerun.");
 if(imu==="PASS"&&gps==="PASS")return devResult("firmware_diag","Firmware IMU + GPS diagnostic","PASS",detail,"This is a bench diagnostic only; it is not flight qualification.");
 return devResult("firmware_diag","Firmware IMU + GPS diagnostic","NOT TESTED",detail,
  imu==="RUNNING"||gps==="RUNNING"?"Keep the Feather stationary and let the firmware finish.":"Repeat on a stationary bench; GPS may need an outdoor fix.");
}


export function developmentSerialAccessCheck({supported=true,state="unknown",message=""}={}){
 const clean=String(message||"").slice(0,160);
 if(!supported)return devResult("serial_access","Web Serial permission","FAIL","Web Serial is unavailable in this browser/context.","Use current Chrome or Edge on Windows from the local app.");
 if(state==="granted")return devResult("serial_access","Web Serial permission","PASS","The user granted serial access in this browser session.","This proves permission only; receiver packet validation is a separate check.");
 if(state==="denied")return devResult("serial_access","Web Serial permission","NOT TESTED","Serial permission was denied or cancelled"+(clean?": "+clean:"."),"Choose Recovery → CONNECT RECEIVER and explicitly select the Arduino Uno when ready.");
 if(state==="error")return devResult("serial_access","Web Serial permission","FAIL","Serial permission/connection failed"+(clean?": "+clean:"."),"Check the USB cable, close other apps using the COM port, then retry.");
 return devResult("serial_access","Web Serial permission","NOT TESTED","No serial permission attempt has been completed in this browser session.","Recovery → CONNECT RECEIVER when the physical Uno is available.");
}

export function safeDevelopmentReport(rows,{timestamp=new Date().toISOString(),previewVersion="V18 offline",gitSha=""}={}){
 const safeText=value=>String(value??"").replace(/[^a-zA-Z0-9 ._/+():-]/g,"").slice(0,120);
 const reportLabels={
  bridge:"Windows bridge / Feather status",sd:"microSD reported ready",imu:"IMU reported ready",
  barometer:"Barometer reported ready",battery:"Battery measurement",calibration:"Stationary calibration evidence",
  offline_cache:"Local offline cache registration",bin_import:"Manual original BIN import + analysis",
  serial_access:"Web Serial permission",receiver:"Uno receiver valid fresh packet",receiver_gps:"Receiver GPS freshness",
  firmware_diag:"Firmware IMU + GPS diagnostic",receiver_manual:"Receiver malformed/stale/disconnect behavior",
  offline_reload:"Actual offline restart",sd_physical:"Physical SD write/read/verify/temp-delete",
  sensor_accuracy:"Barometer/IMU physical accuracy",rf_physical:"GPS/LoRa RF + recovery timing/wake",
  multi_flight:"Two distinct saved hardware flights"
 };
 const report=[
  "GREEN ION V18 PRIVATE TEST RESULTS",
  "Timestamp: "+safeText(timestamp),
  "Preview: "+safeText(previewVersion||"V18 offline"),
  "Git SHA: "+(gitSha?safeText(gitSha):"unavailable from current bridge"),
  "Software checks do not certify hardware"
 ];
 for(const row of Array.isArray(rows)?rows:[]){
  const state=DEVTEST_RESULTS.includes(row?.state)?row.state:"NOT TESTED";
  const id=String(row?.id||"unknown").replace(/[^a-zA-Z0-9_-]/g,"").slice(0,40)||"unknown";
  report.push((reportLabels[id]||"Unknown check")+" | "+state+" | "+id);
 }
 return report.join("\n");
}
