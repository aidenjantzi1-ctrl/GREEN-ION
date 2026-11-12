/* GREEN ION V16 mission controller.
 * Browser workflow state is advisory/persistent. Feather telemetry remains
 * authoritative for launch, flight phase and autonomous SD recording. */

export const MISSION_SCHEMA="GREEN-ION-V16-MISSION-1";
export const MISSION_STATES=Object.freeze([
 "IDLE","PREFLIGHT","READY","PRELAUNCH","ASCENT","DESCENT","LANDED",
 "RECOVERY","DOWNLOADING","ANALYZING","COMPLETE","ERROR","INTERRUPTED"
]);
const ACTIVE_RESTART_STATES=new Set(["PRELAUNCH","ASCENT","DESCENT","DOWNLOADING","ANALYZING"]);
const MAX_TRANSITIONS=80;
const STORAGE_KEY="green-ion-v16-mission";

function nowIso(now=Date.now()){return new Date(now).toISOString();}
function idFallback(now=Date.now()){return "mission-"+now.toString(36)+"-"+Math.random().toString(36).slice(2,10);}
function freshId(){
 try{return globalThis.crypto?.randomUUID?.()||idFallback();}catch{return idFallback();}
}
function cleanText(value,max=160){return String(value??"").replace(/[\u0000-\u001f]/g," ").trim().slice(0,max);}
function cleanState(value){return MISSION_STATES.includes(value)?value:"IDLE";}
function clone(value){return JSON.parse(JSON.stringify(value));}

export function newMission(now=Date.now(),id=null){
 return {
  schema:MISSION_SCHEMA,
  id:id||null,
  state:"IDLE",
  created_at:nowIso(now),
  updated_at:nowIso(now),
  rocket:null,
  motor:null,
  preflight:{
   complete:false,completed_at:null,measured_failures:0,blocking_unverified:0,
   manual_unverified:0,physical_confirmed:false,physical_confirmed_at:null,
   error:"",results:{},evidence:[]
  },
  expected_flight_file:null,
  flight_file:null,
  report_quality:null,
  gps_status:"missing",
  last_telemetry_at:null,
  interrupted_from:null,
  transition_log:[],
  warnings:[]
 };
}

function addTransition(mission,to,event,reason,now){
 const from=mission.state;
 if(from===to)return mission;
 mission.state=to;
 mission.updated_at=nowIso(now);
 mission.transition_log.push({
  from,to,at:mission.updated_at,event,
  reason:cleanText(reason,240)
 });
 if(mission.transition_log.length>MAX_TRANSITIONS)
  mission.transition_log=mission.transition_log.slice(-MAX_TRANSITIONS);
 return mission;
}
function warning(mission,text,now){
 const value=cleanText(text,240);
 if(!value)return mission;
 const last=mission.warnings.at(-1);
 if(!last||last.text!==value){
  mission.warnings.push({at:nowIso(now),text:value});
  if(mission.warnings.length>40)mission.warnings=mission.warnings.slice(-40);
 }
 return mission;
}
function flightStateFromTelemetry(data){
 const raw=data?.readings?.flight_state?.value;
 const value=Number(raw);
 return Number.isInteger(value)?value:null;
}
function liveTelemetry(data){
 return data?.schema==="GREEN-ION-V16-TELEMETRY-1" &&
  ["wifi+lora","wifi","lora"].includes(data.mode);
}
function applyFlightEvidence(mission,data,now){
 if(!liveTelemetry(data))return mission;
 mission.last_telemetry_at=data.generated_utc||nowIso(now);
 mission.gps_status=cleanText(data.readings?.gps_status?.value||"missing",20);
 mission.rocket=cleanText(data.identity?.rocket,40)||mission.rocket;
 mission.motor=cleanText(data.identity?.motor,30)||mission.motor;
 mission.expected_flight_file=cleanText(data.identity?.mission_file_id,180)||mission.expected_flight_file;
 if(mission.gps_status!=="live"&&["ASCENT","DESCENT","LANDED","RECOVERY"].includes(mission.state))
  warning(mission,"GPS is not live; recovery will use the last confirmed fix.",now);
 const phase=flightStateFromTelemetry(data);
 if(mission.state==="IDLE"||mission.state==="COMPLETE")return mission;
 if(phase===3||phase===4){
  if(["PREFLIGHT","READY"].includes(mission.state))
   warning(mission,"Feather reported flight before the local workflow reached PRELAUNCH; mission evidence is incomplete.",now);
  return addTransition(mission,"ASCENT","TELEMETRY","Feather reported ascent/coast.",now);
 }
 if(phase===5)
  return addTransition(mission,"DESCENT","TELEMETRY","Feather reported descent.",now);
 if(phase===6)
  return addTransition(mission,"LANDED","TELEMETRY","Feather reported landed; physical recovery is still required.",now);
 if(phase===7)
  return addTransition(mission,"RECOVERY","TELEMETRY","Feather reported recovery phase.",now);
 if(mission.state==="INTERRUPTED"&&phase!=null&&phase<=2&&mission.interrupted_from==="PRELAUNCH")
  return addTransition(mission,"PRELAUNCH","TELEMETRY","Fresh prelaunch Feather state restored after browser interruption.",now);
 return mission;
}

