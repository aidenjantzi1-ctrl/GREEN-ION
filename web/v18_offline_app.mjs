import {parseRkt4,analyzeFlight,analysisCsv,flightReport} from "./analysis/rkt4.mjs";
import {saveLocalFile,listLocalMetadata,getLocalFile,downloadFile,digest,exportArchive,restoreArchive} from "./v14_store.mjs";
import {deriveFlightState,criticalFlightProblem,optionalFlightWarnings,canPrepare,fileSaveEvidence,parseReceiverLine,recoveryView,storageMessage,developmentStatusChecks,developmentReceiverChecks,developmentFirmwareDiagnostic,developmentSerialAccessCheck,safeDevelopmentReport} from "./v18_offline_core.mjs";

const $=id=>document.getElementById(id);
let lastStatusAt=0,currentStatus=null,connected=false,lastFiles=null,saveVerified=false,lastSaved=null,lastLogFile="",pollTimer=null;
let serialPort=null,serialReader=null,serialStop=false,serialManualDisconnect=false,serialReconnectTimer=null,lastSerialInfo=null,lastPacket=null,lastKnown=null,serialInvalid=0,serialPermissionState="unknown",serialPermissionMessage="",activeAnalysis=null,compareAnalysis=null;
const V18_CACHE_VERSION="green-ion-v18-offline-app-v7";
let devFirmwareRequested=false;\nlet desktopUpdateSupported=false,desktopUpdatePoll=null;

