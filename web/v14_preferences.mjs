/* GREEN ION optional features. Removing this module restores default features. */
const KEY="green-ion-v14-preferences";
export const DEFAULTS=Object.freeze({sensorLab:true,sensorArchive:true,cloudAuto:true,compact:false,imperial:false});
const listeners=new Set();
function read(){
  try{
    const obj=JSON.parse(localStorage.getItem(KEY)||"{}");
    return Object.fromEntries(Object.keys(DEFAULTS).map(k=>[k,typeof obj?.[k]==="boolean"?obj[k]:DEFAULTS[k]]));
  }catch{return {...DEFAULTS};}
}
let state=null;
export function getPreferences(){return {...(state??=read())};}
export function preference(name){return getPreferences()[name]??false;}
export function setPreference(name,value){
  if(!(name in DEFAULTS))throw Error("Unknown GREEN ION preference");
  state={...getPreferences(),[name]:!!value};
  try{localStorage.setItem(KEY,JSON.stringify(state));}catch{/* private browsing */}
  for(const callback of listeners)callback(getPreferences());
  return getPreferences();
}
export function onPreferences(callback){listeners.add(callback);return ()=>listeners.delete(callback);}
export function applyPreferences(doc=document){
  const p=getPreferences();
  doc.body.dataset.compact=String(p.compact);
  doc.body.dataset.imperial=String(p.imperial);
  const lab=doc.getElementById("v14SensorLabCard");
  if(lab)lab.hidden=!p.sensorLab;
  for(const [key,id] of Object.entries({
    sensorLab:"v14PrefSensorLab",sensorArchive:"v14PrefSensorArchive",cloudAuto:"v14PrefCloudAuto",
    compact:"v14PrefCompact",imperial:"v14PrefImperial"
  })){
    const input=doc.getElementById(id);if(input)input.checked=p[key];
  }
}
export function initPreferences(doc=document){
  const map={
    v14PrefSensorLab:"sensorLab",v14PrefSensorArchive:"sensorArchive",v14PrefCloudAuto:"cloudAuto",
    v14PrefCompact:"compact",v14PrefImperial:"imperial"
  };
  for(const [id,key] of Object.entries(map)){
    const input=doc.getElementById(id);if(input)
      input.addEventListener("change",()=>setPreference(key,input.checked));
  }
  onPreferences(()=>applyPreferences(doc));
  applyPreferences(doc);
}
