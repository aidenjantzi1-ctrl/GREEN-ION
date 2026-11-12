/* GREEN ION V16 advanced postflight visualization.
 * Offline-only. Uses immutable RKT4/GIS1 originals and never runs on Feather.
 * 3D trajectory is reconstructed evidence, not an aerodynamic prediction. */
import {getLocalFile,listLocalMetadata} from "./v14_store.mjs";
import {parseRkt4,analyzeFlight} from "./analysis/rkt4.mjs";
import {parseGis1} from "./analysis/gis1.mjs";

export const VIS_SCHEMA="GREEN-ION-V16-VIS-1";
const EARTH_M=6371000;
const DEG=Math.PI/180;
const $=id=>document.getElementById(id);

function finite(v){return Number.isFinite(v);}
function normalizeQ(q){
 const n=Math.hypot(...q)||1;
 return q.map(v=>v/n);
}
function qMul(a,b){
 return [
  a[0]*b[0]-a[1]*b[1]-a[2]*b[2]-a[3]*b[3],
  a[0]*b[1]+a[1]*b[0]+a[2]*b[3]-a[3]*b[2],
  a[0]*b[2]-a[1]*b[3]+a[2]*b[0]+a[3]*b[1],
  a[0]*b[3]+a[1]*b[2]-a[2]*b[1]+a[3]*b[0]
 ];
}
export function rotateVector(q,v){
 const p=[0,...v],conj=[q[0],-q[1],-q[2],-q[3]];
 const r=qMul(qMul(q,p),conj);
 return [r[1],r[2],r[3]];
}
export function integrateGyroOrientation(samples){
 let q=[1,0,0,0],last=null,firstTime=null;
 const out=[];
 for(const s of samples||[]){
  if(!s.imuValid||![s.gyroXDps,s.gyroYDps,s.gyroZDps,s.timeS].every(finite))continue;
  if(firstTime===null)firstTime=s.timeS;
  if(last!==null){
   const dt=s.timeS-last;
   if(dt>0&&dt<=0.25){
    const omega=[0,s.gyroXDps*DEG,s.gyroYDps*DEG,s.gyroZDps*DEG];
    const dq=qMul(q,omega).map(v=>0.5*v*dt);
    q=normalizeQ(q.map((v,i)=>v+dq[i]));
   }
  }
  last=s.timeS;
  if(!out.length||s.timeS-firstTime-out.at(-1).timeS>=0.05)
   out.push({timeS:s.timeS-firstTime,q:[...q]});
 }
 return out;
}
function localXY(lat,lon,lat0,lon0){
 return {
  x:(lon-lon0)*DEG*Math.cos(lat0*DEG)*EARTH_M,
  y:(lat-lat0)*DEG*EARTH_M
 };
}
function nearestPressure(analysis,targetTime){
 const samples=analysis.samples;
 if(!samples.length)return null;
 const target=targetTime;
 let lo=0,hi=samples.length-1;
 while(lo<hi){
  const mid=Math.floor((lo+hi)/2);
  if(samples[mid].timeS<target)lo=mid+1;else hi=mid;
 }
 const candidates=[samples[lo],samples[Math.max(0,lo-1)]].filter(Boolean);
 candidates.sort((a,b)=>Math.abs(a.timeS-target)-Math.abs(b.timeS-target));
 return candidates.find(x=>finite(x.altitudeM))||null;
}
function smoothTrack(points,radius=2){
 if(points.length<3)return points;
 return points.map((point,index)=>{
  if(index===0||index===points.length-1)return point;
  const from=Math.max(0,index-radius),to=Math.min(points.length-1,index+radius);
  let x=0,y=0,n=0;
  for(let i=from;i<=to;i++){x+=points[i].rawX??points[i].x;y+=points[i].rawY??points[i].y;n++;}
  return {...point,x:x/n,y:y/n};
 });
}
function nearestPointIndex(points,timeS){
 if(!points.length)return -1;
 let best=0,delta=Infinity;
 for(let i=0;i<points.length;i++){
  const d=Math.abs(points[i].timeS-timeS);
  if(d<delta){delta=d;best=i;}
 }
 return best;
}
function phaseMarkers(analysis,points,pressureStart){
 return (analysis.phases||[]).map(event=>{
  const relative=event.timeS-pressureStart,index=nearestPointIndex(points,relative);
  return index<0?null:{
   name:event.name,timeS:relative,index,point:points[index],
   confidence:event.confidenceScore??null,confidenceLabel:event.confidence??null
  };
 }).filter(Boolean);
}
export function reconstructFlight(binInput,senInput=null){
 const analysis=analyzeFlight(parseRkt4(binInput));
 const pressureStart=analysis.samples[0]?.timeS||0;
 let sensor=null,points=[],orientation=[],mode="pressure-only",limitations=[];
 if(senInput){
  sensor=parseGis1(senInput);
  const sensorStart=sensor.samples[0]?.timeS??pressureStart;
  orientation=integrateGyroOrientation(sensor.samples).map(item=>({
   ...item,timeS:item.timeS+(sensorStart-pressureStart)
  }));
  const fixes=sensor.samples.filter(s=>s.gpsCurrent&&finite(s.latitude)&&finite(s.longitude));
  if(fixes.length){
   const first=fixes[0];
   let lastT=-Infinity;
   for(const s of fixes){
    const t=s.timeS-pressureStart;
    if(t-lastT<0.2&&s!==fixes.at(-1))continue;
    lastT=t;
    const xy=localXY(s.latitude,s.longitude,first.latitude,first.longitude);
    const p=nearestPressure(analysis,s.timeS);
    const z=p&&finite(p.altitudeM)?p.altitudeM:
      finite(s.gpsAltitudeM)&&finite(first.gpsAltitudeM)?s.gpsAltitudeM-first.gpsAltitudeM:0;
    points.push({
     timeS:t,x:xy.x,y:xy.y,rawX:xy.x,rawY:xy.y,z,latitude:s.latitude,longitude:s.longitude,
     gpsAltitudeM:s.gpsAltitudeM,verticalSource:p?"pressure":"gps",
     satellites:s.satellites
    });
   }
   points=smoothTrack(points);
   mode="gps+pressure";
  }else{
   limitations.push("GIS1 contains no current GPS fixes; horizontal track is unavailable.");
  }
 }else{
  limitations.push("No GIS1 companion; GPS track and relative orientation are unavailable.");
 }
 if(!points.length){
  const stride=Math.max(1,Math.floor(analysis.samples.length/800));
  points=analysis.samples.filter((s,i)=>i%stride===0&&finite(s.altitudeM)).map(s=>({
   timeS:s.timeS-pressureStart,x:0,y:0,z:s.altitudeM,verticalSource:"pressure"
  }));
 }
 const maxAltitude=points.reduce((m,p)=>Math.max(m,finite(p.z)?p.z:0),0);
 let distance=0;
 for(let i=1;i<points.length;i++){
  const ax=points[i-1].rawX??points[i-1].x,ay=points[i-1].rawY??points[i-1].y;
  const bx=points[i].rawX??points[i].x,by=points[i].rawY??points[i].y;
  distance+=Math.hypot(bx-ax,by-ay);
 }
 const markers=phaseMarkers(analysis,points,pressureStart);
 if(mode==="gps+pressure")limitations.push("The displayed horizontal path uses light local smoothing; raw GPS coordinates remain preserved in the original GIS1 file.");
 if(orientation.length)limitations.push("Orientation is relative gyro integration only; heading drifts and is not absolute attitude.");
 limitations.push("The rocket drawing is generic and not to scale. This is recorded-data reconstruction, not an aerodynamic simulation.");
 return {
  schema:VIS_SCHEMA,mode,points,orientation,phaseMarkers:markers,maxAltitudeM:maxAltitude,horizontalTrackM:distance,
  pressureSamples:analysis.samples.length,sensorSamples:sensor?.samples.length||0,
  sensorWarnings:sensor?.warnings||[],limitations
 };
}
function orientationAt(series,t){
 if(!series?.length)return null;
 let best=series[0];
 for(const item of series){if(item.timeS>t)break;best=item;}
 return best.q;
}
function bounds(points){
 let max=1;
 for(const p of points)max=Math.max(max,Math.abs(p.x),Math.abs(p.y),Math.abs(p.z));
 return max;
}
function project(p,view,canvas,extent){
 const cy=Math.cos(view.yaw),sy=Math.sin(view.yaw),cp=Math.cos(view.pitch),sp=Math.sin(view.pitch);
 const x1=cy*p.x-sy*p.y,y1=sy*p.x+cy*p.y,z1=p.z;
 const x2=x1,z2=sp*y1+cp*z1;
 const scale=Math.min(canvas.width,canvas.height)*0.38*view.zoom/extent;
 return [canvas.width/2+x2*scale,canvas.height*0.78-z2*scale];
}
function drawRocket(ctx,point,q,view,canvas,extent){
 const start=project(point,view,canvas,extent);
 let dir=[0,0,1];
 if(q)dir=rotateVector(q,dir);
 const length=Math.max(extent*0.06,1);
 const end=project({x:point.x+dir[0]*length,y:point.y+dir[1]*length,z:point.z+dir[2]*length},view,canvas,extent);
 ctx.beginPath();ctx.moveTo(...start);ctx.lineTo(...end);ctx.lineWidth=4;ctx.strokeStyle="#ffdd88";ctx.stroke();
 ctx.beginPath();ctx.arc(start[0],start[1],5,0,Math.PI*2);ctx.fillStyle="#ffdd88";ctx.fill();
}
function renderer(canvas,state){
 const ctx=canvas.getContext("2d");
 const view={yaw:-0.7,pitch:0.55,zoom:1};
 let dragging=false,last=[0,0];
 function paint(index=state.index){
  state.index=Math.max(0,Math.min(index,state.data.points.length-1));
  ctx.clearRect(0,0,canvas.width,canvas.height);
  const data=state.data,extent=bounds(data.points);
  ctx.fillStyle="#0d1c2a";ctx.fillRect(0,0,canvas.width,canvas.height);
  // Ground axes are schematic local east/north/up.
  const origin={x:0,y:0,z:0};
  for(const [axis,label] of [[{x:extent*.35,y:0,z:0},"E"],[{x:0,y:extent*.35,z:0},"N"],[{x:0,y:0,z:extent*.35},"UP"]]){
   const a=project(origin,view,canvas,extent),b=project(axis,view,canvas,extent);
   ctx.beginPath();ctx.moveTo(...a);ctx.lineTo(...b);ctx.lineWidth=1;ctx.strokeStyle="#35536e";ctx.stroke();
   ctx.fillStyle="#a5bbcd";ctx.fillText(label,b[0]+4,b[1]-4);
  }
  ctx.beginPath();
  data.points.forEach((p,i)=>{const screen=project(p,view,canvas,extent);if(i===0)ctx.moveTo(...screen);else ctx.lineTo(...screen);});
  ctx.lineWidth=2;ctx.strokeStyle="#7bdbff";ctx.stroke();
  for(const marker of data.phaseMarkers||[]){
   const screen=project(marker.point,view,canvas,extent);
   ctx.beginPath();ctx.arc(screen[0],screen[1],5,0,Math.PI*2);
   ctx.fillStyle=marker.name==="apogee"?"#ffdd88":"#72e5ab";ctx.fill();
   ctx.fillStyle="#eff8ff";
   const conf=Number.isFinite(marker.confidence)?" "+Math.round(marker.confidence)+"%":"";
   ctx.fillText(marker.name.replaceAll("_"," ")+conf,screen[0]+7,screen[1]-5);
  }
  const point=data.points[state.index],q=orientationAt(data.orientation,point.timeS);
  drawRocket(ctx,point,q,view,canvas,extent);
  ctx.fillStyle="#eff8ff";ctx.fillText("Recorded-data reconstruction · drag to orbit · wheel to zoom",12,20);
  state.onPaint?.(point,q);
 }
 canvas.addEventListener("pointerdown",e=>{dragging=true;last=[e.clientX,e.clientY];canvas.setPointerCapture(e.pointerId);});
 canvas.addEventListener("pointermove",e=>{if(!dragging)return;view.yaw+=(e.clientX-last[0])*.01;view.pitch=Math.max(-1.2,Math.min(1.2,view.pitch+(e.clientY-last[1])*.01));last=[e.clientX,e.clientY];paint();});
 canvas.addEventListener("pointerup",()=>{dragging=false;});
 canvas.addEventListener("wheel",e=>{e.preventDefault();view.zoom=Math.max(.35,Math.min(4,view.zoom*(e.deltaY>0?.9:1.1)));paint();},{passive:false});
 return {paint,reset(){view.yaw=-.7;view.pitch=.55;view.zoom=1;paint();}};
}
let render=null,playFrame=null,current=null;
async function loadSelected(){
 const select=$("v15ReportFlight"),key=select?.value;
 if(!key)throw Error("Choose a completed BIN in Postflight report first.");
 const bin=await getLocalFile(key);
 if(!bin||bin.kind!=="BIN")throw Error("Selected immutable BIN is unavailable.");
 const metadata=await listLocalMetadata(),stem=bin.name.replace(/\.BIN$/i,"");
 const senMeta=metadata.find(f=>f.kind==="SEN"&&f.name.replace(/\.SEN$/i,"")===stem);
 const sen=senMeta?await getLocalFile(senMeta.key):null;
 const binBytes=bin.bytes.slice(0),senBytes=sen?.bytes?.slice(0)||null;
 const data=await new Promise((resolve,reject)=>{
  const worker=new Worker("./v16_visualization_worker.mjs",{type:"module"});
  const timeout=setTimeout(()=>{worker.terminate();reject(Error("3D reconstruction timed out"));},90000);
  worker.onerror=event=>{clearTimeout(timeout);worker.terminate();reject(Error(event.message||"3D worker failed"));};
  worker.onmessage=event=>{
   clearTimeout(timeout);worker.terminate();
   if(event.data?.error)reject(Error(event.data.error));else resolve(event.data.data);
  };
  worker.postMessage({id:1,binBytes,senBytes},senBytes?[binBytes,senBytes]:[binBytes]);
 });
 return {data,name:bin.name};
}
function stopPlay(){if(playFrame){cancelAnimationFrame(playFrame);playFrame=null;}$("v16VisPlay").textContent="Play replay";}
function setStatus(value){if($("v16VisStatus"))$("v16VisStatus").textContent=value;}
export function initV16Visualization(){
 const history=$("history");if(!history)return;
 if(!$("v16VisCard")){
  const card=document.createElement("div");card.className="card";card.id="v16VisCard";
  card.innerHTML='<h2>3D trajectory + relative digital twin</h2>'+
   '<p class="muted">Offline postflight reconstruction from original RKT4/GIS1. No 3D work runs on the Feather.</p>'+
   '<canvas id="v16TrajectoryCanvas" width="920" height="480" aria-label="3D reconstructed rocket trajectory"></canvas>'+
   '<input id="v16ReplaySlider" type="range" min="0" max="0" value="0" aria-label="Trajectory replay position">'+
   '<div class="row"><span>Replay evidence</span><strong id="v16VisReplay">No flight loaded</strong></div>'+
   '<div class="row"><span>Reconstruction</span><strong id="v16VisMetrics">--</strong></div>'+
   '<div class="actions"><button id="v16VisLoad">Load selected flight</button><button id="v16VisPlay" class="secondary">Play replay</button><button id="v16VisReset" class="secondary">Reset view</button></div>'+
   '<p class="muted">Markers show pressure-derived launch/apogee/descent/landing estimates with confidence where available. The finder/recovery view always uses raw confirmed GPS fixes.</p>'+
   '<p id="v16VisStatus" class="muted">Select a BIN in Postflight report, then load the reconstruction.</p>';
  history.append(card);
 }
 $("v16VisLoad").onclick=async()=>{
  stopPlay();setStatus("Parsing immutable originals locally...");
  try{
   current=await loadSelected();
   const slider=$("v16ReplaySlider");slider.max=Math.max(0,current.data.points.length-1);slider.value=0;
   const state={data:current.data,index:0,onPaint:(p,q)=>{
    $("v16VisReplay").textContent=p.timeS.toFixed(2)+" s · "+p.z.toFixed(1)+" m · "+(q?"relative gyro attitude":"orientation unavailable");
   }};
   render=renderer($("v16TrajectoryCanvas"),state);render.paint(0);
   $("v16VisMetrics").textContent=current.data.mode+" · "+current.data.points.length+" points · max "+current.data.maxAltitudeM.toFixed(1)+" m · horizontal "+current.data.horizontalTrackM.toFixed(1)+" m";
   setStatus(current.data.limitations.join(" "));
  }catch(e){setStatus("3D reconstruction unavailable: "+e.message);}
 };
 $("v16ReplaySlider").oninput=e=>{stopPlay();render?.paint(Number(e.target.value));};
 $("v16VisReset").onclick=()=>render?.reset();
 $("v16VisPlay").onclick=()=>{
  if(!render||!current)return;
  if(playFrame){stopPlay();return;}
  $("v16VisPlay").textContent="Pause replay";
  let last=performance.now();
  const step=now=>{
   if(!playFrame)return;
   if(now-last>=60){
    last=now;
    const slider=$("v16ReplaySlider"),next=Number(slider.value)+1;
    if(next>Number(slider.max)){stopPlay();return;}
    slider.value=next;render.paint(next);
   }
   playFrame=requestAnimationFrame(step);
  };
  playFrame=requestAnimationFrame(step);
 };
}
