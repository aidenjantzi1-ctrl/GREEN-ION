/* GREEN ION V16 Windows runtime diagnostics. */
const $=id=>document.getElementById(id);
const t=window.GREEN_ION_TRANSPORT;
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
export function initV16Runtime(){
 const system=$("system");if(!system)return;
 if(!$("v16RuntimeCard")){
  const card=document.createElement("div");card.className="card";card.id="v16RuntimeCard";
  card.innerHTML='<h2>Windows app runtime</h2>'+
   '<p class="muted">The application folder is replaceable. Flight data stays in GREEN_ION_Data.</p>'+
   '<div class="row"><span>Runtime</span><strong id="v16RuntimeStatus">Checking...</strong></div>'+
   '<div class="row"><span>Data directory</span><strong id="v16RuntimeData">--</strong></div>'+
   '<div class="row"><span>Python / separation</span><strong id="v16RuntimePython">--</strong></div>'+
   '<div class="actions"><button id="v16RuntimeRefresh" class="secondary">Refresh runtime</button>'+
   '<button id="v16RuntimeUpdates" class="secondary">Update tools</button></div>';
  system.prepend(card);
 }
 $("v16RuntimeRefresh").onclick=load;
 $("v16RuntimeUpdates").onclick=()=>document.querySelector('#v16LegacyTabs [data-tab="updates"]')?.click();
 load();
}
