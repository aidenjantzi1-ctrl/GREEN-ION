/* GREEN ION V16.2 multi-flight comparison. Original files remain immutable. */
import {listLocalMetadata,getLocalFile} from "./v14_store.mjs";
const $=id=>document.getElementById(id);let results=[];
function fmt(v,unit="",d=2){return Number.isFinite(v)?v.toFixed(d)+(unit?" "+unit:""):"—";}

export function makeComparisonHeader(names,doc=document){
 const row=doc.createElement("tr"),metric=doc.createElement("th");
 metric.textContent="Metric";row.append(metric);
 for(const name of names){const cell=doc.createElement("th");cell.textContent=String(name);row.append(cell);}
 return row;
}

async function analyze(meta,files){
 const bin=await getLocalFile(meta.key),base=meta.name.replace(/\.BIN$/i,""),senMeta=files.find(x=>x.kind==="SEN"&&x.name.replace(/\.SEN$/i,"")===base),sen=senMeta?await getLocalFile(senMeta.key):null;
 return await new Promise((resolve,reject)=>{
  const w=new Worker("./v16_2_compare_worker.mjs",{type:"module"}),timer=setTimeout(()=>{w.terminate();reject(Error("Comparison worker timed out"));},60000);
  w.onerror=e=>{clearTimeout(timer);w.terminate();reject(Error(e.message||"Worker failed"));};
  w.onmessage=e=>{clearTimeout(timer);w.terminate();e.data.error?reject(Error(e.data.error)):resolve(e.data.result);};
  const bb=bin.bytes.slice(0),sb=sen?.bytes?.slice(0)||null;w.postMessage({id:1,name:meta.name,bin:bb,sen:sb},sb?[bb,sb]:[bb]);
 });
}
function draw(){
 const c=$("v162CompareGraph");if(!c)return;const dpr=Math.min(2,devicePixelRatio||1),w=Math.max(320,c.clientWidth),h=300;c.width=w*dpr;c.height=h*dpr;const ctx=c.getContext("2d");ctx.setTransform(dpr,0,0,dpr,0,0);ctx.fillStyle="#0b1c2b";ctx.fillRect(0,0,w,h);
 const points=results.flatMap(x=>x.graph);if(!points.length)return;
 let minT=0,maxT=Math.max(...points.map(x=>x.t)),minY=Math.min(0,...points.map(x=>x.altitudeM)),maxY=Math.max(1,...points.map(x=>x.altitudeM));if(maxT<=0)maxT=1;if(maxY<=minY)maxY=minY+1;
 const x=t=>48+(t-minT)/(maxT-minT)*(w-66),y=v=>h-35-(v-minY)/(maxY-minY)*(h-58);
 ctx.strokeStyle="#35536e";ctx.lineWidth=1;for(let i=0;i<=4;i++){const yy=20+(h-55)*i/4;ctx.beginPath();ctx.moveTo(48,yy);ctx.lineTo(w-18,yy);ctx.stroke();}
 const colors=["#7bdbff","#72e5ab","#ffdd88","#ff91a0"];results.forEach((r,index)=>{ctx.strokeStyle=colors[index%colors.length];ctx.lineWidth=2;ctx.beginPath();r.graph.forEach((p,i)=>i?ctx.lineTo(x(p.t),y(p.altitudeM)):ctx.moveTo(x(p.t),y(p.altitudeM)));ctx.stroke();ctx.fillStyle=colors[index%colors.length];ctx.fillText(r.name,55,18+index*14);});
 ctx.fillStyle="#a5bbcd";ctx.fillText("Launch-relative time (s)",w-150,h-8);
}
function render(){
 const table=$("v162CompareTable");table.replaceChildren();if(!results.length)return;
 const metrics=[["Max altitude",x=>fmt(x.maxAltitudeM,"m")],["Flight duration",x=>fmt(x.flightDurationS,"s")],["Max estimated velocity",x=>fmt(x.maxVelocityMps,"m/s")],["Max acceleration",x=>fmt(x.maxAccelerationMps2,"m/s²")],["Max G",x=>fmt(x.maxG,"g")+" · "+x.gSource],["Ascent duration",x=>fmt(x.ascentDurationS,"s")],["Descent duration",x=>fmt(x.descentDurationS,"s")],["GPS track distance",x=>fmt(x.gpsDistanceM,"m")],["Data completeness",x=>fmt(x.completenessPct,"%",1)],["Recovery location",x=>x.recoveryLocation?x.recoveryLocation.latitude.toFixed(6)+", "+x.recoveryLocation.longitude.toFixed(6):"—"]];
 const head=makeComparisonHeader(results.map(x=>x.name));table.append(head);
 for(const [label,get] of metrics){const tr=document.createElement("tr"),th=document.createElement("th");th.textContent=label;tr.append(th);for(const r of results){const td=document.createElement("td");td.textContent=get(r.metrics);tr.append(td);}table.append(tr);}
 draw();
}
async function compare(){
 const select=$("v162CompareFlights"),chosen=[...select.selectedOptions].map(x=>x.value).filter(Boolean).slice(0,4),status=$("v162CompareStatus");
 if(chosen.length<2){status.textContent="Select at least two flights (up to four).";return;}
 status.textContent="Comparing immutable originals in Web Workers...";try{
  const files=await listLocalMetadata(),metas=chosen.map(k=>files.find(x=>x.key===k)).filter(Boolean);results=[];
  for(const meta of metas)results.push(await analyze(meta,files));
  render();status.textContent="Comparison ready. Graphs use launch-relative time. Motion values are estimates unless explicitly labeled measured.";
 }catch(error){status.textContent="Comparison failed: "+error.message;}
}
function exportJson(){
 if(!results.length)return;const url=URL.createObjectURL(new Blob([JSON.stringify({schema:"GREEN-ION-V16.2-COMPARISON-1",generatedAt:new Date().toISOString(),flights:results},null,2)],{type:"application/json"})),a=document.createElement("a");a.href=url;a.download="green-ion-v16.2-flight-comparison.json";a.click();setTimeout(()=>URL.revokeObjectURL(url),60000);
}
async function refresh(){
 const select=$("v162CompareFlights"),files=(await listLocalMetadata()).filter(x=>x.kind==="BIN").sort((a,b)=>String(b.createdAt).localeCompare(String(a.createdAt)));select.replaceChildren();for(const f of files)select.append(new Option(f.name,f.key));
}
export function initV162Compare(){
 const history=$("history");if(!history||$("v162CompareCard"))return;const card=document.createElement("div");card.className="card v162-visual-card";card.id="v162CompareCard";
 card.innerHTML='<h2>Compare flights</h2><p class="muted">Select 2–4 saved BIN recordings. Originals are decoded locally; graph time is aligned to estimated launch.</p><label>Flights<select id="v162CompareFlights" multiple size="6" aria-label="Flights to compare"></select></label><div class="actions"><button id="v162CompareRun">Compare selected flights</button><button id="v162CompareExport" class="secondary">Export comparison JSON</button></div><div class="v162-table-wrap"><table id="v162CompareTable"></table></div><canvas id="v162CompareGraph" height="300" aria-label="Launch-relative altitude comparison graph"></canvas><p id="v162CompareStatus" role="status">Choose at least two flights.</p>';
 history.append(card);$("v162CompareRun").onclick=compare;$("v162CompareExport").onclick=exportJson;window.addEventListener("green-ion-history-change",()=>refresh().catch(()=>{}));refresh().catch(e=>$("v162CompareStatus").textContent=e.message);
}