export function reduceMission(current,event,now=Date.now()){
 const mission=clone(current?.schema===MISSION_SCHEMA?current:newMission(now));
 const type=String(event?.type||"");
 const detail=event?.detail||{};
 mission.state=cleanState(mission.state);
 mission.updated_at=nowIso(now);

 if(type==="START_MISSION"){
  const started=newMission(now,cleanText(detail.id,80)||freshId());
  started.rocket=cleanText(detail.rocket,40)||null;
  started.motor=cleanText(detail.motor,30)||null;
  return addTransition(started,"PREFLIGHT",type,"Mission workflow started; run measured preflight checks.",now);
 }
 if(type==="RESET_MISSION")return newMission(now);

 if(type==="PREFLIGHT_RESULT"){
  if(!["PREFLIGHT","ERROR"].includes(mission.state))return mission;
  mission.preflight={
   complete:!!detail.complete,
   completed_at:nowIso(now),
   measured_failures:Math.max(0,Number(detail.fails)||0),
   blocking_unverified:Math.max(0,Number(detail.blocking_unverified)||0),
   manual_unverified:Math.max(0,Number(detail.manual_unverified)||0),
   physical_confirmed:false,physical_confirmed_at:null,
   error:cleanText(detail.error,180),
   results:detail.results&&typeof detail.results==="object"?clone(detail.results):{},
   evidence:Array.isArray(detail.evidence)?clone(detail.evidence.slice(-30)):[]
  };
  if(!mission.preflight.complete||mission.preflight.measured_failures>0||mission.preflight.error){
   const reason=mission.preflight.error||
    (mission.preflight.measured_failures+" measured preflight check(s) failed or the session was incomplete.");
   return addTransition(mission,"ERROR",type,reason,now);
  }
  if(mission.preflight.blocking_unverified>0){
   warning(mission,mission.preflight.blocking_unverified+" non-physical preflight check(s) remain unverified.",now);
  }
  return addTransition(mission,"PREFLIGHT",type,
   "Measured preflight completed. Physical/manual checks are not auto-approved.",now);
 }
 if(type==="CONFIRM_PHYSICAL"){
  if(mission.state!=="PREFLIGHT"||!mission.preflight.complete||
     mission.preflight.measured_failures||mission.preflight.blocking_unverified){
   return warning(mission,"READY blocked: measured or receiver preflight evidence is incomplete.",now);
  }
  mission.preflight.physical_confirmed=true;
  mission.preflight.physical_confirmed_at=nowIso(now);
  return addTransition(mission,"READY",type,"User confirmed required physical/manual checks after measured preflight.",now);
 }
 if(type==="RETRY_PREFLIGHT"&&["ERROR","PREFLIGHT"].includes(mission.state)){
  mission.preflight=newMission(now).preflight;
  return addTransition(mission,"PREFLIGHT",type,"Preflight restarted after review.",now);
 }
 if(type==="BEGIN_PRELAUNCH"&&mission.state==="READY")
  return addTransition(mission,"PRELAUNCH",type,"User began prelaunch monitoring; Feather remains launch authority.",now);
 if(type==="BEGIN_RECOVERY"&&mission.state==="LANDED")
  return addTransition(mission,"RECOVERY",type,"User began physical recovery workflow.",now);

 if(type==="TELEMETRY")return applyFlightEvidence(mission,detail,now);

 if(type==="DOWNLOAD_START"&&["LANDED","RECOVERY","DOWNLOADING"].includes(mission.state)){
  const name=cleanText(detail.name,180);
  if(mission.expected_flight_file&&name&&name.toLowerCase()!==mission.expected_flight_file.toLowerCase())
   return warning(mission,"Ignored unrelated archived flight transfer: "+name,now);
  mission.flight_file=name||mission.flight_file;
  return addTransition(mission,"DOWNLOADING",type,"Completed flight retrieval started after landing.",now);
 }
 if(type==="FLIGHT_SAVED"&&/\.BIN$/i.test(String(detail.name||"")) &&
    (["LANDED","RECOVERY","DOWNLOADING","ANALYZING"].includes(mission.state) ||
     (mission.state==="INTERRUPTED"&&["DOWNLOADING","ANALYZING"].includes(mission.interrupted_from)))){
  const name=cleanText(detail.name,180);
  const expected=mission.expected_flight_file||mission.flight_file;
  if(!expected||name.toLowerCase()!==expected.toLowerCase())
   return warning(mission,"Saved BIN does not match this mission's expected flight file: "+name,now);
  mission.flight_file=name;
  return addTransition(mission,"ANALYZING",type,"Checksum-preserved BIN is available locally for analysis.",now);
 }
 if(type==="REPORT_COMPLETE"&&(mission.state==="ANALYZING" ||
    (mission.state==="INTERRUPTED"&&mission.interrupted_from==="ANALYZING"))){
  const name=cleanText(detail.name,180);
  if(mission.flight_file&&name&&name.toLowerCase()!==mission.flight_file.toLowerCase())
   return warning(mission,"Ignored postflight report for a different recording: "+name,now);
  mission.report_quality=Number.isFinite(Number(detail.quality))?Number(detail.quality):null;
  return addTransition(mission,"COMPLETE",type,"Local postflight analysis completed.",now);
 }
 if(type==="INTERRUPT"&&ACTIVE_RESTART_STATES.has(mission.state)){
  mission.interrupted_from=mission.state;
  return addTransition(mission,"INTERRUPTED",type,
   "Browser mission workflow restarted/interrupted; Feather recording and flight logic are independent.",now);
 }
 return mission;
}

