import {APP_VERSION,REPOSITORY,compareVersions,assetForRelease,releaseFirmwarePackage,verifiedFirmwareBytes} from "./v14_utils.mjs";
const t=window.GREEN_ION_TRANSPORT;
const el=id=>document.getElementById(id);
let active=null,lastCheck=0,installing=false;
function setText(id,value){const node=el(id);if(node)node.textContent=value;}
function feedback(message){
 setText("v14UpdateAction",message);
 setText("v14UpdateCheck",message);
}
function releaseList(payload){
 return Array.isArray(payload)?payload.filter(r=>!r.draft && /^v14\./.test(r.tag_name||"") && !!assetForRelease(r))
  .sort((a,b)=>compareVersions(b.tag_name,a.tag_name)||0):[];
}
// Notify once per published release, including builds deployed before the user visits.
const notifiedKey="green-ion-v16-last-notified-release";
let notifiedInSession=null;
function releaseReady(release){
 if(!release)return false;
 if(compareVersions(release.tag_name,APP_VERSION)<0)return false;
 let previouslyNotified=notifiedInSession;
 try{previouslyNotified=previouslyNotified||localStorage.getItem(notifiedKey);}catch{}
 return !previouslyNotified||compareVersions(release.tag_name,previouslyNotified)>0;
}
function showUpdate(release){
 const asset=assetForRelease(release);
 if(!asset)return;
 active={release,asset};
 notifiedInSession=release.tag_name;
 try{localStorage.setItem(notifiedKey,release.tag_name);}catch{}
 setText("v14AvailableVersion",release.tag_name);
 setText("v14AvailableAsset",asset.name);
 const direct=!!releaseFirmwarePackage(release);
 el("v14DownloadUpdate").disabled=!direct;
 setText("v14UpdateAction",direct?
  "Direct Feather installation requires GitHub access, verified Feather Wi-Fi and bench safety confirmation.":
  "This release lacks the matching app image and checksum manifest. Use the manual download option.");
 el("v14UpdateDialog").hidden=false;
}
export async function checkUpdates(showAll=false){
 const now=Date.now();
 if(now-lastCheck<30000&&!showAll)return;
 lastCheck=now;
 setText("v14UpdateCheck","Checking GitHub releases...");
 try{
  const response=await t.nativeFetch("https://api.github.com/repos/"+REPOSITORY+"/releases?per_page=15",{
   cache:"no-store",headers:{"Accept":"application/vnd.github+json"}
  });
  if(!response.ok)throw Error("GitHub returned HTTP "+response.status);
  const releases=releaseList(await response.json());
  if(!releases.length){setText("v14UpdateCheck","No published V16 firmware package yet.");return;}
  const newest=releases[0];
  setText("v14UpdateCheck","Latest release: "+newest.tag_name+" · installed website "+APP_VERSION);
  let dismissed=null;
  try{dismissed=sessionStorage.getItem("green-ion-v14-later");}catch{}
  if(releaseReady(newest) && (showAll || dismissed!==newest.tag_name))showUpdate(newest);
  else if(showAll)showUpdate(newest);
 }catch(e){setText("v14UpdateCheck","Cannot check GitHub right now: "+e.message);}
}
function manualDownload(){
 if(!active)return;
 const a=document.createElement("a");
 a.href=active.asset.browser_download_url;
 a.target="_blank";a.rel="noopener noreferrer";
 document.body.append(a);a.click();a.remove();
 feedback("Manual GitHub download opened. The file was NOT transferred to the Feather. Use System → Install firmware on the Feather local dashboard when connected.");
 el("v14UpdateDialog").hidden=true;
}
async function currentFeatherStatus(){
 if(!t.wifiFresh() || t.source()!=="wifi")
  throw Error("Feather Wi-Fi is not verified. Connect to GREEN-ION-V14, refresh Status, then retry. The hosted browser may block private-network connections.");
 const response=await t.nativeFetch(t.apiUrl("/api/status"),{cache:"no-store"});
 if(!response.ok)throw Error("Cannot verify Feather hardware (HTTP "+response.status+")");
 const s=await response.json();
 if(s.v13_source==="lora"||s.v13_source==="demo"||!Number.isFinite(Number(s.battery_v)))
  throw Error("Only verified live Feather Wi-Fi permits a firmware update");
 if(!s.ota_available)throw Error("The Feather has no usable OTA slot; use a USB firmware update");
 if(s.recording||s.update_in_progress||s.home_test_active||s.maintenance_mode)
  throw Error("Finish the recording, home test or prior maintenance session first");
 if(Number(s.battery_v)<3.6)throw Error("Battery must measure at least 3.60 V; use a charged LiPo and stable USB power");
 return s;
}
async function installReleaseOnFeather(){
 if(!active||installing)return;
 const choice=active;
 const pkg=releaseFirmwarePackage(choice.release);
 if(!pkg){feedback("No checksum-verified Feather app image was published. Use the manual download.");return;}
 installing=true;
 el("v14DownloadUpdate").disabled=true;
 try{
  await currentFeatherStatus();
  if(typeof window.greenIonInstallFirmwareFile!=="function")
   throw Error("The local Feather OTA installer is unavailable. Use System → Install firmware");
  // A download click never silently flashes. Reconfirmation occurs immediately before
  // maintenance and upload in the existing Feather updater.
  feedback("Retrieving official Feather firmware and SHA-256 manifest from GitHub...");
  // Vercel relays only named official release assets to avoid GitHub CDN CORS.
  // It never proxies commands to the Feather; OTA still requires local Wi-Fi.
  const relay=async(url,options)=>{
   const name=url===pkg.image.browser_download_url?pkg.image.name:
    url===pkg.manifest.browser_download_url?pkg.manifest.name:null;
   if(!name)throw Error("Refusing unknown release asset");
   const endpoint="/api/firmware?tag="+encodeURIComponent(choice.release.tag_name)+"&name="+encodeURIComponent(name);
   return t.nativeFetch(endpoint,options);
  };
  const bytes=await verifiedFirmwareBytes(choice.release,relay,crypto.subtle);
  feedback("Verified "+bytes.byteLength+" bytes against the official SHA-256 manifest. Rechecking Feather...");
  await currentFeatherStatus();
  const file=new File([bytes],pkg.image.name,{type:"application/octet-stream"});
  feedback("Firmware validated. Confirm the separate bench installation prompt to transfer to the Feather.");
  const accepted=await window.greenIonInstallFirmwareFile(file);
  if(accepted){
   feedback("Feather accepted and verified the firmware upload. It is restarting; reconnect, check its firmware version and run the full home test.");
   el("v14UpdateDialog").hidden=true;
  }else{
   feedback("Firmware was not installed. See System → Firmware update for details. If maintenance was entered, exit it or reboot before flight.");
  }
 }catch(e){
  feedback("Direct Feather update unavailable: "+e.message+". No firmware installation was confirmed. Use manual download and Feather's local 192.168.4.1 dashboard if needed.");
 }finally{
  installing=false;
  if(active===choice)el("v14DownloadUpdate").disabled=!releaseFirmwarePackage(choice.release);
 }
}
export function initUpdates(){
 el("v14CheckUpdates").onclick=()=>checkUpdates(true);
 el("v14UpdateLater").onclick=()=>{
  if(active)try{sessionStorage.setItem("green-ion-v14-later",active.release.tag_name);}catch{}
  el("v14UpdateDialog").hidden=true;
 };
 el("v14DownloadUpdate").onclick=installReleaseOnFeather;
 el("v14ManualDownload").onclick=manualDownload;
 el("v14OpenRelease").onclick=()=>{
  if(active)window.open("https://github.com/"+REPOSITORY+"/releases/tag/"+encodeURIComponent(active.release.tag_name),"_blank","noopener,noreferrer");
 };
 setTimeout(()=>checkUpdates(),3000);
 setInterval(()=>checkUpdates(),20*60*1000);
 window.addEventListener("green-ion-link",()=>{
  if(active && releaseReady(active.release) && !el("v14UpdateDialog").hidden)return;
  let dismissed=null;
  try{dismissed=sessionStorage.getItem("green-ion-v14-later");}catch{}
  if(active && releaseReady(active.release) && dismissed!==active.release.tag_name)showUpdate(active.release);
 });
}
