/* Local-first advanced flight analysis and automatic postflight health reports.
 * Cloud backup saves originals; reports can always be regenerated locally. */
import {getLocalFile,listLocalMetadata} from "./v14_store.mjs";
const $=id=>document.getElementById(id);
let report=null,busy=false,timer=null;
function message(value){$("v15ReportStatus").textContent=value;}
function download(name,content){
 const url=URL.createObjectURL(new Blob([content],{type:"application/json"}));
 const a=document.createElement("a");a.href=url;a.download=name;document.body.append(a);a.click();a.remove();
 setTimeout(()=>URL.revokeObjectURL(url),60000);
}
export async function refreshAdvancedList(){
 const files=(await listLocalMetadata()).filter(x=>x.kind==="BIN")
  .sort((a,b)=>String(b.createdAt).localeCompare(String(a.createdAt)));
 const select=$("v15ReportFlight"),old=select.value;
 select.replaceChildren();
 const blank=document.createElement("option");blank.value="";blank.textContent="Choose completed BIN";select.append(blank);
 for(const f of files){
  const o=document.createElement("option");o.value=f.key;o.textContent=f.name;select.append(o);
 }
 if(files.some(f=>f.key===old))select.value=old;
 if(!select.value&&files.length)select.value=files[0].key;
 $("v15ReportRun").disabled=!files.length;
}
function display(r){
 report=r;const out=$("v15ReportSummary");out.replaceChildren();
 const enhanced=r.enhanced||{};
 const compare=enhanced.gpsPressureComparison||{};
 const events=Array.isArray(enhanced.events)?enhanced.events:[];
 const values=[
  ["Recording",r.name],["Data completeness",r.dataCompletenessScore+"/100 (not sensor accuracy)"],
  ["RKT4 pressure records",r.observed.validPressureSamples+"/"+r.observed.sampleCount],
  ["Recording duration",r.observed.durationS.toFixed(2)+" seconds"],
  ["Estimated apogee",r.estimates.apogeeM===null?"Unavailable":r.estimates.apogeeM.toFixed(2)+" m"],
  ["Estimated max ascent",r.estimates.maxAscentMps===null?"Unavailable":r.estimates.maxAscentMps.toFixed(2)+" m/s"],
  ["Estimated max descent",enhanced.maxDescentMps==null?"Unavailable":enhanced.maxDescentMps.toFixed(2)+" m/s"],
  ["Flight event confidence",enhanced.averageEventConfidence==null?"Insufficient evidence":Math.round(enhanced.averageEventConfidence)+"/100 · "+enhanced.eventConfidence?.label],
  ["Detected events",events.length?events.map(e=>e.name.replaceAll("_"," ")+" "+Math.round(e.confidence)+"%").join(" · "):"Not reliably detected"],
  ["Measured total acceleration",r.sensors?.maxMeasuredTotalG==null?"GIS1 unavailable":r.sensors.maxMeasuredTotalG.toFixed(2)+" g"],
  ["Measured GPS speed",r.sensors?.maxMeasuredGpsSpeedMps==null?"GIS1 unavailable":r.sensors.maxMeasuredGpsSpeedMps.toFixed(2)+" m/s"],
  ["Pressure vs GPS altitude",compare.status?compare.status+(compare.rmseM==null?"":" · RMSE "+compare.rmseM.toFixed(1)+" m"):"Not compared"],
  ["Last recorded battery",r.observed.endBatteryV==null?"Unavailable":r.observed.endBatteryV.toFixed(3)+" V"],
  ["Timestamp gaps",String(r.observed.timestampGaps)],
  ["RKT4 trailing bytes",String(r.observed.trailingBytes)],
  ["GIS1 CRC evidence",r.sensors?(r.sensors.crc32ValidatedPrefix?"Validated decoded records":"Stopped at corrupt record"):"No sensor companion"]
 ];
 for(const [name,value] of values){
  const row=document.createElement("div");row.className="row";
  const label=document.createElement("span"),data=document.createElement("strong");
  label.textContent=name;data.textContent=value;row.append(label,data);out.append(row);
 }
 $("v15ReportWarnings").textContent=r.warnings.slice(0,12).join(" · ")||"No file-integrity warnings";
 $("v15ReportExport").disabled=false;
 message("Postflight report calculated locally from original files. Critical summary is shown first; raw/high-rate evidence stays in the downloadable report. No internet required.");
}
export async function analyzeSelected({automatic=false}={}){
 if(busy)return;
 const key=$("v15ReportFlight").value;
 if(!key)return;
 busy=true;$("v15ReportRun").disabled=true;
 message("Analyzing immutable RKT4/GIS1 originals in a background worker...");
 try{
  const record=await getLocalFile(key);
  if(!record||record.kind!=="BIN")throw Error("Original RKT4 file missing");
  const files=await listLocalMetadata();
  const stem=record.name.replace(/\.BIN$/i,"");
  const companion=files.find(f=>f.kind==="SEN"&&f.name.replace(/\.SEN$/i,"")===stem);
  const sen=companion?await getLocalFile(companion.key):null;
  const r=await new Promise((resolve,reject)=>{
   const worker=new Worker("./v15_report_worker.mjs",{type:"module"});
   const timeout=setTimeout(()=>{worker.terminate();reject(Error("Analysis timed out"));},90000);
   worker.onerror=e=>{clearTimeout(timeout);worker.terminate();reject(Error(e.message||"Analysis worker failed"));};
   worker.onmessage=e=>{
    clearTimeout(timeout);worker.terminate();
    if(e.data.error)reject(Error(e.data.error));else resolve(e.data.report);
   };
   const binBytes=record.bytes.slice(0);
   const senBytes=sen?.bytes.slice(0)||null;
   const transfers=senBytes?[binBytes,senBytes]:[binBytes];
   worker.postMessage({id:1,binBytes,senBytes,name:record.name,checksum:record.checksum},transfers);
  });
  display(r);
  window.dispatchEvent(new CustomEvent("green-ion-postflight-report",{detail:{name:r.name,quality:r.dataCompletenessScore,report:r}}));
 }catch(e){message("Advanced analysis unavailable: "+e.message+(automatic?". Retry from History.":""));}
 finally{busy=false;$("v15ReportRun").disabled=false;}
}
export function initAdvanced(){
 $("v15ReportRun").onclick=()=>analyzeSelected();
 $("v15ReportExport").onclick=()=>{
  if(!report)return;
  download(report.name.replace(/\.BIN$/i,"")+"-health-report.json",JSON.stringify(report,null,2));
 };
 window.addEventListener("green-ion-flight-saved",event=>{
  const name=event.detail?.name||"";
  if(!/\.(BIN|SEN)$/i.test(name))return;
  // For completed flights, analyze the newly arrived BIN, not an older selected
  // flight. A companion SEN arriving moments later upgrades the same report.
  const stem=name.replace(/\.(BIN|SEN)$/i,"");
  clearTimeout(timer);
  timer=setTimeout(async()=>{
   await refreshAdvancedList();
   const choices=(await listLocalMetadata()).filter(x=>x.kind==="BIN");
   const match=choices.find(x=>x.name.replace(/\.BIN$/i,"")===stem);
   if(match)$("v15ReportFlight").value=match.key;
   await analyzeSelected({automatic:true});
  },2500);
 });
 window.addEventListener("green-ion-history-change",()=>refreshAdvancedList().catch(()=>{}));
 window.addEventListener("green-ion-link",()=>{
  const t=window.GREEN_ION_TRANSPORT;
  const radio=t.state.radio,phase=Number(radio?.flight_state);
  if(t.radioFresh()&&(phase===6||phase===7)){
   $("v15ReportLive").textContent="LoRa reports landed/recovery. This is preliminary telemetry only; full report follows SD retrieval.";
  }
 });
 refreshAdvancedList().catch(e=>message(e.message));
}