export function restoreMission(raw,now=Date.now()){
 let mission;
 try{
  const parsed=typeof raw==="string"?JSON.parse(raw):raw;
  mission=parsed?.schema===MISSION_SCHEMA?parsed:newMission(now);
 }catch{mission=newMission(now);}
 mission=clone(mission);
 mission.state=cleanState(mission.state);
 if(ACTIVE_RESTART_STATES.has(mission.state))
  mission=reduceMission(mission,{type:"INTERRUPT"},now);
 return mission;
}

export function missionView(mission){
 const state=cleanState(mission?.state);
 const pf=mission?.preflight||{};
 const views={
  IDLE:["Start a mission when the rocket and motor profile are ready.","START MISSION","start"],
  PREFLIGHT:[
   pf.complete?(pf.blocking_unverified?
    "Measured checks finished, but receiver/non-physical evidence is still missing. Re-run preflight.":
    "Measured checks passed. Complete the physical checklist, then confirm it below."):
    "Run the full preflight. Measured failures cannot be manually overridden.",
   pf.complete&&!pf.blocking_unverified?"CONFIRM PHYSICAL CHECKS":"RUN FULL PREFLIGHT",
   pf.complete&&!pf.blocking_unverified?"physical":"preflight"],
  READY:["Measured preflight passed and physical checks were confirmed. Begin prelaunch monitoring when ready.","BEGIN PRE-LAUNCH","prelaunch"],
  PRELAUNCH:["Waiting for the Feather's real launch detection. Do not rely on the browser to start recording.","WAITING FOR FEATHER","wait"],
  ASCENT:["Flight in progress. SD recording has priority; live links may change or disappear.","FLIGHT IN PROGRESS","wait"],
  DESCENT:["Descent reported by Feather telemetry. Prepare for recovery; do not assume landing yet.","DESCENDING","wait"],
  LANDED:["Feather reports landed. Confirm the area is safe, then begin recovery.","BEGIN RECOVERY","recovery"],
  RECOVERY:["Use the last confirmed GPS fix and LoRa evidence. Rejoin Feather Wi-Fi after retrieval.","OPEN RECOVERY","open-recovery"],
  DOWNLOADING:["Retrieving the completed immutable flight recording. Partial transfers are rejected.","DOWNLOADING","wait"],
  ANALYZING:["Original BIN is saved. Local integrity-aware analysis is running or ready.","OPEN REPORT","open-report"],
  COMPLETE:["Mission data is saved and analyzed. Review the report before starting another mission.","NEW MISSION","new"],
  ERROR:["Mission workflow is blocked by failed/incomplete evidence. Fix the problem and repeat preflight.","RETRY PREFLIGHT","retry"],
  INTERRUPTED:["The browser workflow was interrupted. Waiting for fresh Feather/LoRa evidence to restore the real phase.","WAITING FOR TELEMETRY","wait"]
 };
 const [instruction,label,action]=views[state]||views.IDLE;
 return {state,instruction,label,action};
}

