/* Optional GREEN ION Sensor Lab. Read-only and removable in Settings. */
import {SENSOR_GROUPS,sensorRows,SENSOR_KEYS} from "./v14_sensor_catalog.mjs";
import {initV163LiveGraphs} from "./v16_3_live_graphs.mjs";
import {preference,onPreferences} from "./v14_preferences.mjs";
const $=id=>document.getElementById(id);
const groups=new Map(),samples=[];
let snapshot=null,lastSample=0,lastPaint=0,queued=false;
let peaks={alt:-Infinity,g:-Infinity,battery:Infinity};
function groupEl(group,index){
 const details=document.createElement("details");details.className="v14-sensor-group";details.open=index<2;
 const title=document.createElement("summary");title.textContent=group.name;details.append(title);
 const entries=document.createElement("div");entries.className="v14-sensor-rows";details.append(entries);
 const cells=new Map();
 for(const [key,label] of group.fields){
  const row=document.createElement("div");row.className="row";
  const name=document.createElement("span");name.textContent=label;
  const value=document.createElement("strong");value.textContent="—";value.dataset.sensorKey=key;
  row.append(name,value);entries.append(row);cells.set(key,value);
 }
 groups.set(group.name,cells);return details;
}
function getLive(t){
 const source=t.source();
 if(source==="wifi"&&t.wifiFresh())return {source:"wifi",status:t.state.lastStatus??{},ageMs:Date.now()-(t.state.lastStatusAt||Date.now())};
 if(source==="lora"&&t.radioFresh())return {source:"lora",status:t.statusFromRadio(),ageMs:Date.now()-t.state.radioAt};
 return {source:"offline",status:{},ageMs:null};
}
function observe(view){
 if(view.source==="offline"||Date.now()-lastSample<1000)return;
 lastSample=Date.now();
 const altitude=view.status.relative_altitude_m??view.status.altitude_m;
 const g=view.status.acceleration_g,bat=view.status.battery_v;
 if(typeof altitude==="number"&&Number.isFinite(altitude))peaks.alt=Math.max(peaks.alt,altitude);
 if(typeof g==="number"&&Number.isFinite(g))peaks.g=Math.max(peaks.g,g);
 if(typeof bat==="number"&&Number.isFinite(bat))peaks.battery=Math.min(peaks.battery,bat);
 samples.push({time:new Date().toISOString(),source:view.source,...Object.fromEntries(
  Object.entries(view.status).filter(([k,v])=>SENSOR_KEYS.has(k)&&v!==null&&v!==undefined&&
   (typeof v==="string"||typeof v==="boolean"||typeof v==="number"&&Number.isFinite(v))))});
 if(samples.length>180)samples.splice(0,samples.length-180);
}
function update(t){
 if(!preference("sensorLab")||document.visibilityState==="hidden")return;
 const view=getLive(t),rows=sensorRows(view.status,view.source==="lora"?t.state.radio:null,preference("imperial"));
 snapshot={view,rows};observe(view);
 $("v14SensorSource").textContent="Source: "+(view.source==="wifi"?"Feather Wi-Fi":view.source==="lora"?"Uno/LoRa receive-only":"OFFLINE / no live reading")+
  (view.ageMs===null?"":" · "+(view.ageMs/1000).toFixed(1)+" s old");
 for(const group of rows){
  const cells=groups.get(group.name);if(!cells)continue;
  for(const row of group.rows){
   const node=cells.get(row.key);if(!node)continue;
   if(node.textContent!==row.formatted)node.textContent=row.formatted;
   node.dataset.available=String(row.present);
  }
 }
 $("v14SensorPeaks").textContent="Session: peak altitude "+
  (Number.isFinite(peaks.alt)?peaks.alt.toFixed(1)+" m":"—")+
  " · peak G "+(Number.isFinite(peaks.g)?peaks.g.toFixed(2)+" g":"—")+
  " · minimum battery "+(Number.isFinite(peaks.battery)?peaks.battery.toFixed(2)+" V":"—");
 lastPaint=Date.now();
}
function exportJSON(value){
 const url=URL.createObjectURL(new Blob([JSON.stringify(value,null,2)],{type:"application/json"}));
 const a=document.createElement("a");a.href=url;a.download="green-ion-sensor-snapshot.json";
 document.body.append(a);a.click();a.remove();setTimeout(()=>URL.revokeObjectURL(url),60000);
}
export function initSensorLab(t){
 const root=$("v14SensorGroups");if(!root)return;
 initV163LiveGraphs(t);
 for(const [index,group] of SENSOR_GROUPS.entries())root.append(groupEl(group,index));
 const schedule=()=>{
  if(queued)return;queued=true;
  const paint=()=>{queued=false;update(t);};
  if(typeof requestAnimationFrame==="function")requestAnimationFrame(paint);else paint();
 };
 $("v14SensorExport").onclick=()=>{
  const value=snapshot;
  if(!value||value.view.source==="offline"){
   $("v14SensorExportStatus").textContent="No live readings to export. Saved flights remain in History.";
   return;
  }
  exportJSON({format:"GREEN-ION-LIVE-SENSORS-1",exportedAt:new Date().toISOString(),
   source:value.view.source,ageMs:value.view.ageMs,
   readings:Object.fromEntries(value.rows.flatMap(g=>g.rows.filter(r=>r.present).map(r=>[r.key,r.raw]))),
   observed:samples.slice(),note:"Derived flight motion is estimated; radio telemetry is receive-only."});
  $("v14SensorExportStatus").textContent="Exported "+samples.length+" recent observed samples.";
 };
 $("v14SensorResetPeaks").onclick=()=>{peaks={alt:-Infinity,g:-Infinity,battery:Infinity};samples.length=0;schedule();};
 onPreferences(schedule);window.addEventListener("green-ion-link",schedule);
 document.addEventListener("visibilitychange",schedule);
 setInterval(()=>{
  if(!preference("sensorLab")||document.visibilityState==="hidden")return;
  if(document.querySelector('nav.tabs button[data-tab="status"].active')&&Date.now()-lastPaint>2400)schedule();
 },2500);
 schedule();
}
