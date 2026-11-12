/* GREEN ION V16 data/package integrity layer.
 * Local originals remain authoritative. This module groups immutable BIN/SEN/TXT
 * siblings, verifies stored bytes, and exposes cloud status without making cloud
 * availability part of mission recording or analysis. */
import {listLocalMetadata,getLocalFile,digest} from "./v14_store.mjs";

export const DATA_SCHEMA="GREEN-ION-V16-DATA-1";
const KINDS=["BIN","SEN","TXT"];
const $=id=>document.getElementById(id);

export function flightBase(name){
 return String(name||"").replace(/.(BIN|SEN|TXT)$/i,"");
}
export function buildFlightPackages(files){
 const groups=new Map();
 for(const item of files||[]){
  const kind=String(item.kind||"").toUpperCase();
  if(!KINDS.includes(kind))continue;
  const base=flightBase(item.name);
  const key=base.toLowerCase();
  let group=groups.get(key);
  if(!group){
   group={schema:DATA_SCHEMA,flight_id:base,files:{},complete:false,status:"PARTIAL",total_bytes:0};
   groups.set(key,group);
  }
  const candidate={
   name:item.name,kind,sha256:item.checksum,size:Number(item.size)||0,
   origin:item.origin||"unknown",created_at:item.createdAt||null,key:item.key
  };
  // Never silently replace one sibling with another. Keep deterministic first
  // record and surface a collision for review.
  if(group.files[kind]&&group.files[kind].sha256!==candidate.sha256){
   group.status="COLLISION";
   group.collision=true;
   continue;
  }
  group.files[kind]=candidate;
 }
 const out=[...groups.values()];
 for(const group of out){
  group.total_bytes=Object.values(group.files).reduce((sum,f)=>sum+f.size,0);
  group.complete=KINDS.every(k=>!!group.files[k]);
  if(group.status!=="COLLISION")
   group.status=group.complete?"COMPLETE":group.files.BIN?"RECORDED_PARTIAL":"COMPANION_ONLY";
  group.missing=KINDS.filter(k=>!group.files[k]);
 }
 return out.sort((a,b)=>a.flight_id.localeCompare(b.flight_id,undefined,{numeric:true}));
}
export async function verifyLocalOriginals({limit=500}={}){
 const metadata=await listLocalMetadata();
 const files=metadata.slice(0,limit);
 const results=[];
 for(const meta of files){
  const file=await getLocalFile(meta.key);
  if(!file){results.push({name:meta.name,status:"MISSING",expected:meta.checksum,actual:null});continue;}
  const actual=await digest(file.bytes);
  results.push({name:meta.name,status:actual===meta.checksum?"VERIFIED":"MISMATCH",expected:meta.checksum,actual});
 }
 return {
  schema:DATA_SCHEMA,
  verified_at:new Date().toISOString(),
  checked:results.length,
  verified:results.filter(x=>x.status==="VERIFIED").length,
  problems:results.filter(x=>x.status!=="VERIFIED"),
  results
 };
}
export async function buildDataManifest(){
 const files=await listLocalMetadata();
 const packages=buildFlightPackages(files);
 return {
  schema:DATA_SCHEMA,
  generated_at:new Date().toISOString(),
  authoritative_store:"local immutable originals",
  packages,
  file_count:files.length,
  package_count:packages.length,
  complete_packages:packages.filter(x=>x.complete).length,
  partial_packages:packages.filter(x=>!x.complete).length,
  limitations:"A complete BIN/SEN/TXT package proves local file presence and checksums, not sensor accuracy, RF range, launch safety or successful hardware validation."
 };
}
function downloadJson(name,value){
 const url=URL.createObjectURL(new Blob([JSON.stringify(value,null,2)],{type:"application/json"}));
 const a=document.createElement("a");a.href=url;a.download=name;document.body.append(a);a.click();a.remove();
 setTimeout(()=>URL.revokeObjectURL(url),60000);
}
function cloudState(){
 try{return JSON.parse(localStorage.getItem("green-ion-v16-cloud-state")||"null");}catch{return null;}
}
async function paint(){
 const node=$("v16DataSummary");if(!node)return;
 try{
  const manifest=await buildDataManifest(),cloud=cloudState();
  node.textContent=manifest.package_count+" flight package(s) · "+manifest.complete_packages+
   " complete · "+manifest.partial_packages+" partial · "+
   (cloud?.status?("cloud "+String(cloud.status).toLowerCase()):"cloud status unavailable");
 }catch(e){node.textContent="Data summary unavailable: "+e.message;}
}
export function initV16Data(){
 const history=$("history");if(!history)return;
 if(!$("v16DataCard")){
  const card=document.createElement("div");card.className="card";card.id="v16DataCard";
  card.innerHTML='<h2>V16 data integrity · local first</h2>'+
   '<p class="muted">BIN/SEN/TXT originals stay immutable. Cloud is optional backup, never the live source of truth.</p>'+
   '<div class="row"><span>Local packages</span><strong id="v16DataSummary">Checking...</strong></div>'+
   '<div class="actions"><button id="v16VerifyData">Verify local checksums</button>'+
   '<button id="v16ExportManifest" class="secondary">Export package manifest</button></div>'+
   '<p id="v16DataNotice" class="muted">Verification rereads stored originals without modifying them.</p>';
  history.prepend(card);
 }
 $("v16VerifyData").onclick=async()=>{
  const button=$("v16VerifyData");button.disabled=true;
  $("v16DataNotice").textContent="Verifying local originals...";
  try{
   const report=await verifyLocalOriginals();
   $("v16DataNotice").textContent=report.problems.length?
    report.problems.length+" problem(s) found. Export the manifest and review before relying on the archive.":
    "Verified "+report.verified+" original file(s) by SHA-256.";
  }catch(e){$("v16DataNotice").textContent="Verification failed safely: "+e.message;}
  finally{button.disabled=false;}
 };
 $("v16ExportManifest").onclick=async()=>downloadJson("green-ion-v16-data-manifest.json",await buildDataManifest());
 window.addEventListener("green-ion-flight-saved",paint);
 window.addEventListener("green-ion-history-change",paint);
 window.addEventListener("green-ion-v16-cloud-state",paint);
 paint();
}
