#pragma once
// V17 direct-Feather Quick Fly dashboard.
// Same-origin, offline launch-day UI. Advanced controls remain on "/".
const char QUICK_FLY_HTML[] PROGMEM = R"QFLY(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<meta name="theme-color" content="#081420">
<link rel="manifest" href="/manifest.webmanifest">
<title>GREEN ION V17 · Quick Fly</title>
<style>
:root{--bg:#081420;--card:#152b40;--line:#35536e;--txt:#eff8ff;--muted:#a5bbcd;--blue:#7bdbff;--green:#72e5ab;--yellow:#ffdd88;--red:#ff91a0}
*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--txt);font:16px/1.45 system-ui,-apple-system,sans-serif}main{width:min(760px,100%);margin:auto;padding:max(14px,env(safe-area-inset-top)) 12px max(28px,env(safe-area-inset-bottom))}
header{display:flex;justify-content:space-between;gap:10px;align-items:center;margin-bottom:12px}h1{margin:0;font-size:clamp(25px,7vw,36px)}h2{margin:0 0 12px;font-size:18px}small,.muted{display:block;color:var(--muted)}.ok{color:var(--green)!important}.bad{color:var(--red)!important}.warn{color:var(--yellow)!important}
.pill{border:1px solid var(--line);border-radius:999px;padding:9px 12px;font-size:12px;font-weight:800}.card,.state{background:var(--card);border:1px solid var(--line);border-radius:18px;padding:16px;margin-bottom:12px}.state{min-height:118px;display:grid;place-items:center;text-align:center;border-width:2px}.state small{letter-spacing:.15em}.live{font-size:clamp(40px,13vw,70px);line-height:1;font-weight:950}.live.good{color:var(--green)}.live.busy{color:var(--yellow)}.live.error{color:var(--red)}
.progress{height:14px;background:#314d66;border-radius:999px;overflow:hidden;margin:10px 0}.progress div{height:100%;width:0;background:var(--blue);transition:width .2s}.row{display:flex;justify-content:space-between;gap:12px;padding:9px 0;border-bottom:1px solid var(--line)}.row:last-child{border:0}.row strong{text-align:right;overflow-wrap:anywhere}
.grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:9px}.metric{border:1px solid var(--line);border-radius:13px;padding:11px;min-width:0}.metric span{display:block;color:var(--muted);font-size:11px;text-transform:uppercase}.metric strong{display:block;margin-top:5px;overflow-wrap:anywhere}
button,a.btn{width:100%;border:0;border-radius:14px;min-height:58px;padding:14px 15px;font:850 16px system-ui;text-align:center;text-decoration:none;display:grid;place-items:center;background:var(--blue);color:#07131f;cursor:pointer}.primary{min-height:74px!important;font-size:20px!important;background:var(--green)!important}.secondary{background:#34546f!important;color:var(--txt)!important}button:disabled{opacity:.4}.actions{display:grid;gap:10px}.notice{background:#233f57;border-radius:12px;padding:12px;margin-top:10px;overflow-wrap:anywhere}.history{display:flex;align-items:center;justify-content:space-between;gap:8px;padding:10px 0;border-bottom:1px solid var(--line)}.history div:last-child{display:flex;gap:6px;flex-wrap:wrap;justify-content:flex-end}.history a{display:inline-block;text-decoration:none;border-radius:9px;padding:8px 10px;background:var(--blue);color:#07131f;font-weight:800;font-size:12px}.history a.secondary{color:var(--txt)}.links{display:grid;grid-template-columns:1fr 1fr;gap:8px}
@media(max-width:520px){.history{display:block}.history div:last-child{justify-content:flex-start;margin-top:8px}.links{grid-template-columns:1fr}}@media(max-width:350px){.grid{grid-template-columns:1fr}}@media(pointer:coarse){button,a.btn{min-height:60px}}
</style>
</head>
<body>
<main>
<header><div><h1>GREEN ION</h1><small>V17 · Quick Fly · Direct Feather</small></div><div id="connection" class="pill">CONNECTING</div></header>
<section class="state"><div><small>LIVE FLIGHT STATUS</small><div id="live" class="live error">ERROR</div></div></section>
<section class="card">
<h2>Launch preparation</h2><div class="progress"><div id="progress"></div></div><small id="progressText">Waiting for firmware status</small>
<div class="row"><span>Firmware phase</span><strong id="phase">--</strong></div>
<div class="actions" style="margin-top:12px"><button id="prepare" class="primary">PREPARE &amp; ARM</button><button id="quick">QUICK FLY · REPEAT FLIGHT</button><button id="again" class="primary" hidden>FLY AGAIN</button></div>
<div id="message" class="notice" role="status"></div><small style="margin-top:8px">Status and progress come from the Feather. Quick Fly still verifies microSD writing, critical sensors, battery and a fresh altitude baseline.</small>
</section>
<section class="card"><h2>Flight-ready essentials</h2><div class="grid">
<div class="metric"><span>Flight counter</span><strong id="counter">--</strong><small id="nextFile">--</small></div>
<div class="metric"><span>microSD · mandatory</span><strong id="sd">--</strong></div>
<div class="metric"><span>Battery</span><strong id="battery">--</strong><small id="batteryNote">--</small></div>
<div class="metric"><span>Sensors</span><strong id="sensors">--</strong></div>
<div class="metric"><span>Storage</span><strong id="storage">--</strong><small id="storageNote">--</small></div>
<div class="metric"><span>GPS</span><strong id="gps">--</strong></div>
<div class="metric"><span>LoRa</span><strong id="lora">--</strong></div>
</div><div id="warnings" class="notice"></div></section>
<section id="saved" class="card" hidden><h2>Last flight saved</h2><div class="row"><span>File</span><strong id="savedName">--</strong></div><div class="row"><span>Size</span><strong id="savedSize">--</strong></div><div class="row"><span>Duration</span><strong id="savedDuration">--</strong></div><div class="row"><span>Verified save</span><strong id="savedVerified">NO</strong></div></section>
<section class="card"><h2>Flight history</h2><div id="history" class="muted">Loading SD flights…</div><div class="links" style="margin-top:12px"><button id="refresh" class="secondary">Refresh history</button><a class="btn secondary" href="/">Advanced dashboard</a></div><small style="margin-top:8px">Use the full dashboard for replay, comparison, analysis, diagnostics and settings.</small></section>
</main>
<script>
"use strict";
const $=id=>document.getElementById(id);
let status=null,files={files:[]},connected=false,lock=false,lastCommand=0,verified=false,verifiedFile="",lastMode=null,savingAt=0,pollTimer=null,failures=0;
const num=v=>Number.isFinite(Number(v))?Number(v):null;
function fmtBytes(v){const n=num(v);if(n===null)return"--";if(n>=1048576)return(n/1048576).toFixed(n>=10485760?1:2)+" MB";if(n>=1024)return(n/1024).toFixed(1)+" KB";return Math.round(n)+" B"}
function duration(ms){const n=num(ms);if(n===null)return"--";const s=n/1000;return s<120?s.toFixed(1)+" s":Math.floor(s/60)+"m "+Math.round(s%60)+"s"}
function meta(text){const o={};String(text||"").split(/\r?\n/).forEach(line=>{const i=line.indexOf("=");if(i>0)o[line.slice(0,i).trim()]=line.slice(i+1).trim()});return{durationMs:num(o.duration_ms),fileBytes:num(o.file_bytes)}}
function state(){if(!connected||!status||status.flight_prep_failure||status.recorder_mode===4||status.sd_ok===false)return"ERROR";if(status.recording||status.recorder_mode===2)return"FLYING";if(status.flight_prep_active)return"TESTING";if(status.armed||status.flight_prep_ready||status.recorder_mode===1)return"ARMED";if(status.recorder_mode===3)return verified?"SAVED":"SAVING";return"READY"}
function nextNumber(){const n=num(files.next_sequence);if(n!==null)return Math.floor(n)+1;let max=-1;(files.files||[]).forEach(f=>{const m=/^V12D(\d{3})\.BIN$/.exec(f.name||"");if(m)max=Math.max(max,+m[1])});return max+2}
async function api(path,method="GET",body=null,wantText=false){const opt={method,cache:"no-store"};if(body){opt.headers={"Content-Type":"application/x-www-form-urlencoded"};opt.body=new URLSearchParams(body).toString()}const r=await fetch("/api/"+path,opt);const data=wantText?await r.text():await r.json().catch(()=>({}));if(!r.ok||(!wantText&&data.success===false))throw Error((!wantText&&data.message)||("HTTP "+r.status));return data}
function render(){
 const s=state(),d=status||{},live=$("live");live.textContent=s;live.className="live "+(["READY","ARMED","SAVED"].includes(s)?"good":(["TESTING","SAVING"].includes(s)?"busy":"error"));
 $("connection").textContent=connected?"LIVE · V17":"RECONNECTING";$("connection").className="pill"+(connected?"":" bad");
 const p=Math.max(0,Math.min(100,num(d.flight_prep_progress)||0));$("progress").style.width=p+"%";$("progressText").textContent=d.flight_prep_active?(p+"% · "+(d.flight_prep_phase||"TESTING")):(s==="ARMED"?"100% · ARMED":s==="SAVED"?"100% · VERIFIED SAVE":"Ready for command");$("phase").textContent=d.flight_prep_phase||"--";
 $("sd").textContent=d.sd_ok?(d.sd_integrity_verified?"VERIFIED":"DETECTED"):"NOT READY";$("sd").className=d.sd_ok?"ok":"bad";
 $("battery").textContent=num(d.battery_v)===null?"--":Number(d.battery_v).toFixed(2)+" V";$("batteryNote").textContent=num(d.battery_v)===null?"Battery unavailable":d.battery_v<3.60?"CRITICAL · firmware blocks arming":d.battery_v<3.75?"LOW · recharge before launch":"Battery OK";$("batteryNote").className=d.battery_v<3.60?"bad":d.battery_v<3.75?"warn":"ok";
 $("sensors").textContent=d.barometer_ready&&d.imu_ok?"BARO + IMU OK":"CHECK SENSORS";$("gps").textContent=d.gps_status==="live"?"LIVE · "+(d.satellites||0)+" sats":"OPTIONAL · "+String(d.gps_status||"missing").toUpperCase();$("lora").textContent=d.radio_ok?"OPTIONAL · READY":"OPTIONAL · UNAVAILABLE";
 const free=num(d.sd_free_bytes)!==null?num(d.sd_free_bytes):(num(files.total)!==null&&num(files.used)!==null?num(files.total)-num(files.used):null),total=num(d.sd_total_bytes)||num(files.total);$("storage").textContent=free===null?"--":fmtBytes(free)+" free";const low=free!==null&&total&&((free<8388608)||(free/total<.05));$("storageNote").textContent=low?"LOW STORAGE · back up old flights":"Storage OK";$("storageNote").className=low?"warn":"ok";
 $("counter").textContent="Flight "+nextNumber();const seq=num(files.next_sequence);$("nextFile").textContent=seq===null?"Firmware reserves filename":"V12D"+String(Math.floor(seq)).padStart(3,"0")+".BIN";
 const w=[];if(d.gps_status!=="live")w.push("GPS unavailable/stale — optional recovery aid.");if(!d.radio_ok)w.push("LoRa unavailable — optional. microSD recording remains mandatory.");$("warnings").innerHTML=w.length?w.map(x=>"<div class='warn'>⚠ "+x+"</div>").join(""):"<div class='ok'>Optional GPS and LoRa status available.</div>";
 const blocked=lock||!connected||d.recording||d.flight_prep_active||d.armed||d.recorder_mode===5||d.recorder_mode===6;$("prepare").disabled=blocked;$("quick").disabled=blocked;$("again").hidden=s!=="SAVED";$("again").disabled=s!=="SAVED"||lock||!connected;
 if(d.flight_prep_failure)$("message").textContent="ERROR · "+d.flight_prep_failure;
}
async function history(show=false){if(!connected||status&&status.recording)return;try{files=await api("files");renderHistory();render();if(show)$("message").textContent="Flight history refreshed."}catch(e){if(show)$("message").textContent="History · "+e.message}}
function renderHistory(){const host=$("history"),list=[...(files.files||[])].sort((a,b)=>String(b.name).localeCompare(String(a.name))).slice(0,8);if(!list.length){host.className="muted";host.textContent="No flight BIN files reported by the Feather.";return}host.className="";host.innerHTML="";list.forEach(f=>{const row=document.createElement("div");row.className="history";const info=document.createElement("div");const strong=document.createElement("strong");strong.textContent=f.name;const small=document.createElement("small");small.textContent=fmtBytes(f.size)+" · "+(f.recording_complete?"VERIFIED COMPLETE":"CHECK RECOVERY");info.append(strong,small);const act=document.createElement("div");const dl=document.createElement("a");dl.href="/api/download?name="+encodeURIComponent(f.name);dl.textContent="Download";dl.download="";const adv=document.createElement("a");adv.href="/";adv.textContent="Analyze";adv.className="secondary";act.append(dl,adv);row.append(info,act);host.append(row)})}
async function verify(){const name=String(status&&status.log_file||"").replace(/^\//,"");if(!name||verified&&verifiedFile===name)return;try{files=await api("files");const f=(files.files||[]).find(x=>x.name===name);if(!f||f.recording_complete!==true){if(savingAt&&Date.now()-savingAt>12000)$("message").textContent="Save verification is taking longer than expected. Keep power connected.";renderHistory();return}const t=await api("metadata?name="+encodeURIComponent(name),"GET",null,true),m=meta(t);verified=true;verifiedFile=name;savingAt=0;$("saved").hidden=false;$("savedName").textContent=name;$("savedSize").textContent=fmtBytes(f.size);$("savedDuration").textContent=duration(m.durationMs);$("savedVerified").textContent="YES · completion metadata";renderHistory();render()}catch(e){$("message").textContent="Save verification · "+e.message}}
async function prepare(mode){const now=Date.now();if(lock||now-lastCommand<1400||!connected||status&&((status.recording)||(status.flight_prep_active)||(status.armed)))return;lock=true;lastCommand=now;verified=false;verifiedFile="";render();$("message").textContent=mode==="quick"?"Starting repeat-flight checks…":"Starting complete Prepare & Arm…";try{const r=await api("preflight/start","POST",{mode});$("message").textContent=r.message||"Firmware accepted preparation."}catch(e){$("message").textContent="ERROR · "+e.message}finally{lock=false;poll(true)}}
async function poll(force=false){clearTimeout(pollTimer);try{const d=await api("status");if(num(d.version)!==17)throw Error("V17 firmware required");status=d;connected=true;failures=0;if(lastMode===2&&d.recorder_mode===3){verified=false;savingAt=Date.now()}lastMode=d.recorder_mode;render();if(!d.recording&&(force||!files.files.length))await history(false);if(d.recorder_mode===3&&!verified)await verify();pollTimer=setTimeout(()=>poll(),850)}catch(e){connected=false;failures++;$("message").textContent="Reconnecting · "+e.message;render();pollTimer=setTimeout(()=>poll(),Math.min(5000,900+650*failures))}}
$("prepare").onclick=()=>prepare("full");$("quick").onclick=()=>prepare("quick");$("again").onclick=()=>prepare("quick");$("refresh").onclick=()=>history(true);poll(true);
if("serviceWorker"in navigator)navigator.serviceWorker.register("/service-worker.js").catch(()=>{});
</script>
</body>
</html>
)QFLY";
