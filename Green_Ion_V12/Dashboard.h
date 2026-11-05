#pragma once

#include <Arduino.h>

// Diagnostics are request-only and are rejected during recording. While launch
// watching, they only observe existing samples and never reinitialize hardware.
static const char DASHBOARD_HTML[] PROGMEM = R"HTML(
<!doctype html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>GREEN ION V12</title><style>
:root{color-scheme:dark;--g:#6df49d;--y:#ffd66d;--r:#ff7b7b;--m:#9bb5a5}*{box-sizing:border-box}body{margin:0;background:#07110d;color:#effff5;font:15px system-ui}main{max-width:960px;margin:auto;padding:20px}h1{letter-spacing:.08em}h2{margin-top:30px}.note{color:var(--m)}.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(190px,1fr));gap:12px}.card{background:#10231a;border:1px solid #2b4b39;border-radius:14px;padding:14px;overflow-wrap:anywhere}.label{color:var(--m);font-size:11px;letter-spacing:.12em;text-transform:uppercase}.value{font-size:21px;margin-top:6px;font-variant-numeric:tabular-nums}.detail{color:var(--m);margin-top:6px;line-height:1.45}.ok{color:var(--g)}.warn{color:var(--y)}.bad{color:var(--r)}.actions{display:flex;flex-wrap:wrap;gap:10px;margin:14px 0}button{border:1px solid #4d8b67;border-radius:10px;background:#173b28;color:#fff;padding:12px 16px;font-weight:700;font-size:15px}button:disabled{opacity:.42}.message{min-height:24px;color:var(--y)}</style></head>
<body><main><h1>GREEN ION · V12</h1><p class="note">Recorder and recovery aid only</p>
<div class="grid">
<div class="card"><div class="label">Recording</div><div id="recording" class="value">—</div></div>
<div class="card"><div class="label">Log file</div><div id="file" class="value">—</div></div>
<div class="card"><div class="label">Pressure</div><div id="pressure" class="value">—</div></div>
<div class="card"><div class="label">Barometric altitude</div><div id="altitude" class="value">—</div></div>
<div class="card"><div class="label">Battery</div><div id="battery" class="value">—</div></div>
<div class="card"><div class="label">Health bits</div><div id="health" class="value">—</div></div>
</div>
<h2>Hardware Tests</h2><p class="note">Keep the Feather still for the five-second IMU test. GPS communication can pass indoors even when the fix is inconclusive. Healthy sensors are observed passively; reconnecting a missing sensor restarts preflight calibration. Tests stop if recording begins.</p>
<div class="actions"><button data-test="imu">Test IMU</button><button data-test="gps">Test GPS</button><button data-test="all">Test All Sensors</button></div><div id="testMessage" class="message"></div>
<div class="grid">
<div class="card"><div class="label">IMU test</div><div id="imuResult" class="value">IDLE</div><div id="imuDetail" class="detail">—</div></div>
<div class="card"><div class="label">Live acceleration X / Y / Z / total</div><div id="imuAccel" class="value">—</div></div>
<div class="card"><div class="label">Live gyro X / Y / Z</div><div id="imuGyro" class="value">—</div><div id="imuTemp" class="detail">—</div></div>
<div class="card"><div class="label">GPS test</div><div id="gpsResult" class="value">IDLE</div><div id="gpsComm" class="detail">—</div></div>
<div class="card"><div class="label">GPS fix</div><div id="gps" class="value">—</div><div id="gpsPosition" class="detail">—</div></div>
<div class="card"><div class="label">BMP388 status</div><div id="barometer" class="value">—</div><div class="detail">Included in Test All as a live readiness check</div></div>
</div>
<h2>Recorder Evidence</h2><div class="grid">
<div class="card"><div class="label">Recovery</div><div id="recovery" class="value">—</div></div>
<div class="card"><div class="label">Missed sample periods</div><div id="missed" class="value">—</div></div>
<div class="card"><div class="label">Metadata</div><div id="metadata" class="value">—</div></div>
</div><p class="note" id="updated"></p></main><script>
const q=id=>document.getElementById(id),f=(v,n=1)=>v!==null&&Number.isFinite(Number(v))?Number(v).toFixed(n):'—';
const resultClass=v=>v==='PASS'?'ok':v==='FAIL'?'bad':(v==='INCONCLUSIVE'||v==='DEFERRED')?'warn':'';
function setResult(id,v){const e=q(id);e.textContent=v||'IDLE';e.className='value '+resultClass(v)}
async function runTest(sensor){q('testMessage').textContent='Requesting '+sensor.toUpperCase()+' test…';try{const r=await fetch('/api/hardware-test',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'sensor='+encodeURIComponent(sensor)});const d=await r.json();q('testMessage').textContent=d.accepted?'Test started. Keep the Feather still and leave this page open.':(d.error||'Test refused')}catch{q('testMessage').textContent='Could not reach the Feather'}}
document.querySelectorAll('button[data-test]').forEach(b=>b.onclick=()=>runTest(b.dataset.test));
async function tick(){try{const d=await fetch('/api/status',{cache:'no-store'}).then(r=>r.json());q('recording').textContent=d.recording?'ACTIVE':'IDLE';q('recording').className='value '+(d.recording?'ok':'');q('file').textContent=d.log_file||'—';q('pressure').textContent=f(d.pressure_pa,0)+' Pa';q('altitude').textContent=f(d.altitude_m,2)+' m';q('battery').textContent=f(d.battery_v,2)+' V';q('health').textContent=d.health;setResult('imuResult',d.imu_test_result);q('imuDetail').textContent=d.imu_test_message+' · ±32 g verified: '+(d.imu_range_32g?'YES':'NO')+' · samples '+d.imu_test_samples+' · mean '+f(d.imu_test_mean_g,3)+' g · bias '+[d.imu_test_bias_x_dps,d.imu_test_bias_y_dps,d.imu_test_bias_z_dps].map(v=>f(v,2)).join(' / ')+' °/s · '+f(d.imu_test_temperature_c,1)+' °C';q('imuAccel').textContent=[d.acceleration_x_g,d.acceleration_y_g,d.acceleration_z_g,d.acceleration_g].map(v=>f(v,3)).join(' / ')+' g';q('imuGyro').textContent=[d.rotation_x_dps,d.rotation_y_dps,d.rotation_z_dps].map(v=>f(v,2)).join(' / ')+' °/s';q('imuTemp').textContent='Temperature '+f(d.imu_temperature_c,1)+' °C';setResult('gpsResult',d.gps_test_result);q('gpsComm').textContent=d.gps_test_message+' · Communication: '+(d.gps_test_communication?'PASS':d.gps_test_result==='RUNNING'?'TESTING':'NOT CONFIRMED')+' · NMEA '+d.gps_test_nmea+' · parsed '+d.gps_test_parsed;q('gps').textContent=d.gps_status.toUpperCase()+' · '+d.satellites+' satellites';q('gpsPosition').textContent='Lat '+f(d.latitude,7)+' · Lon '+f(d.longitude,7)+' · Alt '+f(d.gps_altitude_m,1)+' m · Speed '+f(d.gps_speed_mps,2)+' m/s · UTC '+String(d.gps_utc_hour).padStart(2,'0')+':'+String(d.gps_utc_minute).padStart(2,'0')+':'+String(d.gps_utc_second).padStart(2,'0')+' · age '+(d.gps_fix_age_ms===4294967295?'none':d.gps_fix_age_ms+' ms');q('barometer').textContent=d.barometer_ready&&Number.isFinite(Number(d.pressure_pa))?'PASS':d.barometer_ready?'INCONCLUSIVE':'FAIL';q('barometer').className='value '+(q('barometer').textContent==='PASS'?'ok':q('barometer').textContent==='FAIL'?'bad':'warn');q('recovery').textContent=d.recovery_status||'—';q('missed').textContent=d.missed_sample_periods;q('metadata').textContent=(d.metadata_file||'—')+' · '+d.metadata_failures+' errors';document.querySelectorAll('button[data-test]').forEach(b=>b.disabled=!d.hardware_tests_allowed);if(!d.hardware_tests_allowed)q('testMessage').textContent='Hardware tests locked: recording is active.';q('updated').textContent='Updated '+new Date().toLocaleTimeString()}catch{q('updated').textContent='Recorder connection unavailable'}}
setInterval(tick,1000);tick();</script></body></html>
)HTML";
