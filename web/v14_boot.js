/* Loaded before the preserved V13 dashboard scripts. Only /api/ uses this adapter.
 * Fail closed for controls when Feather status cannot be verified. LoRa is read-only.
 * Cross-origin HTTPS-to-local-HTTP permissions vary by browser, particularly Safari. */
(function(){
"use strict";
const nativeFetch=window.fetch.bind(window);
const FEATHER="http://192.168.4.1";
const local=location.hostname==="192.168.4.1" ||
  ((location.hostname==="localhost"||location.hostname==="127.0.0.1")&&location.port==="8765");
const state={wifiAt:0,wifiVersion:null,radio:null,radioAt:0,lastPosition:null,lastPositionAt:0,radioError:"",lastStatus:null,lastWifiStatus:null,lastStatusAt:0,bridge:local&&location.hostname!=="192.168.4.1"};
function wifiFresh(){return Date.now()-state.wifiAt<5500;}
function radioFresh(){return !!state.radio && Date.now()-state.radioAt<4400;}
function validPosition(lat,lon){return Number.isFinite(+lat)&&Number.isFinite(+lon)&&Math.abs(+lat)<=90&&Math.abs(+lon)<=180&&!(+lat===0&&+lon===0);}
function source(){return wifiFresh()?"wifi":radioFresh()?"lora":"none";}
function feedRadio(raw){
  if(!raw || !["telemetry","radio_bench"].includes(raw.type))return false;
  if(!Number.isFinite(+raw.altitude_m)||!Number.isFinite(+raw.battery_v)||
     Math.abs(+raw.altitude_m)>100000||+raw.battery_v<0||+raw.battery_v>8)return false;
  const packet=Object.assign({},raw,{receivedAt:Date.now()});
  state.radio=packet;state.radioAt=packet.receivedAt;
  state.lastStatus=radioData();state.lastStatusAt=packet.receivedAt;
  if(validPosition(packet.latitude,packet.longitude)){
    state.lastPosition=[+packet.latitude,+packet.longitude];state.lastPositionAt=Date.now();
  }
  window.dispatchEvent(new Event("green-ion-link"));
  return true;
}
function radioData(){
  const p=state.radio||{}, gps=p.gps_status==="live"&&validPosition(p.latitude,p.longitude);
  return {
    v13_source:"lora",v13_wifi_connected:false,recording:false,recorder_mode:-1,
    flight_state:p.flight_state??null,relative_altitude_m:p.altitude_m??null,
    pressure_pa:p.pressure_pa??null,battery_v:p.battery_v??null,
    acceleration_g:p.acceleration_g??null,acceleration_x_g:p.acceleration_x_g??null,
    acceleration_y_g:p.acceleration_y_g??null,acceleration_z_g:p.acceleration_z_g??null,
    rotation_x_dps:p.rotation_x_dps??null,rotation_y_dps:p.rotation_y_dps??null,
    rotation_z_dps:p.rotation_z_dps??null,vertical_speed_mps:null,
    gps_speed_mps:p.speed_mps??null,gps_altitude_m:p.gps_altitude_m??null,
    gps_status:gps?"live":state.lastPosition?"stale":"missing",
    latitude:gps?+p.latitude:state.lastPosition?.[0]??null,
    longitude:gps?+p.longitude:state.lastPosition?.[1]??null,
    gps_fix_age_ms:p.gps_fix_age_ms??null,satellites:p.satellites??null,
    preflight_state:"WIFI_UNAVAILABLE",preflight_flags:0,
    hardware_tests_allowed:false,ota_available:false,
    sd_ok:false,imu_ok:false,barometer_ready:false,radio_ok:true,
    selected_rocket:"Received LoRa telemetry",
    firmware_build:"Feather Wi-Fi unavailable",
    radio_packet_age_ms:Date.now()-state.radioAt,
    radio_rssi_dbm:p.rssi_dbm??null
  };
}
function responseForRadio(){
  return new Response(JSON.stringify(radioData()),{
    status:200,headers:{"Content-Type":"application/json","Cache-Control":"no-store"}
  });
}
async function interceptedFetch(input, options){
  const path=typeof input==="string"?input:input?.url;
  if(typeof path!=="string"||!path.startsWith("/api/"))return nativeFetch(input,options);
  const isStatus=/^\/api\/status(?:[?]|$)/.test(path);
  const method=(options?.method||"GET").toUpperCase();
  if(!isStatus && !wifiFresh() && !state.bridge){
    return new Response(JSON.stringify({success:false,message:"Feather Wi-Fi not verified. Controls and SD access require the Feather local connection."}),{status:503,headers:{"Content-Type":"application/json"}});
  }
  try{
    const url=local?path:FEATHER+path;
    const response=await nativeFetch(url,options);
    if(isStatus){
      if(!response.ok)throw Error("Feather status unavailable");
      const data=await response.clone().json();
      const verified=!!data && (
        (data.v13_source==="wifi" && data.v13_wifi_connected===true) ||
        (typeof data.relative_altitude_m!=="undefined" &&
          (typeof data.recorder_mode!=="undefined" || typeof data.firmware_build==="string"))
      );
      if(verified && data.v13_source!=="lora" && data.v13_source!=="demo"){
        state.wifiAt=Date.now();
        state.wifiVersion=Number(data.version??13);
        state.lastStatus=data;state.lastWifiStatus=data;state.lastStatusAt=state.wifiAt;
      }else{
        state.wifiAt=0;
      }
      window.dispatchEvent(new Event("green-ion-link"));
      if(data.v13_source==="lora")return response;
      if(!verified)throw Error("This address did not return a GREEN ION Feather status");
    }
    return response;
  }catch(error){
    if(isStatus){
      state.wifiAt=0;
      window.dispatchEvent(new Event("green-ion-link"));
      if(radioFresh())return responseForRadio();
    }
    throw error;
  }
}
window.GREEN_ION_TRANSPORT={
  state,source,wifiFresh,radioFresh,feedRadio,
  apiUrl:path=>local?path:FEATHER+path,
  nativeFetch,localBridge:state.bridge,
  lastPosition:()=>state.lastPosition,
  positionAge:()=>state.lastPosition?Date.now()-state.lastPositionAt:null,
  statusFromRadio:radioData
};
window.greenIonApiUrl=window.GREEN_ION_TRANSPORT.apiUrl;
window.fetch=interceptedFetch;
})();
