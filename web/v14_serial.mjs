/* Receive-only USB LoRa receiver. No serial commands or rocket controls. */
const t=window.GREEN_ION_TRANSPORT;
let port=null,reader=null,stopping=false;
const el=id=>document.getElementById(id);
function message(text){if(el("v14SerialStatus"))el("v14SerialStatus").textContent=text;}
export function serialSupported(){return !!navigator.serial && location.protocol==="https:" || (!!navigator.serial && location.hostname==="localhost");}
export async function connectSerial(){
 if(!serialSupported()){message("USB serial requires Chrome or Edge on Windows. iPhone/iPad Safari does not support Web Serial.");return;}
 if(port)return;
 try{
  port=await navigator.serial.requestPort();
  await port.open({baudRate:115200,bufferSize:4096});
  stopping=false;
  el("v14SerialConnect").disabled=true;el("v14SerialDisconnect").disabled=false;
  message("Arduino Uno connected; waiting for LoRa packets (115200 baud).");
  let pending="";
  const decoder=new TextDecoder();
  while(port?.readable&&!stopping){
   reader=port.readable.getReader();
   try{
    while(!stopping){
      const {value,done}=await reader.read();
      if(done)break;
      pending+=decoder.decode(value,{stream:true});
      if(pending.length>131072){pending="";message("Receiver line too long; invalid data discarded.");}
      let sep;
      while((sep=pending.indexOf("\n"))>=0){
        const line=pending.slice(0,sep).trim();pending=pending.slice(sep+1);
        if(!line.startsWith("{")||line.length>8192)continue;
        try{
          const packet=JSON.parse(line);
          if(t.feedRadio(packet))message("Receiving "+packet.type+" · RSSI "+(packet.rssi_dbm??"--")+" dBm");
          else if(packet.type==="receiver_status")message("Uno connected · "+(packet.radio_ready?"radio ready":"radio not ready"));
        }catch{/* ignore bad serial lines; no controls transmitted */}
      }
    }
   }finally{reader?.releaseLock();reader=null;}
  }
 }catch(e){message("USB error: "+e.message);}
 finally{
  try{await port?.close();}catch{/* disconnected while closing */}
  port=null;reader=null;
  if(el("v14SerialConnect"))el("v14SerialConnect").disabled=false;
  if(el("v14SerialDisconnect"))el("v14SerialDisconnect").disabled=true;
  if(!stopping)message("Arduino Uno disconnected.");
 }
}
export async function disconnectSerial(){
 stopping=true;
 try{await reader?.cancel();}catch{/* reader may already be closed */}
 message("Disconnecting Arduino Uno...");
}
