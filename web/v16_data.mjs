/* GREEN ION V16 local-first flight data library.
 * Immutable local originals are authoritative. Cloud and Windows disk mirrors
 * are additional verified copies, never prerequisites for recording/analysis. */
import {listLocalMetadata,getLocalFile,digest} from "./v14_store.mjs";

export const DATA_SCHEMA="GREEN-ION-V16-DATA-1";
export const TRANSFER_SCHEMA="GREEN-ION-V16-TRANSFER-1";
const KINDS=Object.freeze(["BIN","SEN","TXT"]);
const TRANSFER_KEY="green-ion-v16-transfer-journal";
const LIBRARY_META_KEY="green-ion-v16.1-library-meta";
const MAX_JOURNAL=120;
const $=id=>document.getElementById(id);

export function flightBase(name){
 return String(name||"").replace(/\.(BIN|SEN|TXT)$/i,"");
}
function cleanChecksum(value){
 const text=String(value||"").toLowerCase();
 return /^[a-f0-9]{64}$/.test(text)?text:null;
}
function cleanKind(value){
 const kind=String(value||"").toUpperCase();
 return KINDS.includes(kind)?kind:null;
}
function compareFlightId(a,b){
 return String(a).localeCompare(String(b),undefined,{numeric:true,sensitivity:"base"});
}
function copyStatus(status,checksum){
 if(!status||!checksum)return "UNKNOWN";
 const item=status.per_file?.[checksum];
 return item?.status||"UNKNOWN";
}

/** One library entry per base flight name. Never silently merges conflicts. */
export function buildFlightPackages(files,{cloudState=null,pcChecksums=null}={}){
 const groups=new Map();
 for(const item of files||[]){
  const kind=cleanKind(item.kind);
  const checksum=cleanChecksum(item.checksum);
  if(!kind||!checksum)continue;
  const base=flightBase(item.name);
  if(!base)continue;
  const key=base.toLowerCase();
  let group=groups.get(key);
  if(!group){
   group={
    schema:DATA_SCHEMA,flight_id:base,files:{},conflicts:[],
    has_bin:false,full_companions:false,status:"COMPANION_ONLY",
    total_bytes:0,missing:[],local_verified:"NOT_CHECKED"
   };
   groups.set(key,group);
  }
  const candidate={
   name:String(item.name),kind,sha256:checksum,size:Math.max(0,Number(item.size)||0),
   origin:String(item.origin||"unknown"),created_at:item.createdAt||null,key:item.key||null,
   cloud:copyStatus(cloudState,checksum),
   windows:pcChecksums?.has?.(checksum)?"VERIFIED":"UNKNOWN"
  };
  if(group.files[kind]&&group.files[kind].sha256!==candidate.sha256){
   group.conflicts.push({kind,kept:group.files[kind],conflicting:candidate});
   group.status="COLLISION";
   continue;
  }
  group.files[kind]=candidate;
 }
 const out=[...groups.values()];
 for(const group of out){
  group.total_bytes=Object.values(group.files).reduce((sum,f)=>sum+f.size,0);
  group.has_bin=!!group.files.BIN;
  group.full_companions=KINDS.every(kind=>!!group.files[kind]);
  group.missing=KINDS.filter(kind=>!group.files[kind]);
  if(group.conflicts.length)group.status="COLLISION";
  else if(group.full_companions)group.status="FULL";
  else if(group.has_bin)group.status="RECORDED";
  else group.status="COMPANION_ONLY";
  group.cloud_status=Object.values(group.files).length&&Object.values(group.files).every(f=>f.cloud==="SYNCED")
   ?"SYNCED":Object.values(group.files).some(f=>["PENDING","UPLOADING","RETRYING","FAILED"].includes(f.cloud))
   ?"PENDING":"LOCAL";
  group.windows_status=Object.values(group.files).length&&Object.values(group.files).every(f=>f.windows==="VERIFIED")
   ?"VERIFIED":"UNKNOWN";
 }
 return out.sort((a,b)=>compareFlightId(b.flight_id,a.flight_id));
}

