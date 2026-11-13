/* GREEN ION V16.1 simple diagnostics.
 * Keeps launch-day UI concise while preserving deeper evidence in a downloadable report. */
import {listLocalMetadata} from "./v14_store.mjs";

export const DIAGNOSTIC_SCHEMA="GREEN-ION-V16.3-DIAGNOSTICS-1";
const $=id=>document.getElementById(id);
const t=window.GREEN_ION_TRANSPORT;
let lastReport=null,running=null;

function cloudState(){
 try{return JSON.parse(localStorage.getItem("green-ion-v16-cloud-state")||"null");}
 catch{return null;}
}
function text(id,value){const n=$(id);if(n)n.textContent=value;}
function age(ms){
 if(!Number.isFinite(ms))return "unknown";
 return ms<1000?Math.max(0,Math.round(ms))+" ms":(ms/1000).toFixed(ms<10000?1:0)+" s";
}
function status(value){
 return String(value||"UNKNOWN").replaceAll("_"," ").toUpperCase();
}
function download(name,value){
 const url=URL.createObjectURL(new Blob([JSON.stringify(value,null,2)],{type:"application/json"}));
 const a=document.createElement("a");a.href=url;a.download=name;document.body.append(a);a.click();a.remove();
 setTimeout(()=>URL.revokeObjectURL(url),60000);
}
async function fetchLocalDiagnostics(){
 if(!t.localBridge)return null;
 const response=await t.nativeFetch("/api/app/diagnostics",{cache:"no-store"});
 if(!response.ok)throw Error("Windows diagnostics HTTP "+response.status);
 return response.json();
}
function browserSnapshot(){
 const wifi=t.state.lastWifiStatus||{};
 const radio=t.state.radio||{};
 const cloud=cloudState();
 return {
  source:t.source(),
  wifi:{
   fresh:t.wifiFresh(),
   age_ms:t.state.wifiAt?Date.now()-t.state.wifiAt:null,
   status_source:wifi.v13_source||null,
   connection_state:wifi.v16_connection_state||null,
   sd_ok:wifi.sd_ok??null,
   recording:wifi.recording??null,
   firmware_name:wifi.firmware_name||null,
   firmware_version:wifi.firmware_version||null,
   imu_ok:wifi.imu_ok??null,
   barometer_ok:wifi.barometer_ready??null,
   battery_v:wifi.battery_v??null,
   gps_ok:wifi.gps_ok??null,
   gps_status:wifi.gps_status||null,
   reset_reason:wifi.reset_reason||null,
   sd_recoveries:wifi.sd_recoveries??null,
   imu_recoveries:wifi.imu_recoveries??null,
   gps_recoveries:wifi.gps_recoveries??null,
   last_error:wifi.last_error||wifi.error||null
  },
  lora:{
   fresh:t.radioFresh(),
   age_ms:t.state.radioAt?Date.now()-t.state.radioAt:null,
   rssi_dbm:radio.rssi_dbm??null,
   snr_db:radio.snr_db??null,
   gps_status:radio.gps_status||null,
   sequence:radio.sequence??null,
   flight_state:radio.flight_state??null
  },
  gps:{
   position_available:!!t.lastPosition(),
   age_ms:t.positionAge()
  },
  shared_spi:{
   owner:wifi.shared_spi_owner??null,
   sd_sections:wifi.shared_spi_sd_sections??null,
   radio_sections:wifi.shared_spi_radio_sections??null,
   radio_deferred_busy:wifi.shared_spi_radio_deferred_busy??null,
   radio_deferred_deadline:wifi.shared_spi_radio_deferred_deadline??null,
   ownership_errors:wifi.shared_spi_ownership_errors??null,
   radio_irq_observed:wifi.radio_irq_observed??null,
   radio_irq_serviced:wifi.radio_irq_serviced??null,
   radio_irq_deferred:wifi.radio_irq_deferred??null,
   radio_tx_started:wifi.radio_tx_started??null,
   radio_tx_completed:wifi.radio_tx_completed??null,
   radio_tx_timeouts:wifi.radio_tx_timeouts??null,
   radio_recovery_attempts:wifi.radio_recovery_attempts??null,
   radio_recoveries:wifi.radio_recoveries??null,
   radio_init_failures:wifi.radio_init_failures??null,
   missed_sample_periods:wifi.missed_sample_periods??null,
   sensor_log_dropped:wifi.sensor_log_dropped??null,
   evidence_scope:"software scheduling counters only; not electrical contention measurement"
  },
  cloud:cloud?{
   status:cloud.status||null,
   pending_count:cloud.pending_count??null,
   last_success_at:cloud.last_success_at||null,
   failures:cloud.failures??null
  }:null,
  browser:{
   online:navigator.onLine,
   language:navigator.language,
   platform:navigator.userAgentData?.platform||navigator.platform||"unknown",
   standalone:!!window.matchMedia?.("(display-mode: standalone)")?.matches
  }
 };
}
export async function buildDiagnosticReport(){
 let local=null,localError="";
 try{local=await fetchLocalDiagnostics();}
 catch(error){localError=String(error.message||error);}
 let files=[];
 try{files=await listLocalMetadata();}
 catch{}
 const browser=browserSnapshot();
 const issues=[];
 if(!browser.wifi.fresh)issues.push("Feather Wi-Fi: no recent status. Keep the Windows app open, join GREEN-ION-V16-3, and let automatic reconnect continue.");
 if(!browser.lora.fresh)issues.push("LoRa: no fresh receive-only packet. Check Uno USB, antenna, radio power/wiring and coordinated frequency; SD recording is independent.");
 if(browser.wifi.fresh&&browser.wifi.sd_ok===false)issues.push("SD: Feather reports the card unavailable. Stop mission activity, inspect/reseat the card, and rerun SD/home verification.");
 if(!browser.gps.position_available)issues.push("GPS: no confirmed position received. Move outdoors with a clear sky and wait for a current fix.");
 if(Number(browser.shared_spi.ownership_errors)>0)
  issues.push("Shared SPI ownership error recorded. Stop bench testing and inspect the SD/LoRa scheduling evidence and physical perfboard before relying on the shared bus.");
 if(Number(browser.shared_spi.radio_tx_timeouts)>0)
  issues.push("LoRa transmissions timed out. SD recording remains primary; check the RFM9x antenna, reset/DIO0/CS wiring, power, and rerun Full Home Test.");
 if(Number(browser.shared_spi.missed_sample_periods)>30)
  issues.push("Recorder timing exceeded the Full Home Test missed-period tolerance. Review SD/radio workload and physical power before hardware use.");
 if(browser.wifi.imu_ok===false)issues.push("IMU: LSM6DSO32 is unavailable. Check STEMMA QT power/cable and rerun the home test.");
 if(browser.wifi.barometer_ok===false)issues.push("Barometer: BMP388 is unavailable. Check cable/power before recording.");
 if(Number.isFinite(Number(browser.wifi.battery_v))&&Number(browser.wifi.battery_v)<3.45)
  issues.push("Battery: measured voltage is low. Recharge/inspect the protected 1S LiPo; voltage alone does not predict endurance.");
 if(local?.receiver?.state==="disconnected"||local?.receiver?.state==="error")
  issues.push("Windows Uno receiver is "+local.receiver.state+".");
 if(localError)issues.push("Windows diagnostic endpoint unavailable: "+localError);
 return {
  schema:DIAGNOSTIC_SCHEMA,
  captured_at:new Date().toISOString(),
  app_context:t.localBridge?"windows-local":"browser-hosted-or-feather",
  browser,
  windows:local,
  local_archive:{file_count:files.length},
  issues,
  note:"Diagnostic evidence only. It does not certify RF range, sensor accuracy, SD integrity, battery safety or launch readiness."
 };
}
function renderFrom(report){
 const b=report.browser,w=report.windows;
 text("v161DiagApp",t.localBridge?
   ((w?.app?.version?"V"+w.app.version+" · ":"")+"WINDOWS LOCAL"):
   "BROWSER / PWA");
 text("v161DiagFeather",b.wifi.fresh?"LIVE · "+age(b.wifi.age_ms):
   status(b.wifi.connection_state||"not verified"));
 const receiver=w?.receiver;
 text("v161DiagUno",receiver?
   (receiver.port?(status(receiver.state)+" · "+receiver.port):
    status(receiver.state||"searching")):
   (b.lora.fresh?"PACKETS RECEIVED":"NOT AVAILABLE HERE"));
 text("v161DiagLora",b.lora.fresh?
   "LIVE · "+age(b.lora.age_ms)+" · "+(b.lora.rssi_dbm??"--")+" dBm":
   "NO FRESH PACKETS");
 text("v161DiagGps",b.gps.position_available?"FIX RECEIVED · "+age(b.gps.age_ms)+" old":"NO FIX RECEIVED");
 text("v161DiagSd",b.wifi.fresh?
   (b.wifi.sd_ok===true?"READY"+(b.wifi.sd_recoveries?" · recovered "+b.wifi.sd_recoveries+"×":""):b.wifi.sd_ok===false?"NOT READY":"UNKNOWN"):
   "REQUIRES FEATHER WI-FI");
 text("v162DiagFirmware",b.wifi.firmware_version?(b.wifi.firmware_name||"Feather")+" · "+b.wifi.firmware_version:"Not reported");
 text("v162DiagImu",b.wifi.fresh?(b.wifi.imu_ok===true?"READY"+(b.wifi.imu_recoveries?" · recovered "+b.wifi.imu_recoveries+"×":""):b.wifi.imu_ok===false?"NOT READY":"UNKNOWN"):"REQUIRES FEATHER WI-FI");
 text("v162DiagBaro",b.wifi.fresh?(b.wifi.barometer_ok===true?"READY":b.wifi.barometer_ok===false?"NOT READY":"UNKNOWN"):"REQUIRES FEATHER WI-FI");
 text("v162DiagBattery",Number.isFinite(Number(b.wifi.battery_v))?Number(b.wifi.battery_v).toFixed(3)+" V measured":"Unavailable");
 text("v161DiagCloud",b.cloud?status(b.cloud.status):"OPTIONAL / NOT CONFIGURED");
 text("v161DiagArchive",report.local_archive.file_count+" original file(s) on this device");
 text("v161DiagSummary",report.issues.length?
   report.issues.length+" item(s) need attention. Open details or download the report.":
   "No software/link issue detected right now. Physical hardware checks are separate.");
 const details=$("v161DiagIssues");
 if(details){
  details.replaceChildren();
  if(!report.issues.length)details.textContent="No current software/link issues.";
  else for(const issue of report.issues){const li=document.createElement("li");li.textContent=issue;details.append(li);}
 }
}
async function run(){
 if(running)return running;
 running=(async()=>{
  text("v161DiagSummary","Checking connections and local storage...");
  try{
   lastReport=await buildDiagnosticReport();
   renderFrom(lastReport);
  }catch(error){text("v161DiagSummary","Diagnostic check failed safely: "+error.message);}
  finally{running=null;}
 })();
 return running;
}
export function initV161Diagnostics(){
 const system=$("system");if(!system)return;
 if(!$("v161DiagnosticsCard")){
  const card=document.createElement("div");card.className="card";card.id="v161DiagnosticsCard";
  card.innerHTML='<h2>System Health</h2>'+
   '<p class="muted">Simple launch-day view. Detailed evidence stays behind this card and in the downloadable report.</p>'+
   '<div class="row"><span>App</span><strong id="v161DiagApp">Checking...</strong></div>'+
   '<div class="row"><span>Feather Wi-Fi</span><strong id="v161DiagFeather">Checking...</strong></div>'+
   '<div class="row"><span>Uno USB</span><strong id="v161DiagUno">Checking...</strong></div>'+
   '<div class="row"><span>Receive-only LoRa</span><strong id="v161DiagLora">Checking...</strong></div>'+
   '<div class="row"><span>GPS evidence</span><strong id="v161DiagGps">Checking...</strong></div>'+
   '<div class="row"><span>Feather firmware</span><strong id="v162DiagFirmware">Checking...</strong></div>'+
   '<div class="row"><span>SD card</span><strong id="v161DiagSd">Checking...</strong></div>'+
   '<div class="row"><span>IMU</span><strong id="v162DiagImu">Checking...</strong></div>'+
   '<div class="row"><span>Barometer</span><strong id="v162DiagBaro">Checking...</strong></div>'+
   '<div class="row"><span>Battery</span><strong id="v162DiagBattery">Checking...</strong></div>'+
   '<div class="row"><span>Cloud backup</span><strong id="v161DiagCloud">Checking...</strong></div>'+
   '<div class="row"><span>Local archive</span><strong id="v161DiagArchive">Checking...</strong></div>'+
   '<div class="actions"><button id="v161DiagRun">Run connection check</button>'+
   '<button id="v161DiagDownload" class="secondary">Download diagnostic report</button></div>'+
   '<p id="v161DiagSummary" class="muted" role="status">Checking...</p>'+
   '<details><summary>What needs attention?</summary><ul id="v161DiagIssues"></ul></details>';
  system.prepend(card);
 }
 $("v161DiagRun").onclick=run;
 $("v161DiagDownload").onclick=async()=>{
  if(!lastReport)lastReport=await buildDiagnosticReport();
  download("green-ion-v16-diagnostic-"+new Date().toISOString().replace(/[:.]/g,"-")+".json",lastReport);
 };
 window.addEventListener("green-ion-link",run);
 window.addEventListener("green-ion-v16-cloud-state",run);
 window.addEventListener("green-ion-history-change",run);
 setInterval(run,10000);
 run();
}
