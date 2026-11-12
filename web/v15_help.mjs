/* Searchable offline documentation assistant. It does not invent answers,
 * connect to an LLM, or send flight/GPS/account data to a search provider. */
export const TOPICS=Object.freeze([
 {title:"Run one-button full preflight",terms:"tests sensor check preflight ready launch automatic all",tab:"testing",id:"homeVerificationCard",
  guide:"Remove all motors, secure payload, attach radio antennas and safe power. Select RUN FULL PREFLIGHT. Real tests run automatically; physical checks that cannot be measured are marked NOT VERIFIED. Review the report, then complete remaining inspections."},
 {title:"Find the rocket after landing",terms:"where gps map coordinates recovery locate lost track position",tab:"recovery",id:"v15RecoveryCard",
  guide:"Open Recovery. Read the last observed GPS fix and its age, confirm whether LoRa is still receiving, and open the map link when internet is available. The last position may not be the landing position."},
 {title:"Connect Arduino Uno LoRa",terms:"radio com port lora connection serial usb receiver",tab:"connections",id:"v14SerialStatus",
  guide:"On Windows, connect the Uno using USB and close Arduino Serial Monitor. OPEN_GROUND_STATION.cmd now auto-detects a recognizable Uno when unambiguous. If needed, run it with --port COM5. The local server reads radio independently; do not also open the same port with Web Serial."},
 {title:"Join Feather Wi-Fi",terms:"wifi wireless connect feather ip access point",tab:"connections",id:"v14WifiStatusDetail",
  guide:"On Windows join the GREEN-ION-V14 access point. Open http://127.0.0.1:8765 after starting OPEN_GROUND_STATION.cmd, or use the Feather fallback at http://192.168.4.1. This access point does not provide internet."},
 {title:"Save flights to the computer",terms:"download file local disk archive storage flight saved",tab:"history",id:"v15LocalCard",
  guide:"Run OPEN_GROUND_STATION.cmd. After landing, reconnect Feather Wi-Fi; completed BIN/SEN/TXT files import automatically into IndexedDB and are SHA-256 mirrored under ~/GREEN_ION_Data/flights. Click Reconcile PC archive to restore originals from disk."},
 {title:"Cloud backup after internet returns",terms:"supabase cloud sync offline internet upload automatic account",tab:"cloud",id:"v14CloudAccount",
  guide:"Sign in once to GREEN ION Cloud and keep Automatic cloud sync enabled. Originals remain local when internet is unavailable. On internet return, the browser automatically retries private checksummed backups; verify the reported upload status. The dashboard tab must be open for browser sync."},
 {title:"Advanced analysis and postflight reports",terms:"graphs apogee health velocity g-force analysis report data quality",tab:"history",id:"v15ReportCard",
  guide:"In History select an original BIN and choose Analyze + health report. The browser worker uses its corresponding SEN file when available. Export the JSON health report; motion estimates and data-quality limitations are labeled."},
 {title:"Plan battery endurance",terms:"lipo battery forecast capacity remaining time fly record voltage",tab:"system",id:"v15BatteryCard",
  guide:"Enter tested LiPo capacity, measured operating current and the required operation/recovery minutes. Battery voltage alone cannot establish remaining runtime. A positive estimate is not a launch clearance."},
 {title:"Check LoRa link quality",terms:"rssi snr packets loss duplicate signal radio range",tab:"connections",id:"v15LinkCard",
  guide:"Open Connections. Compare simultaneous Wi-Fi and LoRa freshness, receiver-side packet counts, estimated gaps and RSSI. Actual RF range must be tested separately. LoRa never proves SD recording."},
 {title:"Analyze IMU and GPS recordings",terms:"sen gis1 accelerometer gyroscope sensor gps analysis",tab:"history",id:"v14SensorArchiveCard",
  guide:"Choose a saved GIS1 SEN file in History, select Analyze sensor file, change the measurement, or export verified sensor CSV/GPS GeoJSON."},
 {title:"Test microSD card",terms:"sd error recording test verify card",tab:"storage",id:"sdtest",
  guide:"With recording inactive and the Feather connected over Wi-Fi, use Detect & Test SD Card. It performs temporary write/read/delete verification without erasing saved flights."},
 {title:"Update or roll back Feather firmware",terms:"ota firmware header dashboard.h update new version flash rollback",tab:"system",id:"installFirmware",
  guide:"Bench only: remove rocket motors, finish recording, connect stable USB power and charged LiPo. Use the verified main .ino.bin app image, not ZIP or merged image. Every included .h file and the onboard dashboard are compiled into that application. Recheck hardware after reboot."},
 {title:"Use the Windows local server",terms:"launcher computer browser server localhost ground station windows",tab:"connections",id:"v14ClientInfo",
  guide:"Run Ground_Station/OPEN_GROUND_STATION.cmd. It starts http://127.0.0.1:8765, serves the current blue website, proxies only the verified Feather API, and receives Uno LoRa serial. Keep the command window open."},
 {title:"Restore local flight originals",terms:"restore recover indexeddb archive backup files",tab:"history",id:"v15LocalCard",
  guide:"Use Reconcile PC archive to import SHA-256 verified BIN/SEN/TXT originals from the Windows disk folder. You can also import saved originals through History. Browser storage may be evicted; keep disk backups."},
 {title:"3D flight and digital twin",terms:"3d simulation trajectory digital twin modeling future",tab:"history",id:"v15ReportCard",
  guide:"These are future-only features. The design calls for optional computer/cloud processing of original BIN/SEN/GPS data; no 3D simulation runs on the Feather and no such visualization is currently installed."}
]);
export function searchHelp(query){
 const words=String(query||"").toLowerCase().match(/[a-z0-9.]+/g)||[];
 if(!words.length)return TOPICS.slice(0,6);
 return TOPICS.map(item=>{
  const title=item.title.toLowerCase(),hay=(item.title+" "+item.terms+" "+item.guide).toLowerCase();
  const score=words.reduce((n,w)=>n+(title.includes(w)?4:0)+(item.terms.includes(w)?3:0)+(hay.includes(w)?1:0),0);
  return {item,score};
 }).filter(x=>x.score>0).sort((a,b)=>b.score-a.score).slice(0,6).map(x=>x.item);
}
const $=id=>document.getElementById(id);
function showLocation(topic){
 const tab=document.querySelector('[data-tab="'+topic.tab+'"]');
 if(tab)tab.click();
 const target=$(topic.id)||$(topic.tab);
 if(target){
  target.scrollIntoView({block:"center",behavior:"smooth"});
  target.classList.add("v15-help-highlight");
  setTimeout(()=>target.classList.remove("v15-help-highlight"),2500);
 }
}
function render(query){
 const target=$("v15HelpResults");target.replaceChildren();
 const results=searchHelp(query);
 $("v15HelpStatus").textContent=results.length?
  "Offline guide — "+results.length+" relevant topic(s). No internet required.":
  "No matching GREEN ION guide. Try 'LoRa', 'preflight', 'cloud', 'GPS', or 'firmware'.";
 for(const item of results){
  const box=document.createElement("div");box.className="card";
  const heading=document.createElement("h3");heading.textContent=item.title;
  const guide=document.createElement("p");guide.textContent=item.guide;
  const button=document.createElement("button");button.className="secondary";button.textContent="Show where in the dashboard";
  button.onclick=()=>showLocation(item);
  box.append(heading,guide,button);target.append(box);
 }
}
export function initHelp(){
 $("v15HelpSearch").onkeydown=e=>{if(e.key==="Enter"){e.preventDefault();render(e.target.value);}};
 $("v15HelpAsk").onclick=()=>render($("v15HelpSearch").value);
 render("");
}
