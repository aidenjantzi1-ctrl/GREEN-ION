/* GREEN ION V16.2 genuine WebGL postflight viewer.
 * Scientific reconstruction remains worker-derived; WebGL is display only. */
import {listLocalMetadata,getLocalFile} from "./v14_store.mjs";
import {rotateVector} from "./v16_visualization.mjs";

const $=id=>document.getElementById(id);
const finite=Number.isFinite;
let data=null,glState=null,currentTime=0,playing=false,lastTick=0,speed=1,frame=0,visible=true;
let camera={yaw:-.8,pitch:.45,distance:120,target:[0,0,0],pan:[0,0,0],mode:"free"};
let pointerMap=new Map(),gesture=null,lastMvp=null,effects=true;
let replayGraphSeries=[];
function replayDuration(){
 return Math.max(0,data?.timeline?.at(-1)?.timeS||0,
  data?.sensorTimeline?.at(-1)?.timeS||0,data?.points?.at(-1)?.timeS||0);
}
function prepareReplayGraphSeries(){
 const axes=[
  ["Altitude (estimated)","m",data?.timeline||[],s=>s.altitudeM],
  ["Vertical speed (estimated)","m/s",data?.timeline||[],s=>s.velocityMps],
  ["Acceleration (estimated)","m/s²",data?.timeline||[],s=>s.accelerationMps2],
  ["Angular rate (measured IMU)","°/s",data?.sensorTimeline||[],
   s=>[s.gyroXDps,s.gyroYDps,s.gyroZDps].every(finite)?
    Math.hypot(s.gyroXDps,s.gyroYDps,s.gyroZDps):null]
 ];
 replayGraphSeries=axes.map(([label,unit,raw,value])=>{
  const stride=Math.max(1,Math.ceil(raw.length/450)),samples=[];
  for(let i=0;i<raw.length;i+=stride){
   const n=value(raw[i]),timeS=raw[i].timeS;
   samples.push({timeS,value:finite(n)?n:null});
  }
  if(raw.length&&raw.at(-1)!==raw[Math.floor((raw.length-1)/stride)*stride]){
   const tail=raw.at(-1),n=value(tail);samples.push({timeS:tail.timeS,value:finite(n)?n:null});
  }
  const valid=samples.map(s=>s.value).filter(finite),min=Math.min(0,...valid),max=Math.max(0,...valid);
  return {label,unit,samples,min,max:max>min?max:min+1};
 });
}
function drawReplayGraphs(){
 const canvas=$("v163ReplayGraphs");if(!canvas||!data||!replayGraphSeries.length||document.hidden)return;
 const dpr=Math.min(2,window.devicePixelRatio||1),
  w=Math.max(220,Math.round(canvas.clientWidth||320)),h=330;
 if(canvas.width!==Math.round(w*dpr)||canvas.height!==Math.round(h*dpr)){
  canvas.width=Math.round(w*dpr);canvas.height=Math.round(h*dpr);
 }
 const ctx=canvas.getContext("2d");if(!ctx)return;
 ctx.setTransform(dpr,0,0,dpr,0,0);ctx.fillStyle="#0b1c2b";ctx.fillRect(0,0,w,h);
 const duration=Math.max(.01,replayDuration()),left=48,right=w-12,colors=["#7bdbff","#72e5ab","#ffdd88","#ff91a0"];
 const gx=t=>left+clamp(t/duration,0,1)*(right-left);
 replayGraphSeries.forEach((series,index)=>{
  const top=8+index*80,bottom=top+58;
  ctx.strokeStyle="#35536e";ctx.lineWidth=1;ctx.strokeRect(left,top,right-left,bottom-top);
  ctx.fillStyle="#d9efff";ctx.font="11px system-ui";
  ctx.fillText(series.label+" ("+series.unit+")",left+5,top+12);
  ctx.strokeStyle=colors[index];ctx.lineWidth=1.75;ctx.beginPath();
  let open=false;
  for(const p of series.samples){
   if(!finite(p.value)||!finite(p.timeS)){open=false;continue;}
   const y=bottom-5-(p.value-series.min)/(series.max-series.min)*(bottom-top-16);
   if(open)ctx.lineTo(gx(p.timeS),y);else ctx.moveTo(gx(p.timeS),y);
   open=true;
  }
  ctx.stroke();
  ctx.strokeStyle="#eff8ff";ctx.lineWidth=1;const marker=gx(currentTime);
  ctx.beginPath();ctx.moveTo(marker,top);ctx.lineTo(marker,bottom);ctx.stroke();
  ctx.fillStyle="#b3cada";ctx.fillText(series.max.toFixed(1),3,top+12);
  ctx.fillText(series.min.toFixed(1),3,bottom-5);
 });
 ctx.fillStyle="#b3cada";ctx.fillText("Synchronized replay "+currentTime.toFixed(2)+" / "+duration.toFixed(2)+" s",left,h-5);
}


