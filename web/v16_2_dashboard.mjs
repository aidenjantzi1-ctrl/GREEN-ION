/* GREEN ION V16.2 Dashboard presentation and lightweight live rocket visual.
 * Rendering is advisory only and never participates in recording or control. */
import {getLocalFile} from "./v14_store.mjs";
import {parseRkt4,analyzeFlight} from "./analysis/rkt4.mjs";

export const V162_VISUAL_LABEL="Flight Visualization — Sensor-Based / Estimated";
export const V162_VISIBILITY_KEY="green-ion-v16.2-visibility";
const $=id=>document.getElementById(id);
const PHASES={0:"Boot",1:"Idle",2:"Prelaunch",3:"Ascent",4:"Coast",5:"Descent",6:"Landed",7:"Recovery"};
let preset="balanced",latest=null,lastObservationAt=0,observedMax=10,angle=0,lastFrame=0,visible=true;
let replay=null,replayStarted=0,replayPausedAt=0,replayRaf=0;

function finite(v){return typeof v==="number"&&Number.isFinite(v);}
function clamp(v,a,b){return Math.max(a,Math.min(b,v));}
function reading(data,name){return data?.readings?.[name]||null;}
function profile(){
 try{return JSON.parse(localStorage.getItem("green-ion-v16.2-active-profile")||"null")||{};}
 catch{return {};}
}
export function visualStateFromTelemetry(data,now=Date.now()){
 const altitude=reading(data,"altitude_m"),speed=reading(data,"gps_speed_mps");
 const flight=reading(data,"flight_state"),battery=reading(data,"battery_v");
 const rot=reading(data,"rotation_z_dps")||reading(data,"rotation_y_dps");
 const freshness=altitude?.freshness||data?.links?.wifi?.freshness||data?.links?.lora?.freshness||"missing";
 const age=Math.max(0,Number(altitude?.age_ms??data?.links?.wifi?.age_ms??data?.links?.lora?.age_ms??Infinity));
 const live=freshness==="live"&&age<=6500;
 const phase=PHASES[Number(flight?.value)]||"Unknown";
 const p=profile(),recovery=String(p.recoveryType||"").toLowerCase();
 return {
  mode:"live",live,stale:!live&&freshness!=="missing",phase,
  altitudeM:finite(altitude?.value)?altitude.value:null,
  speedMps:finite(speed?.value)?speed.value:null,
  batteryV:finite(battery?.value)?battery.value:null,
  rotationDps:finite(rot?.value)?rot.value:null,
  source:data?.primary_source||"none",ageMs:Number.isFinite(age)?age:null,
  flameIllustrative:live&&phase==="Ascent",
  parachuteIllustrative:live&&phase==="Descent"&&recovery.includes("parachute"),
  observedAt:now
 };
}
function phaseForReplay(analysis,t){
 let phase="Prelaunch";
 for(const event of analysis.phases||[]){
  if(event.timeS>t)break;
  if(event.name==="launch")phase="Ascent";
  else if(event.name==="apogee")phase="Coast";
  else if(event.name==="descent")phase="Descent";
  else if(event.name==="landing")phase="Landed";
 }
 return phase;
}
function replayState(){
 if(!replay)return null;
 const duration=Math.max(.001,replay.analysis.samples.at(-1)?.timeS-replay.startTime);
 const elapsed=replay.paused?replay.pauseTime:
  Math.min(duration,(performance.now()-replayStarted)/1000*replay.speed);
 const absolute=replay.startTime+elapsed;
 let index=0;
 for(let i=1;i<replay.analysis.samples.length;i++){
  if(replay.analysis.samples[i].timeS>absolute)break;
  index=i;
 }
 const sample=replay.analysis.samples[index]||{};
 return {
  mode:"replay",live:true,stale:false,phase:phaseForReplay(replay.analysis,absolute),
  altitudeM:finite(sample.altitudeM)?sample.altitudeM:null,
  speedMps:finite(sample.velocityMps)?sample.velocityMps:null,
  batteryV:finite(sample.batteryMv)?sample.batteryMv/1000:null,
  rotationDps:null,source:"saved RKT4",ageMs:null,
  flameIllustrative:phaseForReplay(replay.analysis,absolute)==="Ascent",
  parachuteIllustrative:phaseForReplay(replay.analysis,absolute)==="Descent"&&
   String(profile().recoveryType||"").toLowerCase().includes("parachute"),
  replayTimeS:absolute-replay.startTime,durationS:duration
 };
}
function setPreset(value){
 preset=["simple","balanced","advanced"].includes(value)?value:"balanced";
 try{localStorage.setItem(V162_VISIBILITY_KEY,preset);}catch{}
 document.body.dataset.v162Visibility=preset;
 const select=$("v162Visibility");if(select)select.value=preset;
 if(preset==="simple"){
  document.querySelectorAll(".v16-section-group").forEach(n=>n.open=false);
  const detail=$("v16DashboardDetails");if(detail)detail.open=false;
 }else if(preset==="advanced"){
  const detail=$("v16DashboardDetails");if(detail)detail.open=true;
 }
}
function rounded(ctx,x,y,w,h,r){
 const radius=Math.min(r,w/2,h/2);ctx.beginPath();
 ctx.moveTo(x+radius,y);ctx.arcTo(x+w,y,x+w,y+h,radius);
 ctx.arcTo(x+w,y+h,x,y+h,radius);ctx.arcTo(x,y+h,x,y,radius);
 ctx.arcTo(x,y,x+w,y,radius);ctx.closePath();
}
function drawParachute(ctx,x,y,scale){
 ctx.save();ctx.translate(x,y);ctx.lineWidth=Math.max(1,scale*.018);
 ctx.strokeStyle="#d9efff";ctx.fillStyle="rgba(123,219,255,.25)";
 ctx.beginPath();ctx.arc(0,-45*scale,34*scale,Math.PI,Math.PI*2);ctx.lineTo(34*scale,-45*scale);ctx.closePath();ctx.fill();ctx.stroke();
 ctx.beginPath();ctx.moveTo(-30*scale,-45*scale);ctx.lineTo(-7*scale,0);ctx.moveTo(30*scale,-45*scale);ctx.lineTo(7*scale,0);ctx.stroke();ctx.restore();
}
function drawRocket(ctx,x,y,scale,tilt,flame,parachute){
 ctx.save();ctx.translate(x,y);ctx.rotate(tilt);
 const p=profile(),aspect=Number(p.lengthCm)>0&&Number(p.diameterCm)>0?Number(p.lengthCm)/Number(p.diameterCm):10;
 const h=94*scale*clamp(aspect/10,.72,1.35),w=22*scale*clamp(10/aspect,.72,1.35);
 // Shadowed body and nose.
 const grad=ctx.createLinearGradient(-w,0,w,0);grad.addColorStop(0,"#9eb7c9");grad.addColorStop(.48,"#f4fbff");grad.addColorStop(1,"#7a98af");
 ctx.fillStyle=grad;rounded(ctx,-w/2,-h*.45,w,h*.68,5*scale);ctx.fill();
 ctx.fillStyle="#dcecf7";ctx.beginPath();ctx.moveTo(-w/2,-h*.45);ctx.lineTo(0,-h*.72);ctx.lineTo(w/2,-h*.45);ctx.closePath();ctx.fill();
 ctx.fillStyle="#7bdbff";ctx.fillRect(-w/2,-h*.1,w,h*.12);
 ctx.fillStyle="#6b879e";ctx.beginPath();ctx.moveTo(-w/2,h*.18);ctx.lineTo(-w*.95,h*.43);ctx.lineTo(-w/2,h*.32);ctx.closePath();ctx.fill();
 ctx.beginPath();ctx.moveTo(w/2,h*.18);ctx.lineTo(w*.95,h*.43);ctx.lineTo(w/2,h*.32);ctx.closePath();ctx.fill();
 ctx.fillStyle="#364d60";ctx.fillRect(-w*.25,h*.23,w*.5,h*.08);
 if(flame){
  const pulse=.82+.18*Math.sin(performance.now()/55);
  const flameGrad=ctx.createLinearGradient(0,h*.3,0,h*.72);
  flameGrad.addColorStop(0,"#fff0a5");flameGrad.addColorStop(.45,"#ff9b45");flameGrad.addColorStop(1,"rgba(255,105,40,0)");
  ctx.fillStyle=flameGrad;ctx.beginPath();ctx.moveTo(-w*.22,h*.3);ctx.quadraticCurveTo(-w*.42,h*.5,0,h*.72*pulse);ctx.quadraticCurveTo(w*.42,h*.5,w*.22,h*.3);ctx.closePath();ctx.fill();
 }
 ctx.restore();
 if(parachute)drawParachute(ctx,x,y-h*.58,scale);
}
function paint(){
 const canvas=$("v162RocketCanvas");if(!canvas||!visible)return;
 const dpr=Math.min(2,window.devicePixelRatio||1),rect=canvas.getBoundingClientRect();
 const w=Math.max(300,Math.round(rect.width)),h=Math.max(250,Math.round(rect.height||360));
 if(canvas.width!==w*dpr||canvas.height!==h*dpr){canvas.width=w*dpr;canvas.height=h*dpr;}
 const ctx=canvas.getContext("2d");ctx.setTransform(dpr,0,0,dpr,0,0);
 const state=replayState()||latest||{mode:"live",live:false,stale:false,phase:"No telemetry",altitudeM:null,source:"none"};
 const now=performance.now();
 if(lastFrame&&now-lastFrame<30){requestFrame();return;}
 const dt=Math.min(.08,Math.max(0,(now-lastFrame)/1000));lastFrame=now;
 if(state.live&&finite(state.rotationDps))angle+=state.rotationDps*Math.PI/180*dt;
 angle=Math.max(-Math.PI*.7,Math.min(Math.PI*.7,angle));
 if(state.phase==="Prelaunch"||state.phase==="Idle"||state.phase==="Landed")angle*=.85;
 const altitude=Math.max(0,finite(state.altitudeM)?state.altitudeM:0);observedMax=Math.max(observedMax,altitude);
 const level=Math.min(1,altitude/Math.max(20,observedMax*1.15));
 const sky=ctx.createLinearGradient(0,0,0,h);sky.addColorStop(0,"#091b2d");sky.addColorStop(1,"#143b57");ctx.fillStyle=sky;ctx.fillRect(0,0,w,h);
 // faint altitude bands
 ctx.strokeStyle="rgba(123,219,255,.14)";ctx.lineWidth=1;ctx.font="11px system-ui";ctx.fillStyle="#a5bbcd";
 for(let i=1;i<5;i++){const yy=h-(h-48)*i/5-26;ctx.beginPath();ctx.moveTo(12,yy);ctx.lineTo(w-12,yy);ctx.stroke();ctx.fillText(Math.round(observedMax*i/5)+" m",16,yy-4);}
 const groundY=h-34;ctx.fillStyle="#132a2f";ctx.fillRect(0,groundY,w,h-groundY);ctx.strokeStyle="#72e5ab";ctx.beginPath();ctx.moveTo(0,groundY);ctx.lineTo(w,groundY);ctx.stroke();
 const rocketY=groundY-38-level*(h-110),rocketX=w*.5;
 drawRocket(ctx,rocketX,rocketY,Math.max(.65,Math.min(1,w/520)),angle,state.flameIllustrative,state.parachuteIllustrative);
 ctx.fillStyle="#eff8ff";ctx.font="700 14px system-ui";ctx.fillText(state.phase,16,24);
 ctx.font="12px system-ui";ctx.fillStyle="#a5bbcd";
 const source=state.mode==="replay"?"SAVED FLIGHT REPLAY — NOT LIVE":
  state.live?("LIVE · "+String(state.source).toUpperCase()):
  state.stale?("TELEMETRY DELAYED · LAST OBSERVATION "+Math.round((state.ageMs||0)/1000)+" s OLD"):"NO LIVE TELEMETRY";
 ctx.fillText(source,16,43);
 if(state.flameIllustrative){ctx.fillStyle="#ffdd88";ctx.fillText("Exhaust: illustrative phase effect",16,h-12);}
 if(state.parachuteIllustrative){ctx.fillStyle="#ffdd88";ctx.fillText("Parachute: illustrative from recovery profile, not deployment confirmation",16,h-12);}
 const evidence=$("v162RocketEvidence");
 if(evidence)evidence.textContent=(state.mode==="replay"?"REPLAY · ":"")+
  (finite(state.altitudeM)?state.altitudeM.toFixed(1)+" m estimated altitude":"Altitude unavailable")+" · "+
  (finite(state.speedMps)?state.speedMps.toFixed(1)+" m/s estimated speed":"Speed unavailable");
 if(replay&&!replay.paused&&state.replayTimeS>=state.durationS-.02){replay.paused=true;replay.pauseTime=state.durationS;}
 // Keep animation bounded (~33 fps) only while live telemetry or active replay needs motion.
 if((replay&&!replay.paused)||latest?.live)requestFrame();
}
function requestFrame(){
 if(!visible||document.hidden)return;
 cancelAnimationFrame(replayRaf);replayRaf=requestAnimationFrame(paint);
}
async function startReplay(){
 const select=$("v15ReportFlight")||$("v14AnalysisFlight"),key=select?.value;
 const status=$("v162RocketStatus");
 if(!key){if(status)status.textContent="Choose a saved BIN in Flights & Analysis first.";return;}
 try{
  if(status)status.textContent="Loading saved flight for clearly labeled Dashboard replay...";
  const rec=await getLocalFile(key);if(!rec||rec.kind!=="BIN")throw Error("Selected BIN is unavailable");
  const analysis=analyzeFlight(parseRkt4(rec.bytes));
  if(!analysis.samples.length)throw Error("Recording has no complete samples");
  replay={analysis,startTime:analysis.samples[0].timeS,speed:1,paused:false,pauseTime:0};
  replayStarted=performance.now();observedMax=Math.max(10,analysis.peakAltitudeM||10);angle=0;
  if(status)status.textContent="Replay mode is active. This is saved data, not live hardware.";
  $("v162RocketLive").hidden=false;requestFrame();
 }catch(error){if(status)status.textContent="Replay unavailable: "+error.message;}
}
function stopReplay(){
 replay=null;angle=0;$("v162RocketLive").hidden=true;
 const status=$("v162RocketStatus");if(status)status.textContent="Live mode. Visual effects are never flight-control evidence.";requestFrame();
}
function injectDashboard(){
 const target=$("status"),summary=$("v16DashboardSummary");if(!target||$("v162RocketCard"))return;
 const card=document.createElement("div");card.className="card v162-visual-card";card.id="v162RocketCard";
 card.innerHTML='<h2>'+V162_VISUAL_LABEL+'</h2>'+
  '<canvas id="v162RocketCanvas" height="360" role="img" aria-label="Animated side-view rocket visualization using live or replay telemetry"></canvas>'+
  '<div class="row"><span>Visualization evidence</span><strong id="v162RocketEvidence">Waiting for telemetry</strong></div>'+
  '<p id="v162RocketStatus" class="muted" role="status">Live mode. Visual effects are never flight-control evidence.</p>'+
  '<details><summary>What does this animation mean?</summary><p class="muted">Altitude, timestamps, battery and sensor readings use received/recorded data when valid. Flight phase, velocity and gyro-integrated orientation are estimates. Flame, smoke, parachute and ground contact are illustrative unless separate hardware evidence exists. Stale telemetry freezes the observed state instead of inventing a trajectory.</p>'+
  '<div class="actions"><button id="v162RocketReplay" class="secondary">Replay selected flight</button><button id="v162RocketLive" class="secondary" hidden>Return to live</button></div></details>';
 if(summary)summary.after(card);else target.prepend(card);
 $("v162RocketReplay").onclick=startReplay;$("v162RocketLive").onclick=stopReplay;
 if("IntersectionObserver" in window)new IntersectionObserver(entries=>{visible=!!entries[0]?.isIntersecting;if(visible)requestFrame();},{rootMargin:"100px"}).observe(card);
}
function injectVisibility(){
 const target=$("v16SettingsGeneral")||$("system");if(!target||$("v162VisibilityCard"))return;
 const card=document.createElement("div");card.className="card";card.id="v162VisibilityCard";
 card.innerHTML='<h3>Interface detail</h3><label>Visibility preset<select id="v162Visibility"><option value="simple">Simple</option><option value="balanced">Balanced</option><option value="advanced">Advanced</option></select></label><p class="muted">Balanced is the default. This changes presentation only; recording quality and safety protections never change.</p>';
 target.append(card);$("v162Visibility").onchange=e=>setPreset(e.target.value);setPreset(preset);
}
export function initV162Dashboard(){
 try{preset=localStorage.getItem(V162_VISIBILITY_KEY)||"balanced";}catch{preset="balanced";}
 const batteryLabel=$("batteryPercent")?.closest(".metric")?.querySelector(".label");if(batteryLabel)batteryLabel.textContent="BATTERY";
 const batteryNote=$("batteryVoltage")?.nextElementSibling;if(batteryNote)batteryNote.textContent="Voltage-based rough estimate; voltage is measured";
 injectDashboard();injectVisibility();setPreset(preset);
 window.addEventListener("green-ion-v16-telemetry",event=>{
  if(replay)return;
  latest=visualStateFromTelemetry(event.detail,Date.now());lastObservationAt=Date.now();
  if(finite(latest.altitudeM))observedMax=Math.max(observedMax,Math.max(0,latest.altitudeM));requestFrame();
 });
 window.addEventListener("green-ion-link",()=>{
  if(replay)return;
  if(latest&&Date.now()-lastObservationAt>6500){latest={...latest,live:false,stale:true,ageMs:Date.now()-lastObservationAt};requestFrame();}
 });
 document.addEventListener("visibilitychange",()=>{if(!document.hidden)requestFrame();});
 requestFrame();
}
