import {deriveFlightState,fileSaveEvidence,optionalFlightWarnings,storageMessage} from "../v18_offline_core.mjs";
import {ReceiverSerial} from "./serial.mjs";
import {analyzeImportedBin,analysisSummary,artifactWarnings,comparisonRows,reportJson,analysisCsv,drawAltitudeGraph} from "./data.mjs";
import {saveImportedFlight,listFlightMetadata,getFlight,requestPersistentStorage,persistentStorageStatus} from "./storage.mjs";

const $=id=>document.getElementById(id);
const text=(id,value)=>{const n=$(id);if(n)n.textContent=value;};
const show=(id,visible)=>{const n=$(id);if(n)n.hidden=!visible;};
const fmt=(v,d=2)=>Number.isFinite(Number(v))?Number(v).toFixed(d):"--";
let latestStatus=null,saveVerified=false,currentRecord=null,currentAnalysis=null,compareIds=new Set();

function base(){
  const saved=localStorage.getItem("green-ion-v18-feather-base");
  return (saved||"http://192.168.4.1").replace(/\/$/,"");
}
async function api(path,options={}){
  const controller=new AbortController(),timer=setTimeout(()=>controller.abort(),7000);
  try{
    const response=await fetch(base()+path,{cache:"no-store",...options,signal:controller.signal});
    if(!response.ok)throw new Error("HTTP "+response.status);
    return await response.json();
  }finally{clearTimeout(timer);}
}
export function actionableFailure(reason){
  const r=String(reason||"");
  if(!r)return "";
  if(/microSD|SD/i.test(r))return r+" Check the card, power and SD wiring, then retry PREPARE & ARM.";
  if(/barometer|BMP/i.test(r))return r+" Keep the rocket stationary and check the BMP388 connection.";
  if(/IMU/i.test(r))return r+" Keep the rocket stationary and check the IMU connection.";
  if(/battery|voltage/i.test(r))return r+" Recharge or replace the flight battery before arming.";
  return r;
}
async function verifySavedFile(status){
  if(!status?.log_file||status.recording)return false;
  try{
    const files=await api("/api/files");
    return !!fileSaveEvidence(files,status.log_file);
  }catch{return false;}
}
function renderFlight(status){
  latestStatus=status;
  const state=deriveFlightState(status,{saveVerified,connected:true});
  text("flightState",state);
  text("battery",Number.isFinite(Number(status.battery_v))?fmt(status.battery_v,2)+" V":"--");
  text("sdIndicator",status.sd_ok===true?(status.sd_integrity_verified?"VERIFIED":"MOUNTED"):status.sd_ok===false?"ERROR":"--");
  text("gpsIndicator",String(status.gps_status||"--").toUpperCase());
  text("loraIndicator",status.radio_ok===true?"READY":status.radio_ok===false?"WARNING":"--");
  text("prepPhase",String(status.flight_prep_phase||"IDLE"));
  const progress=Math.max(0,Math.min(100,Number(status.flight_prep_progress||0)));
  $("prepBar").style.width=progress+"%";text("prepPercent",progress+"%");
  const failure=actionableFailure(status.flight_prep_failure);
  text("flightMessage",failure||(
    state==="ARMED"?"Automatic arming complete. Recorder is waiting for launch.":
    state==="FLYING"?"Flight recording is active. Controls and unsafe settings are locked.":
    state==="SAVING"?"Landing detected; waiting for verified completed-file evidence.":
    state==="SAVED"?"Flight save verified. FLY AGAIN is available.":
    state==="TESTING"?"Running firmware-owned readiness checks and calibration.":
    "Ready to start firmware-owned PREPARE & ARM."
  ));
  $("prepareArm").disabled=!!status.recording||!!status.flight_prep_active||state==="ARMED"||state==="SAVING";
  show("flyAgain",state==="SAVED");
  const details=[];
  for(const warning of optionalFlightWarnings(status))details.push(warning);
  if(failure)details.unshift(failure);
  text("flightDetails",details.join(" ")||"Details stay compact unless a warning or error needs attention.");
  const locked=!!status.recording;
  $("featherBase").disabled=locked;
  text("settingsLock",locked?"Settings that can disrupt the local connection are locked while recording.":"Connection settings are editable while not recording.");
}
async function pollFlight(){
  try{
    const status=await api("/api/status");
    if(!status.recording&&latestStatus?.recording)saveVerified=await verifySavedFile(status);
    else if(!saveVerified&&status.recorder_mode===3)saveVerified=await verifySavedFile(status);
    if(status.recording)saveVerified=false;
    renderFlight(status);
  }catch(error){
    latestStatus=null;
    text("flightState","OFFLINE");text("flightMessage","Feather status unavailable: "+error.message);
    $("prepareArm").disabled=true;show("flyAgain",false);
  }
}
async function prepare(){
  $("prepareArm").disabled=true;saveVerified=false;text("flightMessage","Starting PREPARE & ARM...");
  try{
    await api("/api/preflight/start",{method:"POST"});
    await pollFlight();
  }catch(error){text("flightMessage","Could not start PREPARE & ARM: "+error.message);}
}
function tabs(){
  document.querySelectorAll("[data-tab]").forEach(button=>button.addEventListener("click",()=>{
    const id=button.dataset.tab;
    document.querySelectorAll("[data-tab]").forEach(b=>b.classList.toggle("active",b===button));
    document.querySelectorAll(".tab-page").forEach(page=>page.classList.toggle("active",page.id===id));
    location.hash=id;
  }));
  const initial=["flight","recovery","data","settings"].includes(location.hash.slice(1))?location.hash.slice(1):"flight";
  document.querySelector('[data-tab="'+initial+'"]')?.click();
}