export async function verifyLocalOriginals({limit=1000}={}){
 const metadata=(await listLocalMetadata()).slice(0,Math.max(0,limit));
 const results=[];
 for(const meta of metadata){
  const file=await getLocalFile(meta.key);
  if(!file){
   results.push({name:meta.name,key:meta.key,status:"MISSING",expected:meta.checksum,actual:null});
   continue;
  }
  const actual=await digest(file.bytes);
  results.push({
   name:meta.name,key:meta.key,
   status:actual===meta.checksum?"VERIFIED":"MISMATCH",
   expected:meta.checksum,actual
  });
 }
 return {
  schema:DATA_SCHEMA,verified_at:new Date().toISOString(),checked:results.length,
  verified:results.filter(x=>x.status==="VERIFIED").length,
  problems:results.filter(x=>x.status!=="VERIFIED"),results
 };
}

export async function buildDataManifest({cloudState=null,pcChecksums=null}={}){
 const files=await listLocalMetadata();
 const packages=buildFlightPackages(files,{cloudState,pcChecksums});
 return {
  schema:DATA_SCHEMA,generated_at:new Date().toISOString(),
  authoritative_store:"local immutable originals",
  file_count:files.length,package_count:packages.length,
  recorded_packages:packages.filter(x=>x.has_bin).length,
  full_packages:packages.filter(x=>x.full_companions).length,
  collisions:packages.filter(x=>x.status==="COLLISION").length,
  packages,
  limitations:"Checksums prove stored-byte integrity only; they do not prove sensor accuracy, RF range, launch safety, GPS quality, battery safety or flight validity."
 };
}

function safeParse(value,fallback){
 try{return JSON.parse(value);}catch{return fallback;}
}
export function loadLibraryMeta(storage=globalThis.localStorage){
 let data=null;
 try{data=safeParse(storage?.getItem?.(LIBRARY_META_KEY)||"null",null);}catch{}
 if(!data||typeof data!=="object"||Array.isArray(data))return {};
 return data;
}
export function saveLibraryMeta(flightId,patch,storage=globalThis.localStorage){
 const all=loadLibraryMeta(storage),key=String(flightId||"").toLowerCase();
 if(!key)return null;
 const prior=all[key]||{};
 const tags=Array.isArray(patch?.tags)?patch.tags:
  String(patch?.tags??prior.tags??"").split(",").map(x=>x.trim()).filter(Boolean);
 all[key]={
  favorite:patch?.favorite===undefined?!!prior.favorite:!!patch.favorite,
  tags:[...new Set(tags.map(x=>String(x).slice(0,32)))].slice(0,12),
  note:String(patch?.note??prior.note??"").trim().slice(0,1200),
  updated_at:new Date().toISOString()
 };
 try{storage?.setItem?.(LIBRARY_META_KEY,JSON.stringify(all));}catch{}
 if(typeof window!=="undefined")
  window.dispatchEvent(new CustomEvent("green-ion-v16-library-meta",{detail:{flight_id:flightId,meta:all[key]}}));
 return all[key];
}
export function libraryMetaFor(flightId,storage=globalThis.localStorage){
 return loadLibraryMeta(storage)[String(flightId||"").toLowerCase()]||{favorite:false,tags:[],note:""};
}
export function filterFlightPackages(packages,{query="",favoritesOnly=false,tag="",storage=globalThis.localStorage}={}){
 const q=String(query||"").trim().toLowerCase(),wantedTag=String(tag||"").trim().toLowerCase();
 return (packages||[]).filter(pkg=>{
  const meta=libraryMetaFor(pkg.flight_id,storage);
  if(favoritesOnly&&!meta.favorite)return false;
  if(wantedTag&&!meta.tags.some(x=>String(x).toLowerCase()===wantedTag))return false;
  if(!q)return true;
  const haystack=[
   pkg.flight_id,pkg.status,pkg.cloud_status,pkg.windows_status,meta.note,
   ...meta.tags,...Object.values(pkg.files||{}).map(x=>x.name)
  ].join(" ").toLowerCase();
  return haystack.includes(q);
 });
}
export function loadTransferJournal(storage=globalThis.localStorage){
 let data=null;
 try{data=safeParse(storage?.getItem?.(TRANSFER_KEY)||"null",null);}catch{}
 if(!data||data.schema!==TRANSFER_SCHEMA||typeof data.entries!=="object")
  return {schema:TRANSFER_SCHEMA,updated_at:null,entries:{}};
 return data;
}
function transferKey(name,size){return String(name||"").toLowerCase()+":"+Math.max(0,Number(size)||0);}
export function transferEntry(name,size,storage=globalThis.localStorage){
 return loadTransferJournal(storage).entries[transferKey(name,size)]||null;
}
export function recordTransfer(name,size,status,extra={},storage=globalThis.localStorage){
 const journal=loadTransferJournal(storage);
 const key=transferKey(name,size),now=new Date().toISOString();
 const prior=journal.entries[key]||{};
 journal.entries[key]={
  name:String(name||""),size:Math.max(0,Number(size)||0),status:String(status||"UNKNOWN"),
  attempts:Number(prior.attempts||0)+(status==="DOWNLOADING"?1:0),
  updated_at:now,...extra
 };
 const entries=Object.entries(journal.entries).sort((a,b)=>
  String(b[1].updated_at||"").localeCompare(String(a[1].updated_at||"")));
 journal.entries=Object.fromEntries(entries.slice(0,MAX_JOURNAL));
 journal.updated_at=now;
 try{storage?.setItem?.(TRANSFER_KEY,JSON.stringify(journal));}catch{}
 if(typeof window!=="undefined")
  window.dispatchEvent(new CustomEvent("green-ion-v16-transfer",{detail:journal.entries[key]}));
 return journal.entries[key];
}
export function clearFinishedTransfers(storage=globalThis.localStorage){
 const journal=loadTransferJournal(storage);
 for(const [key,item] of Object.entries(journal.entries))
  if(item.status==="VERIFIED"||item.status==="SKIPPED")delete journal.entries[key];
 journal.updated_at=new Date().toISOString();
 try{storage?.setItem?.(TRANSFER_KEY,JSON.stringify(journal));}catch{}
 return journal;
}

