const DB_NAME="green-ion-v18-offline";
const DB_VERSION=1;
const STORE="flights";

const reqResult=req=>new Promise((resolve,reject)=>{
  req.onsuccess=()=>resolve(req.result);
  req.onerror=()=>reject(req.error||new Error("IndexedDB request failed"));
});

export function historyKey(checksum){return String(checksum||"").toLowerCase();}
export function sortFlightMetadata(records){
  return [...records].sort((a,b)=>String(b.importedAt||"").localeCompare(String(a.importedAt||"")));
}
export function metadataOnly(record){
  if(!record)return null;
  const {originalBin,...meta}=record;
  return meta;
}
export async function sha256Hex(input){
  const bytes=input instanceof Uint8Array?input:new Uint8Array(input);
  if(!globalThis.crypto?.subtle)throw new Error("SHA-256 is unavailable in this browser");
  const digest=await globalThis.crypto.subtle.digest("SHA-256",bytes);
  return [...new Uint8Array(digest)].map(v=>v.toString(16).padStart(2,"0")).join("");
}
function openDb(){
  if(!globalThis.indexedDB)throw new Error("IndexedDB is unavailable");
  return new Promise((resolve,reject)=>{
    const req=indexedDB.open(DB_NAME,DB_VERSION);
    req.onupgradeneeded=()=>{
      const db=req.result;
      if(!db.objectStoreNames.contains(STORE)){
        const store=db.createObjectStore(STORE,{keyPath:"id"});
        store.createIndex("checksum","checksum",{unique:true});
        store.createIndex("importedAt","importedAt");
      }
    };
    req.onsuccess=()=>resolve(req.result);
    req.onerror=()=>reject(req.error||new Error("Could not open local flight history"));
  });
}
async function withStore(mode,fn){
  const db=await openDb();
  try{
    const tx=db.transaction(STORE,mode),store=tx.objectStore(STORE);
    const value=await fn(store,tx);
    if(mode==="readwrite")await new Promise((resolve,reject)=>{
      tx.oncomplete=resolve;tx.onerror=()=>reject(tx.error);tx.onabort=()=>reject(tx.error);
    });
    return value;
  }finally{db.close();}
}
export async function saveImportedFlight({name,bytes,label="",analysisSummary={}}){
  const source=bytes instanceof Uint8Array?bytes:new Uint8Array(bytes);
  if(!/\.BIN$/i.test(String(name||"")))throw new Error("Select an original RKT4 .BIN file");
  if(source.byteLength<32)throw new Error("BIN file is too short");
  const copy=source.slice();
  const checksum=await sha256Hex(copy);
  const id=historyKey(checksum);
  const existing=await withStore("readonly",store=>reqResult(store.get(id)));
  if(existing)return {record:metadataOnly(existing),duplicate:true};
  const record={
    id,checksum,originalName:String(name),label:String(label||"").trim(),
    importedAt:new Date().toISOString(),byteLength:copy.byteLength,
    analysisSummary:{...analysisSummary},originalBin:copy.buffer
  };
  await withStore("readwrite",store=>{store.add(record);});
  return {record:metadataOnly(record),duplicate:false};
}
export async function listFlightMetadata(){
  const all=await withStore("readonly",store=>reqResult(store.getAll()));
  return sortFlightMetadata(all.map(metadataOnly));
}
export async function getFlight(id){
  return await withStore("readonly",store=>reqResult(store.get(String(id))));
}
export async function updateFlightLabel(id,label){
  return await withStore("readwrite",async store=>{
    const record=await reqResult(store.get(String(id)));
    if(!record)throw new Error("Flight not found");
    record.label=String(label||"").trim();
    store.put(record);
    return metadataOnly(record);
  });
}
export async function deleteFlight(id){
  await withStore("readwrite",store=>{store.delete(String(id));});
}
export async function requestPersistentStorage(){
  if(!globalThis.navigator?.storage?.persist)return {supported:false,persisted:false};
  const persisted=await navigator.storage.persist();
  return {supported:true,persisted:!!persisted};
}
export async function persistentStorageStatus(){
  if(!globalThis.navigator?.storage?.persisted)return {supported:false,persisted:false};
  return {supported:true,persisted:!!(await navigator.storage.persisted())};
}