function clamp(v,a,b){return Math.max(a,Math.min(b,v));}
function v3sub(a,b){return [a[0]-b[0],a[1]-b[1],a[2]-b[2]]}
function v3dot(a,b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]}
function v3cross(a,b){return [a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]]}
function v3norm(v){const n=Math.hypot(...v)||1;return v.map(x=>x/n)}
export function perspectiveMatrix(fov,aspect,near,far){
 const f=1/Math.tan(fov/2),nf=1/(near-far);
 return new Float32Array([f/aspect,0,0,0,0,f,0,0,0,0,(far+near)*nf,-1,0,0,2*far*near*nf,0]);
}
export function lookAtMatrix(eye,center,up=[0,0,1]){
 const z=v3norm(v3sub(eye,center)),x=v3norm(v3cross(up,z)),y=v3cross(z,x);
 return new Float32Array([
  x[0],y[0],z[0],0,x[1],y[1],z[1],0,x[2],y[2],z[2],0,
  -v3dot(x,eye),-v3dot(y,eye),-v3dot(z,eye),1
 ]);
}
export function multiplyMatrix(a,b){
 const o=new Float32Array(16);
 for(let c=0;c<4;c++)for(let r=0;r<4;r++){
  let n=0;for(let k=0;k<4;k++)n+=a[k*4+r]*b[c*4+k];o[c*4+r]=n;
 }return o;
}
function transformPoint(m,p){
 const x=p[0],y=p[1],z=p[2],w=m[3]*x+m[7]*y+m[11]*z+m[15];
 if(!w)return null;
 return [(m[0]*x+m[4]*y+m[8]*z+m[12])/w,(m[1]*x+m[5]*y+m[9]*z+m[13])/w,(m[2]*x+m[6]*y+m[10]*z+m[14])/w];
}
function compile(gl,type,source){
 const s=gl.createShader(type);gl.shaderSource(s,source);gl.compileShader(s);
 if(!gl.getShaderParameter(s,gl.COMPILE_STATUS))throw Error(gl.getShaderInfoLog(s)||"WebGL shader compile failed");
 return s;
}
function createProgram(gl){
 const vs=compile(gl,gl.VERTEX_SHADER,`
 attribute vec3 aPosition;uniform mat4 uMvp;uniform float uPointSize;
 void main(){gl_Position=uMvp*vec4(aPosition,1.0);gl_PointSize=uPointSize;}`);
 const fs=compile(gl,gl.FRAGMENT_SHADER,`
 precision mediump float;uniform vec4 uColor;void main(){gl_FragColor=uColor;}`);
 const p=gl.createProgram();gl.attachShader(p,vs);gl.attachShader(p,fs);gl.linkProgram(p);
 gl.deleteShader(vs);gl.deleteShader(fs);
 if(!gl.getProgramParameter(p,gl.LINK_STATUS))throw Error(gl.getProgramInfoLog(p)||"WebGL link failed");
 return {program:p,pos:gl.getAttribLocation(p,"aPosition"),mvp:gl.getUniformLocation(p,"uMvp"),
  color:gl.getUniformLocation(p,"uColor"),pointSize:gl.getUniformLocation(p,"uPointSize")};
}
function buffer(gl,values,usage=gl.STATIC_DRAW){
 const b=gl.createBuffer();gl.bindBuffer(gl.ARRAY_BUFFER,b);gl.bufferData(gl.ARRAY_BUFFER,new Float32Array(values),usage);return b;
}
function lineVertices(points,raw=false){
 const out=[];for(const p of points)out.push(raw?(p.rawX??p.x):(p.x??0),raw?(p.rawY??p.y):(p.y??0),p.z??0);return out;
}
function gridVertices(extent){
 const out=[],step=Math.max(5,Math.pow(10,Math.floor(Math.log10(Math.max(10,extent)))-1));
 const limit=Math.ceil(extent/step)*step;
 for(let v=-limit;v<=limit;v+=step){out.push(-limit,v,0,limit,v,0,v,-limit,0,v,limit,0);}
 out.push(0,0,0,Math.max(10,extent*.4),0,0,0,0,0,0,Math.max(10,extent*.4),0,0,0,0,0,Math.max(10,extent*.4));
 return out;
}
function rocketBase(){
 const out=[],segments=12,r=.13,z0=-.35,z1=.38,tip=.68;
 for(let i=0;i<segments;i++){
  const a=i*Math.PI*2/segments,b=(i+1)*Math.PI*2/segments;
  const p0=[Math.cos(a)*r,Math.sin(a)*r,z0],p1=[Math.cos(b)*r,Math.sin(b)*r,z0],
   p2=[Math.cos(a)*r,Math.sin(a)*r,z1],p3=[Math.cos(b)*r,Math.sin(b)*r,z1];
  out.push(...p0,...p1,...p2,...p1,...p3,...p2);
  out.push(...p2,...p3,0,0,tip);
 }
 for(let i=0;i<4;i++){
  const a=i*Math.PI/2,c=Math.cos(a),d=Math.sin(a),px=-d,py=c;
  const root=[c*r,d*r,-.25],rear=[c*r,d*r,z0],wide=[c*r+px*.32,d*r+py*.32,z0-.08];
  out.push(...root,...rear,...wide);
 }
 return out;
}
const ROCKET=rocketBase();
function nearest(series,t){
 if(!series?.length)return null;let lo=0,hi=series.length-1;
 while(lo<hi){const mid=(lo+hi)>>1;if(series[mid].timeS<t)lo=mid+1;else hi=mid;}
 const a=series[lo],b=series[Math.max(0,lo-1)];
 return !b||Math.abs(a.timeS-t)<Math.abs(b.timeS-t)?a:b;
}
function pointAt(t){
 const pts=data?.points||[];if(!pts.length)return null;
 if(t<=pts[0].timeS)return pts[0];if(t>=pts.at(-1).timeS)return pts.at(-1);
 let lo=1,hi=pts.length-1;while(lo<hi){const mid=(lo+hi)>>1;if(pts[mid].timeS<t)lo=mid+1;else hi=mid;}
 const b=pts[lo],a=pts[lo-1],f=clamp((t-a.timeS)/Math.max(.001,b.timeS-a.timeS),0,1);
 return {timeS:t,x:a.x+(b.x-a.x)*f,y:a.y+(b.y-a.y)*f,z:a.z+(b.z-a.z)*f,
  latitude:a.latitude??b.latitude,longitude:a.longitude??b.longitude};
}
function orientationAt(t){
 const series=data?.orientation;if(!series?.length)return null;
 let lo=0,hi=series.length-1;
 while(lo<hi){const mid=Math.ceil((lo+hi)/2);if(series[mid].timeS<=t)lo=mid;else hi=mid-1;}
 return series[lo].q;
}
function phaseAt(t){
 const markers=data?.phaseMarkers||[];let name=t<=0?"prelaunch":"flight";
 for(const marker of markers){if(marker.timeS>t)break;name=marker.name;}
 if(name==="launch")name="ascent";return name;
}
function worldExtent(){
 let max=10;for(const p of data?.points||[])max=Math.max(max,Math.abs(p.x||0),Math.abs(p.y||0),Math.abs(p.z||0));return max;
}
function cameraMvp(canvas,point){
 const extent=worldExtent(),aspect=Math.max(.2,canvas.clientWidth/Math.max(1,canvas.clientHeight));
 let target=[camera.target[0]+camera.pan[0],camera.target[1]+camera.pan[1],camera.target[2]+camera.pan[2]];
 if(camera.mode==="follow"&&point)target=[point.x,point.y,point.z];
 let yaw=camera.yaw,pitch=camera.pitch,dist=Math.max(3,camera.distance);
 if(camera.mode==="top"){pitch=1.48;yaw=-Math.PI/2;target=[0,0,Math.max(0,(data?.maxAltitudeM||0)*.35)];}
 if(camera.mode==="side"){pitch=.18;yaw=0;target=[0,0,Math.max(0,(data?.maxAltitudeM||0)*.35)];}
 if(camera.mode==="ground"){pitch=.25;yaw=-.75;target=[0,0,Math.min(20,extent*.08)];}
 const cp=Math.cos(pitch),eye=[target[0]+dist*cp*Math.cos(yaw),target[1]+dist*cp*Math.sin(yaw),target[2]+dist*Math.sin(pitch)];
 const up=Math.abs(pitch)>1.42?[0,1,0]:[0,0,1];
 return multiplyMatrix(perspectiveMatrix(Math.PI/4,aspect,Math.max(.1,dist/1000),Math.max(1000,dist*20+extent*10)),lookAtMatrix(eye,target,up));
}
function drawBuffer(gl,buf,count,mode,color,mvp,pointSize=4){
 if(!buf||!count)return;const p=glState.shader;gl.useProgram(p.program);gl.bindBuffer(gl.ARRAY_BUFFER,buf);
 gl.enableVertexAttribArray(p.pos);gl.vertexAttribPointer(p.pos,3,gl.FLOAT,false,0,0);gl.uniformMatrix4fv(p.mvp,false,mvp);
 gl.uniform4fv(p.color,color);gl.uniform1f(p.pointSize,pointSize);gl.drawArrays(mode,0,count);
}
function displayProfile(){try{return JSON.parse(localStorage.getItem("green-ion-v16.2-active-profile")||"{}");}catch{return {};}}
function mountingVector(v){
 const axis=String(displayProfile().mountingAxis||"+Z");
 const [x,y,z]=v;
 if(axis==="-Z")return [x,-y,-z];
 if(axis==="+X")return [z,y,-x];
 if(axis==="-X")return [-z,y,x];
 if(axis==="+Y")return [x,z,-y];
 if(axis==="-Y")return [x,-z,y];
 return v;
}
function transformedRocket(point,q,scale){
 const p=displayProfile(),aspect=Number(p.lengthCm)>0&&Number(p.diameterCm)>0?Number(p.lengthCm)/Number(p.diameterCm):10;
 const axial=clamp(aspect/10,.65,1.7),radial=clamp(10/aspect,.7,1.4),out=[];
 for(let i=0;i<ROCKET.length;i+=3){
  let local=[ROCKET[i]*scale*radial,ROCKET[i+1]*scale*radial,ROCKET[i+2]*scale*axial];
  local=mountingVector(local);const v=q?rotateVector(q,local):local;
  out.push(point.x+v[0],point.y+v[1],point.z+v[2]);
 }return out;
}
function effectGeometry(point,scale,phase){
 if(!effects)return {flame:[],chute:[]};const result={flame:[],chute:[]};
 const launch=data?.phaseMarkers?.find(x=>x.name==="launch")?.timeS??0;
 if((phase==="ascent"||phase==="flight")&&currentTime-launch>=0&&currentTime-launch<=3){
  result.flame=[point.x-.12*scale,point.y,point.z-.35*scale,point.x+.12*scale,point.y,point.z-.35*scale,point.x,point.y,point.z-.9*scale];
 }
 let recovery="";try{recovery=JSON.parse(localStorage.getItem("green-ion-v16.2-active-profile")||"{}").recoveryType||"";}catch{}
 if(phase==="descent"&&String(recovery).toLowerCase().includes("parachute")){
  const z=point.z+.9*scale,r=.5*scale;for(let i=0;i<=12;i++){const a=Math.PI*i/12;result.chute.push(point.x+Math.cos(a)*r,point.y, z+Math.sin(a)*r);}
  result.chute.push(point.x-r,point.y,z,point.x,point.y,point.z+.35*scale,point.x+r,point.y,z,point.x,point.y,point.z+.35*scale);
 }
 return result;
}
function render(){
 if(!visible||!data||!glState||document.hidden)return;
 const canvas=glState.canvas,gl=glState.gl,dpr=Math.min(2,devicePixelRatio||1),w=Math.max(320,Math.round(canvas.clientWidth*dpr)),h=Math.max(280,Math.round(canvas.clientHeight*dpr));
 if(canvas.width!==w||canvas.height!==h){canvas.width=w;canvas.height=h;}gl.viewport(0,0,w,h);gl.enable(gl.DEPTH_TEST);gl.clearColor(.031,.075,.12,1);gl.clear(gl.COLOR_BUFFER_BIT|gl.DEPTH_BUFFER_BIT);
 const point=pointAt(currentTime)||data.points[0],mvp=cameraMvp(canvas,point);lastMvp=mvp;
 drawBuffer(gl,glState.grid,glState.gridCount,gl.LINES,[.20,.36,.46,.65],mvp,1);
 if(data.mode==="gps+pressure"&&glState.raw)drawBuffer(gl,glState.raw,glState.trackCount,gl.LINE_STRIP,[.55,.65,.72,.35],mvp,1);
 drawBuffer(gl,glState.track,glState.trackCount,gl.LINE_STRIP,[.48,.86,1,1],mvp,1);
 drawBuffer(gl,glState.markers,glState.markerCount,gl.POINTS,[1,.86,.45,1],mvp,9);
 const scale=Math.max(2,worldExtent()*.035),q=orientationAt(currentTime),rocket=transformedRocket(point,q,scale);
 const rb=buffer(gl,rocket,gl.DYNAMIC_DRAW);drawBuffer(gl,rb,rocket.length/3,gl.TRIANGLES,[.86,.94,1,1],mvp,1);gl.deleteBuffer(rb);
 const phase=phaseAt(currentTime),fx=effectGeometry(point,scale,phase);
 if(fx.flame.length){const b=buffer(gl,fx.flame,gl.DYNAMIC_DRAW);drawBuffer(gl,b,3,gl.TRIANGLES,[1,.48,.13,.75],mvp);gl.deleteBuffer(b);}
 if(fx.chute.length){const b=buffer(gl,fx.chute,gl.DYNAMIC_DRAW);drawBuffer(gl,b,fx.chute.length/3,gl.LINE_STRIP,[.72,.9,1,.75],mvp);gl.deleteBuffer(b);}
 updateReadout(point,phase);
 drawReplayGraphs();
}
function updateReadout(point,phase){
 const t=nearest(data.timeline,currentTime)||{},sen=nearest(data.sensorTimeline,currentTime)||{};
 const set=(id,v)=>{if($(id))$(id).textContent=v};
 set("v1623dTime",currentTime.toFixed(2)+" s · "+phase.toUpperCase());
 set("v1623dAlt",finite(t.altitudeM)?t.altitudeM.toFixed(2)+" m estimated":"Unavailable");
 set("v1623dVel",finite(t.velocityMps)?t.velocityMps.toFixed(2)+" m/s estimated":"Unavailable");
 set("v1623dAccel",finite(t.accelerationMps2)?t.accelerationMps2.toFixed(2)+" m/s² estimated":"Unavailable");
 set("v1623dG",finite(sen.accelerationG)?sen.accelerationG.toFixed(2)+" g measured":finite(t.estimatedVerticalG)?t.estimatedVerticalG.toFixed(2)+" g estimated":"Unavailable");
 set("v1623dGps",sen.gpsCurrent&&finite(sen.latitude)?sen.latitude.toFixed(6)+", "+sen.longitude.toFixed(6)+" measured":"No current GPS sample at this time");
 set("v1623dPressure",finite(t.pressurePa)?Math.round(t.pressurePa)+" Pa measured":"Unavailable");
 const r=[sen.gyroXDps,sen.gyroYDps,sen.gyroZDps];
 set("v163ReplayRotation",r.every(finite)?Math.hypot(...r).toFixed(2)+" °/s measured":"Unavailable");
 const slider=$("v1623dTimeline");if(slider&&document.activeElement!==slider)slider.value=String(currentTime);
}
function dispose(){
 cancelAnimationFrame(frame);if(!glState)return;const gl=glState.gl;
 for(const key of ["grid","track","raw","markers"])if(glState[key])gl.deleteBuffer(glState[key]);
 if(glState.shader?.program)gl.deleteProgram(glState.shader.program);glState=null;
}
function buildGl(canvas){
 dispose();const gl=canvas.getContext("webgl",{antialias:true,alpha:false,preserveDrawingBuffer:false});
 if(!gl)throw Error("WebGL is unavailable in this browser/device");
 glState={canvas,gl,shader:createProgram(gl)};
 const extent=worldExtent(),grid=gridVertices(extent),track=lineVertices(data.points),raw=lineVertices(data.points,true);
 const markers=[];for(const m of data.phaseMarkers||[])markers.push(m.point.x,m.point.y,m.point.z);
 Object.assign(glState,{grid:buffer(gl,grid),gridCount:grid.length/3,track:buffer(gl,track),trackCount:track.length/3,
  raw:data.mode==="gps+pressure"?buffer(gl,raw):null,markers:buffer(gl,markers),markerCount:markers.length/3});
 camera.distance=Math.max(40,extent*2.4);camera.target=[0,0,Math.max(0,(data.maxAltitudeM||0)*.35)];camera.pan=[0,0,0];
}
function setTime(value,{emit=true}={}){
 if(!data)return;const duration=replayDuration();currentTime=clamp(Number(value)||0,0,duration);
 if(emit)window.dispatchEvent(new CustomEvent("green-ion-v16-replay-time",{detail:{timeS:currentTime,ratio:duration?currentTime/duration:0,source:"3d"}}));
 render();
}
function tick(now){
 if(!playing||!data)return;const dt=lastTick?Math.min(.15,(now-lastTick)/1000):0;lastTick=now;
 const duration=replayDuration();setTime(currentTime+dt*speed);
 if(currentTime>=duration){playing=false;$("v1623dPlay").textContent="Play";return;}
 frame=requestAnimationFrame(tick);
}
export function replaySampleStep(times,t,direction){
 if(!Array.isArray(times)||!times.length)return t;
 if(direction<0){
  for(let i=times.length-1;i>=0;i--)if(Number.isFinite(times[i].timeS)&&times[i].timeS<t-0.00001)return times[i].timeS;
  return times[0].timeS;
 }
 for(const item of times)if(Number.isFinite(item.timeS)&&item.timeS>t+0.00001)return item.timeS;
 return times.at(-1).timeS;
}
function playToggle(){
 if(!data)return;
 if(currentTime>=replayDuration())setTime(0);
 playing=!playing;$("v1623dPlay").textContent=playing?"Pause":"Play";
 lastTick=0;if(playing)frame=requestAnimationFrame(tick);
}
function jump(name){const m=data?.phaseMarkers?.find(x=>x.name===name);if(m)setTime(m.timeS);}
function installPointer(canvas){
 let down=null;
 canvas.addEventListener("pointerdown",e=>{canvas.setPointerCapture(e.pointerId);pointerMap.set(e.pointerId,[e.clientX,e.clientY]);down=[e.clientX,e.clientY];});
 canvas.addEventListener("pointermove",e=>{
  if(!pointerMap.has(e.pointerId))return;const old=pointerMap.get(e.pointerId);pointerMap.set(e.pointerId,[e.clientX,e.clientY]);
  const pts=[...pointerMap.values()];
  if(pts.length===1){
   const dx=e.clientX-old[0],dy=e.clientY-old[1];
   if(e.shiftKey||e.buttons===4){const k=camera.distance*.0015;camera.pan[0]-=dx*k;camera.pan[2]+=dy*k;}
   else{camera.yaw-=dx*.007;camera.pitch=clamp(camera.pitch+dy*.007,-1.35,1.45);camera.mode="free";}
  }else if(pts.length>=2){
   const p0=pts[0],p1=pts[1],dist=Math.hypot(p1[0]-p0[0],p1[1]-p0[1]),center=[(p0[0]+p1[0])/2,(p0[1]+p1[1])/2];
   if(gesture){camera.distance=clamp(gesture.distance*(gesture.pinch/Math.max(10,dist)),2,worldExtent()*30);
    const k=camera.distance*.001;camera.pan[0]-=(center[0]-gesture.center[0])*k;camera.pan[1]+=(center[1]-gesture.center[1])*k;}
   gesture={distance:camera.distance,pinch:dist,center};
  }render();
 });
 const end=e=>{
  const start=down,current=[e.clientX,e.clientY];pointerMap.delete(e.pointerId);if(pointerMap.size<2)gesture=null;
  if(start&&Math.hypot(current[0]-start[0],current[1]-start[1])<5)pick(current[0],current[1],canvas);down=null;
 };
 canvas.addEventListener("pointerup",end);canvas.addEventListener("pointercancel",end);
 canvas.addEventListener("wheel",e=>{e.preventDefault();camera.distance=clamp(camera.distance*Math.exp(e.deltaY*.001),2,worldExtent()*30);camera.mode="free";render();},{passive:false});
}
function pick(clientX,clientY,canvas){
 if(!lastMvp||!data)return;const rect=canvas.getBoundingClientRect(),x=(clientX-rect.left)/rect.width*canvas.width,y=(clientY-rect.top)/rect.height*canvas.height;
 let best=null,bestD=18*(devicePixelRatio||1);
 for(const p of data.points){const ndc=transformPoint(lastMvp,[p.x,p.y,p.z]);if(!ndc||ndc[2]<-1||ndc[2]>1)continue;
  const px=(ndc[0]*.5+.5)*canvas.width,py=(1-(ndc[1]*.5+.5))*canvas.height,d=Math.hypot(px-x,py-y);
  if(d<bestD){bestD=d;best=p;}}
 if(best)setTime(best.timeS);
}
async function load(){
 const select=$("v1623dFlight"),key=select?.value,status=$("v1623dStatus");if(!key){status.textContent="Choose a saved BIN.";return;}
 status.textContent="Reconstructing verified originals in a Web Worker...";
 try{
  const files=await listLocalMetadata(),bin=await getLocalFile(key);if(!bin||bin.kind!=="BIN")throw Error("Selected BIN missing");
  const base=bin.name.replace(/\.BIN$/i,""),senMeta=files.find(x=>x.kind==="SEN"&&x.name.replace(/\.SEN$/i,"")===base),sen=senMeta?await getLocalFile(senMeta.key):null;
  data=await new Promise((resolve,reject)=>{
   const worker=new Worker("./v16_visualization_worker.mjs",{type:"module"}),timer=setTimeout(()=>{worker.terminate();reject(Error("3D reconstruction timed out"));},60000);
   worker.onerror=e=>{clearTimeout(timer);worker.terminate();reject(Error(e.message||"3D worker failed"));};
   worker.onmessage=e=>{clearTimeout(timer);worker.terminate();e.data.error?reject(Error(e.data.error)):resolve(e.data.data);};
   const bb=bin.bytes.slice(0),sb=sen?.bytes?.slice(0)||null;worker.postMessage({id:1,name:bin.name,bin:bb,sen:sb},sb?[bb,sb]:[bb]);
  });
  currentTime=0;playing=false;$("v1623dPlay").textContent="Play";
  prepareReplayGraphSeries();
  const duration=replayDuration(),slider=$("v1623dTimeline");slider.min="0";slider.max=String(duration);slider.step=".01";slider.value="0";
  buildGl($("v1623dCanvas"));status.textContent=(data.mode==="gps+pressure"?"3D GPS + pressure reconstruction":"Pressure-only vertical 3D replay")+
   " · "+data.points.length+" display point(s). Raw originals remain unchanged.";
  $("v1623dLimits").textContent=[...(data.sensorWarnings||[]),...(data.limitations||[])].join(" · ");render();
 }catch(error){data=null;dispose();status.textContent="3D viewer unavailable: "+error.message+" Use the offline analysis graphs instead.";}
}
async function refresh(){
 const select=$("v1623dFlight");if(!select)return;const previous=select.value,files=(await listLocalMetadata()).filter(x=>x.kind==="BIN").sort((a,b)=>String(b.createdAt).localeCompare(String(a.createdAt)));
 select.replaceChildren(new Option("Choose saved BIN",""));for(const f of files)select.append(new Option(f.name,f.key));if(files.some(x=>x.key===previous))select.value=previous;
}
function cameraMode(mode){camera.mode=mode;if(mode==="free"){camera.yaw=-.8;camera.pitch=.45;}render();}
function inject(){
 const history=$("history");if(!history||$("v1623dCard"))return;
 const card=document.createElement("div");card.className="card v162-visual-card";card.id="v1623dCard";
 card.innerHTML='<h2>Interactive 3D Flight Viewer · WebGL</h2>'+
  '<p class="muted">True perspective/depth reconstruction from immutable RKT4/GIS1 originals. GPS is measured; pressure motion and gyro-integrated attitude are estimates. WebGL never runs on the Feather.</p>'+
  '<label>Flight <select id="v1623dFlight"><option value="">Choose saved BIN</option></select></label>'+
  '<div class="actions"><button id="v1623dLoad">Load 3D flight</button><button id="v1623dEffects" class="secondary">Effects: ON</button></div>'+
  '<canvas id="v1623dCanvas" height="520" tabindex="0" role="img" aria-label="Interactive WebGL 3D rocket trajectory. Drag to orbit, Shift-drag to pan, wheel or pinch to zoom."></canvas>'+
  '<div class="actions v162-camera"><button data-camera="free" class="secondary">Free</button><button data-camera="follow" class="secondary">Follow rocket</button><button data-camera="ground" class="secondary">Ground</button><button data-camera="top" class="secondary">Top-down</button><button data-camera="side" class="secondary">Side</button><button id="v1623dReset" class="secondary">Reset</button></div>'+
  '<label>Replay timeline <input id="v1623dTimeline" type="range" min="0" max="1" step=".01" value="0"></label>'+
  '<canvas id="v163ReplayGraphs" height="330" style="width:100%;max-width:100%;display:block" aria-label="Four synchronized replay charts: altitude velocity acceleration angular rate"></canvas>'+
  '<div class="actions"><button id="v1623dPlay">Play</button><button id="v1623dRestart" class="secondary">Restart</button><button id="v1623dBack" class="secondary">Step back</button><button id="v1623dForward" class="secondary">Step forward</button><select id="v1623dSpeed" aria-label="Replay speed"><option>.25</option><option>.5</option><option selected>1</option><option>2</option><option>4</option></select></div>'+
  '<div class="actions"><button data-jump="launch" class="secondary">Launch</button><button data-jump="apogee" class="secondary">Apogee</button><button data-jump="descent" class="secondary">Descent</button><button data-jump="landing" class="secondary">Landing</button></div>'+
  '<div class="row"><span>Angular rate</span><strong id="v163ReplayRotation">--</strong></div>'+
  '<div class="row"><span>Replay</span><strong id="v1623dTime">--</strong></div><div class="row"><span>Altitude</span><strong id="v1623dAlt">--</strong></div><div class="row"><span>Velocity</span><strong id="v1623dVel">--</strong></div><div class="row"><span>Acceleration</span><strong id="v1623dAccel">--</strong></div><div class="row"><span>G evidence</span><strong id="v1623dG">--</strong></div><div class="row"><span>Pressure</span><strong id="v1623dPressure">--</strong></div><div class="row"><span>GPS</span><strong id="v1623dGps">--</strong></div>'+
  '<p id="v1623dStatus" role="status">Choose a saved flight.</p><details><summary>Evidence and limitations</summary><p id="v1623dLimits" class="muted">No flight loaded.</p><p class="muted">Raw and lightly smoothed GPS tracks are both drawn when GPS exists. Exhaust/parachute effects are illustrative only.</p></details>';
 history.append(card);
 $("v1623dLoad").onclick=load;$("v1623dPlay").onclick=playToggle;$("v1623dTimeline").oninput=e=>setTime(e.target.value);
 $("v1623dRestart").onclick=()=>{playing=false;$("v1623dPlay").textContent="Play";setTime(0);};
 $("v1623dBack").onclick=()=>setTime(replaySampleStep(data?.timeline,currentTime,-1));
 $("v1623dForward").onclick=()=>setTime(replaySampleStep(data?.timeline,currentTime,1));
 $("v1623dSpeed").onchange=e=>speed=Number(e.target.value)||1;$("v1623dEffects").onclick=e=>{effects=!effects;e.target.textContent="Effects: "+(effects?"ON":"OFF");render();};
 card.querySelectorAll("[data-camera]").forEach(b=>b.onclick=()=>cameraMode(b.dataset.camera));
 card.querySelectorAll("[data-jump]").forEach(b=>b.onclick=()=>jump(b.dataset.jump));
 $("v1623dReset").onclick=()=>{camera={yaw:-.8,pitch:.45,distance:Math.max(40,worldExtent()*2.4),target:[0,0,Math.max(0,(data?.maxAltitudeM||0)*.35)],pan:[0,0,0],mode:"free"};render();};
 installPointer($("v1623dCanvas"));
 if("IntersectionObserver" in window)new IntersectionObserver(entries=>{visible=!!entries[0]?.isIntersecting;if(visible)render();}).observe(card);
 window.addEventListener("green-ion-v16-graph-progress",e=>{if(data&&e.detail?.source!=="3d")setTime(replayDuration()*clamp(e.detail?.ratio||0,0,1),{emit:false});});
 window.addEventListener("green-ion-history-change",()=>refresh().catch(()=>{}));
 window.addEventListener("pagehide",dispose);
 refresh().catch(error=>$("v1623dStatus").textContent=error.message);
}
/* Live orientation preview reuses this same WebGL rocket mesh and shader,
 * not a second rocket model. Unlike replay this has no simulated trajectory. */
