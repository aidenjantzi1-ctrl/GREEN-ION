/* GREEN ION V16.3 shared-SPI evidence.
 * Presentation only: never changes recorder/radio state and never turns software
 * evidence into physical launch approval. */
const $=id=>document.getElementById(id);
const t=window.GREEN_ION_TRANSPORT;
const KEY="green-ion-v16-3-shared-spi-evidence";
let activeStress=null,lastEvidence=loadEvidence();

function loadEvidence(){
 try{
  const value=JSON.parse(localStorage.getItem(KEY)||"null");
  return value&&value.schema==="GREEN-ION-V16.3-SPI-EVIDENCE-1"?value:null;
 }catch{return null;}
}
function saveEvidence(value){
 lastEvidence=value;
 try{localStorage.setItem(KEY,JSON.stringify(value));}catch{}
}
function row(label,id){
 const node=document.createElement("div");node.className="row";
 node.innerHTML="<span>"+label+"</span><strong id='"+id+"'>NOT TESTED</strong>";
 return node;
}
function result(value){
 const v=String(value||"NOT_TESTED").replaceAll("_"," ").toUpperCase();
 if(v==="PASS"||v==="FAIL"||v==="RUNNING"||v==="INCONCLUSIVE")return v;
 return "NOT TESTED";
}
function radioEvidence(){return window.GREEN_ION_RADIO_EVIDENCE||{};}
function receiverPresent(){
 const m=radioEvidence(),state=String(m.receiverState||"").toLowerCase();
 return t.radioFresh()||["ready","connected"].includes(state);
}
function beginStress(w){
 if(activeStress||String(w.home_record_result||"").toUpperCase()!=="RUNNING"||w.home_radio_expected!==true)return;
 const m=radioEvidence();
 activeStress={
  startedAt:Date.now(),
  benchStart:Number(m.benchCount||0),
  packetsStart:Number(m.packets||0),
  target:Math.max(1,Number(w.home_radio_target)||8),
  receiverPresentAtStart:receiverPresent()
 };
}
function finishStress(w){
 if(!activeStress||String(w.home_record_result||"").toUpperCase()==="RUNNING")return;
 const m=radioEvidence(),benchDelta=Math.max(0,Number(m.benchCount||0)-activeStress.benchStart);
 const feather=result(w.home_shared_spi_result);
 let receiver="INCONCLUSIVE";
 if(benchDelta>=activeStress.target)receiver="PASS";
 else if(!activeStress.receiverPresentAtStart&&!receiverPresent())receiver="NOT TESTED";
 const evidence={
  schema:"GREEN-ION-V16.3-SPI-EVIDENCE-1",
  captured_at:new Date().toISOString(),
  feather_result:feather,
  receiver_result:receiver,
  receiver_bench_packets:benchDelta,
  receiver_target:activeStress.target,
  receiver_rssi_dbm:Number.isFinite(Number(m.rssi))?Number(m.rssi):null,
  receiver_snr_db:Number.isFinite(Number(m.snr))?Number(m.snr):null,
  sample_missed_periods:Number(w.home_record_missed)||0,
  telemetry_queue_dropped:Number(w.home_telemetry_dropped)||0,
  sensor_log_dropped:Number(w.home_sensor_log_dropped)||0,
  ownership_errors:Number(w.home_spi_ownership_errors)||0,
  radio_tx_timeouts:Number(w.home_radio_tx_timeouts)||0,
  note:"Feather result is software-observed shared-bus scheduling/recording evidence. Receiver PASS requires actual bench packets received by the connected Uno. RF range and electrical signal integrity are separate physical tests."
 };
 saveEvidence(evidence);activeStress=null;
}
function guide(w,evidence){
 if(!w||!String(w.firmware_version||"").startsWith("16.3"))
  return "Install GREEN ION Feather V16.3 before using the shared-SPI evidence check.";
 if(w.sd_ok===false)return "SD unavailable — verify the microSD card is inserted and the SD CS/wiring are correct.";
 if(w.radio_ok===false)return "LoRa transmitter unavailable — check the RFM9x wiring, antenna, reset/DIO0, and matching radio configuration. SD recording remains the priority.";
 if(String(w.home_record_result||"").toUpperCase()==="RUNNING")
  return "Running 50 Hz temporary SD recording while bounded LoRa packets are scheduled. Leave Feather power connected.";
 if(!evidence)return "Run Full Home Test with motors removed. Connect the Uno receiver to Windows and attach suitable antennas to both LoRa modules.";
 if(evidence.feather_result==="FAIL")return "Shared-bus software evidence failed — review missed samples, ownership errors and radio timeouts before hardware use.";
 if(evidence.receiver_result==="NOT TESTED")return "NOT TESTED — LoRa receiver not connected. Connect the Arduino Uno receiver to the Windows laptop and rerun Full Home Test.";
 if(evidence.receiver_result==="INCONCLUSIVE")return "Radio transmission ran, but the expected Uno bench packets were not confirmed. Check Uno USB, both antennas and matching LoRa settings.";
 return "Concurrent SD + LoRa software evidence passed. Real perfboard timing, RF range and power integrity still require the documented physical stress test.";
}
function renderTesting(){
 const host=$("v162TestEvidence");if(!host)return;
 if(!$("v163TestSpi"))host.append(row("Shared SD + LoRa SPI","v163TestSpi"),row("Uno packets during SPI stress","v163TestReceiver"));
 if(!$("v163SpiGuide")){
  const notice=document.createElement("div");notice.className="notice";notice.id="v163SpiGuide";
  host.after(notice);
 }
 const w=t.state.lastWifiStatus||{};
 beginStress(w);finishStress(w);
 const current=String(w.home_record_result||"").toUpperCase()==="RUNNING"&&w.home_radio_expected===true;
 const feather=current?"RUNNING":result(w.home_shared_spi_result)||lastEvidence?.feather_result||"NOT TESTED";
 const receiver=current?"RUNNING":lastEvidence?.receiver_result||"NOT TESTED";
 const target=Math.max(1,Number(w.home_radio_target)||lastEvidence?.receiver_target||8);
 const m=radioEvidence();
 $("v163TestSpi").textContent=feather+(current?" · 50 Hz SD + bounded LoRa":"");
 let receiverText=receiver;
 if(receiver==="RUNNING")receiverText+=" · waiting for "+target+" actual packets";
 else if(lastEvidence){
  receiverText+=" · "+lastEvidence.receiver_bench_packets+"/"+lastEvidence.receiver_target+" bench packets";
  if(lastEvidence.receiver_rssi_dbm!==null)receiverText+=" · "+lastEvidence.receiver_rssi_dbm.toFixed(1)+" dBm";
  if(lastEvidence.receiver_snr_db!==null)receiverText+=" · SNR "+lastEvidence.receiver_snr_db.toFixed(1)+" dB";
 }else if(receiverPresent())receiverText+=" · receiver connected; rerun Full Home Test";
 $("v163TestReceiver").textContent=receiverText;
 $("v163SpiGuide").textContent=guide(w,lastEvidence);
}
function renderAdvanced(){
 const card=$("v161DiagnosticsCard");if(!card)return;
 if(!$("v163SpiDetails")){
  const details=document.createElement("details");details.id="v163SpiDetails";
  details.innerHTML='<summary>Shared SD + LoRa SPI details</summary>'+
   '<div class="row"><span>Bus owner</span><strong id="v163SpiOwner">--</strong></div>'+
   '<div class="row"><span>SD / radio sections</span><strong id="v163SpiSections">--</strong></div>'+
   '<div class="row"><span>Radio deferrals</span><strong id="v163SpiDeferrals">--</strong></div>'+
   '<div class="row"><span>Ownership errors</span><strong id="v163SpiErrors">--</strong></div>'+
   '<div class="row"><span>Radio TX start / complete / timeout</span><strong id="v163SpiTx">--</strong></div>'+
   '<div class="row"><span>Recording deadlines missed</span><strong id="v163SpiMissed">--</strong></div>'+
   '<p class="muted">These are software scheduling counters, not a measurement of electrical bus contention. Physical perfboard validation is separate.</p>';
  card.append(details);
 }
 const w=t.state.lastWifiStatus||{};
 const set=(id,value)=>{const n=$(id);if(n)n.textContent=value;};
 const owner=["IDLE","SD","RADIO"][Number(w.shared_spi_owner)]||"UNKNOWN";
 set("v163SpiOwner",owner);
 set("v163SpiSections",(w.shared_spi_sd_sections??"--")+" / "+(w.shared_spi_radio_sections??"--"));
 set("v163SpiDeferrals",(w.shared_spi_radio_deferred_busy??"--")+" busy · "+(w.shared_spi_radio_deferred_deadline??"--")+" sample-deadline");
 set("v163SpiErrors",String(w.shared_spi_ownership_errors??"--"));
 set("v163SpiTx",(w.radio_tx_started??"--")+" / "+(w.radio_tx_completed??"--")+" / "+(w.radio_tx_timeouts??"--"));
 set("v163SpiMissed",String(w.missed_sample_periods??"--"));
}
function render(){renderTesting();renderAdvanced();}
export function initV163SharedSpiEvidence(){
 render();
 window.addEventListener("green-ion-link",render);
 window.addEventListener("green-ion-v16-preflight",render);
 setInterval(render,1500);
}
