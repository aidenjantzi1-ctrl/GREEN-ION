/* GREEN ION V16 hardware-validation evidence ledger.
 * This module records evidence; it never grants launch permission or performs
 * energetic/flight-control actions. Physical tests default to NOT_TESTED. */
export const VALIDATION_SCHEMA="GREEN-ION-V16-VALIDATION-1";
export const ALLOWED_STATUS=Object.freeze(["NOT_TESTED","PASS","FAIL","INCONCLUSIVE"]);
export const REQUIREMENTS=Object.freeze([
 {id:"windows_reconnect_soak",label:"Windows + Feather reconnect soak",scope:"bench",required:true,
  method:"Run the Windows app through repeated Feather Wi-Fi loss/rejoin and long transfers; record duration, dropouts and automatic recovery."},
 {id:"sd_recording_loss",label:"SD recording through network loss",scope:"bench",required:true,
  method:"Record on real hardware, remove browser/network access, then verify the original BIN/SEN/TXT and sample timing after reconnect."},
 {id:"imu_barometer",label:"IMU + barometer measured bench test",scope:"bench",required:true,
  method:"Run the guided hardware test stationary and with controlled movement; retain measured output, not just source/compile evidence."},
 {id:"gps_outdoor",label:"Outdoor GPS current-fix behavior",scope:"field",required:true,
  method:"Obtain a current outdoor fix, then verify live→stale behavior after signal loss and last-known recovery coordinates."},
 {id:"lora_receiver_range",label:"Receiver-side LoRa integrity/range",scope:"field",required:true,
  method:"Use the Uno receiver with antennas fitted; record valid packets, sequence gaps, loss, RSSI/SNR and recovery after signal loss at coordinated frequency."},
 {id:"shared_spi",label:"SD + LoRa shared-SPI sustained operation",scope:"bench",required:true,
  method:"Sustain real SD recording while receiving LoRa evidence; verify recorder timing remains primary and files stay readable."},
 {id:"power_lipo",label:"LiPo power / voltage-divider / brownout behavior",scope:"bench",required:true,
  method:"Measure battery voltage against a meter and observe power stability/current behavior in realistic states."},
 {id:"ota_rollback",label:"Bench OTA + rollback + interruption recovery",scope:"bench",required:true,
  method:"On stable bench power only, verify expected image/version/checksum, reboot, rollback and controlled interruption recovery; rerun home test afterward."},
 {id:"mobile_offline",label:"Phone/tablet offline history and recovery workflow",scope:"bench",required:false,
  method:"Verify installed PWA/history analysis works offline and document local-Feather fallback behavior on the actual phone/tablet."},
 {id:"mechanical_rf",label:"Mechanical mounting / retention / antenna installation",scope:"field",required:true,
  method:"Inspect real payload mounting, retention, antenna routing, wiring strain relief and fit/CG with the actual rocket."},
 {id:"actual_flight",label:"Supervised real flight evidence",scope:"flight",required:true,
  method:"After bench review, document one supervised flight and recovery; preserve originals and compare live/recovery evidence with recorded files."}
]);
const STORAGE_KEY="green-ion-v16-validation-evidence";
const $=id=>document.getElementById(id);

