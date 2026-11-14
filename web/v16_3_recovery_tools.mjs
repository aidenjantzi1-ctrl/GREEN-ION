/* V16.3 opt-in offline recovery tools inside existing recovery card.
 * Does not alter the live rocket fix or radio transmission schedule. */
import {listLocalMetadata,getLocalFile} from "./v14_store.mjs";
import {savedTrack,validFix} from "./v16_3_recovery_data.mjs";
const $=id=>document.getElementById(id);
let latest=null,received=[],archived=[],user=null,archiveName="";
const STORE="green-ion-v16-3-archived-gps";
function coord(f){return f.lat.toFixed(7)+", "+f.lon.toFixed(7);}
function dist(a,b){
 const rad=x=>x*Math.PI/180,R=6371000,dLat=rad(b.lat-a.lat),dLon=rad(b.lon-a.lon),p=rad(a.lat),q=rad(b.lat);
 const h=Math.sin(dLat/2)**2+Math.cos(p)*Math.cos(q)*Math.sin(dLon/2)**2;
 const meters=2*R*Math.atan2(Math.sqrt(h),Math.sqrt(Math.max(0,1-h)));
 const y=Math.sin(dLon)*Math.cos(q),x=Math.cos(p)*Math.sin(q)-Math.sin(p)*Math.cos(q)*Math.cos(dLon);
 return {meters,bearing:(Math.atan2(y,x)*180/Math.PI+360)%360};
}
export function recoveryEvidence(f,now=Date.now()){
 if(!f||!validFix(f.lat,f.lon))return "NO GPS FIX";
 if(f.archived)return "SAVED GPS TRACK · NOT LIVE";
 const age=f.at===null||!Number.isFinite(f.at)?Infinity:now-f.at;
 return age>=0&&age<=5000?"CONFIRMED FRESH GPS FIX": "STALE · LAST KNOWN GPS";
}
function plot(){
 const canvas=$("v163SavedGpsPlot");if(!canvas)return;
 const w=Math.max(220,Math.min(500,Math.round(canvas.clientWidth||340))),h=200,dpr=Math.min(2,window.devicePixelRatio||1);
 canvas.width=Math.round(w*dpr);canvas.height=h*dpr;
 const ctx=canvas.getContext("2d");ctx.setTransform(dpr,0,0,dpr,0,0);
 ctx.fillStyle="#0b1c2b";ctx.fillRect(0,0,w,h);ctx.fillStyle="#d9efff";ctx.font="12px system-ui";
 ctx.fillText("Saved GPS · relative positions only (offline, no terrain)",10,18);
 if(!archived.length){ctx.fillText("No saved track loaded",10,60);return;}
 const lat=archived.map(x=>x.lat),lon=archived.map(x=>x.lon),a=Math.min(...lat),b=Math.min(...lon),
  latRange=Math.max(.00001,Math.max(...lat)-a),lonRange=Math.max(.00001,Math.max(...lon)-b);
 const xy=f=>[25+(f.lon-b)/lonRange*(w-50),h-20-(f.lat-a)/latRange*(h-65)];
 ctx.beginPath();ctx.lineWidth=2;ctx.strokeStyle="#7bdbff";
 archived.forEach((p,i)=>{const v=xy(p);if(i)ctx.lineTo(...v);else ctx.moveTo(...v);});ctx.stroke();
 const first=xy(archived[0]),last=xy(archived.at(-1));
 ctx.fillStyle="#72e5ab";ctx.beginPath();ctx.arc(...first,5,0,7);ctx.fill();
 ctx.fillStyle="#ffdd88";ctx.beginPath();ctx.arc(...last,6,0,7);ctx.fill();
 ctx.fillStyle="#d9efff";ctx.fillText("N ↑",w-40,40);
}
function render(){
 const age=latest?.at&&Number.isFinite(latest.at)?Math.max(0,Date.now()-latest.at):null,
  radio=[...received].reverse().find(x=>x.source==="LoRa");
 $("v163GpsEvidence").textContent=recoveryEvidence(latest)+
  (age!==null?" · "+(age/1000).toFixed(1)+" s since fix":"");
 $("v163GpsSatellites").textContent=Number.isInteger(latest?.satellites)?String(latest.satellites)+" satellites":"Unavailable";
 $("v163RadioFix").textContent=radio?coord(radio)+" · "+((Date.now()-radio.at)/1000).toFixed(1)+" s old":"Not received";
 const t=window.GREEN_ION_TRANSPORT;
 $("v163RadioPacketAt").textContent=t?.state?.radioAt?new Date(t.state.radioAt).toLocaleString():"No LoRa packet received";
 $("v163GpsOther").textContent=latest?
  (Number.isFinite(latest.gpsAltitudeM)?latest.gpsAltitudeM.toFixed(1)+" m GPS altitude · ":"")+
  (Number.isFinite(latest.gpsSpeedMps)?latest.gpsSpeedMps.toFixed(1)+" m/s GPS ground speed · ":"")+
  (Number.isFinite(latest.hdop)?"HDOP "+latest.hdop.toFixed(1)+" (not meter accuracy)":"Accuracy unavailable"):
  "No GPS fix available";
 for(const [id,base] of [["v163Google","https://www.google.com/maps/search/?api=1&query="],
  ["v163Apple","https://maps.apple.com/?ll="]]){
  const node=$(id);if(latest){node.href=base+encodeURIComponent(coord(latest));node.removeAttribute("aria-disabled");}
  else{node.removeAttribute("href");node.setAttribute("aria-disabled","true");}
 }
 $("v163SavedTrackInfo").textContent=archived.length?
  archiveName+" · "+archived.length+" verified saved GPS points · final "+coord(archived.at(-1))+
   " · timestamps relative to recording; NOT live":
  "No saved GPS track loaded. Live last-known coordinates remain unchanged.";
 if(user&&latest){
  const d=dist(user,latest);
  $("v163ManualDistance").textContent=Math.round(d.meters)+" m · "+Math.round(d.bearing)+"° bearing from your entered location";
 }else $("v163ManualDistance").textContent="Optional: enter recovery coordinates below (no permission required)";
 plot();
}
async function files(){
 const select=$("v163TrackFile"),previous=select.value;
 const records=(await listLocalMetadata()).filter(x=>x.kind==="SEN").sort((a,b)=>String(b.createdAt).localeCompare(String(a.createdAt)));
 select.replaceChildren(new Option("Choose a saved SEN",""));
 for(const file of records)select.add(new Option(file.name,file.key));
 if(records.some(x=>x.key===previous))select.value=previous;
}
async function importTrack(){
 const key=$("v163TrackFile").value,note=$("v163RecoveryMessage");
 if(!key){note.textContent="Select a saved SEN file first.";return;}
 note.textContent="Verifying and decoding GIS1 in a browser worker...";
 try{
  const f=await getLocalFile(key);if(!f||f.kind!=="SEN")throw Error("Saved SEN unavailable");
  const decoded=await new Promise((resolve,reject)=>{
   const w=new Worker("./sensor-worker.mjs",{type:"module"}),timer=setTimeout(()=>{w.terminate();reject(Error("GPS decoder timeout"));},60000);
   w.onerror=e=>{clearTimeout(timer);w.terminate();reject(Error(e.message||"GPS worker failed"));};
   w.onmessage=e=>{clearTimeout(timer);w.terminate();e.data.error?reject(Error(e.data.error)):resolve(e.data.parsed);};
   const bytes=f.bytes.slice(0);w.postMessage({id:1,name:f.name,bytes},[bytes]);
  });
  const track=savedTrack(decoded.samples,120);
  if(!track.length)throw Error("No verified current-GPS records in this SEN");
  archived=track;archiveName=f.name;
  try{localStorage.setItem(STORE,JSON.stringify({name:archiveName,points:archived}));}catch{}
  note.textContent="Saved track loaded separately; last received location remains authoritative.";
  render();
 }catch(e){note.textContent="Saved track unavailable: "+e.message;}
}
export function initV163RecoveryTools(){
 const root=$("v15RecoveryCard");if(!root||$("v163RecoveryTools"))return;
 const block=document.createElement("div");block.id="v163RecoveryTools";block.style.cssText="min-width:0;max-width:100%;margin-top:12px";
 block.innerHTML='<h3>GPS evidence and offline recovery</h3>'+
  '<div class="row"><span>Current GPS evidence</span><strong id="v163GpsEvidence">No fix</strong></div>'+
  '<div class="row"><span>Satellites</span><strong id="v163GpsSatellites">Unavailable</strong></div>'+
  '<div class="row"><span>GPS details</span><strong id="v163GpsOther">Unavailable</strong></div>'+
  '<div class="row"><span>Last LoRa GPS fix</span><strong id="v163RadioFix">Not received</strong></div>'+
  '<div class="row"><span>Last LoRa packet received</span><strong id="v163RadioPacketAt">Not received</strong></div>'+
  '<div class="actions"><button id="v163CopyFix" class="secondary">Copy last GPS</button><a id="v163Apple" aria-disabled="true">Apple Maps</a><a id="v163Google" aria-disabled="true">Google Maps</a></div>'+
  '<label>Your manually entered latitude<input id="v163ManualLat" type="number" min="-90" max="90" step="any" placeholder="Latitude"></label>'+
  '<label>Your manually entered longitude<input id="v163ManualLon" type="number" min="-180" max="180" step="any" placeholder="Longitude"></label>'+
  '<div class="actions"><button id="v163ManualSubmit" class="secondary">Use entered position</button></div>'+
  '<p id="v163ManualDistance" class="muted">No position permission needed for manual coordinates.</p>'+
  '<h4>Recovery from a recorded flight</h4><label>Imported SEN<select id="v163TrackFile"></select></label>'+
  '<div class="actions"><button id="v163TrackLoad" class="secondary">Open saved GPS track</button></div>'+
  '<p id="v163SavedTrackInfo" class="muted">No saved GPS track loaded.</p>'+
  '<canvas id="v163SavedGpsPlot" height="200" style="width:100%;max-width:100%;display:block" aria-label="Offline plot of verified saved SEN GPS coordinates"></canvas>'+
  '<p id="v163RecoveryMessage" role="status" class="muted">Saved tracks do not replace live recovery coordinates.</p>';
 root.append(block);
 try{const item=JSON.parse(localStorage.getItem(STORE)||"null");if(item?.points?.length){
  archived=item.points.filter(x=>x.archived&&validFix(x.lat,x.lon)).slice(-120);archiveName=item.name||"saved SEN";
 }}catch{}
 $("v163CopyFix").onclick=async()=>{
  if(!latest)return;
  try{await navigator.clipboard.writeText(coord(latest));$("v163RecoveryMessage").textContent="Last confirmed coordinates copied.";}
  catch{$("v163RecoveryMessage").textContent="Clipboard denied; select the coordinates shown in Recovery.";}
 };
 $("v163ManualSubmit").onclick=()=>{
  const a=$("v163ManualLat").value,b=$("v163ManualLon").value,lat=Number(a),lon=Number(b);
  if(!a||!b||!validFix(lat,lon)){$("v163RecoveryMessage").textContent="Enter valid coordinates.";return;}
  user={lat,lon};render();
 };
 $("v163TrackLoad").onclick=importTrack;
 window.addEventListener("green-ion-v16-recovery",e=>{
  latest=e.detail?.latest||null;received=e.detail?.fixes||[];render();
 });
 window.addEventListener("green-ion-history-change",()=>files().catch(()=>{}));
 window.addEventListener("resize",()=>{if(archived.length)plot();});
 files().catch(e=>$("v163RecoveryMessage").textContent=e.message);
 render();
}
