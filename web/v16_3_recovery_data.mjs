/* Strict GPS fix validation; never overwrite last known with an invalid packet. */
export function validFix(lat,lon){
 return typeof lat==="number"&&Number.isFinite(lat)&&Math.abs(lat)<=90&&
  typeof lon==="number"&&Number.isFinite(lon)&&Math.abs(lon)<=180;
}
export function observedFix(raw,source,now=Date.now()){
 if(!raw||raw.gps_status!=="live"||!validFix(raw.latitude,raw.longitude))return null;
 const age=raw.gps_fix_age_ms;
 if(!Number.isFinite(age)||age<0||age>30000||!["Wi-Fi","LoRa"].includes(source))return null;
 return {lat:raw.latitude,lon:raw.longitude,source,at:now-age,receivedAt:now,
  satellites:Number.isInteger(raw.satellites)?raw.satellites:null,
  hdop:Number.isFinite(raw.gps_hdop)&&raw.gps_hdop>0?raw.gps_hdop:null,
  gpsAltitudeM:Number.isFinite(raw.gps_altitude_m)?raw.gps_altitude_m:null,
  gpsSpeedMps:Number.isFinite(raw.gps_speed_mps)?raw.gps_speed_mps:null,
  flightState:raw.flight_state!==null&&raw.flight_state!==undefined&&Number.isInteger(Number(raw.flight_state))&&
   Number(raw.flight_state)>=0&&Number(raw.flight_state)<=7?Number(raw.flight_state):null};
}
export function savedTrack(samples,max=120){
 const fixes=(samples||[]).filter(x=>x.gpsCurrent&&validFix(x.latitude,x.longitude));
 if(!fixes.length)return [];
 const count=Math.max(2,Math.min(500,Math.trunc(max)||120)),step=Math.max(1,Math.ceil(fixes.length/count));
 const selected=fixes.filter((_,i)=>i%step===0);
 if(selected.at(-1)!==fixes.at(-1))selected.push(fixes.at(-1));
 return selected.slice(-count).map(x=>({lat:x.latitude,lon:x.longitude,timeS:x.timeS,
  source:"SAVED SEN",archived:true,at:null,satellites:x.satellites??null,hdop:null}));
}