const previewContexts=new WeakMap();
function eulerQuaternion(pitch=0,roll=0,yaw=0){
 const a=pitch*Math.PI/360,b=roll*Math.PI/360,c=yaw*Math.PI/360;
 const x=[Math.cos(a),Math.sin(a),0,0],y=[Math.cos(b),0,Math.sin(b),0],z=[Math.cos(c),0,0,Math.sin(c)];
 const mul=(u,v)=>[u[0]*v[0]-u[1]*v[1]-u[2]*v[2]-u[3]*v[3],
  u[0]*v[1]+u[1]*v[0]+u[2]*v[3]-u[3]*v[2],
  u[0]*v[2]-u[1]*v[3]+u[2]*v[0]+u[3]*v[1],
  u[0]*v[3]+u[1]*v[2]-u[2]*v[1]+u[3]*v[0]];
 return mul(mul(z,y),x);
}
export function renderLiveRocketModel(canvas,angles={pitch:0,roll:0,yaw:0},view={yaw:-.8,pitch:.45,follow:false}){
 if(!canvas)return false;
 try{
  let state=previewContexts.get(canvas);
  if(!state){
   const gl=canvas.getContext("webgl",{antialias:true,alpha:false,preserveDrawingBuffer:false});
   if(!gl)return false;
   state={gl,shader:createProgram(gl),axes:[
    buffer(gl,[0,0,0,1.2,0,0]),buffer(gl,[0,0,0,0,1.2,0]),buffer(gl,[0,0,0,0,0,1.2])
   ]};previewContexts.set(canvas,state);
  }
  const {gl,shader}=state,dpr=Math.min(2,window.devicePixelRatio||1),
   w=Math.max(180,Math.round((canvas.clientWidth||320)*dpr)),h=Math.max(140,Math.round((canvas.clientHeight||230)*dpr));
  if(canvas.width!==w||canvas.height!==h){canvas.width=w;canvas.height=h;}
  gl.viewport(0,0,w,h);gl.enable(gl.DEPTH_TEST);
  gl.clearColor(.03,.08,.12,1);gl.clear(gl.COLOR_BUFFER_BIT|gl.DEPTH_BUFFER_BIT);
  const pitch=clamp(view.pitch??.45,-1.2,1.2),
   yaw=(view.yaw??-.8)+(view.follow?(angles.yaw||0)*Math.PI/180:0),
   eye=[4*Math.cos(pitch)*Math.cos(yaw),4*Math.cos(pitch)*Math.sin(yaw),4*Math.sin(pitch)];
  const mvp=multiplyMatrix(perspectiveMatrix(Math.PI/3,w/h,.1,100),lookAtMatrix(eye,[0,0,0]));
  function paint(buf,count,kind,color){
   gl.useProgram(shader.program);gl.bindBuffer(gl.ARRAY_BUFFER,buf);
   gl.enableVertexAttribArray(shader.pos);gl.vertexAttribPointer(shader.pos,3,gl.FLOAT,false,0,0);
   gl.uniformMatrix4fv(shader.mvp,false,mvp);
   gl.uniform4fv(shader.color,new Float32Array(color));gl.uniform1f(shader.pointSize,1);
   gl.drawArrays(kind,0,count);
  }
  const axesColor=[[1,.5,.53,1],[.45,.91,.69,1],[.48,.86,1,1]];
  state.axes.forEach((b,i)=>paint(b,2,gl.LINES,axesColor[i]));
  const q=eulerQuaternion(angles.pitch||0,angles.roll||0,angles.yaw||0);
  const vertices=transformedRocket({x:0,y:0,z:0},q,1.55),mesh=buffer(gl,vertices,gl.DYNAMIC_DRAW);
  paint(mesh,vertices.length/3,gl.TRIANGLES,[.86,.94,1,1]);gl.deleteBuffer(mesh);
  return true;
 }catch{return false;}
}

