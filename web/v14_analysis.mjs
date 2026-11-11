/* GREEN ION V14: all recorded flight analysis stays on the user's device.
 * Uses the existing RKT4 Web Worker; original BIN bytes are never changed. */
import {getLocalFile,listLocalFiles} from "./v14_store.mjs";
const $=id=>document.getElementById(id);
const cache=new Map();
let active=null,comparison=null;
const metrics={
 altitudeM:["Estimated altitude (m)",s=>s.altitudeM],
 velocityMps:["Vertical velocity (m/s)",s=>s.velocityMps],
 accelerationMps2:["Estimated acceleration (m/s²)",s=>s.accelerationMps2],
 estimatedVerticalG:["Estimated vertical G",s=>s.estimatedVerticalG],
 pressurePa:["Pressure (Pa)",s=>s.pressurePa],
 batteryMv:["Battery (V)",s=>s.batteryMv/1000]
};
const status=value=>{$("v14AnalysisStatus").textContent=value;};
async function decode(record) {
 if(!record||record.kind!=="BIN")throw Error("Select a recorded RKT4 BIN file.");
 if(cache.has(record.key))return cache.get(record.key);
 const result=await new Promise((resolve,reject)=>{
  const worker=new Worker("./analysis-worker.mjs",{type:"module"});
  let done=false;
  const finish=(error,data)=>{
   if(done)return;
   done=true;clearTimeout(timer);worker.terminate();
   if(error)reject(error);else resolve(data);
  };
  const timer=setTimeout(()=>finish(Error("Flight decoding timed out.")),60000);
  worker.onerror=e=>finish(Error(e.message||"Worker error"));
  worker.onmessage=e=>e.data.error?finish(Error(e.data.error)):finish(null,e.data);
  const bytes=record.bytes.slice(0);
  worker.postMessage({id:1,name:record.name,bytes},[bytes]);
 });
 cache.set(record.key,result);
 if(cache.size>6)cache.delete(cache.keys().next().value);
 return result;
}
function downloadText(name,contents,type){
 const url=URL.createObjectURL(new Blob([contents],{type}));
 const a=document.createElement("a");a.href=url;a.download=name;
 document.body.append(a);a.click();a.remove();
 setTimeout(()=>URL.revokeObjectURL(url),60000);
}
function draw(){
 const canvas=$("v14AnalysisCanvas"),ctx=canvas.getContext("2d"),w=canvas.width,h=canvas.height;
 ctx.fillStyle="#0b1c2b";ctx.fillRect(0,0,w,h);
 if(!active)return;
 const [title,value]=metrics[$("v14GraphMetric").value]||metrics.altitudeM;
 const progress=Number($("v14Replay").value)||100;
 const items=[active,comparison].filter(Boolean).map((item,i)=>{
  const samples=item.data.analysis.samples;
  const count=Math.max(1,Math.ceil(samples.length*progress/100));
  // Draw at most 3000 points per series; never spread a huge array on the JS stack.
  const stride=Math.max(1,Math.ceil(count/3000));
  const points=[];
  for(let n=0;n<count;n+=stride){
   const s=samples[n],y=value(s),x=s.timeS;
   if(Number.isFinite(x)&&Number.isFinite(y))points.push({x,y});
  }
  return {points,name:item.record.name,color:i===0?"#7bdbff":"#72e5ab"};
 });
 const all=items.flatMap(x=>x.points);
 if(all.length<2)return;
 let minX=Infinity,maxX=-Infinity,minY=Infinity,maxY=-Infinity;
 for(const p of all){
  minX=Math.min(minX,p.x);maxX=Math.max(maxX,p.x);
  minY=Math.min(minY,p.y);maxY=Math.max(maxY,p.y);
 }
 if(maxX<=minX)maxX=minX+1;
 if(maxY<=minY){minY-=1;maxY+=1;}
 const pad=(maxY-minY)*.08;minY-=pad;maxY+=pad;
 const x=t=>50+(t-minX)/(maxX-minX)*(w-70);
 const y=v=>h-35-(v-minY)/(maxY-minY)*(h-59);
 ctx.lineWidth=1;ctx.strokeStyle="#3a5871";
 for(let i=0;i<=4;i++){
  const yy=24+(h-59)*i/4;
  ctx.beginPath();ctx.moveTo(50,yy);ctx.lineTo(w-20,yy);ctx.stroke();
 }
 items.forEach(item=>{
  ctx.strokeStyle=item.color;ctx.lineWidth=2;ctx.beginPath();
  item.points.forEach((p,i)=>i?ctx.lineTo(x(p.x),y(p.y)):ctx.moveTo(x(p.x),y(p.y)));
  ctx.stroke();
 });
 ctx.fillStyle="#eff8ff";ctx.font="13px system-ui";
 ctx.fillText(title,52,17);ctx.fillText(minX.toFixed(1)+"s",50,h-10);
 ctx.fillText(maxX.toFixed(1)+"s",w-80,h-10);
 ctx.fillText(minY.toFixed(1),5,h-36);ctx.fillText(maxY.toFixed(1),5,28);
 $("v14ReplayTime").textContent=progress+"% · "+items.map(x=>x.name).join(" vs ");
}
function summary(data,record){
 const node=$("v14AnalysisSummary");node.replaceChildren();
 const d=data.analysis;
 for(const [name,value] of [
  ["Recording",record.name],
  ["Valid samples",d.samples.filter(s=>s.valid).length+" / "+d.samples.length],
  ["Sample rate",d.rateHz+" Hz"],
  ["Apogee",d.peakAltitudeM.toFixed(2)+" m estimated"],
  ["Maximum upward velocity",Number.isFinite(d.maxAscentMps)?d.maxAscentMps.toFixed(2)+" m/s":"not available"],
  ["Maximum downward velocity",Number.isFinite(d.maxDescentMps)?d.maxDescentMps.toFixed(2)+" m/s":"not available"],
  ["Phases",d.phases.map(s=>s.name).join(", ")||"not reliably detected"]]){
  const row=document.createElement("div");row.className="row";
  const label=document.createElement("span");label.textContent=name;
  const text=document.createElement("strong");text.textContent=value;
  row.append(label,text);node.append(row);
 }
 $("v14AnalysisWarnings").textContent=d.warnings.join(" · ");
}
async function analyze(){
 const key=$("v14AnalysisFlight").value;
 if(!key){status("Choose a saved RKT4 BIN first.");return;}
 status("Decoding original flight in an offline Web Worker...");
 try{
  const record=await getLocalFile(key),data=await decode(record);
  active={record,data};comparison=null;
  $("v14CompareSummary").textContent="";
  $("v14Replay").value="100";summary(data,record);draw();
  status("Analysis complete. Original file was not modified.");
 }catch(e){status("Cannot analyze flight: "+e.message);}
}
async function compare(){
 const key=$("v14CompareFlight").value;
 if(!active||!key||key===active.record.key){status("Analyze a flight and choose a different one to compare.");return;}
 try{
  const record=await getLocalFile(key),data=await decode(record);
  comparison={record,data};
  const a=active.data.analysis,b=data.analysis;
  $("v14CompareSummary").textContent="Apogee comparison: "+active.record.name+" "+
   a.peakAltitudeM.toFixed(2)+" m vs "+record.name+" "+b.peakAltitudeM.toFixed(2)+
   " m; difference "+(b.peakAltitudeM-a.peakAltitudeM).toFixed(2)+" m";
  $("v14Replay").value="100";draw();
  status("Comparison ready: blue is first flight, green is second. Estimates only.");
 }catch(e){status("Comparison failed: "+e.message);}
}
export async function refreshHistoryAnalysis(){
 const files=(await listLocalFiles()).filter(f=>f.kind==="BIN")
  .sort((a,b)=>String(b.createdAt).localeCompare(String(a.createdAt)));
 for(const id of ["v14AnalysisFlight","v14CompareFlight"]){
  const select=$(id),previous=select.value;
  select.replaceChildren();
  const blank=document.createElement("option");blank.value="";
  blank.textContent=id==="v14AnalysisFlight"?"Choose saved BIN":"Choose another BIN";
  select.append(blank);
  for(const f of files){
   const opt=document.createElement("option");opt.value=f.key;opt.textContent=f.name;select.append(opt);
  }
  if(files.some(f=>f.key===previous))select.value=previous;
 }
 $("v14Analyze").disabled=!files.length;
 $("v14Compare").disabled=files.length<2;
}
export function initHistoryAnalysis(){
 $("v14Analyze").onclick=analyze;
 $("v14Compare").onclick=compare;
 $("v14GraphMetric").onchange=draw;
 $("v14Replay").oninput=draw;
 $("v14ExportCsv").onclick=()=>{
  if(!active){status("Analyze a BIN first.");return;}
  downloadText(active.record.name.replace(/\.bin$/i,"")+"-analysis.csv",active.data.csv,"text/csv");
 };
 $("v14ExportReport").onclick=()=>{
  if(!active){status("Analyze a BIN first.");return;}
  downloadText(active.record.name.replace(/\.bin$/i,"")+"-report.json",JSON.stringify(active.data.report,null,2),"application/json");
 };
 $("v14PrintReport").onclick=()=>window.print();
 refreshHistoryAnalysis().catch(e=>status(e.message));
}