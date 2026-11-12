/* GREEN ION V16.2 everyday experience: next-action summary, rocket profiles and optional first-run setup. */
const $=id=>document.getElementById(id),PROFILE_KEY="green-ion-v16.2-active-profile",SETUP_KEY="green-ion-v16.2-first-run-complete";
const t=window.GREEN_ION_TRANSPORT;
let missionState="IDLE";
function readProfile(){try{return JSON.parse(localStorage.getItem(PROFILE_KEY)||"null")||{};}catch{return {};}}
function saveProfile(value){try{localStorage.setItem(PROFILE_KEY,JSON.stringify(value));}catch{}window.dispatchEvent(new CustomEvent("green-ion-v16-profile",{detail:value}));}
function nextAction(state){
 const s=String(state||"IDLE").toUpperCase();
 if(s==="IDLE")return "Select your rocket/motor, then Prepare for Flight.";
 if(s==="PREFLIGHT")return "Finish measured preflight checks and required physical inspection.";
 if(s==="READY")return "Keep the rocket still and begin prelaunch monitoring when at the launch area.";
 if(s==="PRELAUNCH")return "Automatic recorder is watching. Leave the browser alone and launch only under normal range safety procedures.";
 if(["ASCENT","DESCENT"].includes(s))return "Flight detected. Do not interact with recorder controls; monitor received telemetry only.";
 if(s==="LANDED")return "Use Recovery and retrieve the rocket using the last confirmed GPS evidence.";
 if(s==="RECOVERY")return "Recover the rocket, reconnect Feather Wi-Fi, then download and verify the matching flight.";
 if(s==="DOWNLOADING")return "Keep Feather Wi-Fi connected while originals are transferred and verified.";
 if(s==="ANALYZING")return "Wait for local analysis to finish; original files remain unchanged.";
 if(s==="COMPLETE")return "Mission complete. View Flight Report or 3D replay; verify backups before SD cleanup.";
 if(s==="INTERRUPTED")return "Mission context was interrupted. Reconnect links and review retained evidence before continuing.";
 if(s==="ERROR")return "Review the visible failure and System Health; do not clear evidence until the cause is understood.";
 return "Review current mission evidence.";
}
function renderSummary(detail=null){
 if(detail?.state)missionState=detail.state;
 const n=$("v162NextAction");if(n)n.textContent=nextAction(missionState);
 const state=$("v16DashMission")?.textContent||missionState;
 const headline=$("v162MissionHeadline");if(headline){
  const s=String(state).toUpperCase();
  headline.textContent=s==="READY"?"MISSION READY":s==="PREFLIGHT"?"PREFLIGHT INCOMPLETE":
   s==="ASCENT"||s==="DESCENT"?"FLIGHT DETECTED":s==="RECOVERY"||s==="LANDED"?"RECOVERY IN PROGRESS":
   s==="COMPLETE"?"DOWNLOAD COMPLETE":"MISSION "+s;
 }
}
function injectSummary(){
 const card=$("v16DashboardSummary");if(!card||$("v162NextAction"))return;
 const headline=document.createElement("div");headline.className="notice v162-mission-headline";headline.innerHTML='<strong id="v162MissionHeadline">MISSION IDLE</strong><div id="v162NextAction">Select your rocket/motor, then Prepare for Flight.</div>';
 card.insertBefore(headline,card.querySelector(".actions"));
}
function fillProfile(){
 const p=readProfile(),set=(id,v)=>{if($(id)&&v!==undefined&&v!==null)$(id).value=String(v)};
 set("v162ProfileName",p.rocketName||"");set("v162ProfileMotor",p.motor||"");set("v162ProfileLength",p.lengthCm||"");set("v162ProfileDiameter",p.diameterCm||"");
 set("v162ProfileRecovery",p.recoveryType||"");set("v162ProfileMount",p.mountingAxis||"+Z");set("v162ProfileRate",p.sampleRate||50);
}
function injectProfile(){
 const target=$("v16SettingsGeneral")||$("system");if(!target||$("v162ProfileCard"))return;
 const card=document.createElement("div");card.className="card";card.id="v162ProfileCard";
 card.innerHTML='<h3>Rocket profile · labels + visualization</h3>'+
  '<p class="muted">Profile entries label flights and tune display orientation/appearance. Recovery type is never proof of deployment. Changing the Feather rocket/motor profile restarts stationary calibration.</p>'+
  '<div class="v162-filter-grid"><label>Rocket name<input id="v162ProfileName" maxlength="32" placeholder="Rocket name"></label>'+
  '<label>Motor<input id="v162ProfileMotor" maxlength="24" placeholder="Example: D12-5"></label>'+
  '<label>Body length (cm)<input id="v162ProfileLength" type="number" min="5" max="1000" step=".1" placeholder="Optional"></label>'+
  '<label>Body diameter (cm)<input id="v162ProfileDiameter" type="number" min=".5" max="100" step=".1" placeholder="Optional"></label>'+
  '<label>Recovery type<select id="v162ProfileRecovery"><option value="">Not specified</option><option>Parachute</option><option>Streamer</option><option>Other</option></select></label>'+
  '<label>Sensor nose direction<select id="v162ProfileMount"><option value="+Z">+Z (default)</option><option value="-Z">-Z</option><option value="+X">+X</option><option value="-X">-X</option><option value="+Y">+Y</option><option value="-Y">-Y</option></select></label>'+
  '<label>Recording rate<select id="v162ProfileRate"><option selected>50</option></select><small class="muted">V16.2 Feather recorder is fixed at the validated 50 Hz cadence.</small></label></div>'+
  '<div class="actions"><button id="v162ProfileSave">Save profile & apply labels</button><button id="v162ProfileLocal" class="secondary">Save display profile only</button></div>'+
  '<p id="v162ProfileStatus" class="muted" role="status"></p>';
 target.append(card);fillProfile();
 const collect=()=>({rocketName:$("v162ProfileName").value.trim(),motor:$("v162ProfileMotor").value.trim(),lengthCm:Number($("v162ProfileLength").value)||null,diameterCm:Number($("v162ProfileDiameter").value)||null,recoveryType:$("v162ProfileRecovery").value,mountingAxis:$("v162ProfileMount").value,sampleRate:Number($("v162ProfileRate").value)||50,updatedAt:new Date().toISOString()});
 $("v162ProfileLocal").onclick=()=>{const p=collect();saveProfile(p);$("v162ProfileStatus").textContent="Display/analysis profile saved locally. No hardware setting changed.";};
 $("v162ProfileSave").onclick=()=>{
  const p=collect();saveProfile(p);
  const preset=$("rocketPreset"),custom=$("rocketCustom"),motor=$("profileMotor"),rate=$("sampleRate");
  if(preset&&p.rocketName){
   const option=[...preset.options].find(o=>o.value===p.rocketName);
   preset.value=option?p.rocketName:"Custom";preset.dispatchEvent(new Event("change",{bubbles:true}));
   if(!option&&custom){custom.disabled=false;custom.value=p.rocketName;}
  }
  if(motor)motor.value=p.motor;if(rate&&[...rate.options].some(o=>Number(o.value)===p.sampleRate))rate.value=String(p.sampleRate);
  if(t.controlsVerified?.()&&$("saveFlightProfile")){$("saveFlightProfile").click();$("v162ProfileStatus").textContent="Profile saved locally and sent to Feather; keep it stationary while calibration restarts.";}
  else $("v162ProfileStatus").textContent="Profile saved locally. Connect fresh Feather Wi-Fi to apply rocket/motor labels to firmware.";
 };
}
function setupStatus(){
 const w=t.state.lastWifiStatus||{},items=[
  ["Windows ground station",t.localBridge?"PASS":"OPTIONAL / browser mode"],
  ["Feather Wi-Fi",t.controlsVerified?.()?"PASS":"CONNECT"],
  ["Uno receiver",t.radioFresh()?"PACKETS RECEIVED":t.localBridge?"CONNECT UNO / WAIT":"WINDOWS ONLY"],
  ["Sensors",w.barometer_ready&&w.imu_ok?"BMP + IMU READY":"RUN HOME TEST"],
  ["Dashboard","READY"]
 ];return items;
}
function injectFirstRun(){
 const target=$("system");if(!target||$("v162FirstRunCard"))return;
 const done=(()=>{try{return localStorage.getItem(SETUP_KEY)==="1";}catch{return false;}})();
 const card=document.createElement("details");card.className="card v16-section-group";card.id="v162FirstRunCard";card.open=!done;
 card.innerHTML='<summary>First-run setup · optional</summary><p class="muted">A short setup helper. Completing it is remembered on this browser and never required again.</p><div id="v162FirstRunSteps"></div><div class="actions"><button id="v162FirstRunRefresh" class="secondary">Refresh setup status</button><button id="v162FirstRunDone">Finish setup</button></div><p id="v162FirstRunStatus" class="muted"></p>';
 target.prepend(card);
 const render=()=>{const box=$("v162FirstRunSteps");box.replaceChildren();setupStatus().forEach(([label,value],i)=>{const row=document.createElement("div");row.className="row";row.innerHTML="<span>"+(i+1)+" · "+label+"</span><strong>"+value+"</strong>";box.append(row);});};
 $("v162FirstRunRefresh").onclick=render;$("v162FirstRunDone").onclick=()=>{try{localStorage.setItem(SETUP_KEY,"1");}catch{}$("v162FirstRunStatus").textContent="Setup helper completed. You can reopen it here any time.";card.open=false;};render();
}
function explainMissing(){
 const w=t.state.lastWifiStatus||{},map=[["altitudeNote",w.barometer_ready===false?"Barometer unavailable":!t.wifiFresh()&&!t.radioFresh()?"No live telemetry":"Awaiting calibrated pressure altitude"]];
 for(const [id,value] of map){const n=$(id);if(n&&(n.textContent==="--"||/Awaiting/.test(n.textContent)))n.textContent=value;}
}
export function initV162Experience(){
 injectSummary();injectProfile();injectFirstRun();renderSummary();
 window.addEventListener("green-ion-v16-mission",e=>renderSummary(e.detail));
 window.addEventListener("green-ion-link",()=>{explainMissing();if($("v162FirstRunCard")?.open)$("v162FirstRunRefresh")?.click();});
}
