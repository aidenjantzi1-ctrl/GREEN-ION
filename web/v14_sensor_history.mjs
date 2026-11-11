/* Optional GIS1 historical sensor chart: local Web Worker and manual exports only. */
import {getLocalFile,listLocalFiles} from "./v14_store.mjs";
import {SENSOR_GRAPH_METRICS,crc32} from "./analysis/gis1.mjs";
import {preference,onPreferences} from "./v14_preferences.mjs";
const $=id=>document.getElementById(id);
let active=null,cache=new Map();
function note(value){$("v14SensorArchiveStatus").textContent=value;}
function download(name,data,type){
 const url=URL.createObjectURL(new Blob([data],{type}));
 const a=document.createElement("a");a.href=url;a.download=name;
 document.body.append(a);a.click();a.remove();setTimeout(()=>URL.revokeObjectURL(url),60000);
}
async function parseRecord(record){
 if(cache.has(record.key))return cache.get(record.key);
 const result=await new Promise((resolve,reject)=>{
  const worker=new Worker("./sensor-worker.mjs",{type:"module"});
  const timer=setTimeout(()=>{worker.terminate();reject(Error("GIS1 decoder timed out"));},60000);
  worker.onerror=e=>{clearTimeout(timer);worker.terminate();reject(Error(e.message||"Sensor worker failed"));};
  worker.onmessage=e=>{
   clearTimeout(timer);worker.terminate();
   e.data.error?reject(Error(e.data.error)):resolve(e.data);
  };
  const bytes=record.bytes.slice(0);worker.postMessage({id:1,name:record.name,bytes},[bytes]);
 });
 cache.set(record.key,result);if(cache.size>3)cache.delete(cache.keys().next().value);
 return result;
}
function draw(){
 const canvas=$("v14SenCanvas"),ctx=canvas.getContext("2d");
 const w=canvas.width,h=canvas.height;
 ctx.fillStyle="#0b1c2b";ctx.fillRect(0,0,w,h);
 if(!active)return;
 const metric=$("v14SenMetric").value,label=SENSOR_GRAPH_METRICS[metric]?.[0]||metric;
 const all=active.data.parsed.samples,percent=+$("v14SenReplay").value||100;
 const count=Math.max(1,Math.ceil(all.length*percent/100)),stride=Math.max(1,Math.ceil(count/2500));
 const pts=[];
 for(let i=0;i<count;i+=stride){
  const s=all[i],v=s[metric];
  if(typeof v==="number"&&Number.isFinite(v))pts.push([s.timeS,v]);
 }
 $("v14SenReplayLabel").textContent=percent+"% of verified records · "+label;
 if(pts.length<2)return;
 let minX=Infinity,maxX=-Infinity,minY=Infinity,maxY=-Infinity;
 for(const [x,y] of pts){minX=Math.min(minX,x);maxX=Math.max(maxX,x);minY=Math.min(minY,y);maxY=Math.max(maxY,y);}
 if(maxX<=minX)maxX=minX+1;if(maxY<=minY){minY-=1;maxY+=1;}
 const p=(maxY-minY)*.08;minY-=p;maxY+=p;
 const fx=x=>50+(x-minX)/(maxX-minX)*(w-70);
 const fy=y=>h-30-(y-minY)/(maxY-minY)*(h-55);
 ctx.strokeStyle="#3a5871";ctx.lineWidth=1;
 for(let i=0;i<=4;i++){const y=20+i*(h-50)/4;ctx.beginPath();ctx.moveTo(50,y);ctx.lineTo(w-20,y);ctx.stroke();}
 ctx.strokeStyle="#7bdbff";ctx.lineWidth=2.2;ctx.beginPath();
 pts.forEach(([x,y],i)=>i?ctx.lineTo(fx(x),fy(y)):ctx.moveTo(fx(x),fy(y)));
 ctx.stroke();ctx.fillStyle="#eff8ff";ctx.font="13px system-ui";
 ctx.fillText(label,55,15);ctx.fillText(minX.toFixed(1)+" s",50,h-8);
 ctx.fillText(maxX.toFixed(1)+" s",w-90,h-8);
 ctx.fillText(minY.toFixed(2),3,h-27);ctx.fillText(maxY.toFixed(2),3,25);
}
export async function refreshSensorHistory(){
 const node=$("v14SenFlight");if(!node)return;
 const existing=node.value;
 const files=(await listLocalFiles()).filter(f=>f.kind==="SEN")
  .sort((a,b)=>String(b.createdAt).localeCompare(String(a.createdAt)));
 node.replaceChildren();
 const blank=document.createElement("option");blank.value="";blank.textContent="Choose a SEN sensor recording";
 node.append(blank);
 for(const f of files){
  const option=document.createElement("option");option.value=f.key;option.textContent=f.name;node.append(option);
 }
 if(files.some(f=>f.key===existing))node.value=existing;
 $("v14SenAnalyze").disabled=!files.length;
 $("v14SenAvailable").textContent=files.length+" GIS1 sensor recording(s) saved locally.";
}
async function analyze(){
 const key=$("v14SenFlight").value;
 if(!key){note("Select a SEN file first.");return;}
 note("Verifying GIS1 CRC32 records in an offline worker...");
 try{
  const record=await getLocalFile(key),data=await parseRecord(record);
  const files=await listLocalFiles(),name=record.name.replace(/\.sen$/i,".BIN");
  const companion=files.find(f=>f.kind==="BIN"&&f.name.toUpperCase()===name.toUpperCase());
  let match="No matching BIN imported (sensor data remains usable).";
  if(companion){
   const prefix=new Uint8Array(companion.bytes).subarray(0,32);
   if(prefix.length!==32||crc32(prefix)!==data.parsed.rkt4HeaderCrc32)
    throw Error("Companion BIN header CRC mismatch. The files may be from different flights.");
   match="Verified associated RKT4 BIN using header CRC32.";
  }
  active={record,data};
  const p=data.parsed,sum=p.summary;
  $("v14SenSummary").textContent=p.decoded+" verified / "+p.count+" record(s), "+
   p.rateHz+" Hz · "+sum.validImu+" valid IMU · "+sum.currentGps+" live GPS records · "+
   "peak total "+sum.peakTotalG.toFixed(2)+" g · max GPS speed "+sum.maxSpeedMps.toFixed(2)+" m/s · "+
   "max satellites "+sum.maxSatellites+". "+match;
  $("v14SenWarnings").textContent=p.warnings.join(" · ")||"All included records passed CRC32 and range validation.";
  $("v14SenReplay").value="100";draw();
  $("v14SenCsv").disabled=false;$("v14SenGps").disabled=!data.geojson.features.length;
  note("Sensor analysis complete. Original SEN and BIN remain unchanged.");
 }catch(e){active=null;$("v14SenCsv").disabled=true;$("v14SenGps").disabled=true;note("Sensor analysis rejected: "+e.message);}
}
export function initSensorHistory(){
 const root=$("v14SensorArchiveCard");if(!root)return;
 root.hidden=!preference("sensorArchive");
 onPreferences(p=>{root.hidden=!p.sensorArchive;});
 $("v14SenAnalyze").onclick=analyze;
 $("v14SenReplay").oninput=draw;
 $("v14SenMetric").onchange=draw;
 $("v14SenCsv").disabled=true;$("v14SenGps").disabled=true;
 $("v14SenCsv").onclick=()=>{
  if(active)download(active.record.name.replace(/\.sen$/i,"-sensors.csv"),active.data.csv,"text/csv");
 };
 $("v14SenGps").onclick=()=>{
  if(active&&active.data.geojson.features.length)
   download(active.record.name.replace(/\.sen$/i,"-gps.geojson"),
    JSON.stringify(active.data.geojson,null,2),"application/geo+json");
 };
 refreshSensorHistory().catch(e=>note(e.message));
}
