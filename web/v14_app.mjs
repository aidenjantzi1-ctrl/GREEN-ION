import {connectSerial,disconnectSerial,serialSupported} from "./v14_serial.mjs";
import {saveLocalFile,listLocalMetadata,getLocalFile,hasLocalName,downloadFile,exportArchive,restoreArchive,importLegacyHistory} from "./v14_store.mjs";
import {initCloud,backupNow} from "./v14_cloud.mjs";
import {initUpdates,checkUpdates} from "./v14_updates.mjs";
import {initHistoryAnalysis,refreshHistoryAnalysis} from "./v14_analysis.mjs";
import {initSensorHistory,refreshSensorHistory} from "./v14_sensor_history.mjs";
import {APP_VERSION} from "./v14_utils.mjs";
import {initRecovery} from "./v15_recovery.mjs";
import {initAdvanced} from "./v15_advanced.mjs";
import {initLocalArchive} from "./v15_local.mjs";
import {initBattery} from "./v15_battery.mjs";
import {initLink} from "./v15_link.mjs";
import {initHelp} from "./v15_help.mjs";
import {initPreferences} from "./v14_preferences.mjs";
import {initSensorLab} from "./v14_sensor_lab.mjs";
import {requiresFeather,initTabs} from "./v14_ui.mjs";
const $=id=>document.getElementById(id),t=window.GREEN_ION_TRANSPORT;
let importing=false,lastAutoImport=0,lastRadioAt=0,radioPackets=[];
function text(id,value){if($(id))$(id).textContent=value;}
function note(value){text("v14ConnectionNotice",value);text("v14ConnectionNoticeDetail",value);}
function formatAge(ms){return ms==null?"--":ms<1000?Math.round(ms)+" ms":(ms/1000).toFixed(1)+" s";}
function mode(){
 const wifi=t.wifiFresh();
 if(wifiWasConnected&&!wifi){importSeen.clear();lastAutoImport=0;}
 if(!wifiWasConnected&&wifi){importSeen.clear();lastAutoImport=0;}
 wifiWasConnected=wifi;
 const source=t.source(),packet=t.state.radio;
 for(const id of ["v14WifiStatus","v14WifiStatusDetail"])text(id,"Feather Wi-Fi: "+(wifi?"LIVE":"NOT VERIFIED"));
 for(const id of ["v14LoraStatus","v14LoraStatusDetail"])text(id,"Uno/LoRa: "+(t.radioFresh()?"RECEIVING":"NO LIVE PACKETS"));
 for(const id of ["v14SourceStatus","v14SourceStatusDetail"])text(id,"Active source: "+(source==="wifi"?"FEATHER WI-FI":source==="lora"?"RECEIVE-ONLY LORA":"OFFLINE"));
 text("v14PacketStatus","Radio age: "+formatAge(t.state.radioAt?Date.now()-t.state.radioAt:null)+" · RSSI: "+(packet?.rssi_dbm??"--")+" dBm");
 const pos=t.lastPosition(),age=t.positionAge();
 text("v14GpsStatus",pos?"Last valid GPS: "+pos[0].toFixed(6)+", "+pos[1].toFixed(6)+" · "+formatAge(age)+" old":"Last valid GPS: not received");
 const message=source==="wifi"?"Feather status verified. SD, guided tests, calibration and bench OTA require Feather Wi-Fi.":
  source==="lora"?"Radio telemetry only. SD recording state is unknown; all Feather controls and downloads are blocked.":
  "No live Feather or LoRa readings. The installed website and locally saved flight history remain available.";
 note(message);
 document.body.dataset.greenIonSource=source;
 if(source!=="wifi"&&source!=="lora"){
  const connection=$("connection");
  if(connection){connection.textContent="● OFFLINE";connection.style.color="#ff91a0";}
 }else if(source==="lora"){
  const connection=$("connection");
  if(connection){connection.textContent="● LORA TRACKING";connection.style.color="#ffdd88";}
 }
}
function guardLiveControls(event){
 const button=event.target.closest?.("button");
 if(!button || t.wifiFresh() || !requiresFeather(button.id))return;
 // Only block commands requiring hardware. Offline graphs, saved-flight
 // analysis, CSV/PDF export, theme and tab navigation remain usable.
 event.preventDefault();event.stopImmediatePropagation();
 note("Feather Wi-Fi connection required for this hardware command. Saved flights and graphs remain available offline.");
}
async function renderHistory(){
 await Promise.all([refreshHistoryAnalysis(),refreshSensorHistory()]);
 const target=$("v14HistoryList");target.replaceChildren();
 try{
  const items=(await listLocalMetadata()).sort((a,b)=>String(b.createdAt).localeCompare(String(a.createdAt)));
  text("v14LocalCount",items.length+" original file(s) saved on this device.");
  if(!items.length){target.textContent="No local files yet. Import BIN/SEN/TXT or connect to Feather Wi-Fi after a flight.";return;}
  for(const f of items){
   const row=document.createElement("div");row.className="row";
   const label=document.createElement("span");label.textContent=f.name+" · "+(f.size/1024).toFixed(1)+" KB · "+f.origin;
   const button=document.createElement("button");button.className="secondary";button.textContent="Download";
   button.onclick=async()=>{try{const original=await getLocalFile(f.key);
    if(!original)throw Error("Original file missing");
    downloadFile(original);
   }catch(error){text("v14HistoryNotice","Download failed: "+error.message);}};
   row.append(label,button);target.append(row);
  }
 }catch(e){target.textContent="History unavailable: "+e.message;}
}
async function importFiles(list){
 let count=0,duplicates=0;
 for(const file of Array.from(list||[])){
  try{const result=await saveLocalFile({name:file.name,bytes:await file.arrayBuffer()});if(result.duplicate)duplicates++;else count++;}
  catch(e){text("v14HistoryNotice","Could not import "+file.name+": "+e.message);return;}
 }
 text("v14HistoryNotice","Imported "+count+" original recordings; "+duplicates+" duplicate(s) skipped. Data remains local until signed-in cloud backup completes.");
 await renderHistory();
}
function downloadText(filename,value){
 const url=URL.createObjectURL(new Blob([value],{type:"application/json"}));
 const anchor=document.createElement("a");anchor.download=filename;anchor.href=url;
 document.body.append(anchor);anchor.click();anchor.remove();
 setTimeout(()=>URL.revokeObjectURL(url),60000);
}
async function exportReceiver(){
 if(!radioPackets.length){text("v14SerialStatus","No LoRa packets received to export.");return;}
 const names=["received_at","type","sequence","uptime_ms","altitude_m","pressure_pa","battery_v","acceleration_g",
 "rotation_x_dps","rotation_y_dps","rotation_z_dps","speed_mps","latitude","longitude","gps_status",
 "rssi_dbm","flight_state","satellites"];
 const quote=v=>'"'+String(v??"").replace(/"/g,'""')+'"';
 const csv=names.map(quote).join(",")+"\n"+radioPackets.map(p=>names.map(n=>quote(p[n])).join(",")).join("\n");
 const url=URL.createObjectURL(new Blob([csv],{type:"text/csv"}));
 const anchor=document.createElement("a");anchor.href=url;anchor.download="green-ion-lora-receiver.csv";
 document.body.append(anchor);anchor.click();anchor.remove();
 setTimeout(()=>URL.revokeObjectURL(url),60000);
}
const importSeen=new Set();
let wifiWasConnected=false;
async function autoSaveOneCompletedFlight(){
 if(importing || !t.wifiFresh() || Date.now()-lastAutoImport<8000)return;
 lastAutoImport=Date.now();importing=true;
 try{
  const statusResponse=await fetch("/api/status",{cache:"no-store"});
  if(!statusResponse.ok)return;
  const status=await statusResponse.json();
  if(status.v13_source==="lora"||status.v13_source==="demo"||
     status.recording||status.maintenance_mode||status.home_test_active||status.update_in_progress)return;
  const response=await fetch("/api/files",{cache:"no-store"});
  if(!response.ok)return;
  const result=await response.json(),files=Array.isArray(result.files)?result.files:[];
  let saved=0;
  for(const file of files){
   if(!t.wifiFresh())break;
   if(!/\.BIN$/i.test(file.name||"")||Number(file.size)<32||Number(file.size)>52428800)continue;
   const signature=file.name+":"+file.size;
   if(importSeen.has(signature))continue;
   const url="/api/download?name="+encodeURIComponent(file.name);
   const bin=await fetch(url,{cache:"no-store"});
   if(!bin.ok)continue;
   const data=await bin.arrayBuffer();
   if(data.byteLength!==Number(file.size)){
    text("v14HistoryNotice","Partial Feather transfer rejected: "+file.name+" (expected "+file.size+" bytes, received "+data.byteLength+")");
    continue;
   }
   const result=await saveLocalFile({name:file.name,bytes:data,origin:"Feather SD completed flight"});
   importSeen.add(signature);
   for(const [route,ext] of [["sensors","SEN"],["metadata","TXT"]]){
    const sibling=file.name.replace(/\.BIN$/i,"."+ext);
    try{
     const raw=await fetch("/api/"+route+"?name="+encodeURIComponent(file.name),{cache:"no-store"});
     if(!raw.ok)continue;
     const bytes=await raw.arrayBuffer();
     if(bytes.byteLength>0&&bytes.byteLength<=52428800)
      await saveLocalFile({name:sibling,bytes,origin:"Feather SD companion"});
    }catch{/* older firmware may not have this companion; keep BIN safe */}
   }
   if(!result.duplicate)saved++;
   if(saved>=4)break; // Short bounded batches; recorder remains higher priority.
  }
  if(saved){
   text("v14HistoryNotice","Saved "+saved+" finished flight(s) with available companions locally. PC disk mirror and optional cloud sync run independently.");
   await renderHistory();
  }
 }catch(e){text("v14HistoryNotice","Automatic SD import deferred: "+e.message);}
 finally{importing=false;}
}
async function downloadApiAnchor(event){
 const anchor=event.target.closest?.('a[href^="/api/"]');
 if(!anchor)return;
 event.preventDefault();
 if(!t.wifiFresh()){note("Feather Wi-Fi required for SD download.");return;}
 try{
  const response=await fetch(anchor.getAttribute("href"),{cache:"no-store"});
  if(!response.ok)throw Error("HTTP "+response.status);
  const filename=new URL(anchor.href).searchParams.get("name")||"green-ion-download.bin";
  downloadFile({name:filename,bytes:await response.arrayBuffer()});
 }catch(e){note("SD download failed: "+e.message);}
}
function renderBrowserInfo(){
 text("v14ClientInfo",/iPhone|iPad|iPod/i.test(navigator.userAgent)?
  "Phone/iPad mode: Feather Wi-Fi and offline history. Uno Web Serial is not supported by Safari.":
  serialSupported()?"Desktop mode: connect the Uno with USB using Chrome/Edge; Feather Wi-Fi remains the priority.":
  "Web Serial is unavailable in this browser. Feather Wi-Fi and saved files still work.");
}
async function init(){
 document.querySelector("header small").textContent="Flight logger · V15 local-first operations";
 text("v14AppBuild","Website V15 local-first beta 1 · Feather firmware channel "+APP_VERSION+" (check actual installed build in System)");
 renderBrowserInfo();
 initPreferences();
 initSensorLab(t);
 initTabs(document.querySelector("nav.tabs"));
 $("v14SerialConnect").onclick=connectSerial;
 $("v14SerialDisconnect").onclick=disconnectSerial;
 $("v14SerialDisconnect").disabled=true;
 $("v14ReceiverExport").onclick=exportReceiver;
 $("v14FileImport").onchange=e=>importFiles(e.target.files);
 $("v14LocalExport").onclick=async()=>{try{downloadText("green-ion-v14-local-backup.json",await exportArchive());}
  catch(e){text("v14HistoryNotice","Export failed: "+e.message);}};
 $("v14LocalRestore").onchange=async e=>{
  const file=e.target.files?.[0];if(!file)return;
  try{const result=await restoreArchive(await file.text());text("v14HistoryNotice","Restored "+result.restored+" files; "+result.duplicates+" duplicates.");await renderHistory();}
  catch(err){text("v14HistoryNotice","Restore rejected: "+err.message);}
 };
 $("v14RefreshLocal").onclick=renderHistory;
 $("v14OpenFeather").href="http://192.168.4.1/";
 document.addEventListener("click",guardLiveControls,true);
 document.addEventListener("click",downloadApiAnchor,true);
 window.addEventListener("green-ion-flight-saved",renderHistory);
 window.addEventListener("green-ion-history-change",renderHistory);
 window.addEventListener("green-ion-link",()=>{
  const packet=t.state.radio;
  if(packet&&packet.receivedAt!==lastRadioAt){
   lastRadioAt=packet.receivedAt;
   radioPackets.push({...packet,received_at:new Date(packet.receivedAt).toISOString()});
   if(radioPackets.length>20000)radioPackets.shift();
  }
  mode();
 });
 initHistoryAnalysis();initSensorHistory();initRecovery();initLink();initBattery();initLocalArchive();initAdvanced();initHelp();initCloud();initUpdates();
 try{const n=await importLegacyHistory();if(n)text("v14HistoryNotice","Imported "+n+" older V11-V13 recordings into V14 storage.");}
 catch{/* historical DB may not exist */}
 await renderHistory();mode();
 setInterval(()=>{mode();autoSaveOneCompletedFlight();},2200);
 if("serviceWorker" in navigator && (location.protocol==="https:"||location.hostname==="localhost")){
  navigator.serviceWorker.register("./service-worker.js").catch(e=>text("v14OfflineNotice","Offline installation unavailable: "+e.message));
 }
 if(navigator.storage?.persist)navigator.storage.persist().catch(()=>{});
}
init().catch(e=>{const n=document.getElementById("v14ConnectionNotice");if(n)n.textContent="V14 startup error: "+e.message;});