export function runWebglSelfTest(canvas){
 const gl=canvas?.getContext?.("webgl",{antialias:false,alpha:false});
 if(!gl)return {supported:false,passed:false,reason:"no-webgl-context"};
 const shader=createProgram(gl),triangle=[-0.7,-0.7,0,0.7,-0.7,0,0,0.7,0],b=buffer(gl,triangle);
 canvas.width=64;canvas.height=64;gl.viewport(0,0,64,64);gl.clearColor(0,0,0,1);gl.clear(gl.COLOR_BUFFER_BIT);
 gl.useProgram(shader.program);gl.bindBuffer(gl.ARRAY_BUFFER,b);gl.enableVertexAttribArray(shader.pos);
 gl.vertexAttribPointer(shader.pos,3,gl.FLOAT,false,0,0);
 gl.uniformMatrix4fv(shader.mvp,false,new Float32Array([1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1]));
 gl.uniform4fv(shader.color,new Float32Array([0.3,0.85,1,1]));gl.uniform1f(shader.pointSize,1);gl.drawArrays(gl.TRIANGLES,0,3);
 const pixel=new Uint8Array(4);gl.readPixels(32,32,1,1,gl.RGBA,gl.UNSIGNED_BYTE,pixel);
 gl.deleteBuffer(b);gl.deleteProgram(shader.program);
 const passed=pixel[2]>100&&pixel[3]>0;return {supported:true,passed,pixel:[...pixel]};
}
export function initV162Webgl(){inject();}
