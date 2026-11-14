/* GREEN ION live graph extension: advisory browser-only, bounded and source-aware. */
export const LIVE_GRAPH_FIELDS=Object.freeze({
 altitude_m:{label:"Relative altitude · estimated",unit:"m"},
 acceleration_g:{label:"Total acceleration · IMU",unit:"g"},
 rotation_magnitude_dps:{label:"Angular rate magnitude · IMU",unit:"°/s"},
 pressure_pa:{label:"Pressure · measured",unit:"Pa"},
 vertical_velocity_mps:{label:"Vertical velocity · barometric estimate",unit:"m/s"}
});
const MAX_POINTS=300;
const finite=x=>typeof x==="number"&&Number.isFinite(x);
const $=id=>document.getElementById(id);
function numReading(doc,key){
 const r=doc?.readings?.[key];
 return r&&r.freshness==="live"&&finite(r.value)&&finite(Number(r.age_ms))&&Number(r.age_ms)<=6500?r.value:null;
}
export function liveGraphSample(doc,at=Date.now()){
 const value=k=>numReading(doc,k);
 const gx=value("rotation_x_dps"),gy=value("rotation_y_dps"),gz=value("rotation_z_dps");
 const angular=[gx,gy,gz].every(finite)?Math.hypot(gx,gy,gz):null;
 return {at,source:doc?.primary_source||"none",values:{
  altitude_m:value("altitude_m"),acceleration_g:value("acceleration_g"),
  rotation_magnitude_dps:angular,pressure_pa:value("pressure_pa"),
  // GPS ground speed is intentionally never used as vertical speed.
  vertical_velocity_mps:null
 },quality:doc?.primary_source==="none"?"missing":"received"};
}
export function liveGraphSampleFromStatus(status,source,at=Date.now()){
 const active=source==="wifi"||source==="lora";
 const v=key=>active&&finite(status?.[key])?status[key]:null;
 const gx=v("rotation_x_dps"),gy=v("rotation_y_dps"),gz=v("rotation_z_dps");
 return {at,source:source||"none",values:{
  altitude_m:v("relative_altitude_m")??v("altitude_m"),
  acceleration_g:v("acceleration_g"),
  rotation_magnitude_dps:[gx,gy,gz].every(finite)?Math.hypot(gx,gy,gz):null,
  pressure_pa:v("pressure_pa"),
  // Only the actual vertical estimator is usable, never horizontal GPS speed.
  vertical_velocity_mps:v("vertical_velocity_mps")??v("velocity_mps")
 },quality:active?"received":"missing"};
}
export function appendBoundedGraph(samples,sample,{maxPoints=MAX_POINTS,maxAgeMs=60000}={}){
 if(!sample||!finite(sample.at))return samples;
 const cap=Math.max(1,Math.min(1200,Math.trunc(maxPoints)||MAX_POINTS));
 const windowMs=Math.max(1000,Math.min(300000,maxAgeMs));
 const next=samples.filter(x=>finite(x.at)&&x.at>=sample.at-windowMs&&x.at<sample.at);
 next.push(sample);
 return next.slice(-cap);
}
function draw(canvas,records,key,seconds){
 const r=canvas.getBoundingClientRect(),w=Math.max(200,Math.round(r.width||320)),h=230,dpr=Math.min(2,window.devicePixelRatio||1);
 if(canvas.width!==Math.round(w*dpr)||canvas.height!==Math.round(h*dpr)){canvas.width=Math.round(w*dpr);canvas.height=Math.round(h*dpr);}
 const c=canvas.getContext("2d");if(!c)return;
 c.setTransform(dpr,0,0,dpr,0,0);c.fillStyle="#0b1c2b";c.fillRect(0,0,w,h);
 const now=Date.now(),visible=records.filter(s=>now-s.at<=seconds*1000&&now>=s.at);
 const points=visible.filter(x=>finite(x.values[key]));c.font="12px system-ui";
 if(!points.length){c.fillStyle="#c6dbea";c.fillText("No fresh readings in this time window",12,h/2);return;}
 let min=Math.min(...points.map(x=>x.values[key])),max=Math.max(...points.map(x=>x.values[key]));
 const pad=Math.max((max-min)*.12,.01);min-=pad;max+=pad;
 const x=t=>42+(t-(now-seconds*1000))/(seconds*1000)*(w-56);
 const y=v=>h-24-(v-min)/(max-min)*(h-42);
 c.strokeStyle="#35536e";c.lineWidth=1;
 for(let i=0;i<5;i++){const yy=15+(h-40)*i/4;c.beginPath();c.moveTo(42,yy);c.lineTo(w-12,yy);c.stroke();}
 c.strokeStyle="#7bdbff";c.lineWidth=2;c.beginPath();
 let drawing=false,prevAt=0;
 for(const s of visible){const v=s.values[key];if(!finite(v)){drawing=false;continue;}
  const xx=x(s.at),yy=y(v);
  if(!drawing||s.at-prevAt>2500)c.moveTo(xx,yy);else c.lineTo(xx,yy);
  drawing=true;prevAt=s.at;
 }
 c.stroke();c.fillStyle="#c6dbea";c.fillText(max.toFixed(2),4,15);c.fillText(min.toFixed(2),4,h-24);
 c.fillText(seconds+" s",w-42,h-5);
}
export function initV163LiveGraphs(transport){
 const card=$("v14SensorLabCard");if(!card||$("v163GraphBlock"))return;
 const section=document.createElement("div");section.id="v163GraphBlock";section.className="v162-visual-card";
 section.style.cssText="margin-top:12px;min-width:0;max-width:100%";
 section.innerHTML='<h3>Live sensor graphs</h3><p class="muted">Selected sensor trend only. Missing/stale measurements create gaps; sampled display values do not affect recording.</p>'+
  '<div class="actions" style="display:flex;flex-wrap:wrap;gap:8px">'+
  '<label>Measurement<select id="v163GraphMetric"></select></label>'+
  '<label>Window<select id="v163GraphWindow"><option value="10">10 s</option><option value="30" selected>30 s</option><option value="60">60 s</option></select></label></div>'+
  '<canvas id="v163GraphCanvas" height="230" style="width:100%;max-width:100%;display:block" aria-label="Bounded live sensor trend graph"></canvas>'+
  '<p id="v163GraphStatus" class="muted" role="status">Waiting for telemetry.</p>';
 card.append(section);
 const metric=$("v163GraphMetric");
 for(const [key,info] of Object.entries(LIVE_GRAPH_FIELDS))metric.add(new Option(info.label+" ("+info.unit+")",key));
 let records=[],lastAppend=0,lastDraw=0;
 function accept(s){
  if(!s||s.at-lastAppend<200)return;
  lastAppend=s.at;records=appendBoundedGraph(records,s);
  paint();
 }
 function paint(){
  if(document.hidden||!section.getClientRects().length)return;
  const now=Date.now();if(now-lastDraw<180)return;lastDraw=now;
  const key=metric.value,sec=Number($("v163GraphWindow").value),recent=records.at(-1);
  draw($("v163GraphCanvas"),records,key,sec);
  const v=recent?.values[key],age=recent?Math.max(0,now-recent.at):Infinity;
  $("v163GraphStatus").textContent=finite(v)&&age<=6500?
   v.toFixed(2)+" "+LIVE_GRAPH_FIELDS[key].unit+" · "+recent.source+" · sampled "+Math.round(age/1000)+" s ago":
   "Unavailable/stale · no fresh "+LIVE_GRAPH_FIELDS[key].label.toLowerCase()+" reading";
 }
 metric.onchange=paint;$("v163GraphWindow").onchange=paint;
 window.addEventListener("green-ion-v16-telemetry",event=>{
  const s=liveGraphSample(event.detail),raw=transport?.state?.lastWifiStatus??transport?.state?.lastStatus;
  // Full Feather status may include a barometric vertical estimator that is not
  // carried by receive-only radio telemetry or the unified telemetry schema.
  // Do not reuse GPS ground speed for this graph.
  if(transport?.wifiFresh?.()&&raw&&event.detail?.primary_source==="wifi"){
   const v=raw.vertical_velocity_mps??raw.velocity_mps;
   if(typeof v==="number"&&Number.isFinite(v))s.values.vertical_velocity_mps=v;
  }
  accept(s);
 });
 window.addEventListener("green-ion-link",()=>{
  if(window.GREEN_ION_UNIFIED_TELEMETRY)return;
  const src=transport.source(),raw=src==="wifi"?transport.state.lastWifiStatus??transport.state.lastStatus:
    src==="lora"?transport.statusFromRadio():{};
  accept(liveGraphSampleFromStatus(raw,src));
 });
 document.addEventListener("visibilitychange",paint);
 window.addEventListener("resize",paint);
}
