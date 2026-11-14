const $=id=>document.getElementById(id);

export const QUICK_FLY_STATES=Object.freeze(["READY","TESTING","ARMED","FLYING","SAVING","SAVED","ERROR"]);

export function finiteNumber(value){
 const n=Number(value);
 return Number.isFinite(n)?n:null;
}

export function deriveQuickFlyState(status,{connected=true,verifiedSaved=false}={}){
 if(!connected||!status)return "ERROR";
 if(status.flight_prep_failure||status.recorder_mode===4||status.sd_ok===false)return "ERROR";
 if(status.recording||status.recorder_mode===2)return "FLYING";
 if(status.flight_prep_active)return "TESTING";
 if(status.armed||status.flight_prep_ready||status.recorder_mode===1)return "ARMED";
 if(status.recorder_mode===3)return verifiedSaved?"SAVED":"SAVING";
 return "READY";
}

export function nextFlightNumber(filePayload){
 const explicit=finiteNumber(filePayload?.next_sequence);
 if(explicit!==null&&explicit>=0&&explicit<1000)return Math.floor(explicit)+1;
 let highest=-1;
 for(const file of filePayload?.files||[]){
  const match=/^V12D(\d{3})\.BIN$/.exec(String(file?.name||""));
  if(match)highest=Math.max(highest,Number(match[1]));
 }
 return highest+2;
}

export function parseFlightMetadata(text){
 const result={};
 for(const raw of String(text||"").split(/\r?\n/)){
  const split=raw.indexOf("=");
  if(split<=0)continue;
  result[raw.slice(0,split).trim()]=raw.slice(split+1).trim();
 }
 return {
  durationMs:finiteNumber(result.duration_ms),
  fileBytes:finiteNumber(result.file_bytes),
  samples:finiteNumber(result.samples),
  missedSamplePeriods:finiteNumber(result.missed_sample_periods),
  raw:result
 };
}

export function formatBytes(value){
 const n=finiteNumber(value);
 if(n===null)return "--";
 if(n>=1024*1024)return (n/(1024*1024)).toFixed(n>=10*1024*1024?1:2)+" MB";
 if(n>=1024)return (n/1024).toFixed(1)+" KB";
 return Math.round(n)+" B";
}

export function batteryAdvisory(volts){
 const v=finiteNumber(volts);
 if(v===null)return {level:"unknown",text:"Battery unavailable"};
 if(v<3.60)return {level:"critical",text:"CRITICAL — below 3.60 V; firmware will block arming"};
 if(v<3.75)return {level:"warning",text:"LOW — recharge before launch"};
 return {level:"ok",text:"Battery OK"};
}

export function storageAdvisory(free,total){
 const f=finiteNumber(free),t=finiteNumber(total);
 if(f===null||t===null||t<=0)return {level:"unknown",text:"Storage capacity unavailable"};
 const ratio=f/t;
 if(f<8*1024*1024||ratio<0.05)return {level:"warning",text:"LOW STORAGE — clear old flights after verifying backups"};
 return {level:"ok",text:"Storage OK"};
}

export function optionalWarnings(status){
 const warnings=[];
 if(status?.gps_status!=="live")warnings.push("GPS unavailable or stale — recovery aid only; does not block SD recording.");
 if(status?.radio_ok!==true)warnings.push("LoRa unavailable — optional; microSD remains the required flight record.");
 return warnings;
}

function apiUrl(path){
 return window.greenIonApiUrl?window.greenIonApiUrl(path):path;
}

async function request(path,{method="GET",body=null,text=false}={}){
 const options={method,cache:"no-store"};
 if(body){
  options.headers={"Content-Type":"application/x-www-form-urlencoded"};
  options.body=new URLSearchParams(body).toString();
 }
 const response=await fetch(path,options);
 const payload=text?await response.text():await response.json().catch(()=>({}));
 if(!response.ok||(!text&&payload?.success===false)){
  throw Error((!text&&payload?.message)||("HTTP "+response.status));
 }
 return payload;
}

function humanDuration(ms){
 const n=finiteNumber(ms);
 if(n===null)return "--";
 const seconds=n/1000;
 if(seconds<120)return seconds.toFixed(1)+" s";
 return Math.floor(seconds/60)+"m "+Math.round(seconds%60)+"s";
}

function setTone(element,level){
 element.classList.remove("ok","warning","bad");
 if(level==="ok")element.classList.add("ok");
 else if(level==="critical")element.classList.add("bad");
 else if(level==="warning")element.classList.add("warning");
}