function cleanText(value,max=1000){
 return String(value??"").replace(/[\u0000-\u001f]/g," ").trim().slice(0,max);
}
export function newLedger(now=Date.now()){
 return {
  schema:VALIDATION_SCHEMA,
  created_at:new Date(now).toISOString(),
  updated_at:new Date(now).toISOString(),
  records:Object.fromEntries(REQUIREMENTS.map(r=>[r.id,{
   status:"NOT_TESTED",note:"",evidence_ref:"",measured_at:null,updated_at:null
  }])),
  diagnostic_snapshots:[]
 };
}
export function normalizeLedger(raw,now=Date.now()){
 const base=newLedger(now);
 if(!raw||raw.schema!==VALIDATION_SCHEMA)return base;
 for(const req of REQUIREMENTS){
  const record=raw.records?.[req.id]||{};
  base.records[req.id]={
   status:ALLOWED_STATUS.includes(record.status)?record.status:"NOT_TESTED",
   note:cleanText(record.note),
   evidence_ref:cleanText(record.evidence_ref,300),
   measured_at:record.measured_at||null,
   updated_at:record.updated_at||null
  };
 }
 if(Array.isArray(raw.diagnostic_snapshots))
  base.diagnostic_snapshots=raw.diagnostic_snapshots.slice(-20);
 base.created_at=raw.created_at||base.created_at;
 base.updated_at=raw.updated_at||base.updated_at;
 return base;
}
export function updateEvidence(ledger,id,patch,now=Date.now()){
 const next=normalizeLedger(ledger,now);
 if(!REQUIREMENTS.some(r=>r.id===id))return next;
 const status=ALLOWED_STATUS.includes(patch?.status)?patch.status:next.records[id].status;
 next.records[id]={
  status,
  note:cleanText(patch?.note??next.records[id].note),
  evidence_ref:cleanText(patch?.evidence_ref??next.records[id].evidence_ref,300),
  measured_at:status==="NOT_TESTED"?null:(patch?.measured_at||new Date(now).toISOString()),
  updated_at:new Date(now).toISOString()
 };
 next.updated_at=new Date(now).toISOString();
 return next;
}
export function validationSummary(ledger){
 const value=normalizeLedger(ledger);
 const required=REQUIREMENTS.filter(r=>r.required);
 const counts=Object.fromEntries(ALLOWED_STATUS.map(s=>[s,0]));
 for(const req of REQUIREMENTS)counts[value.records[req.id].status]++;
 const failed=required.filter(r=>value.records[r.id].status==="FAIL");
 const incomplete=required.filter(r=>value.records[r.id].status!=="PASS");
 let state;
 if(failed.length)state="REVIEW_FAILED_EVIDENCE";
 else if(!incomplete.length)state="READY_FOR_HUMAN_REVIEW_NOT_FLIGHT_AUTHORIZATION";
 else state="PHYSICAL_EVIDENCE_PENDING";
 return {
  state,counts,
  required_total:required.length,
  required_passed:required.length-incomplete.length,
  incomplete_required:incomplete.map(r=>r.id),
  failed_required:failed.map(r=>r.id),
  flight_authorized:false,
  note:"GREEN ION validation evidence is advisory. It never authorizes a launch."
 };
}
export function addDiagnosticSnapshot(ledger,snapshot,now=Date.now()){
 const next=normalizeLedger(ledger,now);
 next.diagnostic_snapshots.push({
  captured_at:new Date(now).toISOString(),
  source:"local software diagnostics",
  data:snapshot
 });
 next.diagnostic_snapshots=next.diagnostic_snapshots.slice(-20);
 next.updated_at=new Date(now).toISOString();
 return next;
}
async function captureDiagnostics(){
 const endpoints=["/api/app/health","/api/ground-link","/api/v16/telemetry"];
 const out={};
 for(const path of endpoints){
  try{
   const r=await fetch(path,{cache:"no-store"});
   out[path]=r.ok?await r.json():{http_status:r.status};
  }catch(e){out[path]={unavailable:String(e.message||e)};}
 }
 return out;
}
function downloadJson(name,value){
 const url=URL.createObjectURL(new Blob([JSON.stringify(value,null,2)],{type:"application/json"}));
 const a=document.createElement("a");a.href=url;a.download=name;document.body.append(a);a.click();a.remove();
 setTimeout(()=>URL.revokeObjectURL(url),60000);
}
function load(){
 try{return normalizeLedger(JSON.parse(localStorage.getItem(STORAGE_KEY)||"null"));}catch{return newLedger();}
}
function save(ledger){
 try{localStorage.setItem(STORAGE_KEY,JSON.stringify(ledger));}catch{}
}
export function initV16Validation(){
 if(typeof document==="undefined")return;
 const testing=$("testing");if(!testing)return;
 let ledger=load();
 const card=document.createElement("div");card.className="card";card.id="v16ValidationCard";
 card.innerHTML='<h2>V16 hardware evidence · not flight authorization</h2>'+
  '<p class="muted">Software/CI cannot pass physical tests. Record actual measured evidence here; every item starts NOT TESTED.</p>'+
  '<div class="row"><span>Evidence state</span><strong id="v16ValidationState">PHYSICAL EVIDENCE PENDING</strong></div>'+
  '<div id="v16ValidationList"></div>'+
  '<div class="actions"><button id="v16CaptureDiagnostics" class="secondary">Capture current diagnostics</button>'+
  '<button id="v16ExportValidation" class="secondary">Export evidence JSON</button></div>'+
  '<p id="v16ValidationNotice" class="muted">Capturing diagnostics does not automatically pass a physical test.</p>';
 testing.prepend(card);
 function render(){
  const summary=validationSummary(ledger);
  $("v16ValidationState").textContent=summary.state.replaceAll("_"," ")+" · "+summary.required_passed+"/"+summary.required_total+" required PASS";
  const list=$("v16ValidationList");list.replaceChildren();
  for(const req of REQUIREMENTS){
   const row=document.createElement("details");row.className="v16-section-group";
   const head=document.createElement("summary");head.textContent=req.label+" · "+ledger.records[req.id].status+(req.required?" · REQUIRED":" · OPTIONAL");
   const p=document.createElement("p");p.className="muted";p.textContent=req.method;
   const select=document.createElement("select");
   for(const status of ALLOWED_STATUS){const o=document.createElement("option");o.value=status;o.textContent=status.replace("_"," ");select.append(o);}
   select.value=ledger.records[req.id].status;
   const note=document.createElement("textarea");note.rows=2;note.placeholder="Measured result / setup / limitation";note.value=ledger.records[req.id].note;
   const ref=document.createElement("input");ref.placeholder="Evidence file/report/reference";ref.value=ledger.records[req.id].evidence_ref;
   const button=document.createElement("button");button.className="secondary";button.textContent="Save evidence";
   button.onclick=()=>{
    ledger=updateEvidence(ledger,req.id,{status:select.value,note:note.value,evidence_ref:ref.value});
    save(ledger);render();
   };
   row.append(head,p,select,note,ref,button);list.append(row);
  }
 }
 $("v16CaptureDiagnostics").onclick=async()=>{
  const button=$("v16CaptureDiagnostics");button.disabled=true;
  $("v16ValidationNotice").textContent="Capturing local diagnostics...";
  try{
   ledger=addDiagnosticSnapshot(ledger,await captureDiagnostics());save(ledger);
   $("v16ValidationNotice").textContent="Diagnostics captured. No physical test status was changed.";
  }catch(e){$("v16ValidationNotice").textContent="Diagnostics capture failed safely: "+e.message;}
  finally{button.disabled=false;}
 };
 $("v16ExportValidation").onclick=()=>downloadJson("green-ion-v16-validation-evidence.json",{
  ...ledger,summary:validationSummary(ledger),requirements:REQUIREMENTS
 });
 render();
}
