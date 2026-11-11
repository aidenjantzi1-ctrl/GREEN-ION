/* Optional offline-first Supabase Auth + private Storage sync. No secret credentials.
   The rocket and local IndexedDB never depend on cloud availability. */
import {listLocalFiles,getLocalFile,saveLocalFile,digest} from "./v14_store.mjs";
import {safeFileName,fileKind} from "./v14_utils.mjs";
import {preference,onPreferences} from "./v14_preferences.mjs";
const cfg=window.GREEN_ION_CONFIG||{};
const url=String(cfg.supabaseUrl||"").replace(/\/$/,"");
const key=String(cfg.supabaseKey||"");
const enabled=/^https:\/\/[a-z0-9-]+\.supabase\.co$/.test(url)&&key.startsWith("sb_publishable_");
const SESSION_KEY="green-ion-v14-cloud-session",BUCKET="green-ion-flights";
const PAGE_SIZE=250,MAX_REMOTE_FILES=10000,MAX_FILE_BYTES=52428800;
let session=null,busy=false,refreshing=null,failures=0,retryAt=0,lastCompletedAt=0;
const $=id=>document.getElementById(id);
const message=text=>{if($("v14CloudStatus"))$("v14CloudStatus").textContent=text;};
const detail=text=>{if($("v14CloudProgress"))$("v14CloudProgress").textContent=text;};
function loadSession(){
 try{const obj=JSON.parse(localStorage.getItem(SESSION_KEY)||"null");
   session=obj?.access_token&&obj?.refresh_token?obj:null;}catch{session=null;}
}
function remember(s){
 session=s;
 try{if(s)localStorage.setItem(SESSION_KEY,JSON.stringify(s));else localStorage.removeItem(SESSION_KEY);}catch{}
}
function headers(access,json=true){
 const h={apikey:key};
 if(access)h.Authorization="Bearer "+access;
 if(json)h["Content-Type"]="application/json";
 return h;
}
async function checkedFetch(endpoint,options={},timeoutMs=60000){
 const controller=new AbortController();
 const timer=setTimeout(()=>controller.abort(),timeoutMs);
 try{
  return await fetch(endpoint,{...options,signal:controller.signal});
 }finally{clearTimeout(timer);}
}
async function authApi(path,body){
 const response=await checkedFetch(url+"/auth/v1/"+path,{
  method:"POST",headers:headers(null),body:JSON.stringify(body)
 });
 let data={};try{data=await response.json();}catch{}
 if(!response.ok)throw Error(data.msg||data.error_description||data.message||"Authentication request failed");
 return data;
}
async function token(){
 if(!enabled||!session)throw Error("Sign in to GREEN ION Cloud first");
 if(Date.now()/1000 >= (session.expires_at||0)-90){
  if(!refreshing){
   const prior=session;
   refreshing=authApi("token?grant_type=refresh_token",{refresh_token:prior.refresh_token})
    .then(data=>{
     // A sign-out that happened during refresh must not resurrect the session.
     if(session===prior)remember({...data,expires_at:Math.floor(Date.now()/1000)+(data.expires_in||3600)});
     return data;
    }).finally(()=>{refreshing=null;});
  }
  await refreshing;
 }
 if(!session?.access_token)throw Error("Session ended; sign in again");
 return session.access_token;
}
async function user(){
 const access=await token();
 const response=await checkedFetch(url+"/auth/v1/user",{headers:headers(access,false)});
 if(!response.ok)throw Error("Cloud session is not valid. Sign out and sign in again.");
 const data=await response.json();
 if(!/^[0-9a-f-]{36}$/.test(data.id||""))throw Error("Invalid account ID");
 return data;
}
async function api(path,options={},timeoutMs=60000){
 const access=await token();
 const response=await checkedFetch(url+path,{
  ...options,
  headers:{...headers(access,typeof options.body==="string"),...(options.headers||{})}
 },timeoutMs);
 if(!response.ok){
  let reason="HTTP "+response.status;
  try{const data=await response.json();reason=data.message||data.error||reason;}catch{}
  const err=Error(reason);err.status=response.status;throw err;
 }
 return response;
}
function renderAccount(){
 const logged=!!session;
 $("v14CloudLogin").hidden=logged;
 $("v14CloudAccount").hidden=!logged;
 if(logged)$("v14CloudIdentity").textContent=session.user?.email||"Authenticated account";
}
export async function signIn(email,password,signup=false){
 if(!enabled)throw Error("Missing cloud project configuration");
 const data=await authApi(signup?"signup":"token?grant_type=password",{email,password});
 if(data.access_token)remember({...data,expires_at:Math.floor(Date.now()/1000)+(data.expires_in||3600)});
 return !!data.access_token;
}
export async function signOut(){
 remember(null);failures=0;retryAt=0;
 renderAccount();
 message("Signed out. Existing local flights were not changed.");
 detail("No cloud session");
}
export async function listCloudFiles(){
 const all=[];
 for(let offset=0;offset<MAX_REMOTE_FILES;offset+=PAGE_SIZE){
  const path="/rest/v1/flight_backups?select=checksum,original_name,file_kind,byte_size,storage_path,created_at&order=created_at.desc&limit="+PAGE_SIZE+"&offset="+offset;
  const response=await api(path);
  const entries=await response.json();
  if(!Array.isArray(entries))throw Error("Invalid cloud backup listing");
  all.push(...entries);
  if(entries.length<PAGE_SIZE)return all;
 }
 throw Error("More than "+MAX_REMOTE_FILES+" cloud files: export/delete unneeded recordings before syncing");
}
function describeFiles(files){
 const bytes=files.reduce((sum,f)=>sum+(Number(f.byte_size)||0),0);
 return files.length+" privately stored files · "+(bytes/1048576).toFixed(1)+" MiB";
}
async function refreshCloudList(){
 if(!session)return;
 try{
  const files=await listCloudFiles();
  $("v14CloudCount").textContent=describeFiles(files);
  if($("v14CloudRecent")){
   const node=$("v14CloudRecent");node.replaceChildren();
   for(const f of files.slice(0,5)){
    const row=document.createElement("div");row.className="row";
    const label=document.createElement("span");
    label.textContent=safeFileName(f.original_name);
    const info=document.createElement("strong");info.textContent=(Number(f.byte_size)/1024).toFixed(1)+" KiB";
    row.append(label,info);node.append(row);
   }
  }
 }catch(e){$("v14CloudCount").textContent="Could not load private backup listing: "+e.message;}
}
function canAutoSync(){
 return enabled&&!!session&&preference("cloudAuto")&&navigator.onLine&&!navigator.connection?.saveData;
}
function scheduleRetry(){
 failures=Math.min(failures+1,7);
 const delay=Math.min(30*60*1000,30000*Math.pow(2,failures-1));
 retryAt=Date.now()+delay;
 detail("Sync will retry in about "+Math.ceil(delay/60000)+" minute(s) when connected. Local files remain safe.");
}
export async function backupNow({automatic=false}={}){
 if(busy||!session)return;
 if(automatic&&(!canAutoSync()||Date.now()<retryAt))return;
 if(!navigator.onLine){message("Offline. Local recordings are queued until internet returns.");return;}
 busy=true;
 message("Checking private cloud backups...");
 try{
  const account=await user(),remote=await listCloudFiles();
  const existing=new Set(remote.map(f=>f.checksum));
  const files=await listLocalFiles();
  const pending=files.filter(f=>!existing.has(f.checksum));
  detail(pending.length+" file(s) awaiting backup · "+existing.size+" already in cloud");
  let uploaded=0;
  for(const file of pending){
   if(!session)throw Error("Cloud session ended");
   if(file.size>MAX_FILE_BYTES||file.size<1)throw Error("File size not accepted: "+file.name);
   const storagePath=account.id+"/"+file.checksum+"."+file.kind.toLowerCase();
   detail("Backing up "+(uploaded+1)+"/"+pending.length+": "+file.name);
   try{
    await api("/storage/v1/object/"+BUCKET+"/"+storagePath,{
      method:"POST",body:file.bytes,headers:{
       "Content-Type":file.kind==="TXT"?"text/plain":"application/octet-stream",
       "x-upsert":"false"
      }
    },120000);
   }catch(e){
    // Another authorized device may have uploaded the same checksum first.
    if(e.status!==409&&!/already exists|duplicate/i.test(e.message))throw e;
   }
   await api("/rest/v1/flight_backups?on_conflict=user_id,checksum",{
    method:"POST",headers:{"Content-Type":"application/json","Prefer":"resolution=merge-duplicates,return=minimal"},
    body:JSON.stringify({
     user_id:account.id,checksum:file.checksum,original_name:file.name,file_kind:file.kind,
     byte_size:file.size,storage_path:storagePath,flight_name:file.name.replace(/\.[^.]+$/,""),
     metadata:{origin:file.origin,createdAt:file.createdAt}
    })
   });
   uploaded++;existing.add(file.checksum);
  }
  failures=0;retryAt=0;lastCompletedAt=Date.now();
  message("Cloud backup complete: "+uploaded+" uploaded, "+(files.length-uploaded)+" already stored.");
  detail("Last successful sync: "+new Date(lastCompletedAt).toLocaleString());
  await refreshCloudList();
 }catch(e){
  message("Cloud sync paused: "+e.message+". Original files remain stored on this device.");
  scheduleRetry();
 }finally{busy=false;}
}
export async function restoreCloud(){
 if(busy||!session)return;
 if(!navigator.onLine){message("Internet is required to restore cloud originals.");return;}
 busy=true;message("Restoring private cloud recordings...");
 try{
  const account=await user(),files=await listCloudFiles();
  let restored=0,skipped=0;
  for(const file of files){
   const name=safeFileName(file.original_name);
   if(fileKind(name)!==file.file_kind||
      !file.storage_path.startsWith(account.id+"/")||
      !/^[a-f0-9-]{36}\/[a-f0-9]{64}\.(bin|sen|txt)$/.test(file.storage_path)||
      !/^[a-f0-9]{64}$/.test(file.checksum)||
      Number(file.byte_size)<1||Number(file.byte_size)>MAX_FILE_BYTES)
     throw Error("Invalid cloud file metadata for "+name);
   if(await getLocalFile(file.checksum+"."+file.file_kind.toLowerCase())){skipped++;continue;}
   detail("Restoring "+(restored+skipped+1)+"/"+files.length+": "+name);
   const response=await api("/storage/v1/object/authenticated/"+BUCKET+"/"+file.storage_path,{},120000);
   const bytes=await response.arrayBuffer();
   if(bytes.byteLength!==Number(file.byte_size)||await digest(bytes)!==file.checksum)
     throw Error("Cloud file checksum mismatch: "+name);
   const result=await saveLocalFile({name,bytes,origin:"Supabase restore"});
   if(result.duplicate)skipped++;else restored++;
  }
  message("Cloud restore complete: "+restored+" files restored, "+skipped+" duplicates skipped.");
  detail("All downloaded originals verified using SHA-256.");
  window.dispatchEvent(new Event("green-ion-history-change"));
 }catch(e){message("Cloud restore stopped safely: "+e.message+". No existing recording was overwritten.");}
 finally{busy=false;}
}
export function initCloud(){
 loadSession();renderAccount();
 if(!enabled){message("Cloud configuration unavailable. Offline storage still works.");return;}
 $("v14CloudSubmit").onclick=async()=>{
  const email=$("v14CloudEmail").value.trim(),password=$("v14CloudPassword").value;
  if(!email||!password){message("Enter account email and password.");return;}
  try{
   const authenticated=await signIn(email,password,$("v14CloudMode").value==="signup");
   $("v14CloudPassword").value="";
   renderAccount();
   message(authenticated?"Signed in. Private cloud sync is available.":"Account created. Confirm your email if required, then sign in.");
   if(authenticated){await refreshCloudList();backupNow({automatic:true});}
  }catch(e){message("Cloud sign-in failed: "+e.message);}
 };
 $("v14CloudBackup").onclick=()=>backupNow();
 $("v14CloudRestore").onclick=restoreCloud;
 $("v14CloudSignOut").onclick=signOut;
 if(session)refreshCloudList();
 onPreferences(p=>{if(p.cloudAuto&&session)backupNow({automatic:true});});
 window.addEventListener("online",()=>{retryAt=0;backupNow({automatic:true});});
 window.addEventListener("storage",event=>{if(event.key===SESSION_KEY){loadSession();renderAccount();}});
 window.addEventListener("green-ion-flight-saved",()=>backupNow({automatic:true}));
 document.addEventListener("visibilitychange",()=>{
  if(document.visibilityState==="visible"&&Date.now()-lastCompletedAt>120000)backupNow({automatic:true});
 });
 setInterval(()=>backupNow({automatic:true}),60000);
}
