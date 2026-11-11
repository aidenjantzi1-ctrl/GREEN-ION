import {APP_VERSION,REPOSITORY,compareVersions,assetForRelease} from "./v14_utils.mjs";
const t=window.GREEN_ION_TRANSPORT;
const el=id=>document.getElementById(id);
let active=null,lastCheck=0;
function setText(id,text){const node=el(id);if(node)node.textContent=text;}
function releaseList(payload){
 return Array.isArray(payload)?payload.filter(r=>!r.draft && /^v14\./.test(r.tag_name||"") && !!assetForRelease(r))
   .sort((a,b)=>compareVersions(b.tag_name,a.tag_name)||0):[];
}
function releaseReady(release){
 if(!release)return false;
 const appNewer=compareVersions(release.tag_name,APP_VERSION)===1;
 const featherVersion=t.state.wifiVersion;
 const featherNewer=featherVersion!=null && Number(featherVersion)<14;
 return appNewer||featherNewer;
}
function showUpdate(release){
 const asset=assetForRelease(release);if(!asset)return;
 active={release,asset};
 setText("v14AvailableVersion",release.tag_name);
 setText("v14AvailableAsset",asset.name);
 el("v14UpdateDialog").hidden=false;
}
export async function checkUpdates(showAll=false){
 const now=Date.now();
 if(now-lastCheck<30000&&!showAll)return;
 lastCheck=now;
 setText("v14UpdateCheck","Checking GitHub releases...");
 try{
  const response=await t.nativeFetch("https://api.github.com/repos/"+REPOSITORY+"/releases?per_page=15",{cache:"no-store",headers:{"Accept":"application/vnd.github+json"}});
  if(!response.ok)throw Error("GitHub returned HTTP "+response.status);
  const releases=releaseList(await response.json());
  if(!releases.length){setText("v14UpdateCheck","No published V14 firmware package yet. Existing V13 packages remain available.");return;}
  const newest=releases[0];
  setText("v14UpdateCheck","Latest release: "+newest.tag_name+" · installed website "+APP_VERSION);
  const dismissed=sessionStorage.getItem("green-ion-v14-later");
  if(releaseReady(newest) && (showAll || dismissed!==newest.tag_name))showUpdate(newest);
  else if(showAll)showUpdate(newest);
 }catch(e){setText("v14UpdateCheck","Cannot check GitHub right now (offline or request unavailable): "+e.message);}
}
export function initUpdates(){
 el("v14CheckUpdates").onclick=()=>checkUpdates(true);
 el("v14UpdateLater").onclick=()=>{
  if(active)sessionStorage.setItem("green-ion-v14-later",active.release.tag_name);
  el("v14UpdateDialog").hidden=true;
 };
 el("v14DownloadUpdate").onclick=()=>{
  if(!active)return;
  const a=document.createElement("a");
  a.href=active.asset.browser_download_url;a.target="_blank";a.rel="noopener noreferrer";
  document.body.append(a);a.click();a.remove();
  setText("v14UpdateCheck","GitHub release download opened. Verify the image and use bench-only OTA through Feather Wi-Fi; no firmware was flashed.");
  el("v14UpdateDialog").hidden=true;
 };
 el("v14OpenRelease").onclick=()=>{
  if(!active)return;
  window.open("https://github.com/"+REPOSITORY+"/releases/tag/"+encodeURIComponent(active.release.tag_name),"_blank","noopener,noreferrer");
 };
 setTimeout(()=>checkUpdates(),3000);
 setInterval(()=>checkUpdates(),20*60*1000);
 window.addEventListener("green-ion-link",()=>{
  if(active && releaseReady(active.release) && !el("v14UpdateDialog").hidden)return;
  if(active && releaseReady(active.release) && sessionStorage.getItem("green-ion-v14-later")!==active.release.tag_name)showUpdate(active.release);
 });
}
