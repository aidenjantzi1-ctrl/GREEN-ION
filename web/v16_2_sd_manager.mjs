/* GREEN ION V16.2 SD manager safety layer.
 * Destructive actions affect Feather SD only; browser/Windows/cloud originals stay untouched. */
import {listLocalMetadata} from "./v14_store.mjs";
import {digest} from "./v14_store.mjs";

const $=id=>document.getElementById(id);
const t=window.GREEN_ION_TRANSPORT;
let snapshot=new Map(),observer=null;

function cloudState(){try{return JSON.parse(localStorage.getItem("green-ion-v16-cloud-state")||"null");}catch{return null;}}
async function pcFiles(){
 if(!t?.localBridge)return [];
 try{const r=await t.nativeFetch("/api/local/files",{cache:"no-store"});if(!r.ok)return [];return (await r.json()).files||[];}catch{return [];}
}
async function sdFiles(){
 const r=await fetch("/api/files",{cache:"no-store"});const body=await r.json().catch(()=>({}));
 if(!r.ok||!body.success)throw Error(body.message||"Cannot read Feather SD directory");
 return body.files||[];
}
async function evidenceFor(name){
 const [sd,local,pc]=await Promise.all([sdFiles(),listLocalMetadata(),pcFiles()]);
 const item=sd.find(x=>String(x.name).toUpperCase()===String(name).toUpperCase());
 if(!item)throw Error("Flight is no longer present on the Feather SD card");
 const browser=local.find(x=>String(x.name).toUpperCase()===String(name).toUpperCase()&&x.kind==="BIN");
 const checksum=String(browser?.checksum||"").toLowerCase();
 const browserVerified=!!browser&&browser.size===item.size&&/^[a-f0-9]{64}$/.test(checksum);
 const pcVerified=browserVerified&&pc.some(x=>String(x.sha256||"").toLowerCase()===checksum);
 const cloud=cloudState(),cloudStatus=browserVerified?(cloud?.per_file?.[checksum]?.status||"UNKNOWN"):"UNKNOWN";
 return {item,browser,browserVerified,pcVerified,cloudStatus,
  verifiedBackup:browserVerified||pcVerified||cloudStatus==="SYNCED"};
}
function statusText(ev){
 return [
  "File: "+ev.item.name+" · "+bytes(ev.item.size),
  "Recording: "+(ev.item.recording_complete?"COMPLETE":ev.item.recovery_state||"NOT VERIFIED"),
  "Browser copy: "+(ev.browserVerified?"CHECKSUM VERIFIED":"NOT VERIFIED"),
  "Windows archive: "+(ev.pcVerified?"VERIFIED":"NOT VERIFIED"),
  "Cloud backup: "+ev.cloudStatus,
  "Deletion verification: Feather will recheck the SD directory after removal"
 ].join("\n");
}
function bytes(n){n=Number(n)||0;return n>=1048576?(n/1048576).toFixed(2)+" MB":n>=1024?(n/1024).toFixed(1)+" KB":n+" bytes";}
function ensureDialog(){
 if($("v162DeleteDialog"))return;
 const d=document.createElement("dialog");d.id="v162DeleteDialog";d.className="v162-dialog";
 d.innerHTML='<form method="dialog"><h2>Delete completed flight from Feather SD</h2>'+
  '<pre id="v162DeleteEvidence" class="v162-evidence"></pre>'+
  '<p id="v162DeleteWarning" class="notice"></p>'+
  '<label>Type the exact filename to confirm<input id="v162DeleteConfirm" autocomplete="off" spellcheck="false"></label>'+
  '<div class="actions"><button value="cancel" class="secondary">Cancel</button><button id="v162DeleteCommit" value="default" class="danger" type="button">Delete from Feather SD</button></div>'+
  '<p id="v162DeleteStatus" role="status" class="muted"></p></form>';
 document.body.append(d);
}
async function deleteFlight(name){
 ensureDialog();const d=$("v162DeleteDialog"),evidence=$("v162DeleteEvidence"),warning=$("v162DeleteWarning"),status=$("v162DeleteStatus");
 status.textContent="Checking completion and backup evidence...";
 try{
  const ev=await evidenceFor(name);evidence.textContent=statusText(ev);
  if(!ev.item.recording_complete){
   warning.textContent="Deletion blocked: this flight does not have clean completion evidence. Keep interrupted/recovery files for review.";
   $("v162DeleteCommit").disabled=true;
  }else{
   warning.textContent=ev.verifiedBackup?
    "At least one checksum-backed copy is present outside the Feather. Deleting here does not delete browser, Windows, or cloud copies.":
    "WARNING: no checksum-verified backup was found outside the Feather. Download and verify a copy before cleanup if you want recovery redundancy.";
   $("v162DeleteCommit").disabled=false;
  }
  $("v162DeleteConfirm").value="";status.textContent="";
  if(typeof d.showModal==="function")d.showModal();
  else{
   const typed=prompt(evidence.textContent+"\n\n"+warning.textContent+"\n\nType "+name+" exactly to delete:");
   if(typed===name&&ev.item.recording_complete)await commitDelete(name);
  }
  $("v162DeleteCommit").onclick=async()=>{
   if($("v162DeleteConfirm").value!==name){status.textContent="Filename confirmation does not match "+name+".";return;}
   $("v162DeleteCommit").disabled=true;status.textContent="Deleting companions first, BIN last, then verifying SD...";
   try{await commitDelete(name);d.close();}catch(error){status.textContent="Delete failed safely: "+error.message;$("v162DeleteCommit").disabled=false;}
  };
 }catch(error){evidence.textContent="";warning.textContent="Cannot verify deletion safety.";status.textContent=error.message;if(typeof d.showModal==="function")d.showModal();}
}
async function commitDelete(name){
 const r=await fetch("/api/delete?name="+encodeURIComponent(name)+"&confirm="+encodeURIComponent(name),{method:"POST",cache:"no-store"});
 const body=await r.json().catch(()=>({}));if(!r.ok||!body.success||body.verified!==true)throw Error(body.message||"Feather did not verify deletion");
 const after=await sdFiles();if(after.some(x=>x.name===name))throw Error("SD directory still contains "+name);
 const feedback=$("sdDeleteFeedback");if(feedback)feedback.textContent="Deleted "+name+" from Feather SD only · "+body.deleted_files+" file(s) removed · directory rechecked.";
 document.getElementById("refresh")?.click();
 window.dispatchEvent(new CustomEvent("green-ion-v16-sd-change",{detail:{name,action:"deleted"}}));
}
async function downloadOne(url,name,expected=null){
 const r=await fetch(url,{cache:"no-store"});if(!r.ok){if(r.status===404)return {name,status:"MISSING"};throw Error(name+" HTTP "+r.status);}
 const buffer=await r.arrayBuffer(),sha=await digest(buffer);
 if(expected&&expected.toLowerCase()!==sha.toLowerCase())throw Error(name+" checksum differs from verified browser original");
 const a=document.createElement("a"),blobUrl=URL.createObjectURL(new Blob([buffer]));
 a.href=blobUrl;a.download=name;document.body.append(a);a.click();a.remove();setTimeout(()=>URL.revokeObjectURL(blobUrl),60000);
 return {name,status:"VERIFIED_TRANSFER",sha256:sha,size:buffer.byteLength};
}
async function downloadPackage(name){
 const feedback=$("sdDeleteFeedback");if(feedback)feedback.textContent="Downloading and verifying "+name+" package...";
 try{
  const local=await listLocalMetadata(),base=name.replace(/\.BIN$/i,""),lookup=new Map(local.map(x=>[String(x.name).toUpperCase(),x]));
  const specs=[
   ["/api/download?name=",name],
   ["/api/sensors?name=",base+".SEN"],
   ["/api/metadata?name=",base+".TXT"]
  ],result=[];
  for(const [prefix,file] of specs){const meta=lookup.get(file.toUpperCase());result.push(await downloadOne(prefix+encodeURIComponent(name),file,meta?.checksum||null));}
  if(feedback)feedback.textContent=result.map(x=>x.name+" "+x.status).join(" · ")+" · downloaded bytes were SHA-256 checked before saving.";
 }catch(error){if(feedback)feedback.textContent="Package download failed verification: "+error.message;}
}
async function decorate(){
 const list=$("fileList");if(!list)return;
 let sd=[];try{sd=await sdFiles();}catch{return;}snapshot=new Map(sd.map(x=>[x.name,x]));
 const [local,pc]=await Promise.all([listLocalMetadata(),pcFiles()]),cloud=cloudState();
 for(const row of list.querySelectorAll(".file")){
  const match=row.textContent.match(/V12D\d{3}\.BIN/i);if(!match)continue;const name=match[0].toUpperCase(),info=snapshot.get(name);if(!info)continue;
  const meta=local.find(x=>String(x.name).toUpperCase()===name&&x.kind==="BIN"),sha=String(meta?.checksum||"").toLowerCase();
  const browser=!!meta&&meta.size===info.size&&/^[a-f0-9]{64}$/.test(sha),windows=browser&&pc.some(x=>String(x.sha256||"").toLowerCase()===sha),cloudStatus=browser?(cloud?.per_file?.[sha]?.status||"UNKNOWN"):"UNKNOWN";
  let evidence=row.querySelector(".v162-sd-evidence");
  if(!evidence){evidence=document.createElement("small");evidence.className="v162-sd-evidence";row.insertBefore(evidence,row.lastElementChild);}
  evidence.textContent=(info.recording_complete?"Complete":"Review recovery")+" · Browser "+(browser?"✓":"—")+" · PC "+(windows?"✓":"—")+" · Cloud "+(cloudStatus==="SYNCED"?"✓":cloudStatus);
  evidence.title="Backup evidence is advisory; deletion still requires exact filename confirmation.";
  const actions=row.lastElementChild;if(actions&&!actions.querySelector(".v162-package")){
   const b=document.createElement("button");b.className="secondary v162-package";b.textContent="Verified package";b.onclick=e=>{e.stopPropagation();downloadPackage(name);};actions.insertBefore(b,actions.querySelector(".danger"));
  }
 }
}
export function initV162SdManager(){
 ensureDialog();const list=$("fileList");if(!list)return;
 list.addEventListener("click",event=>{
  const button=event.target.closest("button");if(!button||button.textContent.trim()!=="Delete from SD")return;
  event.preventDefault();event.stopImmediatePropagation();const row=button.closest(".file"),name=row?.textContent.match(/V12D\d{3}\.BIN/i)?.[0];if(name)deleteFlight(name.toUpperCase());
 },true);
 observer=new MutationObserver(()=>decorate().catch(()=>{}));observer.observe(list,{childList:true,subtree:true});
 for(const event of ["green-ion-history-change","green-ion-v16-cloud-state","green-ion-v16-transfer"])window.addEventListener(event,()=>decorate().catch(()=>{}));
 decorate().catch(()=>{});
}