const receiver=new ReceiverSerial({
 onState:s=>{text("receiverStatus",s.message);text("receiverConnection",s.state.toUpperCase());},
 onInvalid:()=>text("receiverInvalid",String(receiver.invalidLines)),
 onMessage:(message,view)=>{
   text("receiverConnection","CONNECTED");
   text("packetAge",view.packetAgeMs==null?"--":Math.round(view.packetAgeMs/100)/10+" s");
   text("signalStatus",view.signal.toUpperCase()+(view.rssiDbm!=null?" · "+view.rssiDbm+" dBm":""));
   text("gpsStatus",view.gps.toUpperCase());
   text("coordinates",view.position?fmt(view.position.latitude,6)+", "+fmt(view.position.longitude,6):"--");
   text("positionAge",view.positionAgeMs==null?"--":Math.round(view.positionAgeMs/1000)+" s");
   text("receiverAdvanced",JSON.stringify(message.raw,null,2));
 }
});
async function connectReceiver(){
  try{await receiver.connectReceiver();}catch(error){text("receiverStatus",error.message);}
}
async function disconnectReceiver(){await receiver.disconnect();}
setInterval(()=>{
  const view=receiver.view();
  if(receiver.lastMessage){
    text("packetAge",Math.round((view.packetAgeMs||0)/100)/10+" s");
    text("signalStatus",view.signal.toUpperCase()+(view.rssiDbm!=null?" · "+view.rssiDbm+" dBm":""));
    text("gpsStatus",view.gps.toUpperCase());
    text("positionAge",view.positionAgeMs==null?"--":Math.round(view.positionAgeMs/1000)+" s");
  }
},1000);

