import {parseRkt4,analyzeFlight,analysisCsv,flightReport} from "./analysis/rkt4.mjs";
import {saveLocalFile,listLocalMetadata,getLocalFile,downloadFile,digest,exportArchive,restoreArchive} from "./v14_store.mjs";
import {deriveFlightState,criticalFlightProblem,optionalFlightWarnings,canPrepare,fileSaveEvidence,parseReceiverLine,recoveryView,storageMessage} from "./v18_offline_core.mjs";

const $=id=>document.getElementById(id);
let currentStatus=null,connected=false,lastFiles=null,saveVerified=false,lastSaved=null,lastLogFile="",pollTimer=null;
let serialPort=null,serialReader=null,serialStop=false,lastPacket=null,lastKnown=null,serialInvalid=0,activeAnalysis=null,compareAnalysis=null;

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
async function json(path,options={}){return (await api(path,options)).json();}

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
 document.querySelectorAll("[data-lock-during-flight]").forEach(el=>el.disabled=!!(s?.recording||s?.armed||s?.flight_prep_active));
 text("flightFile",s?.log_file||lastSaved?.name||"—");
 text("flightBytes",lastSaved?fmtBytes(lastSaved.size):"—");
 text("flightDuration",lastSaved?.durationMs!=null?(Number(lastSaved.durationMs)/1000).toFixed(1)+" s":"—");
 text("flightVerification",lastSaved?"Firmware COMPLETE + metadata read":"Not yet verified");
}
async function verifySave(){
 const s=currentStatus;if(!s||s.recording||s.armed||s.flight_prep_active)return;
 const name=String(s.log_file||lastLogFile||"");
 if(!name||!name.toUpperCase().endsWith(".BIN"))return;
 try{
  const files=await json("/api/files");lastFiles=files;
  const evidence=fileSaveEvidence(files,name);
  if(!evidence){saveVerified=false;paintFlight();return;}
  const metaText=await (await api("/api/metadata?name="+encodeURIComponent(name))).text();
  const meta=parseMeta(metaText);
  saveVerified=true;lastSaved={...evidence,durationMs:Number.isFinite(Number(meta.duration_ms))?Number(meta.duration_ms):null};
  paintFlight();
 }catch{saveVerified=false;paintFlight();}
}
async function pollFlight(){
 try{
  const s=await json("/api/status");currentStatus=s;connected=true;
  if(s.log_file)lastLogFile=s.log_file;
  paintFlight();
  const state=deriveFlightState(s,{saveVerified,connected:true});
  if(state==="SAVING"||(!s.recording&&!s.armed&&lastLogFile))await verifySave();
 }catch(e){connected=false;paintFlight();text("flightMessage","Feather status unavailable through the local Windows bridge: "+e.message);}
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
async function connectReceiver(){
 if(!navigator.serial){text("receiverNote","Web Serial requires current Chrome or Edge on Windows.");return;}
 try{
  serialPort=await navigator.serial.requestPort();
  await serialPort.open({baudRate:115200,bufferSize:4096});
  serialStop=false;serialInvalid=0;paintRecovery();
  text("receiverNote","USB permission granted. Waiting for newline-delimited JSON from the Arduino Uno at 115200 baud.");
  const decoder=new TextDecoder();let pending="";
  while(serialPort?.readable&&!serialStop){
   serialReader=serialPort.readable.getReader();
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
 }catch(e){text("receiverNote","Receiver connection failed: "+e.message);}
 finally{
  try{await serialPort?.close();}catch{}
  serialPort=null;serialReader=null;paintRecovery();
  if(!serialStop)text("receiverNote","Receiver disconnected. Reconnect the USB cable, then choose CONNECT RECEIVER again.");
 }
}
async function disconnectReceiver(){
 serialStop=true;
 try{await serialReader?.cancel();}catch{}
 try{await serialPort?.close();}catch{}
 serialPort=null;serialReader=null;paintRecovery();text("receiverNote","Receiver disconnected by user.");
}
function initRecovery(){
 $("connectReceiver").onclick=connectReceiver;$("disconnectReceiver").onclick=disconnectReceiver;
 if(navigator.serial)navigator.serial.addEventListener("disconnect",()=>{serialStop=true;serialPort=null;serialReader=null;paintRecovery();text("receiverNote","Windows reported the Uno was unplugged. Reconnect it and choose CONNECT RECEIVER.");});
 paintRecovery();setInterval(paintRecovery,1000);
}

async function importBins(files){
 let added=0,dupes=0,failed=[];
 for(const file of [...files]){
  try{
   if(!/\.bin$/i.test(file.name))throw Error("Only BIN files are accepted here.");
   const bytes=await file.arrayBuffer();
   parseRkt4(bytes);
   const result=await saveLocalFile({name:file.name,bytes,origin:"manual-v18-import",metadata:{importedAt:new Date().toISOString()}});
   result.duplicate?dupes++:added++;
  }catch(e){failed.push(file.name+": "+e.message);}
 }
 text("importStatus",added+" imported · "+dupes+" duplicate"+(failed.length?" · "+failed.join(" | "):""));
 await refreshHistory();
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
function renderAnalysis(){
 if(!activeAnalysis){text("analysisSummary","Choose a flight.");return;}
 const a=activeAnalysis.analysis;
 const phases=a.phases.map(p=>p.name+" ("+p.confidence+" confidence)").join(", ")||"not reliably detected";
 text("analysisSummary","Apogee "+fmt(a.peakAltitudeM,2)+" m estimated · samples "+a.samples.length+" · "+a.rateHz+" Hz · phases: "+phases);
 text("analysisWarnings",[...new Set(a.warnings)].join(" · "));
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

async function initSettings(){
 let persisted=false;
 try{persisted=await navigator.storage?.persisted?.()||false;if(!persisted&&navigator.storage?.persist)persisted=await navigator.storage.persist();}catch{}
 text("storageStatus",storageMessage(persisted));
 text("secureContext",window.isSecureContext?"YES":"NO");
 text("webSerialSupport",navigator.serial?"YES":"NO");
 text("serviceWorkerSupport","serviceWorker" in navigator?"YES":"NO");
 text("appOrigin",location.origin);
 $("clearCache").onclick=async()=>{if(currentStatus?.recording||currentStatus?.armed)return;text("cacheStatus","Refreshing V18 offline cache…");try{const reg=await navigator.serviceWorker?.getRegistration("./v18_service_worker.js");await reg?.update();text("cacheStatus","Offline cache update check complete.");}catch(e){text("cacheStatus","Cache update failed: "+e.message);}};
}
async function initOffline(){
 if("serviceWorker" in navigator&&(location.protocol==="https:"||location.hostname==="localhost"||location.hostname==="127.0.0.1")){
  try{await navigator.serviceWorker.register("./v18_service_worker.js");text("offlineStatus","Offline app cache registered.");}
  catch(e){text("offlineStatus","Offline cache unavailable: "+e.message);}
 }else text("offlineStatus","Run from the local Windows server to enable offline app caching and Web Serial.");
}

initTabs();initFlight();initRecovery();initData();initSettings();initOffline();
window.addEventListener("beforeunload",()=>{clearInterval(pollTimer);});
