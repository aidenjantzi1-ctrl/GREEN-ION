/* V15 Windows loopback archive. IndexedDB originals are mirrored to a
 * content-addressed folder; no internet and no radio control is required. */
import {getLocalFile,listLocalMetadata,saveLocalFile,digest} from "./v14_store.mjs";
const t=window.GREEN_ION_TRANSPORT;
const $=id=>document.getElementById(id);
let mirroring=new Set(),reconciling=false,backlogOffset=0;
function note(msg){if($("v15LocalStatus"))$("v15LocalStatus").textContent=msg;}
export async function mirrorFile(key){
 if(!t.localBridge||mirroring.has(key))return;
 mirroring.add(key);
 try{
  const file=await getLocalFile(key);
  if(!file)return;
  const res=await fetch("/api/local/save?name="+encodeURIComponent(file.name),{
   method:"POST",cache:"no-store",
   headers:{"Content-Type":"application/octet-stream","X-Green-Ion-SHA256":file.checksum},
   body:file.bytes
  });
  if(!res.ok)throw Error("Windows archive HTTP "+res.status);
  const answer=await res.json();
  if(!answer.success||answer.sha256!==file.checksum)throw Error("Windows archive did not confirm matching checksum");
  note("Computer archive verified: "+file.name+" · "+file.checksum.slice(0,12)+"…");
 }catch(e){note("Original remains in browser; Windows mirror deferred: "+e.message);}
 finally{mirroring.delete(key);}
}
export async function reconcileLocal({limit=6}={}){
 if(!t.localBridge||reconciling)return;
 reconciling=true;
 try{
  const response=await fetch("/api/local/files",{cache:"no-store"});
  if(!response.ok)throw Error("Cannot list PC archive");
  const data=await response.json();
  if(!Array.isArray(data.files))throw Error("Invalid PC archive listing");
  const local=new Set((await listLocalMetadata()).map(f=>f.checksum));
  let count=0;
  for(const item of data.files){
   if(local.has(item.sha256))continue;
   const path="/api/local/file?sha256="+encodeURIComponent(item.sha256)+"&kind="+encodeURIComponent(item.kind.toLowerCase());
   const result=await fetch(path,{cache:"no-store"});
   if(!result.ok)throw Error("Cannot retrieve "+item.name+" from PC archive");
   const bytes=await result.arrayBuffer();
   if(bytes.byteLength!==item.size||await digest(bytes)!==item.sha256)
    throw Error("PC archive checksum mismatch: "+item.name);
   await saveLocalFile({name:item.name,bytes,origin:"Windows local archive"});
   local.add(item.sha256);count++;
   if(count>=limit)break; // Avoid a large blocking restore on startup.
  }
  note(count?"Recovered "+count+" verified original file(s) from Windows disk. Cloud sync will retry when internet returns.":
   "PC archive and browser originals are reconciled.");
 }catch(e){note("PC archive sync deferred: "+e.message);}
 finally{reconciling=false;}
}
async function mirrorBacklog(){
 if(!t.localBridge)return;
 const files=await listLocalMetadata();
 if(backlogOffset>=files.length)return;
 for(const item of files.slice(backlogOffset,backlogOffset+4))await mirrorFile(item.key);
 backlogOffset+=4;
}
export function initLocalArchive(){
 if(!t.localBridge){
  note("Browser storage active. For automatic Windows disk copies, launch OPEN_GROUND_STATION.cmd.");
  return;
 }
 window.addEventListener("green-ion-flight-saved",e=>mirrorFile(e.detail?.key).catch(()=>{}));
 $("v15LocalRestore").onclick=()=>reconcileLocal({limit:50});
 reconcileLocal().catch(()=>{});
 setTimeout(()=>mirrorBacklog().catch(()=>{}),2500);
 setInterval(()=>mirrorBacklog().catch(()=>{}),12000);
 setInterval(()=>reconcileLocal().catch(()=>{}),30000);
}
