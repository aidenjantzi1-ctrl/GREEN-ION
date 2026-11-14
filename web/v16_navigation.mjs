/* GREEN ION V16 primary navigation.
 * Exactly five visible top-level destinations. Legacy data-tab aliases remain
 * hidden only for old code paths and are mapped by the inline openTab helper. */
export const PRIMARY_TABS=Object.freeze([
 ["status","Dashboard"],
 ["flight","Flight"],
 ["history","Flights & Analysis"],
 ["testing","Testing"],
 ["system","Settings & Advanced"]
]);

export function primaryTabForLegacy(page){
 const map={
  status:"status",
  flight:"flight",field:"flight",recovery:"flight",
  storage:"history",history:"history",
  testing:"testing",
  utilities:"system",system:"system",connections:"system",
  cloud:"system",updates:"system",help:"system"
 };
 return map[String(page||"")]||"status";
}

function paint(page){
 const primary=primaryTabForLegacy(page);
 document.body.dataset.v16Group=primary;
 document.body.dataset.v16Page=primary;
 document.querySelectorAll("nav.tabs button").forEach(button=>{
  const active=button.dataset.tab===primary;
  button.classList.toggle("active",active);
  button.setAttribute("aria-selected",active?"true":"false");
 });
}

export function initV16Navigation(){
 const openHash=()=>{
  const page=String(location.hash||"").replace(/^#/,"");
  if(!PRIMARY_TABS.some(([id])=>id===page))return;
  const target=document.querySelector('nav.tabs [data-tab="'+page+'"]');
  if(target&&!target.classList.contains("active"))target.click();
 };
 window.addEventListener("green-ion-v16-page",e=>paint(e.detail?.page||"status"));
 window.addEventListener("hashchange",openHash);
 document.addEventListener("keydown",event=>{
  if(!(event.altKey&&/^[1-5]$/.test(event.key)))return;
  const item=PRIMARY_TABS[Number(event.key)-1];
  const target=item&&document.querySelector('nav.tabs [data-tab="'+item[0]+'"]');
  if(target){event.preventDefault();target.click();target.focus();}
 });
 paint(document.querySelector(".page.active")?.id||"status");
 queueMicrotask(openHash);
}