class QuickFlyApp{
 constructor(){
  this.status=null;
  this.connected=false;
  this.commandLock=false;
  this.lastCommandAt=0;
  this.files={files:[]};
  this.verifiedSaved=false;
  this.verifiedFile="";
  this.lastFlight=null;
  this.failureCount=0;
  this.historyRefreshAt=0;
  this.pollTimer=null;
  this.lastRecorderMode=null;
  this.savingSince=0;
  this.bind();
 }
 bind(){
  $("prepareArm").addEventListener("click",()=>this.prepare("full"));
  $("quickFly").addEventListener("click",()=>this.prepare("quick"));
  $("flyAgain").addEventListener("click",()=>this.prepare("quick"));
  $("refreshHistory").addEventListener("click",()=>this.refreshFiles(true));
  $("advanced").addEventListener("click",()=>location.href="./index.html#history");
  $("retry").addEventListener("click",()=>this.poll(true));
 }
 async start(){
  await this.poll(true);
 }
 schedule(ms){
  clearTimeout(this.pollTimer);
  this.pollTimer=setTimeout(()=>this.poll(),ms);
 }
 async poll(force=false){
  try{
   const status=await request("/api/status");
   if(finiteNumber(status.version)!==17)throw Error("GREEN ION V17 firmware status required");
   this.status=status;
   this.connected=true;
   this.failureCount=0;
   if(this.lastRecorderMode===2&&status.recorder_mode===3){
    this.verifiedSaved=false;
    this.savingSince=Date.now();
   }
   this.lastRecorderMode=status.recorder_mode;
   this.render();
   if(!status.recording&&Date.now()-this.historyRefreshAt>(force?0:3500)){
    await this.refreshFiles(false);
   }
   if(status.recorder_mode===3&&!this.verifiedSaved){
    await this.verifyCompletedFlight();
   }
   this.schedule(850);
  }catch(error){
   this.connected=false;
   this.failureCount++;
   this.render(error);
   this.schedule(Math.min(5000,900+this.failureCount*650));
  }
 }
 async refreshFiles(showMessage=false){
  if(!this.connected||this.status?.recording)return;
  try{
   const payload=await request("/api/files");
   this.files=payload;
   this.historyRefreshAt=Date.now();
   this.renderHistory();
   this.render();
   if(showMessage)this.message("Flight history refreshed.",true);
  }catch(error){
   if(showMessage)this.message("History: "+error.message,false);
  }
 }
 async verifyCompletedFlight(){
  if(this.commandLock||this.status?.recording)return;
  const candidate=String(this.status?.log_file||"").replace(/^\//,"");
  if(!candidate)return;
  if(this.verifiedFile===candidate&&this.verifiedSaved)return;
  try{
   const payload=await request("/api/files");
   this.files=payload;
   this.historyRefreshAt=Date.now();
   const file=(payload.files||[]).find(item=>item.name===candidate);
   if(!file||file.recording_complete!==true){
    if(this.savingSince&&Date.now()-this.savingSince>12000)
     this.message("Save verification is taking longer than expected. Keep power connected and do not remove the SD card.",false);
    this.renderHistory();
    return;
   }
   const metadataText=await request("/api/metadata?name="+encodeURIComponent(candidate),{text:true});
   const metadata=parseFlightMetadata(metadataText);
   this.verifiedSaved=true;
   this.verifiedFile=candidate;
   this.lastFlight={name:candidate,size:file.size,durationMs:metadata.durationMs,recordingComplete:true};
   this.savingSince=0;
   this.renderHistory();
   this.render();
  }catch(error){
   this.message("Save verification: "+error.message,false);
  }
 }
 async prepare(mode){
  const now=Date.now();
  if(this.commandLock||now-this.lastCommandAt<1400)return;
  if(!this.connected){this.message("GREEN ION is reconnecting. Commands stay disabled until live firmware status returns.",false);return;}
  if(this.status?.recording||this.status?.flight_prep_active||this.status?.armed)return;
  this.commandLock=true;
  this.lastCommandAt=now;
  this.verifiedSaved=false;
  this.verifiedFile="";
  this.render();
  this.message(mode==="quick"?"Starting repeat-flight checks...":"Starting complete Prepare & Arm...");
  try{
   const result=await request("/api/preflight/start",{method:"POST",body:{mode}});
   this.message(result.message||"Firmware accepted flight preparation.",true);
  }catch(error){
   this.message(error.message,false);
  }finally{
   this.commandLock=false;
   await this.poll(true);
  }
 }
 message(text,ok=null){
  const el=$("message");
  el.textContent=text;
  el.classList.toggle("good",ok===true);
  el.classList.toggle("error",ok===false);
 }
 render(error=null){
  const d=this.status||{};
  const state=deriveQuickFlyState(d,{connected:this.connected,verifiedSaved:this.verifiedSaved});
  $("liveState").textContent=state;
  $("liveState").dataset.state=state;
  $("connection").textContent=this.connected?"LIVE · V17":"RECONNECTING";
  $("connection").classList.toggle("bad",!this.connected);
  $("retry").hidden=this.connected;

  const progress=Math.max(0,Math.min(100,finiteNumber(d.flight_prep_progress)||0));
  $("progressFill").style.width=progress+"%";
  $("progressText").textContent=d.flight_prep_active?(progress+"% · "+(d.flight_prep_phase||"TESTING")):(state==="ARMED"?"100% · ARMED":(state==="SAVED"?"100% · VERIFIED SAVE":"Ready for command"));
  $("phase").textContent=d.flight_prep_phase||"--";

  const battery=batteryAdvisory(d.battery_v);
  $("battery").textContent=finiteNumber(d.battery_v)===null?"--":Number(d.battery_v).toFixed(2)+" V";
  $("batteryNote").textContent=battery.text; setTone($("batteryNote"),battery.level);
  $("sd").textContent=d.sd_ok===true?(d.sd_integrity_verified?"VERIFIED":"DETECTED"):"NOT READY";
  $("sd").className=d.sd_ok===true?"ok":"bad";
  $("sensors").textContent=(d.barometer_ready&&d.imu_ok)?"BARO + IMU OK":"CHECK SENSORS";
  $("gps").textContent=d.gps_status==="live"?"LIVE · "+(d.satellites??0)+" sats":"OPTIONAL · "+String(d.gps_status||"missing").toUpperCase();
  $("lora").textContent=d.radio_ok?"OPTIONAL · READY":"OPTIONAL · UNAVAILABLE";

  const free=finiteNumber(d.sd_free_bytes)??(finiteNumber(this.files.total)!==null&&finiteNumber(this.files.used)!==null?Number(this.files.total)-Number(this.files.used):null);
  const total=finiteNumber(d.sd_total_bytes)??finiteNumber(this.files.total);
  $("storage").textContent=free===null?"--":formatBytes(free)+" free";
  const storage=storageAdvisory(free,total);
  $("storageNote").textContent=storage.text;setTone($("storageNote"),storage.level);

  const number=nextFlightNumber(this.files);
  $("flightCounter").textContent=Number.isFinite(number)&&number>0?"Flight "+number:"Firmware-managed";
  const nextSeq=finiteNumber(this.files?.next_sequence);
  $("flightFile").textContent=nextSeq===null?"Filename reserved by firmware":"V12D"+String(Math.floor(nextSeq)).padStart(3,"0")+".BIN";

  const warnings=optionalWarnings(d);
  $("warnings").innerHTML=warnings.length?warnings.map(x=>"<div>⚠ "+x+"</div>").join(""):"<div class=\"ok\">Optional GPS and LoRa status available.</div>";
  if(d.flight_prep_failure)$("errorDetail").textContent=d.flight_prep_failure;
  else if(error)$("errorDetail").textContent=error.message||String(error);
  else $("errorDetail").textContent="";

  const blocked=this.commandLock||!this.connected||!!d.recording||!!d.flight_prep_active||!!d.armed||d.recorder_mode===5||d.recorder_mode===6;
  $("prepareArm").disabled=blocked;
  $("quickFly").disabled=blocked;
  $("flyAgain").disabled=state!=="SAVED"||this.commandLock||!this.connected;
  $("flyAgain").hidden=state!=="SAVED";

  if(this.lastFlight){
   $("lastFlight").hidden=false;
   $("lastName").textContent=this.lastFlight.name;
   $("lastSize").textContent=formatBytes(this.lastFlight.size);
   $("lastDuration").textContent=humanDuration(this.lastFlight.durationMs);
   $("lastVerified").textContent=this.lastFlight.recordingComplete?"YES · BIN + completion metadata":"NO";
  }
 }
 renderHistory(){
  const host=$("history");
  const files=[...(this.files?.files||[])].sort((a,b)=>String(b.name).localeCompare(String(a.name))).slice(0,8);
  if(!files.length){host.innerHTML="<div class=\"muted\">No completed flight BIN files reported by the Feather SD card.</div>";return;}
  host.innerHTML="";
  for(const file of files){
   const row=document.createElement("div");row.className="history-row";
   const info=document.createElement("div");
   info.innerHTML="<strong>"+String(file.name).replace(/[<>&]/g,"")+"</strong><small>"+formatBytes(file.size)+" · "+(file.recording_complete?"VERIFIED COMPLETE":"CHECK RECOVERY")+"</small>";
   const actions=document.createElement("div");
   const download=document.createElement("a");download.className="mini";download.textContent="Download";download.href=apiUrl("/api/download?name="+encodeURIComponent(file.name));download.setAttribute("download","");
   const analyze=document.createElement("a");analyze.className="mini secondary";analyze.textContent="Replay / Compare / Analyze";analyze.href="./index.html#history";
   actions.append(download,analyze);row.append(info,actions);host.appendChild(row);
  }
 }
}

function boot(){
 if(!$("quickFlyApp"))return;
 const app=new QuickFlyApp();
 window.GREEN_ION_QUICK_FLY=app;
 app.start();
}
if(typeof document!=="undefined"){
 if(document.readyState==="loading")document.addEventListener("DOMContentLoaded",boot,{once:true});
 else boot();
}
