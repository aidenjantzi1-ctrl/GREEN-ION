/* GREEN ION V16 five-tab interface organizer.
 * Existing feature cards keep their IDs and logic. Legacy pages remain as
 * hidden compatibility mounts while their contents are moved into five visible
 * top-level tabs after the old dashboard setup code has initialized. */
const $=id=>document.getElementById(id);

export const V16_TABS=Object.freeze([
 ["status","Dashboard"],
 ["flight","Flight"],
 ["history","Flights & Analysis"],
 ["testing","Testing"],
 ["system","Settings & Advanced"]
]);

export const LEGACY_TAB_MAP=Object.freeze({
 status:"status",
 flight:"flight",field:"flight",recovery:"flight",
 storage:"history",history:"history",
 testing:"testing",
 utilities:"system",system:"system",connections:"system",
 cloud:"system",updates:"system",help:"system"
});

export function v16TabFor(name){return LEGACY_TAB_MAP[String(name||"")]||"status";}

function details(title,id,open=false){
 const node=document.createElement("details");
 node.className="v16-section-group card";
 node.id=id;
 node.open=open;
 const summary=document.createElement("summary");
 summary.textContent=title;
 node.append(summary);
 return node;
}
function moveChildren(source,target,{prepend=false}={}){
 if(!source||!target||source===target)return;
 const nodes=Array.from(source.childNodes);
 if(prepend){
  const marker=target.firstChild;
  for(const node of nodes)target.insertBefore(node,marker);
 }else for(const node of nodes)target.append(node);
 source.hidden=true;
 source.classList.remove("active");
}
function groupChildren(source,title,id){
 if(!source)return null;
 const nodes=Array.from(source.childNodes);
 if(!nodes.length)return null;
 const group=details(title,id,false);
 for(const node of nodes)group.append(node);
 return group;
}
function organizeDashboard(){
 const target=$("status");if(!target)return;
 const advanced=details("Detailed live telemetry & graphs","v16DashboardDetails",false);
 const metrics=target.querySelector(".metrics");
 if(metrics){
  const cards=Array.from(metrics.children);
  // Keep launch-day Dashboard concise: altitude, battery, vertical speed and
  // peak stay visible; environment/raw detail lives behind one disclosure.
  for(const card of cards.slice(4))advanced.append(card);
 }
 for(const id of ["graphOptionsCard","altGraphCard","pressureGraphCard","tempGraphCard","v14SensorLabCard"]){
  const node=$(id);if(node)advanced.append(node);
 }
 if(advanced.children.length>1)target.append(advanced);
}
function organizeFlight(){
 const target=$("flight");if(!target)return;
 const mission=$("v16MissionCard");
 if(mission)target.prepend(mission);
 const profile=$("flightProfileCard");
 if(profile&&mission)mission.after(profile);
 const field=groupChildren($("field"),"Field controls","v16FieldControls");
 if(field)target.append(field);
 moveChildren($("recovery"),target);
}
function organizeAnalysis(){
 const target=$("history");if(!target)return;
 const source=$("storage");
 const sdHealth=$("sdtest")?.closest(".card");
 const sdManager=$("fileList")?.closest(".card");
 const flightSummary=$("flightSummaryCard");
 if(sdHealth){sdHealth.id="v16SdHealthCard";sdHealth.classList.add("v16-support-card");}
 if(sdManager){sdManager.id="v16SdManagerCard";sdManager.classList.add("v16-primary-card");}
 if(flightSummary)flightSummary.classList.add("v16-primary-card");
 moveChildren(source,target,{prepend:true});
 // Keep destructive SD actions easy to find, then local backups, then analysis.
 if(sdManager)target.prepend(sdManager);
 if(sdHealth&&sdManager)sdManager.after(sdHealth);
}
function organizeSettings(){
 const target=$("system");if(!target)return;
 const existing=Array.from(target.childNodes);
 const general=details("Profiles, sensors & recording","v16SettingsGeneral",false);
 for(const node of existing)general.append(node);
 target.append(general);
 const groups=[
  ["utilities","Utilities & controls","v16SettingsUtilities"],
  ["connections","Connections, radio & diagnostics","v16SettingsConnections"],
  ["cloud","Cloud backup","v16SettingsCloud"],
  ["updates","Firmware & app updates","v16SettingsUpdates"],
  ["help","Help & troubleshooting","v16SettingsHelp"]
 ];
 for(const [sourceId,title,id] of groups){
  const group=groupChildren($(sourceId),title,id);
  if(group)target.append(group);
  const source=$(sourceId);if(source){source.hidden=true;source.classList.remove("active");}
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
 if(scrollId)setTimeout(()=>$(scrollId)?.scrollIntoView({behavior:"smooth",block:"start"}),0);
}
export function initLayout(){
 organizeDashboard();organizeFlight();organizeAnalysis();organizeSettings();
 for(const id of ["field","storage","utilities","connections","recovery","cloud","updates","help"]){
  const source=$(id);if(source){source.hidden=true;source.classList.remove("active");}
 }
 const prepare=$("v16DashboardPrepare");
 if(prepare)prepare.onclick=()=>openV16Tab("flight",{scrollId:"v16MissionCard"});
 const finder=$("v161DashboardFind");
 if(finder)finder.onclick=()=>openV16Tab("recovery",{scrollId:"v15RecoveryCard"});
 window.addEventListener("green-ion-v16-mission",renderSummary);
 window.addEventListener("green-ion-v16-telemetry",renderSummary);
 window.addEventListener("green-ion-link",renderSummary);
 renderSummary();
}
