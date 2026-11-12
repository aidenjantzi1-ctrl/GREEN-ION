/* GREEN ION V16 Windows runtime diagnostics. */
const $=id=>document.getElementById(id);
const t=window.GREEN_ION_TRANSPORT;
const REPO="aidenjantzi1-ctrl/GREEN-ION";
async function load(){
 const out=$("v16RuntimeStatus");if(!out)return;
 if(!t.localBridge){out.textContent="Hosted/browser mode · Windows package diagnostics unavailable here.";return;}
 out.textContent="Checking local Windows runtime...";
 try{
  const r=await t.nativeFetch("/api/app/health",{cache:"no-store"});
  if(!r.ok)throw Error("HTTP "+r.status);
  const d=await r.json();
  out.textContent="V"+d.version+" · "+String(d.mode||"unknown").toUpperCase()+
   (d.receiver_port?" · Uno "+d.receiver_port:" · no Uno selected");
  $("v16RuntimeData").textContent=d.data_root||"Unknown";
  $("v16RuntimePython").textContent="Python "+(d.python||"?")+" · user data outside app: "+(d.user_data_outside_app?"YES":"NO");
 }catch(e){out.textContent="Runtime diagnostics unavailable: "+e.message;}
}
async function checkWindowsRelease(){
 const node=$("v161WindowsUpdate");if(!node)return;
 if(!t.localBridge){node.textContent="Windows package updates apply only to the local Windows app.";return;}
 node.textContent="Checking official V16 releases...";
 try{
  const r=await t.nativeFetch("https://api.github.com/repos/"+REPO+"/releases?per_page=15",{
   cache:"no-store",headers:{"Accept":"application/vnd.github+json"}
  });
  if(!r.ok)throw Error("GitHub HTTP "+r.status);
  const releases=(await r.json()).filter(item=>!item.draft&&/^v16\./i.test(item.tag_name||"")&&
   item.assets?.some(a=>a.name==="GREEN_ION_V16_WINDOWS.zip")&&
   item.assets?.some(a=>a.name==="GREEN_ION_V16_WINDOWS.sha256"));
  if(!releases.length){node.textContent="No official V16 Windows package release published yet.";return;}
  const latest=releases[0];
  const note=String(latest.body||"").replace(/[\r\n]+/g," ").replace(/[\*_>#]+/g," ").trim().slice(0,180);
  node.textContent="Latest: "+latest.tag_name+(latest.name?" · "+latest.name:"")+(note?" · "+note:"")+
   " · use UPDATE_GREEN_ION.cmd to verify and install; ROLLBACK_GREEN_ION.cmd restores the prior app.";
 }catch(e){node.textContent="Update check unavailable: "+e.message+". Current app still works offline.";}
}
export function initV16Runtime(){
 const system=$("system");if(!system)return;
 if(!$("v16RuntimeCard")){
  const card=document.createElement("div");card.className="card";card.id="v16RuntimeCard";
  card.innerHTML='<h2>Windows app runtime</h2>'+
   '<p class="muted">The application folder is replaceable. Flight data stays in GREEN_ION_Data.</p>'+
   '<div class="row"><span>Runtime</span><strong id="v16RuntimeStatus">Checking...</strong></div>'+
   '<div class="row"><span>Data directory</span><strong id="v16RuntimeData">--</strong></div>'+
   '<div class="row"><span>Python / separation</span><strong id="v16RuntimePython">--</strong></div>'+
   '<div class="row"><span>Windows app update</span><strong>UPDATE_GREEN_ION.cmd</strong></div>'+
   '<div class="row"><span>Windows app rollback</span><strong>ROLLBACK_GREEN_ION.cmd</strong></div>'+
   '<p id="v161WindowsUpdate" class="muted">Checking official V16 Windows releases...</p>'+
   '<div class="actions"><button id="v16RuntimeRefresh" class="secondary">Refresh runtime</button>'+
   '<button id="v16RuntimeUpdates" class="secondary">Feather firmware tools</button></div>';
  system.prepend(card);
 }
 $("v16RuntimeRefresh").onclick=load;
 $("v16RuntimeUpdates").onclick=()=>document.querySelector('#v16LegacyTabs [data-tab="updates"]')?.click();
 load();checkWindowsRelease();
}
