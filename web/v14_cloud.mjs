/* Supabase Auth + RLS-protected private backup. Uses only a public project key and user-scoped JWTs. */
import {listLocalFiles,saveLocalFile,digest} from "./v14_store.mjs";
import {safeFileName,fileKind} from "./v14_utils.mjs";
const cfg=window.GREEN_ION_CONFIG||{};
const url=String(cfg.supabaseUrl||"").replace(/\/$/,"");
const key=String(cfg.supabaseKey||"");
const enabled=/^https:\/\/[a-z0-9-]+\.supabase\.co$/.test(url)&&key.startsWith("sb_publishable_");
const SESSION_KEY="green-ion-v14-cloud-session";
let session=null,busy=false;
const $=id=>document.getElementById(id);
function message(text){if($("v14CloudStatus"))$("v14CloudStatus").textContent=text;}
function loadSession(){
 try{const s=JSON.parse(localStorage.getItem(SESSION_KEY)||"null");if(s?.access_token&&s?.refresh_token)session=s;}catch{/* ignore */}
}
function remember(s){session=s;try{if(s)localStorage.setItem(SESSION_KEY,JSON.stringify(s));else localStorage.removeItem(SESSION_KEY);}catch{/* private browsing */}}
function headers(token,json=true){
 const result={"apikey":key};
 if(token)result.Authorization="Bearer "+token;
 if(json)result["Content-Type"]="application/json";
 return result;
}
async function authApi(path,body){
 const response=await fetch(url+"/auth/v1/"+path,{method:"POST",headers:headers(null),body:JSON.stringify(body)});
 let data={};try{data=await response.json();}catch{}
 if(!response.ok)throw Error(data.msg||data.error_description||data.message||"Authentication failed");
 return data;
}
async function token(){
 if(!enabled||!session)throw Error("Sign in to GREEN ION cloud backups first");
 if(Date.now()/1000>=(session.expires_at||0)-90){
  const data=await authApi("token?grant_type=refresh_token",{refresh_token:session.refresh_token});
  remember({...data,expires_at:Math.floor(Date.now()/1000)+(data.expires_in||3600)});
 }
 return session.access_token;
}
async function user(){
 const access=await token();
 const response=await fetch(url+"/auth/v1/user",{headers:headers(access,false)});
 if(!response.ok)throw Error("Session expired; sign in again");
 const data=await response.json();
 if(!/^[0-9a-f-]{36}$/.test(data.id||""))throw Error("Invalid account identifier");
 return data;
}
async function cloudFetch(path,opts={}){
 const access=await token();
 const response=await fetch(url+path,{
  ...opts,
  headers:{...headers(access,opts.body!=null&&typeof opts.body==="string"),...(opts.headers||{})}
 });
 if(!response.ok){let reason="HTTP "+response.status;try{const data=await response.json();reason=data.message||data.error||reason;}catch{}
  throw Error(reason);
 }
 return response;
}
export async function signIn(email,password,signup=false){
 if(!enabled)throw Error("Cloud configuration unavailable");
 const data=await authApi(signup?"signup":"token?grant_type=password",{email,password});
 if(data.access_token)remember({...data,expires_at:Math.floor(Date.now()/1000)+(data.expires_in||3600)});
 return !!data.access_token;
}
export async function signOut(){
 remember(null);message("Signed out. Saved local flights remain on this device.");
 renderAccount();
}
function renderAccount(){
 const signed=!!session;
 $("v14CloudLogin").hidden=signed;$("v14CloudAccount").hidden=!signed;
 if(signed)$("v14CloudIdentity").textContent=session.user?.email||"Signed in";
}
export async function listCloudFiles(){
 const response=await cloudFetch("/rest/v1/flight_backups?select=checksum,original_name,file_kind,byte_size,storage_path,created_at&order=created_at.desc&limit=500");
 return response.json();
}
export async function backupNow(){
 if(busy||!session)return;
 if(!navigator.onLine){message("Offline: files remain safely queued on this device. Cloud sync resumes when internet returns.");return;}
 busy=true;message("Checking private cloud backups...");
 try{
  const account=await user();
  const remote=await listCloudFiles();
  const uploaded=new Set(remote.map(f=>f.checksum));
  let count=0,skipped=0;
  for(const f of await listLocalFiles()){
   if(uploaded.has(f.checksum)){skipped++;continue;}
   if(f.size>52428800)throw Error("File exceeds cloud size limit: "+f.name);
   const storagePath=account.id+"/"+f.checksum+"."+f.kind.toLowerCase();
   const contentType=f.kind==="TXT"?"text/plain":"application/octet-stream";
   try{
    await cloudFetch("/storage/v1/object/green-ion-flights/"+storagePath,{
      method:"POST",body:f.bytes,
      headers:{"Content-Type":contentType,"x-upsert":"false"}
    });
   }catch(e){
    if(!/already exists|duplicate|409/i.test(e.message))throw e;
   }
   await cloudFetch("/rest/v1/flight_backups?on_conflict=user_id,checksum",{
     method:"POST",headers:{"Content-Type":"application/json","Prefer":"resolution=merge-duplicates,return=minimal"},
     body:JSON.stringify({
       user_id:account.id,checksum:f.checksum,original_name:f.name,file_kind:f.kind,
       byte_size:f.size,storage_path:storagePath,
       flight_name:f.name.replace(/\.[^.]+$/,""),metadata:{origin:f.origin,createdAt:f.createdAt}
     })
   });
   uploaded.add(f.checksum);count++;
  }
  message("Cloud backup complete: "+count+" uploaded, "+skipped+" already backed up.");
  await renderCloudList();
 }catch(e){message("Cloud sync incomplete; local data retained: "+e.message);}
 finally{busy=false;}
}
export async function restoreCloud(){
 if(busy||!session)return;
 busy=true;message("Restoring private cloud recordings...");
 try{
  const entries=await listCloudFiles();
  let added=0,skipped=0;
  for(const e of entries){
   const filename=safeFileName(e.original_name);
   if(fileKind(filename)!==e.file_kind||
      !/^[0-9a-f-]{36}\/[a-f0-9]{64}\.(bin|sen|txt)$/.test(e.storage_path) ||
      e.byte_size>52428800)throw Error("Invalid cloud file metadata");
   const res=await cloudFetch("/storage/v1/object/authenticated/green-ion-flights/"+e.storage_path);
   const bytes=await res.arrayBuffer();
   if(bytes.byteLength!==e.byte_size||await digest(bytes)!==e.checksum)throw Error("Checksum mismatch restoring "+filename);
   const result=await saveLocalFile({name:filename,bytes,origin:"Supabase restore"});
   if(result.duplicate)skipped++;else added++;
  }
  message("Cloud restore complete: "+added+" restored, "+skipped+" duplicates skipped.");
  window.dispatchEvent(new Event("green-ion-history-change"));
 }catch(e){message("Cloud restore interrupted: "+e.message);}
 finally{busy=false;}
}
async function renderCloudList(){
 if(!session)return;
 try{
  const items=await listCloudFiles();
  $("v14CloudCount").textContent=items.length+" files stored privately in Supabase.";
 }catch(e){$("v14CloudCount").textContent="Cloud listing unavailable: "+e.message;}
}
export function initCloud(){
 loadSession();renderAccount();
 if(!enabled){message("Cloud configuration is missing. Offline storage remains fully available.");return;}
 $("v14CloudSubmit").onclick=async()=>{
  const email=$("v14CloudEmail").value.trim(),password=$("v14CloudPassword").value;
  if(!email||!password){message("Enter email and password.");return;}
  try{
    const authenticated=await signIn(email,password,$("v14CloudMode").value==="signup");
    $("v14CloudPassword").value="";
    message(authenticated?"Signed in. Local flight files are ready to back up.":"Account created; check your email confirmation, then sign in.");
    renderAccount();if(authenticated)backupNow();
  }catch(e){message("Sign-in failed: "+e.message);}
 };
 $("v14CloudSignOut").onclick=signOut;
 $("v14CloudBackup").onclick=backupNow;
 $("v14CloudRestore").onclick=restoreCloud;
 if(session)renderCloudList();
 window.addEventListener("online",()=>{if(session)backupNow();});
 window.addEventListener("green-ion-flight-saved",()=>{if(session&&navigator.onLine)backupNow();});
 setInterval(()=>{if(session&&navigator.onLine)backupNow();},120000);
}