function downloadJson(filename,value){
 const blob=new Blob([JSON.stringify(value,null,2)],{type:"application/json"});
 const url=URL.createObjectURL(blob),a=document.createElement("a");
 a.href=url;a.download=filename;document.body.append(a);a.click();a.remove();
 setTimeout(()=>URL.revokeObjectURL(url),60000);
}

export function initMission(){
 if(typeof window==="undefined"||typeof document==="undefined")return;
 const $=id=>document.getElementById(id);
 let stored=null;
 try{stored=localStorage.getItem(STORAGE_KEY);}catch{}
 let mission=restoreMission(stored);
 let latestTelemetry=window.GREEN_ION_UNIFIED_TELEMETRY||null;
 const persist=()=>{
  try{localStorage.setItem(STORAGE_KEY,JSON.stringify(mission));}catch{}
  window.GREEN_ION_MISSION=mission;
  window.dispatchEvent(new CustomEvent("green-ion-v16-mission",{detail:clone(mission)}));
 };
 const apply=(event)=>{
  mission=reduceMission(mission,event);
  persist();render();
 };
 function render(){
  const view=missionView(mission);
  if($("v16MissionState"))$("v16MissionState").textContent=view.state;
  if($("v16MissionInstruction"))$("v16MissionInstruction").textContent=view.instruction;
  if($("v16MissionIdentity"))$("v16MissionIdentity").textContent=
   (mission.rocket||latestTelemetry?.identity?.rocket||"Rocket not selected")+
   " · "+(mission.motor||latestTelemetry?.identity?.motor||"Motor not selected");
  if($("v16MissionFile"))$("v16MissionFile").textContent=mission.flight_file||"No completed flight file yet";
  if($("v16MissionGps"))$("v16MissionGps").textContent="GPS: "+String(mission.gps_status||"missing").toUpperCase();
  const button=$("v16MissionPrimary");
  if(button){button.textContent=view.label;button.disabled=view.action==="wait";button.dataset.action=view.action;}
  const confirm=$("v16MissionPhysical");
  const confirmLabel=$("v16MissionPhysicalLabel");
  const showPhysical=view.action==="physical";
  if(confirmLabel)confirmLabel.hidden=!showPhysical;
  if(confirm){
   confirm.disabled=!showPhysical||mission.preflight.blocking_unverified>0;
   if(!showPhysical)confirm.checked=false;
  }
  if(button&&view.action==="physical")
   button.disabled=!confirm?.checked||mission.preflight.blocking_unverified>0;
  if($("v16MissionEvidence")){
   const pf=mission.preflight||{};
   $("v16MissionEvidence").textContent=
    "Measured failures "+(pf.measured_failures||0)+" · blocking unverified "+(pf.blocking_unverified||0)+
    " · physical confirmation "+(pf.physical_confirmed?"YES":"NO")+
    " · transitions "+(mission.transition_log?.length||0);
  }
  const timeline=$("v16MissionTimeline");
  if(timeline){
   timeline.replaceChildren();
   for(const item of (mission.transition_log||[]).slice(-8).reverse()){
    const row=document.createElement("div");row.className="row";
    const left=document.createElement("span");left.textContent=item.from+" → "+item.to;
    const right=document.createElement("strong");right.textContent=new Date(item.at).toLocaleTimeString();
    row.append(left,right);timeline.append(row);
   }
   if(!timeline.childNodes.length)timeline.textContent="No mission transitions yet.";
  }
 }
 const primary=$("v16MissionPrimary");
 if(primary)primary.onclick=()=>{
  const action=primary.dataset.action;
  if(action==="start"){
   const status=window.GREEN_ION_TRANSPORT?.state?.lastWifiStatus||{};
   apply({type:"START_MISSION",detail:{rocket:status.selected_rocket,motor:status.selected_motor}});
  }else if(action==="preflight"){
   document.querySelector('[data-tab="testing"]')?.click();
   $("homeAuto")?.click();
  }else if(action==="physical"){
   if($("v16MissionPhysical")?.checked)apply({type:"CONFIRM_PHYSICAL"});
  }else if(action==="prelaunch")apply({type:"BEGIN_PRELAUNCH"});
  else if(action==="recovery")apply({type:"BEGIN_RECOVERY"});
  else if(action==="open-recovery")document.querySelector('[data-tab="recovery"]')?.click();
  else if(action==="open-report")document.querySelector('[data-tab="history"]')?.click();
  else if(action==="retry"){
   apply({type:"RETRY_PREFLIGHT"});
   document.querySelector('[data-tab="testing"]')?.click();
  }else if(action==="new"){
   if(confirm("Start a new mission workflow? Saved flight files and reports are not deleted."))
    apply({type:"RESET_MISSION"});
  }
 };
 $("v16MissionPhysical")?.addEventListener("change",render);
 if($("v16MissionReset"))$("v16MissionReset").onclick=()=>{
  if(confirm("Reset only the V16 mission workflow? Flight files, telemetry logs and cloud backups are not deleted."))
   apply({type:"RESET_MISSION"});
 };
 if($("v16MissionExport"))$("v16MissionExport").onclick=()=>downloadJson(
  (mission.id||"green-ion-mission")+"-evidence.json",mission);
 window.addEventListener("green-ion-v16-telemetry",e=>{
  latestTelemetry=e.detail;apply({type:"TELEMETRY",detail:e.detail});
 });
 window.addEventListener("green-ion-v16-preflight",e=>apply({type:"PREFLIGHT_RESULT",detail:e.detail}));
 window.addEventListener("green-ion-v16-download-start",e=>apply({type:"DOWNLOAD_START",detail:e.detail}));
 window.addEventListener("green-ion-flight-saved",e=>apply({type:"FLIGHT_SAVED",detail:e.detail}));
 window.addEventListener("green-ion-postflight-report",e=>apply({type:"REPORT_COMPLETE",detail:e.detail}));
 if(latestTelemetry)mission=reduceMission(mission,{type:"TELEMETRY",detail:latestTelemetry});
 persist();render();
}