function readCloudState(){
 try{return safeParse(localStorage.getItem("green-ion-v16-cloud-state")||"null",null);}catch{return null;}
}
async function pcChecksums(){
 const t=window.GREEN_ION_TRANSPORT;
 if(!t?.localBridge)return new Set();
 try{
  const response=await t.nativeFetch("/api/local/files",{cache:"no-store"});
  if(!response.ok)return new Set();
  const data=await response.json();
  return new Set((data.files||[]).map(x=>String(x.sha256||"").toLowerCase()).filter(Boolean));
 }catch{return new Set();}
}
function tag(text,cls=""){
 const span=document.createElement("span");span.className="check"+(cls?" "+cls:"");span.textContent=text;return span;
}
function renderPackages(packages){
 const node=$("v16FlightPackages");if(!node)return;
 node.replaceChildren();
 const query=$("v161LibrarySearch")?.value||"";
 const favoritesOnly=!!$("v161LibraryFavorites")?.checked;
 const tagFilter=$("v161LibraryTag")?.value||"";
 const filtered=filterFlightPackages(packages,{query,favoritesOnly,tag:tagFilter});
 if(!packages.length){node.textContent="No verified local flight originals yet.";return;}
 if(!filtered.length){node.textContent="No flights match the current library filter.";return;}
 for(const pkg of filtered.slice(0,200)){
  const row=document.createElement("div");row.className="card v16-flight-package";
  const meta=libraryMetaFor(pkg.flight_id);
  const title=document.createElement("div");title.className="row";
  const name=document.createElement("strong");name.textContent=(meta.favorite?"★ ":"")+pkg.flight_id;
  const state=document.createElement("span");
  state.textContent=pkg.status+(pkg.status==="COLLISION"?" · REVIEW":pkg.has_bin&&!pkg.full_companions?" · INCOMPLETE":"");
  title.append(name,state);row.append(title);
  const chips=document.createElement("div");chips.className="checks";
  for(const kind of KINDS)chips.append(tag(kind+" "+(pkg.files[kind]?"✓":"—"),pkg.files[kind]?"ok":""));
  chips.append(tag("Cloud "+pkg.cloud_status,pkg.cloud_status==="SYNCED"?"ok":""));
  chips.append(tag("PC "+pkg.windows_status,pkg.windows_status==="VERIFIED"?"ok":""));
  row.append(chips);
  const note=document.createElement("p");note.className="muted";
  note.textContent=pkg.status==="COLLISION"?
   "Conflicting originals detected. Nothing was overwritten; review checksums before relying on this package.":
   pkg.has_bin?(pkg.full_companions?"BIN/SEN/TXT originals grouped and preserved.":"Primary BIN is preserved; optional companions missing: "+pkg.missing.join(", ")) :
   "Companion files exist without the primary BIN recording.";
  row.append(note);
  const actions=document.createElement("div");actions.className="actions";
  const star=document.createElement("button");star.className="secondary";
  star.textContent=meta.favorite?"★ Favorite":"☆ Favorite";
  star.onclick=()=>{saveLibraryMeta(pkg.flight_id,{favorite:!meta.favorite});paint();};
  actions.append(star);row.append(actions);
  const details=document.createElement("details");
  const summary=document.createElement("summary");summary.textContent="Tags & notes";
  const tags=document.createElement("input");tags.placeholder="Tags, comma separated";tags.value=(meta.tags||[]).join(", ");
  const noteInput=document.createElement("textarea");noteInput.rows=2;noteInput.placeholder="Flight/recovery note";noteInput.value=meta.note||"";
  const save=document.createElement("button");save.className="secondary";save.textContent="Save tags & note";
  save.onclick=()=>{saveLibraryMeta(pkg.flight_id,{tags:tags.value,note:noteInput.value});paint();};
  details.append(summary,tags,noteInput,save);row.append(details);
  node.append(row);
 }
}
function downloadJson(name,value){
 const url=URL.createObjectURL(new Blob([JSON.stringify(value,null,2)],{type:"application/json"}));
 const a=document.createElement("a");a.href=url;a.download=name;document.body.append(a);a.click();a.remove();
 setTimeout(()=>URL.revokeObjectURL(url),60000);
}
async function paint(){
 const summary=$("v16DataSummary");if(!summary)return;
 try{
  const cloud=readCloudState(),pc=await pcChecksums();
  const manifest=await buildDataManifest({cloudState:cloud,pcChecksums:pc});
  summary.textContent=manifest.recorded_packages+" recorded flight(s) · "+manifest.file_count+
   " original file(s) · "+manifest.collisions+" collision(s) · cloud "+
   String(cloud?.status||"not configured").toLowerCase();
  renderPackages(manifest.packages);
 }catch(error){summary.textContent="Data library unavailable: "+error.message;}
}
export function initV16Data(){
 const history=$("history");if(!history)return;
 if(!$("v16DataCard")){
  const card=document.createElement("div");card.className="card";card.id="v16DataCard";
  card.innerHTML='<h2>Flight library · local first</h2>'+
   '<p class="muted">One entry per flight. Search the library here; originals stay immutable and detailed analysis remains separate.</p>'+
   '<div class="row"><span>Library status</span><strong id="v16DataSummary">Checking...</strong></div>'+
   '<label>Search flights <input id="v161LibrarySearch" type="search" placeholder="Flight, tag, note, file name"></label>'+
   '<div class="actions"><label><input id="v161LibraryFavorites" type="checkbox"> Favorites only</label>'+
   '<input id="v161LibraryTag" placeholder="Exact tag filter"></div>'+
   '<div class="actions"><button id="v16VerifyData">VERIFY LOCAL CHECKSUMS</button>'+
   '<button id="v16ExportManifest" class="secondary">EXPORT MANIFEST</button></div>'+
   '<p id="v16DataNotice" class="muted">Verification rereads stored bytes without modifying them.</p>'+
   '<div id="v16FlightPackages"></div>';
  history.prepend(card);
 }
 $("v16VerifyData").onclick=async()=>{
  const button=$("v16VerifyData");button.disabled=true;
  $("v16DataNotice").textContent="Re-reading immutable originals and checking SHA-256...";
  try{
   const report=await verifyLocalOriginals();
   $("v16DataNotice").textContent=report.problems.length?
    report.problems.length+" local integrity problem(s) found. Originals were not modified.":
    "Verified "+report.verified+" local original file(s) by SHA-256.";
  }catch(error){$("v16DataNotice").textContent="Verification failed safely: "+error.message;}
  finally{button.disabled=false;paint();}
 };
 $("v16ExportManifest").onclick=async()=>downloadJson("green-ion-v16-data-manifest.json",
  await buildDataManifest({cloudState:readCloudState(),pcChecksums:await pcChecksums()}));
 for(const id of ["v161LibrarySearch","v161LibraryTag"]){
  const input=$(id);if(input)input.addEventListener("input",paint);
 }
 if($("v161LibraryFavorites"))$("v161LibraryFavorites").addEventListener("change",paint);
 for(const event of ["green-ion-flight-saved","green-ion-history-change","green-ion-v16-cloud-state","green-ion-v16-transfer","green-ion-v16-library-meta"])
  window.addEventListener(event,paint);
 paint();
}
