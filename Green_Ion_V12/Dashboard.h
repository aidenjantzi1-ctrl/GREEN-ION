#pragma once

#include <Arduino.h>

// Read-only recorder status. Flight-critical recording never depends on this
// page or on a connected client.
static const char DASHBOARD_HTML[] PROGMEM = R"HTML(
<!doctype html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>GREEN ION V12</title><style>
:root{color-scheme:dark;--g:#6df49d;--m:#9bb5a5}*{box-sizing:border-box}body{margin:0;background:#07110d;color:#effff5;font:15px system-ui}main{max-width:900px;margin:auto;padding:22px}h1{letter-spacing:.08em}.note{color:var(--m)}.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(180px,1fr));gap:12px}.card{background:#10231a;border:1px solid #2b4b39;border-radius:14px;padding:14px}.label{color:var(--m);font-size:11px;letter-spacing:.12em;text-transform:uppercase}.value{font-size:24px;margin-top:6px;font-variant-numeric:tabular-nums}.ok{color:var(--g)}</style></head>
<body><main><h1>GREEN ION · V12</h1><p class="note">Read-only recorder status · recovery aid only</p><div class="grid">
<div class="card"><div class="label">Recording</div><div id="recording" class="value">—</div></div>
<div class="card"><div class="label">Log file</div><div id="file" class="value">—</div></div>
<div class="card"><div class="label">Pressure</div><div id="pressure" class="value">—</div></div>
<div class="card"><div class="label">Barometric altitude</div><div id="altitude" class="value">—</div></div>
<div class="card"><div class="label">Acceleration</div><div id="acceleration" class="value">—</div></div>
<div class="card"><div class="label">Rotation X/Y/Z</div><div id="rotation" class="value">—</div></div>
<div class="card"><div class="label">Battery</div><div id="battery" class="value">—</div></div>
<div class="card"><div class="label">GPS</div><div id="gps" class="value">—</div></div>
<div class="card"><div class="label">Health bits</div><div id="health" class="value">—</div></div>
</div><p class="note" id="updated"></p></main><script>
const q=id=>document.getElementById(id),f=(v,n=1)=>Number.isFinite(Number(v))?Number(v).toFixed(n):'—';
async function tick(){try{const d=await fetch('/api/status',{cache:'no-store'}).then(r=>r.json());q('recording').textContent=d.recording?'ACTIVE':'IDLE';q('recording').className='value '+(d.recording?'ok':'');q('file').textContent=d.log_file||'—';q('pressure').textContent=f(d.pressure_pa,0)+' Pa';q('altitude').textContent=f(d.altitude_m,2)+' m';q('acceleration').textContent=f(d.acceleration_g,2)+' g';q('rotation').textContent=[d.rotation_x_dps,d.rotation_y_dps,d.rotation_z_dps].map(v=>f(v,1)).join(' / ')+' °/s';q('battery').textContent=f(d.battery_v,2)+' V';q('gps').textContent=d.gps_status.toUpperCase()+' · '+d.satellites+' sat';q('health').textContent=d.health;q('updated').textContent='Updated '+new Date().toLocaleTimeString()}catch{q('updated').textContent='Recorder connection unavailable'}}
setInterval(tick,1000);tick();</script></body></html>
)HTML";
