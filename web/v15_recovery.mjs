/* V15 local-first recovery assistant. No web map API or cloud requirement.
 * Coordinates are observed fixes, never fabricated landing predictions. */
import {validCoordinate} from "./v14_utils.mjs";
import {observedFix} from "./v16_3_recovery_data.mjs";
import {initV163RecoveryTools} from "./v16_3_recovery_tools.mjs";
const KEY="green-ion-v15-recovery-fixes",MAX=120;
const $=id=>document.getElementById(id);
const t=window.GREEN_ION_TRANSPORT;
let fixes=[],lastPhase=null,userPosition=null;
function load(){
 try{
  const items=JSON.parse(localStorage.getItem(KEY)||"[]");
  if(Array.isArray(items))fixes=items.filter(x=>validCoordinate(x.lat,x.lon)&&
    Number.isFinite(x.at)&&x.at>0&&x.at<=Date.now()+60000&&["Wi-Fi","LoRa"].includes(x.source)).slice(-MAX);
 }catch{fixes=[];}
}
function capture(raw,source){
 // Only real, age-bounded fixes may enter the saved recovery trail.
 const fix=observedFix(raw,source);if(!fix)return;
 const last=fixes.at(-1);
 if(last&&last.source===source&&last.lat===fix.lat&&last.lon===fix.lon&&Math.abs(fix.at-last.at)<15000)return;
 fixes.push(fix);if(fixes.length>MAX)fixes=fixes.slice(-MAX);
 try{localStorage.setItem(KEY,JSON.stringify(fixes));}catch{}
}
function radians(v){return v*Math.PI/180;}
function distanceAndBearing(a,b){
 const R=6371000,dLat=radians(b.lat-a.lat),dLon=radians(b.lon-a.lon);
 const p1=radians(a.lat),p2=radians(b.lat);
 const h=Math.sin(dLat/2)**2+Math.cos(p1)*Math.cos(p2)*Math.sin(dLon/2)**2;
 const distance=2*R*Math.atan2(Math.sqrt(h),Math.sqrt(Math.max(0,1-h)));
 const y=Math.sin(dLon)*Math.cos(p2);
 const x=Math.cos(p1)*Math.sin(p2)-Math.sin(p1)*Math.cos(p2)*Math.cos(dLon);
 const bearing=(Math.atan2(y,x)*180/Math.PI+360)%360;
 return {distance,bearing};
}
function compass(bearing){
 const names=["N","NE","E","SE","S","SW","W","NW"];
 return names[Math.round(bearing/45)%8];
}
function formatDistance(m){
 if(!Number.isFinite(m))return "Unknown";
 return m<1000?Math.round(m)+" m":(m/1000).toFixed(m<10000?2:1)+" km";
}
function ensureLocationControls(){
 if($("v16RecoveryCurrent"))return;
 const phase=$("v15RecoveryPhase")?.closest(".row");
 const card=$("v15RecoveryCard");
 if(!card)return;
 const current=document.createElement("div");current.className="row";
 current.innerHTML='<span>Your current location</span><strong id="v16RecoveryCurrent">Not requested</strong>';
 const delta=document.createElement("div");delta.className="row";
 delta.innerHTML='<span>Distance / direction to last GPS</span><strong id="v16RecoveryDelta">Need current location</strong>';
 if(phase){phase.after(delta);phase.after(current);}else card.prepend(delta,current);
 const actions=card.querySelector(".actions");
 if(actions){
  const locate=document.createElement("button");locate.id="v16RecoveryLocateMe";locate.className="secondary";locate.textContent="Use my current location";
  const directions=document.createElement("a");directions.id="v16RecoveryDirections";directions.target="_blank";directions.rel="noopener noreferrer";directions.setAttribute("aria-disabled","true");directions.textContent="Google Maps directions";
  actions.prepend(locate,directions);
  locate.onclick=requestCurrentLocation;
 }
}
function requestCurrentLocation(){
 const status=$("v16RecoveryCurrent"),button=$("v16RecoveryLocateMe");
 if(!navigator.geolocation){if(status)status.textContent="Location unavailable in this browser";return;}
 if(button)button.disabled=true;
 if(status)status.textContent="Requesting permission...";
 navigator.geolocation.getCurrentPosition(position=>{
  const lat=Number(position.coords.latitude),lon=Number(position.coords.longitude);
  if(validCoordinate(lat,lon)){
   userPosition={lat,lon,accuracy:Number(position.coords.accuracy)||null,at:Date.now()};
   if(status)status.textContent=lat.toFixed(6)+", "+lon.toFixed(6)+(userPosition.accuracy?" · ±"+Math.round(userPosition.accuracy)+" m":"");
   render();
  }else if(status)status.textContent="Browser returned an invalid position";
  if(button)button.disabled=false;
 },error=>{
  if(status)status.textContent=error.code===1?"Location permission not allowed":"Could not get current location";
  if(button)button.disabled=false;
 },{enableHighAccuracy:true,timeout:10000,maximumAge:15000});
}
function draw(){
 const canvas=$("v15RecoveryTrack");if(!canvas)return;
 const ctx=canvas.getContext("2d"),w=canvas.width,h=canvas.height;
 ctx.fillStyle="#0b1c2b";ctx.fillRect(0,0,w,h);
 ctx.strokeStyle="#294967";ctx.lineWidth=1;
 for(let i=1;i<6;i++){
  ctx.beginPath();ctx.moveTo(i*w/6,0);ctx.lineTo(i*w/6,h);ctx.stroke();
  ctx.beginPath();ctx.moveTo(0,i*h/6);ctx.lineTo(w,i*h/6);ctx.stroke();
 }
 ctx.fillStyle="#b3cada";ctx.font="14px system-ui";
 ctx.fillText("Offline GPS fix plot — relative positions only; no terrain map",12,24);
 if(!fixes.length){ctx.fillText("Waiting for a valid live GPS fix",12,50);return;}
 const latitudes=fixes.map(x=>x.lat),longitudes=fixes.map(x=>x.lon);
 const minLat=Math.min(...latitudes),maxLat=Math.max(...latitudes),
  minLon=Math.min(...longitudes),maxLon=Math.max(...longitudes);
 const latRange=Math.max(0.0001,maxLat-minLat),lonRange=Math.max(0.0001,maxLon-minLon);
 const xy=x=>[42+(x.lon-minLon)/lonRange*(w-84),h-35-(x.lat-minLat)/latRange*(h-75)];
 ctx.beginPath();ctx.lineWidth=2;ctx.strokeStyle="#7bdbff";
 fixes.forEach((f,i)=>{const [x,y]=xy(f);if(i)ctx.lineTo(x,y);else ctx.moveTo(x,y);});
 ctx.stroke();
 const first=xy(fixes[0]),last=xy(fixes.at(-1));
 ctx.fillStyle="#72e5ab";ctx.beginPath();ctx.arc(...first,5,0,Math.PI*2);ctx.fill();
 ctx.fillStyle="#ffcd80";ctx.beginPath();ctx.arc(...last,6,0,Math.PI*2);ctx.fill();
 ctx.fillStyle="#eff8ff";ctx.fillText("N ↑",w-46,25);
}
function render(){
 const latest=fixes.at(-1);
 $("v15RecoveryFix").textContent=latest?
  latest.lat.toFixed(7)+", "+latest.lon.toFixed(7):"No confirmed GPS fix";
 $("v15RecoverySource").textContent=latest?latest.source+" · "+new Date(latest.at).toLocaleString():"Unknown";
 $("v15RecoveryAge").textContent=latest?Math.max(0,Math.floor((Date.now()-latest.at)/1000))+" seconds old":"Unknown";
 $("v15RecoveryRadio").textContent=t.radioFresh()?
  "RECEIVING · "+(t.state.radio?.rssi_dbm??"unknown")+" dBm":
  "No fresh LoRa packet; last known location may be outdated";
 $("v15RecoveryPhase").textContent=lastPhase===6||lastPhase===7?
  "Reported landed/recovery phase — confirm physically":
  "Landing not yet confirmed by recent telemetry";
 const link=$("v15RecoveryNavigate"),directions=$("v16RecoveryDirections"),delta=$("v16RecoveryDelta");
 if(latest){
  link.href="https://www.google.com/maps/search/?api=1&query="+encodeURIComponent(latest.lat+","+latest.lon);
  link.removeAttribute("aria-disabled");link.textContent="Open last GPS fix in Google Maps";
  if(userPosition){
   const d=distanceAndBearing(userPosition,latest);
   if(delta)delta.textContent=formatDistance(d.distance)+" · "+Math.round(d.bearing)+"° "+compass(d.bearing);
   if(directions){
    directions.href="https://www.google.com/maps/dir/?api=1&origin="+
      encodeURIComponent(userPosition.lat+","+userPosition.lon)+"&destination="+
      encodeURIComponent(latest.lat+","+latest.lon);
    directions.removeAttribute("aria-disabled");
   }
  }else{
   if(delta)delta.textContent="Use my current location to compare";
   if(directions){
    directions.href="https://www.google.com/maps/dir/?api=1&destination="+
      encodeURIComponent(latest.lat+","+latest.lon);
    directions.removeAttribute("aria-disabled");
   }
  }
 }else{
  link.removeAttribute("href");link.setAttribute("aria-disabled","true");link.textContent="Google Maps link available after GPS fix";
  if(delta)delta.textContent="Waiting for a confirmed rocket GPS fix";
  if(directions){directions.removeAttribute("href");directions.setAttribute("aria-disabled","true");}
 }
 $("v15RecoveryPoints").textContent=fixes.length+" local GPS points · saved on this browser";
 draw();
 window.dispatchEvent(new CustomEvent("green-ion-v16-recovery",{detail:{
  latest:latest?{...latest,ageMs:Date.now()-latest.at}:null,
  fixes:fixes.map(x=>({...x})),
  userPosition:userPosition?{...userPosition}:null,
  lastPhase
 }}));
}
function tick(){
 if(t.wifiFresh()){
  const wifi=t.state.lastWifiStatus||t.state.lastStatus;
  capture(wifi,"Wi-Fi");
  if(Number.isInteger(Number(wifi?.flight_state)))lastPhase=Number(wifi.flight_state);
 }
 if(t.radioFresh()){
  capture(t.state.radio,"LoRa");
  if(Number.isInteger(Number(t.state.radio?.flight_state)))lastPhase=Number(t.state.radio.flight_state);
 }
 render();
}
export function initRecovery(){
 load();ensureLocationControls();initV163RecoveryTools();render();
 $("v15RecoveryClear").onclick=()=>{
  if(!confirm("Clear this browser's saved GPS recovery positions? Flight files and cloud backups are not affected."))return;
  fixes=[];try{localStorage.removeItem(KEY);}catch{}
  render();
 };
 window.addEventListener("green-ion-link",tick);
 setInterval(tick,1500);
}
