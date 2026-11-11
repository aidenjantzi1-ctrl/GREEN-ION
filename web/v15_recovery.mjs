/* V15 local-first recovery assistant. No web map API or cloud requirement.
 * Coordinates are observed fixes, never fabricated landing predictions. */
import {validCoordinate} from "./v14_utils.mjs";
const KEY="green-ion-v15-recovery-fixes",MAX=120;
const $=id=>document.getElementById(id);
const t=window.GREEN_ION_TRANSPORT;
let fixes=[],lastPhase=null;
function load(){
 try{
  const items=JSON.parse(localStorage.getItem(KEY)||"[]");
  if(Array.isArray(items))fixes=items.filter(x=>validCoordinate(x.lat,x.lon)&&
    Number.isFinite(x.at)&&x.at>0&&x.at<=Date.now()+60000&&["Wi-Fi","LoRa"].includes(x.source)).slice(-MAX);
 }catch{fixes=[];}
}
function capture(raw,source){
 if(!raw||raw.gps_status!=="live"||!validCoordinate(raw.latitude,raw.longitude))return;
 const age=Number(raw.gps_fix_age_ms);
 if(Number.isFinite(age)&&age>30000)return;
 const at=Date.now()-(Number.isFinite(age)&&age>=0?Math.min(30000,age):0);
 const lat=Number(raw.latitude),lon=Number(raw.longitude),last=fixes.at(-1);
 if(last&&last.source===source&&last.lat===lat&&last.lon===lon&&at-last.at<15000)return;
 fixes.push({lat,lon,source,at});
 if(fixes.length>MAX)fixes=fixes.slice(-MAX);
 try{localStorage.setItem(KEY,JSON.stringify(fixes));}catch{}
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
 const link=$("v15RecoveryNavigate");
 if(latest){
  link.href="https://www.openstreetmap.org/?mlat="+latest.lat+"&mlon="+latest.lon+
   "#map=17/"+latest.lat+"/"+latest.lon;
  link.removeAttribute("aria-disabled");link.textContent="Open last received GPS position on map";
 }else{link.removeAttribute("href");link.setAttribute("aria-disabled","true");link.textContent="Map link available after GPS fix";}
 $("v15RecoveryPoints").textContent=fixes.length+" local GPS points · saved on this browser";
 draw();
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
 load();render();
 $("v15RecoveryClear").onclick=()=>{
  if(!confirm("Clear this browser's saved GPS recovery positions? Flight files and cloud backups are not affected."))return;
  fixes=[];try{localStorage.removeItem(KEY);}catch{}
  render();
 };
 window.addEventListener("green-ion-link",tick);
 setInterval(tick,1500);
}
