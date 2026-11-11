import {fileKind,safeFileName} from "./v14_utils.mjs";
const DB="green-ion-v14-recordings",TABLE="files",META="file_meta",MAX_SIZE=52428800;
const p=req=>new Promise((resolve,reject)=>{req.onsuccess=()=>resolve(req.result);req.onerror=()=>reject(req.error);});
async function open(){
 return new Promise((resolve,reject)=>{
  const req=indexedDB.open(DB,2);
  req.onupgradeneeded=()=>{
    const db=req.result;
    let table;
    if(!db.objectStoreNames.contains(TABLE)){
      table=db.createObjectStore(TABLE,{keyPath:"key"});
      table.createIndex("checksum","checksum");
      table.createIndex("name","name");
    }else table=req.transaction.objectStore(TABLE);
    if(!db.objectStoreNames.contains(META)){
      const summary=db.createObjectStore(META,{keyPath:"key"});
      // Upgrade v1 in-place: keep all original BIN/SEN/TXT bytes unchanged.
      const cursor=table.openCursor();
      cursor.onsuccess=event=>{
        const item=event.target.result;
        if(item){const {key,checksum,name,kind,size,origin,createdAt}=item.value;
          summary.put({key,checksum,name,kind,size,origin,createdAt});item.continue();}
      };
    }
  };
  req.onsuccess=()=>resolve(req.result);req.onerror=()=>reject(req.error);
 });
}
export async function digest(bytes){
 const hash=await crypto.subtle.digest("SHA-256",bytes);
 return Array.from(new Uint8Array(hash),b=>b.toString(16).padStart(2,"0")).join("");
}
export async function listLocalFiles(){
 const db=await open();
 try{return await p(db.transaction(TABLE).objectStore(TABLE).getAll());}
 finally{db.close();}
}
/** IndexedDB v2 index without reading full binary ArrayBuffers into the UI. */
export async function listLocalMetadata(){
 const db=await open();
 try{return await p(db.transaction(META).objectStore(META).getAll());}
 finally{db.close();}
}
export async function getLocalFile(key){
 const db=await open();
 try{return await p(db.transaction(TABLE).objectStore(TABLE).get(key));}
 finally{db.close();}
}
export async function hasLocalName(name,size){
 const files=await listLocalMetadata();
 return files.some(f=>f.name===name && (size==null||f.size===size));
}
export async function saveLocalFile({name,bytes,origin="import",metadata={}}){
 const basename=safeFileName(name);
 const buffer=bytes instanceof ArrayBuffer?bytes:
   ArrayBuffer.isView(bytes)?bytes.buffer.slice(bytes.byteOffset,bytes.byteOffset+bytes.byteLength):
   bytes instanceof Blob?await bytes.arrayBuffer():null;
 if(!buffer||buffer.byteLength>MAX_SIZE||buffer.byteLength<1)throw Error("Unsupported or oversized recording");
 const checksum=await digest(buffer);
 const kind=fileKind(basename);
 const key=checksum+"."+kind.toLowerCase();
 const existing=await getLocalFile(key);
 if(existing)return {record:existing,duplicate:true};
 const record={key,checksum,name:basename,kind,size:buffer.byteLength,bytes:buffer,origin,
   metadata:metadata&&typeof metadata==="object"?metadata:{},createdAt:new Date().toISOString()};
 const db=await open();
 try{
  const tx=db.transaction([TABLE,META],"readwrite");
  tx.objectStore(TABLE).put(record);
  tx.objectStore(META).put({key,checksum,name:basename,kind,size:buffer.byteLength,origin,createdAt:record.createdAt});
  await new Promise((resolve,reject)=>{tx.oncomplete=resolve;tx.onerror=()=>reject(tx.error);});
 }finally{db.close();}
 window.dispatchEvent(new CustomEvent("green-ion-flight-saved",{detail:{key,checksum,name:basename}}));
 return {record,duplicate:false};
}
export function downloadFile(record){
 const url=URL.createObjectURL(new Blob([record.bytes],{type:"application/octet-stream"}));
 const a=document.createElement("a");a.href=url;a.download=record.name;document.body.append(a);a.click();a.remove();
 setTimeout(()=>URL.revokeObjectURL(url),60000);
}
const toBase64=bytes=>{
 const arr=new Uint8Array(bytes);let raw="";
 for(let i=0;i<arr.length;i+=16384)raw+=String.fromCharCode(...arr.subarray(i,i+16384));
 return btoa(raw);
};
const fromBase64=s=>{
 const raw=atob(s);const result=new Uint8Array(raw.length);
 for(let i=0;i<raw.length;i++)result[i]=raw.charCodeAt(i);
 return result.buffer;
};
export async function exportArchive(){
 const files=await listLocalFiles();
 return JSON.stringify({format:"GREEN-ION-V14-BACKUP-1",files:files.map(f=>({
  checksum:f.checksum,name:f.name,kind:f.kind,origin:f.origin,
  metadata:f.metadata,createdAt:f.createdAt,base64:toBase64(f.bytes)
 }))});
}
export async function restoreArchive(json){
 const archive=JSON.parse(json);
 if(archive.format!=="GREEN-ION-V14-BACKUP-1"||!Array.isArray(archive.files) || archive.files.length>500)
   throw Error("Invalid V14 backup format");
 let restored=0,duplicates=0,total=0;
 for(const file of archive.files){
  safeFileName(file.name);
  if(typeof file.base64!=="string")throw Error("Missing backup file data");
  const bytes=fromBase64(file.base64);total+=bytes.byteLength;
  if(total>250000000)throw Error("Backup exceeds maximum restore size");
  if(await digest(bytes)!==file.checksum)throw Error("Checksum mismatch: "+file.name);
  const r=await saveLocalFile({name:file.name,bytes,origin:"restored",metadata:file.metadata});
  if(r.duplicate)duplicates++;else restored++;
 }
 return {restored,duplicates};
}
export async function importLegacyHistory(){
 try{
  const legacy=await import("./history-db.mjs");
  const flights=await legacy.listFlights();
  let count=0;
  for(const f of flights){
   if(!f.originalBin||!f.name||!fileKind(f.name))continue;
   const result=await saveLocalFile({name:f.name,bytes:f.originalBin,origin:"V11/V12/V13 history",metadata:f.metadata||{}});
   if(!result.duplicate)count++;
  }
  return count;
 }catch{return 0;}
}
