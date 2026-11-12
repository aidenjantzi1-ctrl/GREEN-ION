/* GREEN ION V16 five-tab interface organizer.
 * Existing feature cards keep their IDs and logic; this module moves them into
 * a simpler top-level information architecture after legacy scripts initialize. */
const $=id=>document.getElementById(id);

export const V16_TABS=Object.freeze([
 ["dashboard","Dashboard"],
 ["flight","Flight"],
 ["analysis","Flights & Analysis"],
 ["testing","Testing"],
 ["settings","Settings & Advanced"]
]);

export const LEGACY_TAB_MAP=Object.freeze({
 status:"dashboard",flight:"flight",field:"flight",recovery:"flight",
 storage:"analysis",history:"analysis",testing:"testing",
 utilities:"settings",system:"settings",connections:"settings",
 cloud:"settings",updates:"settings",help:"settings",
 dashboard:"dashboard",analysis:"analysis",settings:"settings"
});

export function v16TabFor(name){return LEGACY_TAB_MAP[String(name||"")]||"dashboard";}

function details(title,id,open=false){
 const node=document.createElement("details");
 node.className="v16-section-group";
 node.id=id;
 node.open=open;
 const summary=document.createElement("summary");
 summary.textContent=title;
 node.append(summary);
 return node;
}
function moveChildren(source,target){
 if(!source||!target)return;
 while(source.firstChild)target.append(source.firstChild);
}
function moveStatus(){
 const source=$("status"),target=$("dashboard");
 if(!source||!target)return;
 const advanced=details("Detailed live telemetry & graphs","v16DashboardDetails",false);
 for(const child of Array.from(source.children)){
  if(child.classList?.contains("metrics"))target.append(child);
  else advanced.append(child);
 }
 if(advanced.children.length>1)target.append(advanced);
}
function moveFlight(){
 const target=$("flight");
 if(!target)return;
 moveChildren($("legacy-flight"),target);
 const field=$("field");
 if(field?.children.length){
  const group=details("Field controls","v16FieldControls");
  moveChildren(field,group);target.append(group);
 }
 moveChildren($("recovery"),target);
}
function moveAnalysis(){
 const target=$("analysis");if(!target)return;
 moveChildren($("storage"),target);
 moveChildren($("history"),target);
}
function moveTesting(){moveChildren($("testing-source"),$("testing"));}
function moveSettings(){
 const target=$("settings");if(!target)return;
 const groups=[
  ["utilities","Utilities & controls","v16SettingsUtilities"],
  ["system","Profiles, sensors & recording","v16SettingsSystem"],
  ["connections","Connections, radio & diagnostics","v16SettingsConnections"],
  ["cloud","Cloud backup","v16SettingsCloud"],
  ["updates","Firmware & app updates","v16SettingsUpdates"],
  ["help","Help & troubleshooting","v16SettingsHelp"]
 ];
 for(const [sourceId,title,id] of groups){
  const source=$(sourceId);if(!source?.children.length)continue;
  const group=details(title,id,false);moveChildren(source,group);target.append(group);
 }
}
function removeSources(){
 for(const id of ["status","legacy-flight","field","testing-source","utilities","storage","system",
  "connections","recovery","history","cloud","updates","help"]){
  const node=$(id);if(node?.classList.contains("v16-legacy-page"))node.remove();
 }
}
function renderSummary(){
 const telemetry=window.GREEN_ION_UNIFIED_TELEMETRY;
 const mission=window.GREEN_ION_MISSION;
 const wifi=window.GREEN_ION_TRANSPORT?.state?.lastWifiStatus||{};
 const set=(id,value)=>{const n=$(id);if(n)n.textContent=value;};
 set("v16DashMission",mission?.state||"IDLE");
 set("v16DashRocket",(mission?.rocket||telemetry?.identity?.rocket||wifi.selected_rocket||"Not selected")+
   " · "+(mission?.motor||telemetry?.identity?.motor||wifi.selected_motor||"Motor not selected"));
 set("v16DashLinks",telemetry?String(telemetry.mode||"none").toUpperCase():"WAITING");
 const gps=telemetry?.readings?.gps_status;
 set("v16DashGps",gps?(String(gps.value||"missing").toUpperCase()+" · "+String(gps.source||"none")):"MISSING");
 set("v16DashRecorder",wifi.recording?"RECORDING · "+(wifi.log_file||"SD file"):
   wifi.sd_ok===true?"SD READY":wifi.sd_ok===false?"SD NOT READY":"UNKNOWN");
}
export function openV16Tab(name,{scrollId=null}={}){
 const target=v16TabFor(name);
 const button=document.querySelector('nav.tabs [data-tab="'+target+'"]');
 if(button)button.click();
 if(scrollId)setTimeout(()=>$ (scrollId)?.scrollIntoView({behavior:"smooth",block:"start"}),0);
}
export function initLayout(){
 // Inline legacy setup runs before this module, so all cards are already in
 // their final legacy source sections and can be moved without cloning IDs.
 moveStatus();moveFlight();moveAnalysis();moveTesting();moveSettings();removeSources();
 const prepare=$("v16DashboardPrepare");
 if(prepare)prepare.onclick=()=>openV16Tab("flight",{scrollId:"v16MissionCard"});
 window.addEventListener("green-ion-v16-mission",renderSummary);
 window.addEventListener("green-ion-v16-telemetry",renderSummary);
 window.addEventListener("green-ion-link",renderSummary);
 renderSummary();
}