function download(name,type,data){
  const blob=data instanceof Blob?data:new Blob([data],{type});
  const url=URL.createObjectURL(blob),a=document.createElement("a");
  a.href=url;a.download=name;document.body.appendChild(a);a.click();a.remove();
  setTimeout(()=>URL.revokeObjectURL(url),1000);
}
async function importBin(){
  const file=$("binFile").files?.[0];if(!file)return;
  text("dataMessage","Validating and analyzing "+file.name+" locally...");
  try{
    const bytes=new Uint8Array(await file.arrayBuffer());
    const analysis=analyzeImportedBin(bytes),summary=analysisSummary(analysis);
    const result=await saveImportedFlight({name:file.name,bytes,label:$("flightLabel").value,analysisSummary:summary});
    text("dataMessage",result.duplicate?"That exact BIN is already in local history.":"Imported original BIN unchanged; SHA-256 recorded.");
    await refreshHistory();
    await openFlight(result.record.id);
  }catch(error){text("dataMessage","Import failed: "+error.message);}
}
async function refreshHistory(){
  const rows=await listFlightMetadata(),root=$("historyList");root.replaceChildren();
  text("historyCount",rows.length+" local flight"+(rows.length===1?"":"s"));
  for(const meta of rows){
    const row=document.createElement("div");row.className="history-row";
    const title=document.createElement("div");
    title.innerHTML="<strong></strong><small></small>";
    title.querySelector("strong").textContent=meta.label||meta.originalName;
    title.querySelector("small").textContent=(meta.label?meta.originalName+" · ":"")+new Date(meta.importedAt).toLocaleString();
    const actions=document.createElement("div");actions.className="history-actions";
    const view=document.createElement("button");view.textContent="VIEW";view.onclick=()=>openFlight(meta.id);
    const original=document.createElement("button");original.textContent="ORIGINAL";original.className="secondary";original.onclick=()=>downloadOriginal(meta.id);
    const compare=document.createElement("label");compare.className="compare-check";
    const box=document.createElement("input");box.type="checkbox";box.checked=compareIds.has(meta.id);
    box.onchange=()=>{if(box.checked){if(compareIds.size>=2){box.checked=false;return;}compareIds.add(meta.id);}else compareIds.delete(meta.id);renderComparison();};
    compare.append(box,document.createTextNode(" Compare"));
    actions.append(view,original,compare);row.append(title,actions);root.append(row);
  }
}
async function openFlight(id){
  const record=await getFlight(id);if(!record)return;
  currentRecord=record;currentAnalysis=analyzeImportedBin(new Uint8Array(record.originalBin));
  text("selectedFlight",(record.label?record.label+" · ":"")+record.originalName);
  const s=analysisSummary(currentAnalysis);
  text("flightStats",`${s.samples} samples · ${s.sampleRateHz} Hz · ${fmt(s.durationS,2)} s · peak ${fmt(s.peakAltitudeM,2)} m`);
  const warnings=artifactWarnings(currentAnalysis);text("analysisWarnings",warnings.length?warnings.join(" "):"No parser or timing warnings.");
  const phases=$("phaseList");phases.replaceChildren();
  for(const p of currentAnalysis.phases){
    const li=document.createElement("li");li.textContent=`${p.name.toUpperCase()} · estimated ${fmt(p.timeS,2)} s · ${fmt(p.altitudeM,2)} m · ${p.confidence} confidence`;phases.append(li);
  }
  if(!currentAnalysis.phases.length){const li=document.createElement("li");li.textContent="No flight phases could be estimated reliably.";phases.append(li);}
  drawAltitudeGraph($("flightGraph"),currentAnalysis);
}
async function downloadOriginal(id){
  const record=await getFlight(id);if(record)download(record.originalName,"application/octet-stream",record.originalBin);
}
function exportCsv(){
  if(!currentAnalysis)return;
  download((currentRecord?.originalName||"flight.BIN").replace(/\.BIN$/i,".csv"),"text/csv",analysisCsv(currentAnalysis));
}
function exportJson(){
  if(!currentAnalysis)return;
  const report=reportJson(currentAnalysis,currentRecord?.originalName||"flight",currentRecord?.label||"");
  download((currentRecord?.originalName||"flight.BIN").replace(/\.BIN$/i,".json"),"application/json",JSON.stringify(report,null,2));
}
async function renderComparison(){
  if(compareIds.size!==2){text("comparison","Select exactly two flights to compare.");drawAltitudeGraph($("compareGraph"),[]);return;}
  const entries=[];
  for(const id of compareIds){const meta=(await listFlightMetadata()).find(x=>x.id===id),record=await getFlight(id);entries.push({meta,analysis:analyzeImportedBin(new Uint8Array(record.originalBin))});}
  const rows=comparisonRows(entries);
  text("comparison",rows.map(r=>`${r.label||r.originalName}: peak ${fmt(r.peakAltitudeM,2)} m, duration ${fmt(r.durationS,2)} s, ${r.sampleRateHz} Hz`).join(" | "));
  drawAltitudeGraph($("compareGraph"),entries.map(x=>x.analysis));
}

async function storageStatus(){
  const s=await persistentStorageStatus();
  text("storageStatus",s.supported?storageMessage(s.persisted):"Browser persistent-storage API is unavailable. Keep original BIN backups outside the browser.");
}
async function persistStorage(){
  try{const s=await requestPersistentStorage();text("storageStatus",s.supported?storageMessage(s.persisted):"Persistent storage is unsupported.");}
  catch(error){text("storageStatus","Could not request persistent storage: "+error.message);}
}
function saveSettings(){
  if(latestStatus?.recording){text("settingsMessage","Cannot change the Feather connection while recording.");return;}
  const value=$("featherBase").value.trim().replace(/\/$/,"");
  if(!/^https?:\/\//i.test(value)){text("settingsMessage","Enter a local http:// or https:// Feather address.");return;}
  localStorage.setItem("green-ion-v18-feather-base",value);text("settingsMessage","Saved locally in this browser only.");pollFlight();
}

async function boot(){
  tabs();
  $("prepareArm").onclick=prepare;$("flyAgain").onclick=prepare;
  $("connectReceiver").onclick=connectReceiver;$("disconnectReceiver").onclick=disconnectReceiver;
  $("importBin").onclick=importBin;$("exportCsv").onclick=exportCsv;$("exportJson").onclick=exportJson;$("printReport").onclick=()=>window.print();
  $("persistStorage").onclick=persistStorage;$("saveSettings").onclick=saveSettings;
  $("featherBase").value=base();
  receiver.startAutoReconnect();
  await storageStatus();
  await refreshHistory();
  if("serviceWorker" in navigator)navigator.serviceWorker.register("./service-worker.js").catch(error=>text("offlineStatus","Offline cache registration failed: "+error.message));
  text("offlineStatus","App is local-only; no cloud account or remote telemetry upload is configured.");
  pollFlight();setInterval(pollFlight,1200);
}
boot();
