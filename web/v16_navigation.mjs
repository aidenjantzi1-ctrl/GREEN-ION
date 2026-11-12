/* GREEN ION V16 five-tab shell.
 * Keeps legacy feature pages reachable as contextual subpages without
 * exposing thirteen top-level tabs. */
const GROUPS=Object.freeze({
 status:{label:"Mission",pages:[["status","Overview"],["flight","Flight"],["testing","Preflight"]]},
 connections:{label:"Live",pages:[["connections","Links"],["field","Field"]]},
 recovery:{label:"Recovery",pages:[["recovery","Locate"]]},
 history:{label:"Data",pages:[["history","History"],["storage","SD Files"],["cloud","Cloud"]]},
 system:{label:"System",pages:[["system","Device"],["utilities","Utilities"],["updates","Updates"],["help","Help"]]}
});
const PAGE_TO_GROUP=Object.fromEntries(Object.entries(GROUPS).flatMap(([group,data])=>data.pages.map(([page])=>[page,group])));
const $=id=>document.getElementById(id);

function openLegacy(page){
 const hidden=document.querySelector('#v16LegacyTabs [data-tab="'+page+'"]');
 if(hidden){hidden.click();return true;}
 const top=document.querySelector('.tabs [data-tab="'+page+'"]');
 if(top){top.click();return true;}
 return false;
}
function selector(group){
 const data=GROUPS[group],wrap=document.createElement("div");
 wrap.className="card v16-subnav";
 wrap.setAttribute("aria-label",data.label+" tools");
 const title=document.createElement("h2");title.textContent=data.label+" tools";
 const p=document.createElement("p");p.className="muted";
 p.textContent="Focused shortcuts. Detailed tools stay here without adding more top-level tabs.";
 const actions=document.createElement("div");actions.className="actions";
 for(const [page,label] of data.pages){
  const b=document.createElement("button");b.className="secondary";
  b.textContent=label;b.dataset.v16Subpage=page;b.onclick=()=>openLegacy(page);
  actions.append(b);
 }
 wrap.append(title,p,actions);
 return wrap;
}
function paint(page){
 const group=PAGE_TO_GROUP[page]||page;
 document.body.dataset.v16Group=group;
 document.body.dataset.v16Page=page;
 document.querySelectorAll(".v16-subnav button").forEach(button=>{
  button.classList.toggle("active",button.dataset.v16Subpage===page);
 });
}
export function initV16Navigation(){
 for(const group of Object.keys(GROUPS)){
  const page=$(group);if(!page)continue;
  if(!page.querySelector(".v16-subnav"))page.prepend(selector(group));
 }
 window.addEventListener("green-ion-v16-page",e=>paint(e.detail?.page||"status"));
 document.addEventListener("keydown",event=>{
  if(!(event.altKey&&/^[1-5]$/.test(event.key)))return;
  const order=["status","connections","recovery","history","system"];
  const target=document.querySelector('.tabs [data-tab="'+order[Number(event.key)-1]+'"]');
  if(target){event.preventDefault();target.click();target.focus();}
 });
 paint(document.querySelector(".page.active")?.id||"status");
}