function text(id,value){const n=$(id);if(n)n.textContent=value==null?"—":String(value);}
function fmt(v,d=1){return Number.isFinite(Number(v))?Number(v).toFixed(d):"—";}
function fmtBytes(n){n=Number(n);if(!Number.isFinite(n))return"—";if(n>=1048576)return(n/1048576).toFixed(2)+" MB";if(n>=1024)return(n/1024).toFixed(1)+" KB";return Math.round(n)+" B";}
function fmtAge(ms){if(ms==null)return"—";if(ms<1000)return Math.round(ms)+" ms";return(ms/1000).toFixed(ms<10000?1:0)+" s";}
function escapeHtml(s){return String(s??"").replace(/[&<>"']/g,c=>({"&":"&amp;","<":"&lt;",">":"&gt;",'"':"&quot;","'":"&#39;"}[c]));}
function parseMeta(t){const o={};String(t||"").split(/\r?\n/).forEach(line=>{const i=line.indexOf("=");if(i>0)o[line.slice(0,i).trim()]=line.slice(i+1).trim();});return o;}
async function api(path,options={}){
 const response=await fetch(path,{cache:"no-store",...options});
 if(!response.ok){let m="HTTP "+response.status;try{const j=await response.json();m=j.message||j.error||m;}catch{}throw Error(m);}
 return response;
}
async function json(path,options={}){return (await api(path,options)).json();}\nasync function postJson(path,body={}){return json(path,{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify(body)});}\nasync function markDesktopOperation(name,active){if(!desktopUpdateSupported)return;try{await postJson("/api/app/operation",{name,active});}catch{}}

function setTab(name){
 document.querySelectorAll("[data-tab]").forEach(b=>{const a=b.dataset.tab===name;b.classList.toggle("active",a);b.setAttribute("aria-selected",a?"true":"false");});
 document.querySelectorAll(".tab-panel").forEach(p=>p.hidden=p.id!=="tab-"+name);
 location.hash=name;
}
function initTabs(){
 document.querySelectorAll("[data-tab]").forEach(b=>b.onclick=()=>setTab(b.dataset.tab));
 const wanted=location.hash.slice(1);setTab(["flight","recovery","data","settings"].includes(wanted)?wanted:"flight");
}

function paintFlight(){
 const s=currentStatus,state=deriveFlightState(s,{saveVerified,connected});
 text("flightState",state);
 $("flightState").dataset.state=state;
 text("battery",s?.battery_v==null?"—":fmt(s.battery_v,3)+" V");
 const sd=s?.sd_ok===true?"OK":s?.sd_ok===false?"ERROR":"—";
 const gps=s?.gps_status==="live"?"LIVE":s?.gps_status==="stale"?"STALE":s?.gps_status==="missing"?"MISSING":"—";
 const lora=s?.radio_ok===true?"OK":s?.radio_ok===false?"MISSING":"—";
 text("sdIndicator",sd);text("gpsIndicator",gps);text("loraIndicator",lora);
 text("prepPhase",s?.flight_prep_phase||"—");text("prepProgress",s?.flight_prep_active?Math.max(0,Math.min(100,Number(s.flight_prep_progress)||0))+"%":"—");
 const problem=criticalFlightProblem(s),warnings=optionalFlightWarnings(s);
 text("flightMessage",problem||warnings.join(" ")||({
  OFFLINE:"Local app is running. Start the Windows ground station and connect the Feather.",
  READY:"Ready to run firmware-owned preparation.",
  TESTING:"Keep the rocket stationary while firmware checks the critical recorder path.",
  ARMED:"Armed by firmware. Launch detection and SD recording are autonomous.",
  FLYING:"Flight recording is active. Controls are locked.",
  SAVING:"Landing detected. Waiting for firmware-confirmed completed flight file.",
  SAVED:"Flight save verified by firmware file state.",
  ERROR:"Resolve the critical error before arming."
 }[state]||""));
 const prepare=$("prepareArm");
 prepare.hidden=state==="SAVED";
 prepare.disabled=!canPrepare(s,{saveVerified})||!connected;
 $("flyAgain").hidden=state!=="SAVED";
 $("flyAgain").disabled=state!=="SAVED"||!canPrepare(s,{saveVerified:true});
 document.querySelectorAll("[data-lock-during-flight]").forEach(el=>el.disabled=settingsMutationLocked(s));
 paintSettingsStatus(s);
 text("flightFile",s?.log_file||lastSaved?.name||"—");
 text("flightBytes",lastSaved?fmtBytes(lastSaved.size):"—");
 text("flightDuration",lastSaved?.durationMs!=null?(Number(lastSaved.durationMs)/1000).toFixed(1)+" s":"—");
 text("flightVerification",lastSaved?"Completion marker + file bytes/records structurally verified":"Not yet verified");
}
async function verifySave(){
 const SAVE_STATUS_FRESH_MS=3000;
 const statusFresh=()=>{
  const age=Date.now()-Number(lastStatusAt);
  return connected===true&&Number.isFinite(age)&&age>=0&&age<=SAVE_STATUS_FRESH_MS;
 };
 const s=currentStatus;
 saveVerified=false;lastSaved=null;
 if(!s||!statusFresh()||s.recording||s.armed||s.flight_prep_active){paintFlight();return;}
 const name=String(s.log_file||lastLogFile||"");
 if(!name||!name.toUpperCase().endsWith(".BIN")){paintFlight();return;}
 // Pure, fail-closed validator for the existing GreenIonMetadata completion writer.
 // This proves metadata/file structural agreement only; RKT4 v2 has no per-record checksum.
 const validateCompletionMetadata=(rawText,expectedName,fileEvidence)=>{
  const lines=String(rawText||"").split(/\r?\n/).map(x=>x.trim()).filter(Boolean);
  if(lines[0]!=="GREEN-ION-COMPLETION-1"||lines.filter(x=>x==="GREEN-ION-COMPLETION-1").length!==1)return null;
  const fields={};
  for(const line of lines.slice(1)){
   const i=line.indexOf("=");if(i<=0)return null;
   const key=line.slice(0,i).trim(),value=line.slice(i+1).trim();
   if(!key||Object.prototype.hasOwnProperty.call(fields,key))return null;
   fields[key]=value;
  }
  const leaf=String(expectedName||"").replace(/^\/+/, "");
  if(!leaf||fields.status!=="complete"||fields.file!==leaf||String(fileEvidence?.name||"")!==leaf)return null;
  const uint32=value=>{
   if(!/^(0|[1-9]\d*)$/.test(String(value||"")))return null;
   const n=Number(value);return Number.isSafeInteger(n)&&n>=0&&n<=0xffffffff?n:null;
  };
  const durationMs=uint32(fields.duration_ms),fileBytes=uint32(fields.file_bytes),records=uint32(fields.complete_records);
  const listedBytes=Number(fileEvidence?.size);
  if(durationMs===null||durationMs<=0||fileBytes===null||records===null||records<=0)return null;
  if(!Number.isSafeInteger(listedBytes)||listedBytes!==fileBytes)return null;
  if(fileBytes<48||(fileBytes-32)%16!==0||records!==(fileBytes-32)/16)return null;
  return {durationMs,fileBytes,completeRecords:records};
 };
 const stillCurrent=()=>statusFresh()&&currentStatus===s&&!s.recording&&!s.armed&&!s.flight_prep_active&&
  String(s.log_file||lastLogFile||"")===name;
 try{
  const files=await json("/api/files");lastFiles=files;
  if(!stillCurrent()){paintFlight();return;}
  const evidence=fileSaveEvidence(files,name);
  if(!evidence){paintFlight();return;}
  const metaText=await (await api("/api/metadata?name="+encodeURIComponent(name))).text();
  if(!stillCurrent()){paintFlight();return;}
  const verified=validateCompletionMetadata(metaText,name,evidence);
  if(!verified){paintFlight();return;}
  saveVerified=true;lastSaved={...evidence,durationMs:verified.durationMs};
  paintFlight();
 }catch{saveVerified=false;lastSaved=null;paintFlight();}
}
async function pollFlight(){
 try{
  const s=await json("/api/status");currentStatus=s;connected=true;lastStatusAt=Date.now();
  if(s.log_file)lastLogFile=s.log_file;
  paintFlight();
  const state=deriveFlightState(s,{saveVerified,connected:true});
  if(state==="SAVING"||(!s.recording&&!s.armed&&lastLogFile))await verifySave();
 }catch(e){connected=false;currentStatus=null;paintFlight();text("flightMessage","Feather status unavailable through the local Windows bridge: "+e.message);}
}
async function prepare(){
 if(!canPrepare(currentStatus,{saveVerified}))return;
 saveVerified=false;lastSaved=null;
 $("prepareArm").disabled=true;$("flyAgain").disabled=true;
 try{
  await json("/api/preflight/start",{method:"POST",headers:{"Content-Type":"application/x-www-form-urlencoded"},body:""});
  await pollFlight();
 }catch(e){text("flightMessage","PREPARE & ARM failed: "+e.message);}
}
function initFlight(){
 $("prepareArm").onclick=prepare;$("flyAgain").onclick=prepare;
 pollFlight();pollTimer=setInterval(pollFlight,1000);
}

function paintRecovery(){
 const view=recoveryView(lastPacket,lastKnown,Date.now());
 text("receiverState",serialPort?"CONNECTED":"DISCONNECTED");
 text("signalStatus",view.signal.toUpperCase());text("packetAge",fmtAge(view.packetAgeMs));
 text("gpsFix",view.gps.toUpperCase());
 text("coordinates",view.position?view.position.latitude.toFixed(6)+", "+view.position.longitude.toFixed(6):"No valid position");
 text("positionAge",fmtAge(view.positionAgeMs));
 text("rssi",view.rssiDbm==null?"—":fmt(view.rssiDbm,0)+" dBm");
 text("snr",view.snrDb==null?"—":fmt(view.snrDb,1)+" dB");
 text("invalidPackets",serialInvalid);
 text("packetSequence",lastPacket?.sequence??"—");
 text("receiverProtocol",lastPacket?.protocol??lastPacket?.raw?.protocol??"—");
 $("connectReceiver").disabled=!!serialPort;$("disconnectReceiver").disabled=!serialPort;
}
function acceptPacket(item){
 if(item.kind==="receiver_status"){
  text("receiverNote",item.ok?"Uno receiver reports radio ready.":"Uno receiver error: "+(item.error||"radio not ready"));
  lastPacket=item;paintRecovery();return;
 }
 lastPacket=item;
 if(item.gpsStatus==="live"&&item.latitude!=null){lastKnown={latitude:item.latitude,longitude:item.longitude,at:item.receivedAt};}
 text("receiverNote","Valid protocol-v2 "+item.type+" packet received.");
 paintRecovery();
}
function serialInfo(port){try{return port?.getInfo?.()||{};}catch{return {};}}
function sameSerialDevice(port,info){
 if(!info)return true;
 const now=serialInfo(port);
 if(info.usbVendorId!=null&&now.usbVendorId!==info.usbVendorId)return false;
 if(info.usbProductId!=null&&now.usbProductId!==info.usbProductId)return false;
 return true;
}
function scheduleReceiverReconnect(){
 clearTimeout(serialReconnectTimer);
 if(serialManualDisconnect||!navigator.serial?.getPorts)return;
 serialReconnectTimer=setTimeout(()=>reconnectGrantedReceiver(),900);
}
async function runReceiverPort(port,{automatic=false}={}){
 if(serialPort)return;
 serialPort=port;serialStop=false;
 try{
  await port.open({baudRate:115200,bufferSize:4096});
  lastSerialInfo=serialInfo(port);paintRecovery();
  text("receiverNote",automatic?"Arduino Uno reconnected using previously granted serial permission.":"USB permission granted. Waiting for newline-delimited JSON from the Arduino Uno at 115200 baud.");
  const decoder=new TextDecoder();let pending="";
  while(port.readable&&!serialStop){
   serialReader=port.readable.getReader();
   try{
    while(!serialStop){
     const {value,done}=await serialReader.read();if(done)break;
     pending+=decoder.decode(value,{stream:true});
     if(pending.length>131072){pending="";serialInvalid++;text("receiverNote","Oversized serial input discarded.");}
     let i;while((i=pending.indexOf("\n"))>=0){
      const line=pending.slice(0,i).trim();pending=pending.slice(i+1);
      if(!line)continue;
      const item=parseReceiverLine(line,Date.now());
      if(item)acceptPacket(item);else if(line.startsWith("{")){serialInvalid++;paintRecovery();}
     }
    }
   }finally{serialReader?.releaseLock();serialReader=null;}
  }
 }catch(e){
  if(!serialStop)text("receiverNote","Receiver connection failed: "+e.message);
 }finally{
  try{await port.close();}catch{}
  if(serialPort===port)serialPort=null;
  serialReader=null;paintRecovery();
  if(!serialManualDisconnect){text("receiverNote","Receiver disconnected. Reconnect USB; GREEN ION will retry previously granted ports automatically.");scheduleReceiverReconnect();}
 }
}
async function connectReceiver(){
 if(!navigator.serial){text("receiverNote","Web Serial requires current Chrome or Edge on Windows.");return;}
 serialManualDisconnect=false;serialInvalid=0;
 try{
  const port=await navigator.serial.requestPort();serialPermissionState="granted";serialPermissionMessage="";lastSerialInfo=serialInfo(port);await runReceiverPort(port);
 }catch(e){
  const denied=e?.name==="NotFoundError"||e?.name==="SecurityError"||/denied|cancel/i.test(String(e?.message||""));
  serialPermissionState=denied?"denied":"error";serialPermissionMessage=String(e?.message||e?.name||"");
  text("receiverNote",denied?"Receiver permission was not granted. Nothing was connected.":"Receiver permission/connection failed: "+serialPermissionMessage);
 }
}
async function reconnectGrantedReceiver(){
 if(serialPort||serialManualDisconnect||!lastSerialInfo||!navigator.serial?.getPorts)return false;
 try{
  const ports=(await navigator.serial.getPorts()).filter(p=>sameSerialDevice(p,lastSerialInfo));
  if(ports.length!==1){if(ports.length>1)text("receiverNote","Multiple previously granted serial ports are available. Choose CONNECT RECEIVER to select the Arduino Uno.");return false;}
  if(!ports.length)return false;
  runReceiverPort(ports[0],{automatic:true});return true;
 }catch{return false;}
}
async function disconnectReceiver(){
 serialManualDisconnect=true;serialStop=true;clearTimeout(serialReconnectTimer);
 try{await serialReader?.cancel();}catch{}
 try{await serialPort?.close();}catch{}
 serialPort=null;serialReader=null;paintRecovery();text("receiverNote","Receiver disconnected by user.");
}
function initRecovery(){
 $("connectReceiver").onclick=connectReceiver;$("disconnectReceiver").onclick=disconnectReceiver;
 if(navigator.serial){
  navigator.serial.addEventListener("disconnect",()=>{serialStop=true;try{serialReader?.cancel();}catch{}paintRecovery();text("receiverNote","Windows reported the Uno was unplugged. Reconnect USB; automatic reconnect will use previously granted permission.");scheduleReceiverReconnect();});
  navigator.serial.addEventListener("connect",()=>scheduleReceiverReconnect());
  reconnectGrantedReceiver();
 }
 paintRecovery();setInterval(paintRecovery,1000);
}

async function importBins(files){
 let added=0,dupes=0,failed=[];await markDesktopOperation("data-import",true);
 try{for(const file of [...files]){try{if(!/\.bin$/i.test(file.name))throw Error("Only BIN files are accepted here.");const bytes=await file.arrayBuffer();parseRkt4(bytes);const result=await saveLocalFile({name:file.name,bytes,origin:"manual-v18-import",metadata:{importedAt:new Date().toISOString()}});result.duplicate?dupes++:added++;}catch(e){failed.push(file.name+": "+e.message);}}text("importStatus",added+" imported · "+dupes+" duplicate"+(failed.length?" · "+failed.join(" | "):""));await refreshHistory();}finally{await markDesktopOperation("data-import",false);}
}
async function refreshHistory(){
 const files=(await listLocalMetadata()).filter(x=>x.kind==="BIN").sort((a,b)=>String(b.createdAt).localeCompare(String(a.createdAt)));
 const body=$("historyRows");body.replaceChildren();
 for(const f of files){
  const tr=document.createElement("tr");
  tr.innerHTML="<td>"+escapeHtml(f.name)+"</td><td>"+fmtBytes(f.size)+"</td><td><code>"+escapeHtml(f.checksum.slice(0,12))+"…</code></td><td>"+escapeHtml(new Date(f.createdAt).toLocaleString())+"</td>";
  tr.onclick=()=>{ $("analysisFlight").value=f.key; analyzeSelected();};
  body.append(tr);
 }
 for(const id of ["analysisFlight","compareFlight"]){
  const sel=$(id),old=sel.value;sel.replaceChildren();
  const blank=document.createElement("option");blank.value="";blank.textContent=id==="analysisFlight"?"Choose a flight":"Optional comparison";sel.append(blank);
  files.forEach(f=>{const o=document.createElement("option");o.value=f.key;o.textContent=f.name;sel.append(o);});
  if(files.some(f=>f.key===old))sel.value=old;
 }
 text("historyCount",files.length+" local flight"+(files.length===1?"":"s"));
}
async function loadAnalysis(key){
 const record=await getLocalFile(key);if(!record)throw Error("Flight not found.");
 const parsed=parseRkt4(record.bytes),analysis=analyzeFlight(parsed);
 return {record,analysis};
}
function drawAnalysis(){
 const canvas=$("analysisCanvas"),ctx=canvas.getContext("2d"),w=canvas.width,h=canvas.height;ctx.clearRect(0,0,w,h);
 const series=[activeAnalysis,compareAnalysis].filter(Boolean);if(!series.length)return;
 const points=series.map(item=>item.analysis.samples.filter(s=>Number.isFinite(s.timeS)&&Number.isFinite(s.altitudeM)));
 const all=points.flat();if(all.length<2)return;
 const maxX=Math.max(...all.map(p=>p.timeS)),minY=Math.min(...all.map(p=>p.altitudeM)),maxY=Math.max(...all.map(p=>p.altitudeM));
 const x=t=>50+(t/Math.max(1,maxX))*(w-70),y=v=>h-35-(v-minY)/Math.max(1,maxY-minY)*(h-60);
 ctx.strokeStyle="#35536e";ctx.strokeRect(50,20,w-70,h-55);
 points.forEach((arr,index)=>{ctx.strokeStyle=index?"#72e5ab":"#7bdbff";ctx.lineWidth=2;ctx.beginPath();const stride=Math.max(1,Math.ceil(arr.length/2500));for(let i=0;i<arr.length;i+=stride){const p=arr[i];i?ctx.lineTo(x(p.timeS),y(p.altitudeM)):ctx.moveTo(x(p.timeS),y(p.altitudeM));}ctx.stroke();});
 ctx.fillStyle="#dcecff";ctx.font="13px system-ui";ctx.fillText("Pressure-derived altitude (m)",55,16);ctx.fillText(maxX.toFixed(1)+" s",w-65,h-10);
}
function renderPhaseTimeline(analysis){
 const root=$("phaseTimeline");if(!root)return;root.replaceChildren();
 if(!analysis?.phases?.length){const step=document.createElement("div");step.className="timeline-step";const strong=document.createElement("strong");strong.textContent="No reliable phase estimates";const span=document.createElement("span");span.textContent="The analyzer did not find enough pressure evidence.";step.append(strong,span);root.append(step);return;}
 for(const phase of analysis.phases){
  const step=document.createElement("div");step.className="timeline-step";
  const strong=document.createElement("strong");strong.textContent=String(phase.name||"phase").toUpperCase();
  const at=document.createElement("span");at.textContent=fmt(phase.timeS,2)+" s · "+fmt(phase.altitudeM,2)+" m estimated";
  const confidence=document.createElement("span");confidence.textContent=String(phase.confidence||"unknown")+" confidence";
  step.append(strong,at,confidence);root.append(step);
 }
}
function renderAnalysis(){
 if(!activeAnalysis){text("analysisSummary","Choose a flight.");renderPhaseTimeline(null);return;}
 const a=activeAnalysis.analysis;
 const phases=a.phases.map(p=>p.name+" ("+p.confidence+" confidence)").join(", ")||"not reliably detected";
 text("analysisSummary","Apogee "+fmt(a.peakAltitudeM,2)+" m estimated · samples "+a.samples.length+" · "+a.rateHz+" Hz · phases: "+phases);
 text("analysisWarnings",[...new Set(a.warnings)].join(" · "));
 renderPhaseTimeline(a);
 if(compareAnalysis)text("compareSummary","Apogee difference: "+fmt(compareAnalysis.analysis.peakAltitudeM-a.peakAltitudeM,2)+" m (comparison minus primary).");
 else text("compareSummary","No comparison selected.");
 drawAnalysis();
}
async function analyzeSelected(){
 const key=$("analysisFlight").value;if(!key){activeAnalysis=null;renderAnalysis();return;}
 try{activeAnalysis=await loadAnalysis(key);renderAnalysis();}catch(e){text("analysisSummary","Cannot analyze: "+e.message);}
}
async function compareSelected(){
 const key=$("compareFlight").value;if(!key){compareAnalysis=null;renderAnalysis();return;}
 try{compareAnalysis=await loadAnalysis(key);renderAnalysis();}catch(e){text("compareSummary","Cannot compare: "+e.message);}
}
function downloadText(name,data,type){
 const a=document.createElement("a");a.href=URL.createObjectURL(new Blob([data],{type}));a.download=name;document.body.append(a);a.click();const u=a.href;a.remove();setTimeout(()=>URL.revokeObjectURL(u),30000);
}
async function exportBackup(){downloadText("green-ion-v18-local-backup.json",await exportArchive(),"application/json");}
async function restoreBackup(file){if(!file)return;try{const r=await restoreArchive(await file.text());text("backupStatus","Restored "+r.restored+" · duplicates "+r.duplicates);await refreshHistory();}catch(e){text("backupStatus","Restore failed: "+e.message);}}
function initData(){
 $("binImport").onchange=e=>importBins(e.target.files);
 $("analysisFlight").onchange=analyzeSelected;$("compareFlight").onchange=compareSelected;
 $("exportCsv").onclick=()=>{if(activeAnalysis)downloadText(activeAnalysis.record.name.replace(/\.bin$/i,"")+"-analysis.csv",analysisCsv(activeAnalysis.analysis),"text/csv");};
 $("exportJson").onclick=()=>{if(activeAnalysis)downloadText(activeAnalysis.record.name.replace(/\.bin$/i,"")+"-report.json",JSON.stringify(flightReport(activeAnalysis.analysis,activeAnalysis.record.name),null,2),"application/json");};
 $("printReport").onclick=()=>window.print();
 $("downloadOriginal").onclick=()=>{if(activeAnalysis)downloadFile(activeAnalysis.record);};
 $("exportBackup").onclick=exportBackup;$("restoreBackup").onchange=e=>restoreBackup(e.target.files?.[0]);
 refreshHistory();
}

function settingsMutationLocked(s=currentStatus){
 if(!connected||!s)return true;
 return !!(s.recording||s.armed||s.flight_prep_active||[2,5,6].includes(Number(s.recorder_mode)));
}
function paintSettingsStatus(s=currentStatus){
 const locked=settingsMutationLocked(s);
 for(const id of ["restartCalibration","saveProfile","clearCache","runFirmwareDiagnostic"]){const el=$(id);if(el)el.disabled=locked;}
 text("calibrationBaseline",s?.calibration_baseline_pa==null?"—":fmt(s.calibration_baseline_pa,1)+" Pa");
 text("calibrationNoise",s?.calibration_noise_m==null?"—":fmt(s.calibration_noise_m,3)+" m");
 text("calibrationAge",s?.calibration_age_s==null?"—":fmtAge(Number(s.calibration_age_s)*1000));
 text("relativeAltitude",s?.relative_altitude_m==null?"—":fmt(s.relative_altitude_m,2)+" m");
 text("sampleRate",s?.sample_rate_hz==null?"—":String(s.sample_rate_hz)+" Hz");
 text("settingsRecorderMode",s?.recorder_mode??"—");
 text("firmwareVersion",s?.firmware_version||s?.firmware_build||"—");
 text("bootSession",s?.boot_session_id??"—");
 text("resetReason",s?.reset_reason||"—");
 text("missedSamples",s?.missed_sample_periods??"—");
 text("wifiMode",s?.wifi_mode||"—");
 if(s&&$("rocketProfile")&&!$("rocketProfile").dataset.dirty)$("rocketProfile").value=s.selected_rocket||"";
 if(s&&$("motorProfile")&&!$("motorProfile").dataset.dirty)$("motorProfile").value=s.selected_motor||"";
 text("statusJson",s?JSON.stringify(s,null,2):"No Feather status.");
}
async function restartCalibration(){
 if(settingsMutationLocked()){text("calibrationStatus","Calibration is unavailable while disconnected, preparing, armed, recording, maintenance, or home testing.");return;}
 try{
  const result=await json("/api/calibrate",{method:"POST",headers:{"Content-Type":"application/x-www-form-urlencoded"},body:""});
  text("calibrationStatus",result.message||"Stationary calibration restarted. Keep the rocket still.");
  await pollFlight();
 }catch(e){text("calibrationStatus","Calibration request failed: "+e.message);}
}
async function saveFlightProfile(){
 if(settingsMutationLocked()){text("profileStatus","Profile changes are unavailable while disconnected or flight activity is active.");return;}
 const rocket=$("rocketProfile").value.trim(),motor=$("motorProfile").value.trim();
 if(!rocket||rocket.length>24||motor.length>24){text("profileStatus","Rocket must be 1–24 characters; motor may be 0–24 characters.");return;}
 const body=new URLSearchParams({rocket,motor}).toString();
 try{
  const result=await json("/api/profile",{method:"POST",headers:{"Content-Type":"application/x-www-form-urlencoded"},body});
  $("rocketProfile").dataset.dirty="";$("motorProfile").dataset.dirty="";
  text("profileStatus",result.message||"Flight profile saved. Firmware restarted stationary calibration.");
  await pollFlight();
 }catch(e){text("profileStatus","Profile save failed: "+e.message);}
}
// Guided development checks are read-only unless the user explicitly requests the existing firmware IMU+GPS diagnostic.
function devCheck(name,result,action,id="manual"){
 if(!["PASS","FAIL","NOT TESTED"].includes(result))throw Error("Invalid development test result");
 return {id,label:name,state:result,evidence:action,next:""};
}
let lastDevReport="";
const devAdvice={
 calibration:"Connected Feather: keep stationary and use calibration only when safe.",
 offline_cache:"Offline-only: load on localhost, refresh once, then manually test offline reload.",
 bin_import:"Offline-only: import a known-good RKT4 v2 BIN from Data; preserve the original.",
 receiver_manual:"Uno receiver: connect through Recovery; manually verify invalid/stale/disconnect cases.",
 offline_reload:"Offline-only: disconnect networking and reload the cached page manually.",
 sd_physical:"Physical motor-free check: use reserved temporary SD test only; never delete flight BINs.",
 sensor_accuracy:"Physical motor-free check: compare actual sensors with a reference.",
 rf_physical:"Physical motor-free check: verify antenna, GPS, radio and wake separately.",
 multi_flight:"Physical motor-free check: preserve two distinct original BIN files."
};
function safeDevReport(rows){
 // Do not include raw evidence: it may contain coordinates, file paths, serial IDs or secrets.
 const bridgeSha=typeof currentStatus?.preview_git_sha==="string"?currentStatus.preview_git_sha:"";
 const previewVersion=typeof currentStatus?.preview_version==="string"?currentStatus.preview_version:"V18 offline";
 return safeDevelopmentReport(rows,{previewVersion,gitSha:bridgeSha});
}
function renderDevRows(rows){
 const tbody=$("devTestRows");tbody.replaceChildren();
 for(const row of rows){
  const tr=document.createElement("tr");
  const name=document.createElement("td");name.textContent=row.label;
  const result=document.createElement("td");result.textContent=row.state;result.className="dev-result";result.dataset.result=row.state;
  const action=document.createElement("td");action.textContent=[row.evidence,row.state==="PASS"?"":(devAdvice[row.id]||"Check the relevant tab and real-device evidence; do not assume PASS.")].filter(Boolean).join(" ");
  tr.append(name,result,action);tbody.appendChild(tr);
 }
 const pass=rows.filter(r=>r.state==="PASS").length,fail=rows.filter(r=>r.state==="FAIL").length;
 lastDevReport=safeDevReport(rows);$("copyDevResults").disabled=false;
 text("devTestSummary",pass+" PASS · "+fail+" FAIL · "+(rows.length-pass-fail)+" NOT TESTED. PASS is limited to the evidence named in each row.");
}
async function runDevTests(){
 const now=Date.now();
 const rows=[...developmentStatusChecks(currentStatus,{connected,lastStatusAt,now})];
 const s=connected?currentStatus:null;
 const baseline=Number(s?.calibration_baseline_pa),noise=Number(s?.calibration_noise_m);
 rows.push(devCheck("Stationary calibration evidence",
  Number.isFinite(baseline)&&baseline>30000&&Number.isFinite(noise)&&noise>=0?"PASS":"NOT TESTED",
  Number.isFinite(baseline)&&baseline>30000&&Number.isFinite(noise)&&noise>=0?
   "Firmware reports a plausible pressure baseline and calibration noise. This does not prove launch-site stability.":
   "No current valid calibration baseline/noise evidence; keep the device stationary and use firmware calibration only when safe.",
  "calibration"));
 let reg=null,cacheOk=false;
 try{
  if("serviceWorker" in navigator)reg=await navigator.serviceWorker.getRegistration();
  if(typeof caches!=="undefined")cacheOk=await caches.has(V18_CACHE_VERSION);
 }catch{}
 rows.push(devCheck("Local offline cache registration",
  reg?.active&&cacheOk?"PASS":"NOT TESTED",
  reg?.active&&cacheOk?"Active V18 service worker and v4 cache found. Actual offline reload still needs manual verification.":"Start from localhost, refresh once, and wait for the V18 cache before the manual offline-reload test.",
  "offline_cache"));
 let manual=null,analysisOk=false,analysisError="";
 try{
  const imported=(await listLocalMetadata()).filter(x=>x.kind==="BIN"&&x.origin==="manual-v18-import").sort((a,b)=>String(b.createdAt).localeCompare(String(a.createdAt)));
  if(imported.length){
   manual=await getLocalFile(imported[0].key);
   const parsed=parseRkt4(manual.bytes),analysis=analyzeFlight(parsed);
   analysisOk=parsed.samples.length>0&&analysis.samples.length>0;
  }
 }catch(e){analysisError=e.message;}
 rows.push(devCheck("Manual original BIN import + analysis",
  manual&&analysisOk?"PASS":manual?"FAIL":"NOT TESTED",
  manual&&analysisOk?"Reopened "+manual.name+" from IndexedDB and parsed/analyzed its preserved bytes locally.":manual?"Imported BIN analysis failed: "+analysisError:"Go to Data and import a known-good RKT4 v2 BIN; the importer preserves original bytes.",
  "bin_import"));
 rows.push(developmentSerialAccessCheck({supported:!!navigator.serial,state:serialPermissionState,message:serialPermissionMessage}));
 rows.push(...developmentReceiverChecks(lastPacket,lastKnown,{portConnected:!!serialPort,now}));
 rows.push(developmentFirmwareDiagnostic(connected&&now-lastStatusAt<=5000?currentStatus:null,{requested:devFirmwareRequested}));
 rows.push(devCheck("Receiver malformed/stale/disconnect behavior","NOT TESTED","Manually observe an invalid/stale packet and unplug/reconnect the physical Uno; record the UI result.","receiver_manual"));
 for(const [name,action,id] of [
  ["Actual offline restart","After the cache row passes, disconnect networking and reload the local V18 page; confirm all four tabs render.","offline_reload"],
  ["Physical SD write/read/verify/temp-delete","Use the approved motor-free hardware checklist; CHECK EVERYTHING never creates or deletes SD files and never deletes real BINs.","sd_physical"],
  ["Barometer/IMU physical accuracy","Compare actual sensor behavior with a stationary bench reference.","sensor_accuracy"],
  ["GPS/LoRa RF + recovery timing/wake","Use real hardware, approved antennas and the motor-free checklist; browser status is not RF-range evidence.","rf_physical"],
  ["Two distinct saved hardware flights","Use only an approved motor-free procedure and preserve both original BIN files.","multi_flight"]
 ])rows.push(devCheck(name,"NOT TESTED",action,id));
 renderDevRows(rows);
 return rows;
}
async function runFirmwareDiagnostic(){
 if(settingsMutationLocked()){text("devFirmwareStatus","Diagnostic unavailable while disconnected, preparing, armed, recording, maintenance, or home-test activity is active.");await runDevTests();return;}
 const button=$("runFirmwareDiagnostic");button.disabled=true;
 try{
  const result=await json("/api/hardware-test",{method:"POST",headers:{"Content-Type":"application/x-www-form-urlencoded"},body:new URLSearchParams({sensor:"all"}).toString()});
  if(result.accepted!==true)throw Error(result.error||"Firmware did not accept the diagnostic.");
  devFirmwareRequested=true;
  text("devFirmwareStatus","Firmware IMU + GPS diagnostic accepted. Keep the Feather stationary; GPS may need an outdoor fix. No arming, recording, LoRa command, or file deletion was requested.");
  await pollFlight();
 }catch(e){text("devFirmwareStatus","Firmware diagnostic failed to start: "+e.message);}
 finally{button.disabled=settingsMutationLocked();await runDevTests();}
}
function initDevTests(){
 $("openDevTests").onclick=()=>{setTab("settings");$("devTestCard").scrollIntoView({behavior:"smooth",block:"start"});};
 $("copyDevResults").onclick=async()=>{if(!lastDevReport)return;try{await navigator.clipboard.writeText(lastDevReport);text("devCopyStatus","Private redacted results copied locally. Review before sharing.");}catch{text("devCopyStatus","Clipboard permission denied. Use localhost Chrome/Edge and retry.");}};
 $("runDevTests").onclick=()=>{runDevTests().catch(e=>text("devTestSummary","Local check failed: "+e.message));};
 $("runFirmwareDiagnostic").onclick=()=>{runFirmwareDiagnostic().catch(e=>text("devFirmwareStatus","Diagnostic error: "+e.message));};
}

function paintDesktopUpdate(i){if(!i)return;text("installedAppVersion",i.installed_version||"—");text("latestAppVersion",i.latest_version||"NONE");text("appUpdateState",String(i.state||"idle").toUpperCase());text("appUpdateSignature",String(i.signature||"not-checked").toUpperCase());$("appUpdateProgress").value=Math.max(0,Math.min(1,Number(i.download_progress)||0));$("autoUpdateCheck").checked=!!i.auto_check;$("autoUpdateCheck").disabled=!desktopUpdateSupported;$("checkAppUpdate").disabled=!desktopUpdateSupported||i.state==="checking"||i.state==="downloading";$("downloadAppUpdate").disabled=!desktopUpdateSupported||!i.update_available||i.state==="downloading";$("installAppUpdate").disabled=!desktopUpdateSupported||!i.downloaded_verified||!i.safe_to_install;$("rollbackAppUpdate").disabled=!desktopUpdateSupported||!i.safe_to_install;text("appReleaseNotes",i.release_notes||"No release notes available.");const p=[];if(i.error)p.push(i.error);else if(i.update_available)p.push("Approved update "+i.latest_version+" is available.");else if(i.latest_version)p.push("No newer approved V18 Windows release.");else p.push("No approved V18 Windows release checked yet.");if(i.downloaded_verified)p.push("Installer download is SHA-256 verified.");if(i.signature==="unsigned")p.push("Release is unsigned; SHA-256 is verified but publisher identity is not signed.");if(!i.safe_to_install)p.push("Install/rollback deferred: "+(i.safety_reason||"hardware state unknown."));text("appUpdateStatus",p.join(" "));}
async function refreshDesktopUpdate(){try{const i=await json("/api/app/update/status");desktopUpdateSupported=i?.release_only===true;paintDesktopUpdate(i);return i;}catch{desktopUpdateSupported=false;["autoUpdateCheck","checkAppUpdate","downloadAppUpdate","installAppUpdate","rollbackAppUpdate"].forEach(id=>$(id).disabled=true);text("appUpdateState","UNAVAILABLE");text("appUpdateStatus","Integrated updater is available in packaged GREEN_ION.exe.");return null;}}
async function initDesktopUpdates(){const first=await refreshDesktopUpdate();if(!desktopUpdateSupported)return;$("autoUpdateCheck").onchange=async()=>paintDesktopUpdate(await postJson("/api/app/update/preferences",{auto_check:$("autoUpdateCheck").checked}));$("checkAppUpdate").onclick=async()=>{try{paintDesktopUpdate(await postJson("/api/app/update/check"));}catch(e){text("appUpdateStatus","Update check failed: "+e.message);}};$("downloadAppUpdate").onclick=async()=>{clearInterval(desktopUpdatePoll);desktopUpdatePoll=setInterval(refreshDesktopUpdate,800);try{paintDesktopUpdate(await postJson("/api/app/update/download"));}catch(e){text("appUpdateStatus","Update download failed: "+e.message);}finally{clearInterval(desktopUpdatePoll);desktopUpdatePoll=null;await refreshDesktopUpdate();}};$("installAppUpdate").onclick=async()=>{if(confirm("Install the verified GREEN ION update now? Hardware must be freshly known idle."))try{paintDesktopUpdate(await postJson("/api/app/update/install"));}catch(e){text("appUpdateStatus","Installation deferred: "+e.message);}};$("rollbackAppUpdate").onclick=async()=>{if(confirm("Roll back to the previously verified GREEN ION executable?"))try{paintDesktopUpdate(await postJson("/api/app/update/rollback"));}catch(e){text("appUpdateStatus","Rollback unavailable: "+e.message);}};if(first?.state==="downloading")desktopUpdatePoll=setInterval(refreshDesktopUpdate,800);setInterval(refreshDesktopUpdate,5000);}
async function desktopHeartbeat(){if(desktopUpdateSupported)try{await postJson("/api/app/heartbeat");}catch{}}

async function initSettings(){
 let persisted=false;
 try{persisted=await navigator.storage?.persisted?.()||false;if(!persisted&&navigator.storage?.persist)persisted=await navigator.storage.persist();}catch{}
 text("storageStatus",storageMessage(persisted));
 text("secureContext",window.isSecureContext?"YES":"NO");
 text("webSerialSupport",navigator.serial?"YES":"NO");
 text("serviceWorkerSupport","serviceWorker" in navigator?"YES":"NO");
 text("appOrigin",location.origin);
 $("rocketProfile").oninput=()=>{$("rocketProfile").dataset.dirty="1";};
 $("motorProfile").oninput=()=>{$("motorProfile").dataset.dirty="1";};
 $("restartCalibration").onclick=restartCalibration;
 $("saveProfile").onclick=saveFlightProfile;
 $("clearCache").onclick=async()=>{if(settingsMutationLocked())return;text("cacheStatus","Refreshing V18 offline cache…");try{const reg=await navigator.serviceWorker?.getRegistration("./v18_service_worker.js");await reg?.update();text("cacheStatus","Offline cache update check complete.");}catch(e){text("cacheStatus","Cache update failed: "+e.message);}};
 await initDesktopUpdates();
 paintSettingsStatus();
}

async function initOffline(){
 if("serviceWorker" in navigator&&(location.protocol==="https:"||location.hostname==="localhost"||location.hostname==="127.0.0.1")){
  try{await navigator.serviceWorker.register("./v18_service_worker.js");text("offlineStatus","Offline app cache registered.");}
  catch(e){text("offlineStatus","Offline cache unavailable: "+e.message);}
 }else text("offlineStatus","Run from the local Windows server to enable offline app caching and Web Serial.");
}

initTabs();initDevTests();initFlight();initRecovery();initData();initSettings();initOffline();
setInterval(desktopHeartbeat,10000);\nwindow.addEventListener("beforeunload",()=>{clearInterval(pollTimer);clearInterval(desktopUpdatePoll);});
