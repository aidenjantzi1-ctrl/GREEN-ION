/* Independent receive-only radio metrics and Windows loopback Uno bridge.
 * LoRa never authorizes commands or proves that SD has recorded a flight. */
const t=window.GREEN_ION_TRANSPORT,$=id=>document.getElementById(id);
const metrics={benchCount:0,packets:0,missing:0,duplicates:0,outOfOrder:0,
 lastSeq:null,lastUptime:null,lastAt:0,rssi:null,snr:null,receiverState:"unknown"};
window.GREEN_ION_RADIO_EVIDENCE=metrics;
let lastRadioAt=0,lastBridgeKey="",polling=false,bridgeLink=null;
function record(){
 const packet=t.state.radio;
 if(!packet||packet.receivedAt===lastRadioAt)return;
 lastRadioAt=packet.receivedAt;
 metrics.lastAt=packet.receivedAt;
 metrics.packets++;
 if(packet.type==="radio_bench")metrics.benchCount++;
 if(Number.isFinite(Number(packet.rssi_dbm)))metrics.rssi=Number(packet.rssi_dbm);
 if(Number.isFinite(Number(packet.snr_db)))metrics.snr=Number(packet.snr_db);
 const seq=Number(packet.sequence),uptime=Number(packet.uptime_ms);
 if(Number.isSafeInteger(seq)&&seq>=0&&Number.isSafeInteger(uptime)&&uptime>=0){
  if(metrics.lastSeq!==null){
   if(uptime+1000<metrics.lastUptime&&seq<metrics.lastSeq)metrics.lastSeq=null;
   else{
    const delta=(seq-metrics.lastSeq)>>>0;
    if(delta===0)metrics.duplicates++;
    else if(delta>1&&delta<1000000)metrics.missing+=delta-1;
    else if(delta>=1000000)metrics.outOfOrder++;
   }
  }
  metrics.lastSeq=seq;metrics.lastUptime=uptime;
 }
 paint();
}
async function pollBridge(){
 if(!t.localBridge||polling)return;
 polling=true;
 try{
  const [telemetryResponse,linkResponse]=await Promise.all([
   t.nativeFetch("/api/telemetry",{cache:"no-store"}),
   t.nativeFetch("/api/ground-link",{cache:"no-store"})
  ]);
  if(linkResponse.ok)bridgeLink=await linkResponse.json();
  if(telemetryResponse.ok){
   const data=await telemetryResponse.json();
   if(typeof data.bench_packets==="number")metrics.benchCount=data.bench_packets;
   if(typeof data.packets_received==="number")metrics.packets=data.packets_received;
   if(typeof data.packets_missed_estimated==="number")metrics.missing=data.packets_missed_estimated;
   if(typeof data.packets_repeated==="number")metrics.duplicates=data.packets_repeated;
   if(typeof data.packets_out_of_order==="number")metrics.outOfOrder=data.packets_out_of_order;
   if(typeof data.rssi_average_dbm==="number")metrics.rssi=data.rssi_average_dbm;
   metrics.receiverState=data.receiver_state||"unknown";
   if(data.type==="telemetry"&&data.link_status==="live"&&Number(data.packet_age_ms)<3000){
    const key=String(data.sequence)+":"+String(data.received_utc);
    if(key!==lastBridgeKey){lastBridgeKey=key;t.feedRadio(data);}
   }
  }
  // Current counter values come from actual receiver-side serial observations.
  paint();
 }catch{/* cached last-known link state stays visible; do not invent packets */}
 finally{polling=false;}
}
function paint(){
 const wifi=bridgeLink?bridgeLink.wifi?.status==="live":t.wifiFresh();
 const radio=t.radioFresh(),total=metrics.packets+metrics.missing;
 const pct=total?100*metrics.missing/total:0;
 const id=(name,value)=>{const node=$(name);if(node)node.textContent=value;};
 id("v15LinkConcurrent",wifi&&radio?"Wi-Fi + LoRa receiving simultaneously":wifi?"Wi-Fi only (LoRa unverified)":radio?"LoRa only (Wi-Fi out of range)":"Neither live");
 if(t.localBridge&&bridgeLink?.wifi){
  const w=bridgeLink.wifi,state=String(w.connection_state||w.status||"unknown").toUpperCase();
  const age=w.age_ms==null?"no successful response yet":(Math.max(0,Number(w.age_ms))/1000).toFixed(1)+"s since last successful response";
  const active=w.active_operation?" · "+w.active_operation+" active":"";
  const failures=Number(w.consecutive_failures||0);
  id("v15LinkWifi",state+" · "+age+active+(failures?" · "+failures+" failed poll"+(failures===1?"":"s"):""));
 }else{
  id("v15LinkWifi",wifi?"CONNECTED":"NO LIVE WI-FI");
 }
 id("v15LinkCounts",metrics.packets+" received · "+metrics.missing+" inferred missing · "+metrics.duplicates+" duplicates");
 id("v15LinkLoss",total?pct.toFixed(1)+"% estimated (not measured airtime)":"No receiver sequence evidence");
 id("v15LinkSignal",metrics.rssi===null?"No RSSI measurement":metrics.rssi.toFixed(1)+" dBm"+(metrics.snr===null?"":" · SNR "+metrics.snr.toFixed(1)+" dB"));
 id("v15LinkLast",metrics.lastAt?Math.max(0,Math.floor((Date.now()-metrics.lastAt)/1000))+" seconds ago":"No received packet");
 id("v15LinkReceiver",t.localBridge?"Local Uno bridge: "+metrics.receiverState:"Web Serial: connect Uno USB in Chrome/Edge");
}
export function initLink(){
 window.addEventListener("green-ion-link",record);
 setInterval(()=>{record();pollBridge();paint();},1100);
 pollBridge().catch(()=>{});
 paint();
}
