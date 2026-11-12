#pragma once
// Served directly from flash. Standalone header avoids Arduino sketch preprocessing HTML/JS.
const char DASHBOARD_HTML[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport"
 content="width=device-width, initial-scale=1">
<meta name="theme-color" content="#081420">
<link rel="manifest" href="/manifest.webmanifest">
<title>GREEN-ION · Flight Dashboard</title>
<style>
:root{
 --bg:#081420;
 --card:#172b40;
 --line:#35536e;
 --txt:#eff8ff;
 --muted:#a5bbcd;
 --blue:#7bdbff;
 --green:#72e5ab;
 --red:#ff91a0;
 --yellow:#ffdd88
}
*{box-sizing:border-box}
body{margin:0;background:var(--bg);color:var(--txt);
 font:15px/1.5 system-ui,Arial,sans-serif}
main{max-width:1240px;margin:auto;padding:20px clamp(12px,2vw,32px) 55px}
h1{font-size:27px;margin:0}
h2{font-size:19px;margin:0 0 16px}
p{margin-top:12px}
small,.muted{color:var(--muted)}
header,.row,.units{
 display:flex;align-items:center;justify-content:space-between;
 gap:14px}
header{margin-bottom:19px}
.row{border-bottom:1px solid var(--line);padding:12px 0}
.row:last-child{border:0}
.row strong{text-align:right;overflow-wrap:anywhere}
.status{
 border:1px solid var(--line);padding:10px 14px;
 border-radius:11px;color:var(--yellow);font-weight:bold}
.units,.card,.metric{
 background:var(--card);border:1px solid var(--line);
 border-radius:15px;padding:18px}
.units{margin-bottom:18px;flex-wrap:wrap}
button{
 border:0;border-radius:10px;background:var(--blue);
 color:#081420;font:750 14px system-ui,Arial;
 min-height:45px;padding:12px 15px;cursor:pointer}
button:disabled{opacity:.45;cursor:not-allowed}
button.secondary{background:#355773;color:white}
button.danger{background:#934455;color:white}
button.warning{background:var(--yellow)}
button.green{background:var(--green)}
button.active{background:var(--blue)!important;color:#081420!important}
.tabs{display:flex;gap:8px;overflow-x:auto;margin-bottom:18px}
.tabs button{flex:1;min-width:94px;background:#28445f;color:white}
.page{display:none}.page.active{display:block} .launch-guide{border-color:#49849d}.guide-top{display:flex;align-items:flex-start;justify-content:space-between;gap:12px}.guide-top p{margin:0 0 12px}.guide-top strong{color:var(--green);text-align:right;letter-spacing:.05em}.guide-steps{display:grid;grid-template-columns:repeat(5,minmax(0,1fr));gap:7px;margin:10px 0 16px}.guide-steps span{text-align:center;font-size:11px;line-height:1.3;padding:10px 3px;border-radius:8px;background:#223b52;color:#b9ccd9}.guide-steps span.done{background:#224d3f;color:#b5f2d1}.guide-steps span.current{background:#34546f;color:#fff;outline:1px solid #7bdbff}.guide-message{font-size:16px;font-weight:600;padding:12px 0}.guide-actions{display:grid;grid-template-columns:minmax(0,2fr) minmax(0,1fr);gap:10px}.checklist{margin-top:15px;border-top:1px solid var(--line);padding-top:12px}.checklist summary{cursor:pointer;font-weight:650}.checklist li{margin:9px 0}.file{gap:10px}.file>span:last-child{display:flex;gap:6px;flex-wrap:wrap;justify-content:flex-end}.file button{min-height:40px}button:focus-visible,select:focus-visible,input:focus-visible{outline:3px solid var(--green);outline-offset:2px}@media(max-width:570px){.guide-steps{gap:3px}.guide-steps span{font-size:10px;padding:8px 1px}.guide-actions{grid-template-columns:1fr}.guide-top{align-items:center}.file{display:block}.file>span:last-child{justify-content:flex-start;margin-top:9px}}canvas{max-width:100%}@media(min-width:850px){.tabs{overflow-x:visible}.tabs button{min-width:0}.card{padding:24px}}@media(pointer:coarse){button,select,input{min-height:46px}}
.metrics{
 display:grid;grid-template-columns:repeat(2,minmax(0,1fr));
 gap:12px;margin-bottom:17px}
.metric .label{color:var(--muted);font-size:12px}
.value{font-size:clamp(22px,5vw,35px);font-weight:850;
 margin-top:8px;overflow-wrap:anywhere}
.value.big{font-size:clamp(32px,7vw,57px);color:var(--blue)}
#batteryPercent{color:var(--green)}
.note{font-size:12px;color:var(--muted)}
.card{margin-bottom:17px}
.actions{display:grid;grid-template-columns:1fr 1fr;gap:11px}
.actions button{width:100%}.wide{grid-column:1/-1}
.notice{
 background:#2a4660;padding:14px;border-radius:11px;
 margin:14px 0;min-height:50px;overflow-wrap:anywhere}
.checks{display:flex;flex-wrap:wrap;gap:9px;margin:15px 0}
.check{background:#2c4761;padding:8px 11px;border-radius:9px;font-size:12px}
.ok{color:var(--green)}.bad{color:var(--red)}
.progress{height:10px;border-radius:10px;overflow:hidden;
 background:#3a5871;margin:12px 0}
.progress div{height:100%;width:0%;background:var(--blue)}
canvas{width:100%;height:190px;display:block}
select,input[type=range],input[type=text],input[type=file]{display:block;width:100%;
 padding:12px;background:#304b65;color:white;
 border:1px solid var(--line);border-radius:9px;margin:12px 0}
.swatches{display:flex;gap:10px;flex-wrap:wrap;margin:15px 0}
.swatch{height:43px;width:43px;min-height:43px;border-radius:50%;
 border:2px solid white;padding:0}
.file{display:flex;justify-content:space-between;align-items:center;
 flex-wrap:wrap;gap:10px;padding:13px 0;border-bottom:1px solid var(--line)}
.file button{padding:9px}
.guide{display:grid;grid-template-columns:18px 1fr;
 gap:14px;align-items:center}
.dot{width:16px;height:16px;border-radius:50%}
@media(max-width:470px){
 main{padding:14px 11px 45px}
 .card,.metric{padding:15px}
 h1{font-size:23px}
}
.control-buttons{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:12px;margin:16px 0}
.control-buttons button{width:100%;min-width:0;white-space:normal}
.card{min-width:0;overflow-wrap:anywhere}
.card > button{margin-block:8px}
.actions{gap:12px}
input,select,button{max-width:100%;box-sizing:border-box}
input[readonly]{background:#253e54;opacity:.85}
.file div{display:flex;flex-wrap:wrap;gap:8px}
.file{align-items:flex-start;gap:12px}
.file>span:first-child{font-weight:700;overflow-wrap:anywhere}
.file button{flex:1 1 auto;min-height:42px;padding:10px 13px}
#flightSummaryDetails .row{gap:20px}
#sdtest{width:100%;margin-top:18px}
#sdTestFeedback{margin-top:12px}
#fileList{display:grid;gap:8px;margin-top:14px}
#fileList .file{padding:14px 0}
#flightSummaryCard{scroll-margin-top:16px}
.tabs{scrollbar-width:thin}
.page .card{margin-bottom:20px}
.row span{min-width:0}
@media(max-width:600px){
 main{padding:14px 12px 56px}
 .metrics{gap:10px}
 .metric{min-width:0}
 .value{font-size:clamp(21px,5vw,32px)}
 .card{padding:17px 15px}
 .actions{grid-template-columns:1fr 1fr;gap:12px}
 .file{display:block}
 .file>span:first-child{display:block;margin-bottom:10px}
 .file div{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:9px}
 .file button{width:100%;min-width:0}
 .tabs{gap:6px}
 .tabs button{min-width:92px}
 .row{padding:13px 0}
 .row strong{max-width:60%}
}
@media(max-width:360px){.metrics{grid-template-columns:1fr}.file div{grid-template-columns:1fr 1fr}}
@media(max-width:520px){.control-buttons{grid-template-columns:1fr}.actions{gap:12px}.tabs button{padding-inline:10px}}
body.sunlight{--bg:#f3f7fb;--card:#fff;--line:#aac4d4;--txt:#15283c;--muted:#536b7b;--blue:#006c98;--green:#006f47;--red:#aa1e37;--yellow:#805700}
body.sunlight .notice{background:#e3edf5;color:#172b40}
body.sunlight .check{background:#e8f0f6}
body.sunlight select,body.sunlight input[type=text],body.sunlight input[type=file]{background:#fff;color:#172b40;border:1px solid #779eb6}
body.sunlight input[readonly]{background:#eaf0f5;color:#172b40}
body.sunlight .guide-steps span{background:#dfeaf3;color:#223a4a}
body.sunlight .guide-steps span.done{background:#dbf1e3;color:#115d39}
body.sunlight .guide-steps span.current{background:#badced;color:#10344c;outline:2px solid #006c98}
body.sunlight .progress{background:#d2dfeb}
body.sunlight .value.big{color:#006c98}
body.sunlight .tabs button.active{background:#006c98!important;color:#fff!important}
@media(min-width:900px){#status.page.active,#flight.page.active,#testing.page.active,#utilities.page.active,#system.page.active,#storage.page.active{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:16px;align-items:start}#status .metrics,#storage #flightSummaryCard{grid-column:1/-1}#storage .card,#testing .card,#utilities .card,#system .card{margin-bottom:0}.metrics{grid-template-columns:repeat(3,minmax(0,1fr))}}
</style>
</head>
<body>
<main>
<header>
 <div><h1>GREEN ION</h1>
 <small>Flight logger · browser dashboard</small></div>
 <div class="status" id="connection">CONNECTING</div>
</header>

<div class="units">
 <div><strong>Measurement system</strong>
 <div class="note">Live units and graph units</div></div>
 <div>
 <button id="metric" class="secondary active">Metric</button>
 <button id="imperial" class="secondary">Imperial</button>
 <button id="sunlightMode" class="secondary" aria-pressed="false">Sunlight mode</button>
 </div>
</div>

<nav class="tabs" aria-label="Dashboard sections">
 <button class="active" data-tab="status">Status</button>
 <button data-tab="flight">Flight</button>
 <button data-tab="field">Field</button>
 <button data-tab="testing">Testing</button>
 <button data-tab="utilities">Utilities</button>
 <button data-tab="storage">Storage</button>
 <button data-tab="system">System</button>
</nav>

<section id="status" class="page active">
<div class="metrics">
 <div class="metric">
  <div class="label">LIVE ALTITUDE</div>
  <div class="value big" id="altitude">--</div>
  <div class="note" id="altitudeNote">Awaiting calibration</div>
 </div>
 <div class="metric">
  <div class="label">BATTERY ESTIMATE</div>
  <div class="value big" id="batteryPercent">--</div>
  <div class="note" id="batteryVoltage">--</div>
  <div class="note">Voltage-based estimate</div>
 </div>
 <div class="metric">
  <div class="label">Vertical speed</div>
  <div class="value" id="speed">--</div>
 </div>
 <div class="metric">
  <div class="label">Peak altitude</div>
  <div class="value" id="peak">--</div>
 </div>
 <div class="metric">
  <div class="label">Temperature</div>
  <div class="value" id="temperature">--</div>
 </div>
 <div class="metric">
  <div class="label">Atmospheric pressure</div>
  <div class="value" id="pressure">--</div>
  <div class="note" id="psi">-- PSI absolute</div>
 </div>
</div>

<div class="card" id="controlAccessCard">
 <h2>Control access</h2>
 <div class="row"><span>This phone</span>
  <strong id="controlState">VIEW ONLY</strong></div>
 <div class="row"><span>Lease</span>
  <strong id="controlLease">--</strong></div>
 <div class="control-buttons">
 <button id="takeControl" class="secondary">
  TAKE CONTROL
 </button>
 <button id="releaseControl" class="secondary">
  RELEASE CONTROL
 </button>
 </div>
 <p class="muted">Phones, tablets and computers may all view the dashboard. One connected browser at a time can change recorder settings. The control lease renews while that browser is connected.</p>
</div>

<div class="card" id="prepareRecordCard">
 <h2>Prepare and record</h2><p class="muted">V14 calibrates on boot, then watches automatically for launch. Use Prepare only to repeat stationary calibration.</p>
 <input id="missionKind" type="hidden" value="flight">
 <label for="flightLabel">Automatic label</label>
 <input id="flightLabel" type="text" readonly value="Flight 1">
 <details><summary>Optional flight settings</summary>
 <label for="motorLabel">Motor designation</label>
 <input id="motorLabel" type="text" maxlength="24" list="motorList" placeholder="Search motor (or type your own)" autocomplete="off">
 <datalist id="motorList">
 <option value="A8-3"><option value="A8-5"><option value="B4-2"><option value="B4-4"><option value="B6-2"><option value="B6-4"><option value="B6-6"><option value="C6-3"><option value="C6-5"><option value="C6-7"><option value="D12-3"><option value="D12-5"><option value="D12-7"><option value="E12-4"><option value="E12-6"><option value="E9-4"><option value="F15-6">
 </datalist>
 <button id="saveMission" class="secondary" style="display:none">SAVE FLIGHT DETAILS</button>
 <label for="sampleRate">SD samples per second</label>
 <select id="sampleRate">
  <option value="10">10 Hz · longest recordings</option>
  <option value="25" selected>25 Hz · recommended</option>
  <option value="50">50 Hz · fastest detail</option>
 </select>
 <label for="recordingLimit">Automatic recording limit</label>
 <select id="recordingLimit">
  <option value="2">2 minutes</option><option value="5">5 minutes</option>
  <option value="10" selected>10 minutes · recommended</option><option value="20">20 minutes</option>
 </select>
 <button id="saveRate" class="secondary" style="display:none">APPLY RATE &amp; RECORDING LIMIT</button>
 <div class="notice" id="settingsSaveState" aria-live="polite">Flight settings will appear here.</div>
 <p class="muted">Change the rate before arming. Higher rates
 capture fast motion with larger files and more SD writes.</p>
 </details>
 <div class="row"><span>Status</span>
  <strong id="flightState">CONNECTING</strong></div>
 <div class="checks" id="checks"></div>
 <div class="notice" id="feedback" role="status">
  Waiting for connection...
 </div>
 <div id="calArea" style="display:none">
  <div class="progress"><div id="calBar"></div></div>
  <small id="calText">Calibrating...</small>
 </div>
 <div class="actions">
  <button id="prepareAll" class="wide">1 · PREPARE FOR LAUNCH</button>
  <button id="calibrate" style="display:none">CALIBRATE</button>
  <button id="armWatch" class="wide green" style="display:none">AUTOMATIC LAUNCH WATCH</button>
  <button id="arm" style="display:none">ARM LOGGER</button>
  <button id="start" style="display:none">START LAUNCH WATCH</button>
  <button id="cancelWatch" class="secondary" style="display:none">CANCEL WATCH</button>
  <button id="stop" class="danger">STOP &amp; SAVE</button>
  <button id="disconnect" class="secondary">DISCONNECT WI-FI</button>
 </div>
 <p class="muted">Calibration enables live altitude without
 requiring an SD card or starting a flight.</p>
</div>

<div class="card" id="recorderHealthCard">
 <h2>Recorder health</h2>
 <div class="row"><span>Preflight readiness</span><strong id="preflightScore">--</strong></div>
 <div class="notice" id="preflightDetail">Waiting for checks.</div>
 <div class="row"><span>SD card</span><strong id="overviewSD">--</strong></div>
 <div class="row"><span>Recorded samples</span><strong id="overviewSamples">--</strong></div>
 <div class="row"><span>Current file</span><strong id="overviewFile">--</strong></div>
 <div class="row"><span>Recovery</span><strong id="recovery">--</strong></div>
 <div class="row"><span>Missed sample periods</span><strong id="missed">--</strong></div>
 <div class="row"><span>Metadata file</span><strong id="metadata">--</strong></div>
 <button id="openFlightStats" class="secondary" style="width:100%">OPEN SAVED FLIGHTS AND REPORTS</button>
 <button id="downloadPreflight" class="secondary" style="width:100%">DOWNLOAD PREFLIGHT REPORT</button>
 </div>

<div class="card">
 <h2>Live altitude history</h2>
 <canvas id="overviewGraph"></canvas>
</div>
</section>

<section id="flight" class="page">
<div class="card" id="flightProfileCard">
 <h2>Rocket flight profile</h2>
 <p class="muted">Choose the rocket and motor on the bench BEFORE launching. Saves the selection on the Feather and in the flight .TXT metadata. Changing the profile restarts stationary calibration; never edit at the pad.</p>
 <label for="rocketPreset">Rocket</label>
 <select id="rocketPreset">
  <option value="Green Eggs">Estes Green Eggs</option>
  <option value="So Long">Estes Pro Series II So Long</option>
  <option value="Custom">Another rocket</option>
 </select>
 <label for="rocketCustom">Custom rocket name (when selected)</label>
 <input id="rocketCustom" type="text" maxlength="24" placeholder="Your rocket" disabled>
 <label for="profileMotor">Motor designation (optional)</label>
 <input id="profileMotor" type="text" maxlength="24" placeholder="Example: D12-5, F15-8">
 <button id="saveFlightProfile" class="green">SAVE ROCKET PROFILE &amp; RECALIBRATE</button>
 <div class="notice" id="flightProfileFeedback">Waiting for Feather profile...</div>
</div>

 <div class="card"><h2>Automatic flight workflow</h2><p>V14 calibrates and watches automatically. It records RKT4 flight data and a synchronized IMU/GPS sensor file, then writes recovery evidence.</p><div class="row"><span>Active phase</span><strong id="flightPhase">IDLE</strong></div><div class="row"><span>Automatic label</span><strong id="missionIdentity">--</strong></div><div class="row"><span>Prebuffer</span><strong id="prebufferStatus">--</strong></div><div class="progress"><div id="prepareProgress"></div></div><div class="notice" id="prepareStatus">Run Prepare for Launch.</div></div>
</section>

<section id="field" class="page">
 <div class="card"><h2>FIELD MODE</h2><p class="muted">Large launch-day controls.</p>
  <div class="value big" id="fieldBattery">--</div><div class="notice" id="fieldReadiness">Connecting...</div>
  <button id="fieldPrepare" class="wide">1 · PREPARE FOR LAUNCH</button>
  <button id="fieldArm" class="wide green" style="display:none">AUTOMATIC WATCH</button>
  <button id="fieldStop" class="wide danger">STOP &amp; SAVE</button>
  <label><input id="audibleCues" type="checkbox"> Phone speaks preparation results</label>
 </div>
</section>

<section id="testing" class="page">
 <div class="card" id="homeVerificationCard">
  <h2>Full home verification — guided and temporary</h2>
  <p class="muted">A step-by-step check of power, pressure, IMU, GPS, SD card, LoRa, battery and a real 30-second sensor recording. No flight is simulated. Some physical checks need your confirmation. Do not use with motors connected or on a launch pad.</p>
  <div class="row"><span>Progress</span><strong id="homeStepCount">Not started</strong></div>
  <div class="progress"><div id="homeProgressFill"></div></div>
  <h3 id="homeStepTitle">Ready to start</h3>
  <div class="notice" id="homeInstructions" role="status" aria-live="polite">Secure the Feather and remove all rocket motors. Attach radio antennas before transmitting. Tap Start full home test.</div>
  <label id="homeConfirmLabel" style="display:none"><input id="homeConfirm" type="checkbox"><span id="homeConfirmText">I completed the physical check.</span></label>
  <div class="row"><span>Result</span><strong id="homeStepStatus">NOT RUN</strong></div>
  <div class="actions">
   <button id="homeStart" class="green">START FULL HOME TEST</button>
   <button id="homeRun" disabled>RUN / CONFIRM THIS STEP</button>
   <button id="homeNext" disabled>NEXT STEP</button>
   <button id="homeSkip" class="secondary" disabled>MARK NOT VERIFIED</button>
   <button id="homeFinish" class="danger" disabled>END &amp; DELETE TEMP DATA</button>
  </div>
  <div class="notice" id="homeSummary">PASS describes only the check performed, not the entire rocket's flight readiness.</div>
  <button id="homeReport" class="secondary" disabled>DOWNLOAD JSON TEST REPORT</button>
 </div>

 <div class="card" id="hardwareTestsCard">
  <h2>V14 Hardware Tests</h2>
  <p class="muted">Live diagnostics run independently from the recorder. Tests are deferred whenever flight recording is active.</p>
  <div class="actions">
   <button id="testImu" class="green">Test IMU</button>
   <button id="testGps" class="green">Test GPS</button>
   <button id="testAllSensors">Test All Sensors</button>
  </div>
  <div class="row"><span>IMU result</span><strong id="imuTestResult">NOT RUN</strong></div>
  <div class="notice" id="imuTestMessage">Connect the LSM6DSO32 over STEMMA QT and keep it stationary for the bias test.</div>
  <div class="row"><span>Acceleration X / Y / Z</span><strong id="imuAxes">--</strong></div>
  <div class="row"><span>Total acceleration</span><strong id="imuTotal">--</strong></div>
  <div class="row"><span>Gyro X / Y / Z</span><strong id="gyroAxes">--</strong></div>
  <div class="row"><span>Stationary gyro bias</span><strong id="gyroBias">--</strong></div>
  <div class="row"><span>IMU temperature / range</span><strong id="imuTempRange">--</strong></div>
  <hr>
  <div class="row"><span>GPS result</span><strong id="gpsTestResult">NOT RUN</strong></div>
  <div class="notice" id="gpsTestMessage">Communication and satellite fix are reported separately. Test outdoors with a clear sky for a fix.</div>
  <div class="row"><span>Communication / NMEA</span><strong id="gpsCommunication">--</strong></div>
  <div class="row"><span>Fix / satellites / age</span><strong id="gpsFix">--</strong></div>
  <div class="row"><span>Coordinates</span><strong id="gpsCoordinates">--</strong></div>
  <div class="row"><span>GPS altitude / speed / UTC</span><strong id="gpsMotion">--</strong></div>
 </div>
 <div class="card">
  <h2>Integrated preflight bench check</h2>
  <p class="muted">Checks live IMU, GPS communication/fix and a temporary SD write/read/delete. Some checks may be inconclusive without an outdoor GPS fix. This is not a substitute for physical power, radio and vibration testing.</p>
  <button id="groundtest" class="wide green">RUN INTEGRATED BENCH CHECK</button>
  <div class="row"><span>Result</span><strong id="groundTestResult">NOT RUN</strong></div>
  <div class="notice" id="groundTestDetail">Calibrate and test the SD card first.</div>
 </div>
 <div class="card">
  <h2>Sensor testing</h2>
  <button id="testCalibrate">CALIBRATE ALTITUDE</button>
  <div class="notice" id="testFeedback">Ready for testing.</div>
  <div class="actions" style="display:none" aria-hidden="true">
   <button id="simulate" disabled>Retired demo control</button>
   <button id="simstop" disabled>Retired demo control</button>
  </div>
 </div>
 <div class="card" id="graphOptionsCard">
  <h2>Graph options</h2>
  <div class="actions">
   <button id="pause" class="secondary">Pause graphs</button>
   <button id="clear" class="secondary">Clear history</button>
  </div>
  <select id="graphWindow">
   <option value="30">Last 30 readings</option>
   <option value="60">Last 60 readings</option>
   <option value="120" selected>Last 120 readings</option>
  </select>
 </div>
 <div class="card" id="altGraphCard"><h2>Altitude</h2>
  <canvas id="altGraph"></canvas></div>
 <div class="card" id="pressureGraphCard"><h2>Pressure</h2>
  <canvas id="pressureGraph"></canvas></div>
 <div class="card" id="tempGraphCard"><h2>Temperature</h2>
  <canvas id="tempGraph"></canvas></div>
 <div class="card"><h2>Sensor diagnostics</h2>
  <div class="row"><span>Reading freshness</span>
   <strong id="freshness">--</strong></div>
  <div class="row"><span>Good / failed reads</span>
   <strong id="reads">--</strong></div>
  <div class="row"><span>Frozen-reading detector</span><strong id="sensorFrozen">--</strong></div>
  <div class="row"><span>Calibration count</span>
   <strong id="calCount">--</strong></div>
  <div class="row"><span>Calibration age</span>
   <strong id="calAge">--</strong></div>
  <div class="row"><span>Baseline pressure</span>
   <strong id="baseline">--</strong></div>
  <div class="row"><span>Calibration result</span>
   <strong id="calResult">--</strong></div>
  <div class="row"><span>Calibration noise</span>
   <strong id="calNoise">--</strong></div>
  <div class="row"><span>Pressure drift</span>
   <strong id="calDrift">--</strong></div>
 </div>
</section>

<section id="utilities" class="page">
 <div class="card">
  <h2>RGB LED controls</h2>
  <div class="row"><span>Mode</span>
   <strong id="ledMode">AUTO</strong></div>
  <div class="swatches" id="swatches"></div>
  <label for="effect">Effect</label>
  <select id="effect">
   <option value="0">Solid</option>
   <option value="1">Blink</option>
   <option value="2">Breathe</option>
   <option value="3">Rainbow</option>
   <option value="4">Off</option>
  </select>
  <label for="brightness">Brightness
   <strong id="brightnessLabel">45%</strong></label>
  <input id="brightness" type="range" min="1" max="100" value="45">
  <div class="actions">
   <button id="ledoff" class="danger">TURN RGB OFF</button>
   <button id="ledauto" class="secondary">AUTO STATUS</button>
  </div>
  <div class="notice" id="ledFeedback">Automatic status enabled.</div>
 </div>
 <div class="card">
  <h2>Automatic LED guide</h2>
  <div class="guide">
   <div class="dot" style="background:#357bff"></div>
   <div>Blue blink — calibration needed</div>
   <div class="dot" style="background:#22c55e"></div>
   <div>Green solid — calibrated</div>
   <div class="dot" style="background:#eab308"></div>
   <div>Yellow solid — armed</div>
   <div class="dot" style="background:#22c55e"></div>
   <div>Green blink — recording</div>
   <div class="dot" style="background:#ef4444"></div>
   <div>Red blink — sensor / SD fault</div>
   <div class="dot" style="background:#f59e0b"></div>
   <div>Red/orange — critical battery</div>
  </div>
 </div>
</section>

<section id="storage" class="page">
 <div class="card">
  <h2>microSD diagnostics</h2>
  <div class="notice" id="sdStatus">Not tested.</div>
  <div class="row"><span>Card mounted</span><strong id="sdMounted">--</strong></div>
  <div class="row"><span>Read/write test</span><strong id="sdReady">--</strong></div>
  <div class="row"><span>Free space</span><strong id="sdFreeSpace">--</strong></div>
  <button id="sdtest" class="wide">DETECT &amp; TEST SD CARD</button>
  <div class="notice" id="sdTestFeedback" role="status" aria-live="polite">Tap to test. This button can request control automatically.</div>
  <p class="muted">This test creates, reads, and removes a temporary file. It does not erase saved flights. Wait until recording and calibration have stopped.</p>
 </div>
 <div class="card">
  <h2>Flight files</h2>
  <div class="row"><span>Space used</span>
   <strong id="storageUsed">--</strong></div>
  <div class="row"><span>Saved recordings</span><strong id="fileCount">--</strong></div>
  <div class="row"><span>SD test result</span><strong id="sdTestResult">--</strong></div>
  <div class="row"><span>Write errors</span><strong id="storageWriteErrors">--</strong></div>
  <div class="row"><span>Recording integrity</span><strong id="integrityStatus">--</strong></div>
  <div class="row"><span>Recovered interrupted flights</span><strong id="recoveredFlights">--</strong></div>
  <div class="row"><span>Last recovered sample count</span><strong id="recoveredSamples">--</strong></div>
  <div class="row"><span>ESP32 reset reason code</span><strong id="resetReason">--</strong></div>
  <div class="row"><span>Missed samples</span><strong id="storageMissed">--</strong></div>
  <div class="row"><span>Actual sample rate</span><strong id="storageRate">--</strong></div>
  <div class="row"><span>Maximum SD flush</span><strong id="maximumFlush">--</strong></div>
  <div class="row"><span>Recording heartbeat</span><strong id="flushHeartbeat">--</strong></div>
  <button id="refresh" class="secondary">REFRESH FILES</button>
  <a href="/api/manifest" download><button class="secondary">DOWNLOAD FLIGHT MANIFEST</button></a>
  <button id="downloadEverything" class="secondary">DOWNLOAD SELECTED FLIGHT PACKAGE</button>
  <button id="scanIntegrity" class="secondary">SCAN SELECTED FILE</button>
  <div class="notice" id="integrityScanResult">Select a flight to scan.</div>
  <div class="notice" id="sdDeleteFeedback">Deleting here removes the selected flight package from the Feather SD card only. Browser, Windows, and cloud backups are not deleted.</div>
  <label for="fileSort">Sort recordings</label>
  <select id="fileSort"><option value="new">Newest first</option><option value="old">Oldest first</option><option value="size">Largest first</option></select>
  <div id="fileList"></div>
 </div>
 <div class="card" id="flightSummaryCard">
  <h2>Saved flight summary</h2>
  <p class="muted">Select View stats to read stored flight data directly. No download needed.</p>
  <div class="notice" id="flightSummaryStatus">Choose View stats on a saved flight.</div>
  <div id="flightSummaryDetails"></div>
  <h3>Battery / estimated acceleration</h3><div id="flightExtraMetrics" class="notice">Load a flight.</div><h3>Estimated flight phases</h3><div id="flightPhases" class="notice">Select a flight.</div><h3>Data confidence timeline</h3><div id="flightConfidenceTimeline" class="notice">Select a flight.</div><h3>Calculated flight performance</h3>
  <div id="flightPerformance" aria-live="polite">Select a recording to calculate performance.</div>
  <h3>Saved flight graph</h3>
  <label for="savedGraphMetric">Measurement</label>
  <select id="savedGraphMetric"><option value="5">Altitude (m)</option><option value="1">Pressure (Pa)</option><option value="2">Temperature (°C)</option><option value="6">Vertical speed (m/s)</option><option value="3">Battery (V)</option><option value="7">Estimated acceleration (m/s²)</option><option value="8">Estimated vertical apparent G</option></select>
  <canvas id="savedFlightGraph" style="width:100%;height:220px"></canvas>
  <div class="notice" id="savedGraphStatus">Select a flight to load its graph.</div><button id="exportFlightPDF" class="secondary" style="width:100%">PRINT / SAVE FLIGHT PDF</button>
 </div>
</section>

<section id="system" class="page">
 <div class="card" id="v12AvailabilityCard">
  <h2>V14 feature availability</h2>
  <div class="notice">The blue V11 experience is restored around the V14 recorder. Recording, live sensors, hardware tests, SD verification, downloads, browser CSV/PDF analysis, calibration, and safe stop are connected to real V12 firmware.</div>
  <p class="muted"><strong>V14 flight functions:</strong> automatic calibration, launch watching, RKT4/IMU/GPS recording, SD verification, recovery, and offline analysis. OTA and rollback work only in explicit bench maintenance mode. Editable sample-rate/mission labels, manual RGB effects, configuration restore, and deep sleep remain unavailable; their controls are disabled.</p>
 </div>
 <div class="card"><h2>Help and connection</h2>
  <details open><summary>Connect from a phone, iPad or computer</summary><p>Connect to <strong>GREEN-ION-V14</strong> Wi-Fi and open <strong>http://192.168.4.1</strong>. This local Wi-Fi does not provide internet access; remain connected to use the recorder.</p></details>
  <details><summary>Buttons disabled or view-only?</summary><p>When one device is connected, it receives control automatically. With multiple viewers, use <strong>Take Control</strong> under Utilities. Recording or arming temporarily disables some settings.</p></details>
  <details><summary>SD testing or download trouble?</summary><p>Finish recording and disarm, then run <strong>Detect &amp; Test SD Card</strong> under Storage. The check uses only a temporary file. Download saved recordings after logging ends.</p></details>
  <details><summary>Updating the firmware</summary><p>Only update on the bench with a charged LiPo, stable USB power, and no active recording. Entering maintenance disables launch detection. Select the main <strong>.ino.bin</strong> file from a successful GitHub Actions build. Recalibrate and perform all ground tests after a restart.</p></details>
  <details><summary>Reading the flight calculations</summary><p>Altitude comes from atmospheric pressure. Derived speed, acceleration, apparent G, flight phases and landing estimates have uncertainty. Onboard SD logging continues without a live Wi-Fi connection once recording has started.</p></details>
 </div>
 <div class="card">
  <h2>Install firmware from this phone</h2>
  <p>Download the main <strong>.ino.bin</strong> file from the
  latest successful GitHub Actions artifact. Reconnect to
  GREEN-ION, choose that file below, and install it.</p>
  <input id="firmwareFile" type="file" accept=".bin,application/octet-stream">
  <button id="installFirmware" class="warning" style="width:100%">
   INSTALL FIRMWARE &amp; RESTART
  </button>
  <div class="progress" aria-label="Firmware upload progress"><div id="firmwareProgress"></div></div>
  <div class="row"><span>Upload progress</span><strong id="firmwarePercent">0%</strong></div>
  <div class="notice" id="updateFeedback">
   Before updating, charge the LiPo to at least 3.60 V, connect USB power, and verify no flight recording is active. The app enters maintenance and turns launch watching off.
  </div>
  <button id="rollbackFirmware" class="secondary" style="width:100%">ROLL BACK TO PREVIOUS FIRMWARE</button>
  <button id="exitMaintenance" class="secondary" style="width:100%">EXIT MAINTENANCE / RECALIBRATE</button>
  <a href="/api/config" download><button class="secondary" style="width:100%">DOWNLOAD CONFIGURATION BACKUP</button></a>
  <input id="configFile" type="file" accept=".json,application/json">
  <button id="restoreConfig" class="secondary" style="width:100%">RESTORE CONFIGURATION</button>
  <div class="notice" id="maintenanceStatus">Maintenance status loading...</div>
  <p class="muted">Do not use a bootloader, partitions, or merged
  binary. Use the file ending in <strong>.ino.bin</strong>. Keep
  power connected until the board restarts.</p>
 </div>

 <div class="card">
  <h2>Power diagnostics</h2>
  <div class="row"><span>Battery estimate</span>
   <strong id="sysPct">--</strong></div>
  <div class="row"><span>Voltage</span>
   <strong id="sysV">--</strong></div>
  <div class="row"><span>Battery status</span>
   <strong id="sysWarning">--</strong></div>
  <div class="row"><span>USB charging</span>
   <strong>Unknown (not sensed)</strong></div>
  <p class="muted">Battery percentage is approximate.
   The physical CHG LED indicates charging.</p>
 </div>

 <div class="card">
  <h2>Power management</h2>
  <p>Deep sleep stops the ESP32, Wi-Fi, normal data
  collection and powered STEMMA QT / RGB LED rail.</p>

  <button id="shutdown" class="danger"
   style="width:100%">
   SHUT DOWN FEATHER (DEEP SLEEP)
  </button>

  <div class="notice" id="powerFeedback">
   Device running normally.
  </div>

  <p class="muted">
   <strong>Wake up:</strong> Press the physical
   RESET button on the Feather. Wi-Fi then restarts.
  </p>
  <p class="muted">
   For hardware power-off, use an EN-to-GND
   switch. Reopen the switch to power on.
   USB-C battery charging remains available.
  </p>
  <p class="muted">
   Deep sleep is not full electrical power-off.
   External accessories powered directly from BAT
   or USB may remain powered.
  </p>
 </div>

 <div class="card">
  <h2>Recorder diagnostics</h2>
  <a href="/api/diagnostics" download><button class="secondary" style="width:100%">DOWNLOAD DIAGNOSTIC REPORT</button></a>
  <div class="row"><span>Firmware / build</span><strong id="firmwareBuild">--</strong></div>
  <div class="row"><span>Recording limit</span><strong id="recordingLimitStatus">--</strong></div>
  <div class="row"><span>Samples</span>
   <strong id="samples">--</strong></div>
  <div class="row"><span>Sample rate</span>
   <strong id="sampleRateStatus">--</strong></div>
  <div class="row"><span>Invalid / missed</span>
   <strong id="sampleErrors">--</strong></div>
  <div class="row"><span>SD errors</span>
   <strong id="sdErrors">--</strong></div>
  <div class="row"><span>Duration</span>
   <strong id="duration">--</strong></div>
  <div class="row"><span>Current file</span>
   <strong id="filename">--</strong></div>
  <div class="row"><span>Flight label</span>
   <strong id="savedFlightLabel">--</strong></div>
  <div class="row"><span>Motor</span>
   <strong id="savedMotorLabel">--</strong></div>
  <div class="row"><span>Minimum pressure</span>
   <strong id="minimumPressure">--</strong></div>
  <div class="row"><span>Temperature range</span>
   <strong id="temperatureRange">--</strong></div>
  <div class="row"><span>Launch</span>
   <strong id="launched">--</strong></div>
  <div class="row"><span>Finish reason</span>
   <strong id="finish">--</strong></div>
  <div class="row"><span>Uptime</span>
   <strong id="uptime">--</strong></div>
  <div class="row"><span>Last error</span>
   <strong id="error">--</strong></div>
 </div>
</section>
</main>

<script>
"use strict";

const $ = id => document.getElementById(id);
$('flight').append($('prepareRecordCard'),$('recorderHealthCard'));
$('utilities').prepend($('controlAccessCard'));
$('status').append($('graphOptionsCard'),$('altGraphCard'),$('pressureGraphCard'),$('tempGraphCard'));
let state = null;
let sunlight=false;
try{sunlight=localStorage.getItem("greenIonSunlight")==="1";}catch(e){}
document.body.classList.toggle("sunlight",sunlight);
let imperial = false;
let busy = false;
let polling = false;
let paused = false;
let connectedOnce = false;
let previousCalibrating = false;
let previousPrepareReady = false;
let sampleRateDirty=false;
let missionDirty=false;
let sampleRateSaving=false;
let selectedFlight="";
let rgb = [0,180,255];
let clientToken="";
try { clientToken=sessionStorage.getItem("greenIonControlToken")||""; }
catch(e) {}
if(!clientToken) {
 const bytes=new Uint8Array(12);
 if(globalThis.crypto && crypto.getRandomValues) crypto.getRandomValues(bytes);
 else for(let i=0;i<bytes.length;i++) bytes[i]=Math.floor(Math.random()*256);
 clientToken=Array.from(bytes,b=>b.toString(16).padStart(2,"0")).join("");
 try { sessionStorage.setItem("greenIonControlToken",clientToken); }
 catch(e) {}
}

let history = {
 altitude: [],
 pressure: [],
 temperature: []
};

function number(v,n=2) {
 return v===null || v===undefined ||
  !Number.isFinite(Number(v))
  ? "--" : Number(v).toFixed(n);
}

function distance(v) {
 if(v==null)return null;
 return imperial?v*3.280839895:v;
}

function temp(v) {
 if(v==null)return null;
 return imperial?v*9/5+32:v;
}

function pressure(v) {
 if(v==null)return null;
 return imperial?v/3386.389:v/100;
}

function units() {
 return imperial?"ft":"m";
}

function notice(s,ok=null) {
 const color=ok===true?"#72e5ab":
  ok===false?"#ff91a0":"#ffdd88";

 for(const id of ["feedback","testFeedback"]) {
  $(id).textContent=s;
  $(id).style.color=color;
 }
}

function speakCue(text){if(!$("audibleCues").checked||!globalThis.speechSynthesis)return;try{speechSynthesis.cancel();speechSynthesis.speak(new SpeechSynthesisUtterance(text));}catch(e){}}

function check(name,pass) {
 const e=document.createElement("span");
 e.className="check "+(pass?"ok":"bad");
 e.textContent=(pass?"✓ ":"× ")+name;
 return e;
}

function setSunlight(x){sunlight=!!x;document.body.classList.toggle("sunlight",sunlight);$("sunlightMode").textContent=sunlight?"Dark mode":"Sunlight mode";$("sunlightMode").setAttribute("aria-pressed",sunlight?"true":"false");try{localStorage.setItem("greenIonSunlight",sunlight?"1":"0");}catch(e){}drawAll();drawSavedGraph();}
function setUnits(x) {
 imperial=x;
 $("metric").classList.toggle("active",!x);
 $("imperial").classList.toggle("active",x);
 if(state)render(state,false);
 drawAll();
}

function openTab(name,button) {
 document.querySelectorAll(".page").forEach(
  e=>e.classList.remove("active")
 );
 document.querySelectorAll(".tabs button").forEach(
  e=>e.classList.remove("active")
 );
 $(name).classList.add("active");
 button.classList.add("active");
 if(name==="storage")loadFiles();
 drawAll();
}

function render(d,collect=true) {
 state=d;
 $("integrityStatus").textContent=d.integrityStatus||"Unknown";
 $("recoveredFlights").textContent=d.recoveredFlights??0;
 $("recoveredSamples").textContent=d.recoveredSamples??0;
 $("resetReason").textContent=d.resetReason||"Unknown";
 $("connection").textContent="● LIVE";
 $("connection").style.color="#72e5ab";

 if(!connectedOnce) {
  connectedOnce=true;
  notice("ESP32 connected. "+
   (d.calibrated
    ?"Altitude calibrated."
    :"Press CALIBRATE to zero altitude."),true);
 }

 const a=d.simulation?d.simAltitude:d.altitude;
 const v=d.simulation?d.simSpeed:d.speed;

 $("altitude").textContent=
  (d.calibrated||d.simulation)
   ?number(distance(a))+" "+units():"--";

 $("altitudeNote").textContent=
  d.calibrating?"Calibrating...":
  d.simulation?"Simulated altitude":
  d.calibrated?"Live relative altitude":
  "Awaiting calibration";

 $("batteryPercent").textContent=
  d.batteryPercent<0?"--":d.batteryPercent+"%";

 $("batteryVoltage").textContent=
  number(d.battery,3)+" V"+
  (d.batteryCritical?" · CRITICAL":
   d.batteryLow?" · LOW":"");

 $("speed").textContent=
  number(distance(v))+
  (imperial?" ft/s":" m/s");

 $("peak").textContent=number(distance(d.peak))+" "+units();

 $("temperature").textContent=
  number(temp(d.temperature),1)+
  (imperial?" °F":" °C");

 $("pressure").textContent=
  number(pressure(d.pressure),imperial?3:1)+
  (imperial?" inHg":" hPa");

 $("psi").textContent=
  number(d.pressure==null?null:
   d.pressure/6894.757293,3)+" PSI absolute";

 $("flightState").textContent=
  d.shutdownPending?"SHUTTING DOWN":
  d.calibrating?"CALIBRATING":
  d.recording?"IN FLIGHT · "+String(d.activePhase||"").toUpperCase():
  d.launchWatch?"WATCHING FOR LAUNCH":d.armed?"ARMED":d.simulation?"SIMULATING":
  d.calibrated?"PREFLIGHT LIVE":"NOT CALIBRATED";

 $("controlState").textContent=
  d.controller?"ACTIVE CONTROLLER":"VIEW ONLY";
 $("controlState").style.color=
  d.controller?"#72e5ab":"#ffdd88";
 $("controlLease").textContent=
  d.controller?(d.autoControl?"Automatic · only connected device":d.controlLeaseSeconds+" s (auto-renewing)"):
  d.controlAvailable?"Available":"Used by another browser";
 $("takeControl").disabled=
  busy||d.controller||!d.controlAvailable;
 $("releaseControl").disabled=busy||!d.controller;

 $("checks").replaceChildren(
  check("BMP388",d.sensor&&d.reading),
  check("Calibrated",d.calibrated),
  check("SD ready",d.sdVerified),
  check("Battery",!d.batteryCritical)
 );

 const deviceLock=
  busy||d.calibrating||d.prepareActive||d.groundTestActive||d.shutdownPending||d.updateInProgress;
 const lock=deviceLock||!d.controller;

 $("calibrate").disabled=
  lock||d.armed||d.recording||d.simulation;
 $("testCalibrate").disabled=$("calibrate").disabled;

 $("arm").disabled=
  lock||d.armed||d.recording||d.simulation||
  !d.sensor||!d.reading||!d.calibrated||
  !d.sdVerified||d.sdFault||d.batteryCritical;

 $("start").disabled=lock||!d.armed||d.recording||d.launchWatch;
 $("armWatch").disabled=lock||d.armed||d.recording||d.launchWatch||
  !d.prepareReady||!d.calibrationFresh||!d.sdVerified||d.sdFault||d.batteryCritical;
 $("cancelWatch").disabled=lock||(!d.armed&&!d.launchWatch)||d.recording;
 $("prepareAll").disabled=lock||d.armed||d.recording||d.launchWatch||d.simulation;
 $("stop").disabled=lock||(!d.recording&&!d.launchWatch);
 $("disconnect").disabled=lock||!d.recording;
 $("simulate").disabled=
  lock||d.recording||d.armed||d.simulation;
 $("simstop").disabled=
  lock||!d.simulation;
 // Keep the SD test clickable in view-only mode: it requests control
 // and reports errors instead of silently appearing unresponsive.
 $("sdtest").disabled=deviceLock||d.recording||d.armed;
 $("groundtest").disabled=lock||d.recording||d.armed||d.calibrating||d.groundTestActive||!d.calibrated||!d.sdVerified;
 $("shutdown").disabled=lock;
 $("installFirmware").disabled=
  lock||d.armed||d.recording||d.simulation||d.batteryCritical;
 $("saveMission").disabled=lock||d.armed||d.recording;
 $("saveRate").disabled=lock||d.armed||d.recording;
 $("missionKind").disabled=lock||d.armed||d.recording;
 $("motorLabel").disabled=lock||d.armed||d.recording;
 $("sampleRate").disabled=lock||d.armed||d.recording;
 $("recordingLimit").disabled=lock||d.armed||d.recording;
 $("effect").disabled=lock;
 $("brightness").disabled=lock;
 $("ledoff").disabled=lock;
 $("ledauto").disabled=lock;
 document.querySelectorAll(".swatch").forEach(
  button=>button.disabled=lock
 );

 $("calArea").style.display=
  d.calibrating?"block":"none";
 $("calBar").style.width=
  Math.min(100,d.calProgress||0)+"%";
 $("calText").textContent=
  (d.calPhase===2?"Verifying zero":"Collecting pressure")+
  " · Valid "+d.calValid+"/"+d.calTarget;

 if(d.calibrating) {
  notice(d.calStatus+" · "+(d.calProgress||0)+"%");
 }
 if(previousCalibrating&&!d.calibrating) {
  notice(d.calStatus,d.calibrated);
 }
 previousCalibrating=d.calibrating;

 $("freshness").textContent=
  d.freshness==null?"No readings":d.freshness+" ms";
 $("reads").textContent=d.good+" / "+d.bad;
 $("sensorFrozen").textContent=d.sensorFrozen?"FROZEN · CHECK SENSOR":"CHANGING NORMALLY";
 $("calCount").textContent=d.calCount;
 $("calAge").textContent=
  d.calAge==null?"Never":d.calAge+" s ago";
 $("baseline").textContent=number(d.baseline,2)+" Pa";
 $("calResult").textContent=d.calStatus;
 $("calNoise").textContent=d.calNoiseM==null?"--":"±"+number(d.calNoiseM,3)+" m";
 $("calDrift").textContent=d.calDriftPaPerSec==null?"--":number(d.calDriftPaPerSec,3)+" Pa/s";
 $("flightPhase").textContent=String(d.activePhase||"idle").toUpperCase()+
  (d.phaseConfidence?" · "+d.phaseConfidence+"% estimate":"");
 $("missionIdentity").textContent="Mission "+d.missionNumber+" · Card Flight "+d.cardSequence;
 $("prebufferStatus").textContent=d.prebufferSamples+" / "+d.prebufferCapacity+" samples (3 seconds)";
 $("prepareProgress").style.width=(d.prepareProgress||0)+"%";
 $("prepareStatus").textContent=d.prepareStatus||"Not prepared";
 $("fieldBattery").textContent=(d.batteryPercent<0?"Battery --":d.batteryPercent+"%")+" · "+number(d.battery,3)+" V";
 $("fieldReadiness").textContent=(d.prepareReady?"READY":"REVIEW")+" · "+(d.prepareStatus||"")+" · "+d.environmentalWarning;
 $("fieldPrepare").disabled=$("prepareAll").disabled;
 $("fieldArm").disabled=$("armWatch").disabled;
 $("fieldStop").disabled=$("stop").disabled;
 $("maintenanceStatus").textContent=d.maintenanceReminder+" · "+d.estimatedFlightsRemaining+" estimated flights remain · SD safe rate "+d.sdSafeRate+" Hz";
 if(d.prepareReady&&!previousPrepareReady)speakCue("Green Ion preparation passed. Recorder ready.");
 if(!d.prepareActive&&previousPrepareReady&&!d.prepareReady)speakCue("Green Ion requires review.");
 previousPrepareReady=!!d.prepareReady;
 $("downloadPreflight").disabled=!d.preflightReport||d.recording;

 $("sdStatus").textContent=d.sdStatus;
 const checks=[["Sensor",d.sensor&&d.reading],["Calibration",d.calibrated&&!d.calibrating&&d.calibrationFresh],["SD",d.sd&&d.sdVerified&&!d.sdFault],["Battery",!d.batteryCritical&&(!d.batteryPresent||!d.batteryLow)],["Idle",!d.recording&&!d.armed]];
 const passed=checks.filter(x=>x[1]).length;
 $("preflightScore").textContent=passed+"/"+checks.length+" checks";
 $("preflightDetail").textContent=checks.map(x=>(x[1]?"PASS ":"CHECK ")+x[0]).join(" · ")+(d.batteryPresent?"":" · Battery not detected");
 $("storageWriteErrors").textContent=d.sdErrors;
 $("storageMissed").textContent=d.missed;
 $("storageRate").textContent=d.effectiveSampleRate+" Hz · "+d.adaptiveRateStatus;
 $("maximumFlush").textContent=(d.maximumFlushUs||0)+" µs";
 $("flushHeartbeat").textContent=d.recording?"● "+d.flushHeartbeat:d.flushHeartbeat+" completed flushes";
 $("groundTestResult").textContent=d.groundTestActive?"RUNNING":d.groundTestResult;
 $("groundTestDetail").textContent=d.groundTestDetail;
 $("sdMounted").textContent=d.sd?"YES":"NO";
 $("overviewSD").textContent=d.sdVerified?"READY":d.sd?"NEEDS TEST":"NOT DETECTED";
 $("overviewSamples").textContent=d.samples;
 $("overviewFile").textContent=d.file;
 $("sdReady").textContent=d.sdVerified?"PASS":"NOT VERIFIED";
 $("ledMode").textContent=d.ledManual?"MANUAL":"AUTO";

 $("sysPct").textContent=
  d.batteryPercent<0?"Unknown":
  d.batteryPercent+"% estimated";
 $("sysV").textContent=number(d.battery,3)+" V";
 $("sysWarning").textContent=
  d.batteryCritical?"CRITICAL":
  d.batteryLow?"LOW":"No warning";

 $("samples").textContent=d.samples;
 $("sampleRateStatus").textContent=d.sampleRate+" Hz";
 $("firmwareBuild").textContent="V"+d.version+" · "+d.build;
 $("recordingLimitStatus").textContent=d.recordingLimitMinutes+" minutes";
 $("sampleErrors").textContent=d.invalid+" / "+d.missed;
 $("sdErrors").textContent=d.sdErrors;
 $("duration").textContent=number(d.duration,1)+" s";
 $("filename").textContent=d.file;
 $("savedFlightLabel").textContent=d.flightLabel;
 $("savedMotorLabel").textContent=d.motorLabel;
 $("minimumPressure").textContent=
  number(pressure(d.minimumPressure),imperial?3:1)+
  (imperial?" inHg":" hPa");
 $("temperatureRange").textContent=
  number(temp(d.temperatureMin),1)+" to "+
  number(temp(d.temperatureMax),1)+
  (imperial?" °F":" °C");

 $("flightLabel").value=(d.armed||d.recording||d.launchWatch)?d.flightLabel:"Mission "+d.missionNumber+" · Card Flight "+d.cardSequence;
 if(!missionDirty&&document.activeElement!==$("missionKind"))$("missionKind").value=d.missionKind;
 if(!missionDirty&&document.activeElement!==$("motorLabel"))
  $("motorLabel").value=d.motorLabel;
 if(!sampleRateDirty && !sampleRateSaving)$("sampleRate").value=String(d.sampleRate);
 if(!sampleRateDirty && !sampleRateSaving)$("recordingLimit").value=String(d.recordingLimitMinutes);
 $("settingsSaveState").textContent=(missionDirty||sampleRateDirty)?"Unsaved settings · Save manually, or choose SAVE SETTINGS & ARM LOGGER to apply both before arming.":"Flight details and sample rate match the recorder.";
 $("launched").textContent=d.launched?"YES":"NO";
 $("finish").textContent=d.finish||"--";
 $("uptime").textContent=d.uptime+" s";
 $("error").textContent=d.error||"None";

 if(collect&&!paused) {
  append("altitude",a);
  append("pressure",d.pressure);
  append("temperature",d.temperature);
  drawAll();
 }
}

async function api(path,method="GET",body=null) {
 const controller=new AbortController();
 const timeout=setTimeout(()=>controller.abort(),10000);
 try {
  const opts={
   method:method,
   cache:"no-store",
   signal:controller.signal
  };
  if(body) {
   opts.headers={
    "Content-Type":"application/x-www-form-urlencoded"
   };
   opts.body=new URLSearchParams(body).toString();
  }
  const separator=path.includes("?")?"&":"?";
  const response=await fetch(
   "/api/"+path+separator+"token="+encodeURIComponent(clientToken),
   opts
  );
  const data=await response.json();
  if(!response.ok||data.success===false) {
   throw Error(data.message||"HTTP "+response.status);
  }
  return data;
 } finally {
  clearTimeout(timeout);
 }
}

async function poll() {
 if(polling)return;
 polling=true;
 try {
  const latest=await api("status");
  render(latest);
  if(!window.phoneTimeSent){window.phoneTimeSent=true;api("time","POST",{epoch:Date.now(),offset:new Date().getTimezoneOffset()}).catch(()=>{window.phoneTimeSent=false;});}
 }catch(err) {
  $("connection").textContent="● OFFLINE";
  $("connection").style.color="#ff91a0";
  $("prepareStatus").textContent="Recorder offline. If a launch was detected, Wi-Fi intentionally remains off until automatic landing save.";
  if(!connectedOnce)notice(err.message,false);
 }finally{
  polling=false;
 }
}

async function testSDCard() {
 if(busy)return;
 busy=true;
 const feedback=$("sdTestFeedback");
 feedback.textContent="Checking controller access...";
 try {
  if(!state)throw Error("Waiting for the recorder connection. Retry in a moment.");
  if(state.recording||state.armed||state.calibrating)
   throw Error("Stop recording, disarm and finish calibration first.");
  if(!state.controller){
   if(!state.controlAvailable)
    throw Error("Another browser controls GREEN-ION. Release its control or wait for its lease to expire.");
   feedback.textContent="Taking control to run SD test...";
   await api("control","POST");
  }
  feedback.textContent="Writing, reading and deleting a temporary test file...";
  const result=await api("sdtest","POST");
  feedback.textContent=result.message;
  notice(result.message,true);
  await loadFiles();
 }catch(err){
  feedback.textContent="SD TEST: "+err.message;
  notice("SD TEST: "+err.message,false);
 }finally{
  busy=false;
  await poll();
 }
}

async function command(name) {
 if(busy)return;
 busy=true;

 const descriptions={
  calibrate:"Requesting calibration...",
  arm:"Checking preflight...",
  start:"Starting recorder...",
  stop:"Stopping and saving...",
  simulate:"Starting simulation...",
  simstop:"Stopping simulation...",
  sdtest:"Testing microSD card..."
 };
 notice(descriptions[name]||"Sending command...");

 try {
  const result=await api(name,"POST");
  notice(result.message,true);
 }catch(err){
  notice(name.toUpperCase()+": "+err.message,false);
 }finally{
  busy=false;
  await poll();
 }
}

async function saveMission() {
 if(busy)return;
 busy=true;
 try {
  const result=await api("mission","POST",{
   kind:$("missionKind").value,
   motor:$("motorLabel").value
  });
  missionDirty=false;
  notice(result.message,true);
 }catch(err){
  notice(err.message,false);
 }finally{
  busy=false;
  await poll();
 }
}

async function saveThenArm(){
 if(busy)return;
 busy=true;notice("Saving flight settings and checking preflight...");
 try{
  if(missionDirty){await api("mission","POST",{kind:$("missionKind").value,motor:$("motorLabel").value});missionDirty=false;}
  if(sampleRateDirty){await api("settings","POST",{rate:$("sampleRate").value,limit:$("recordingLimit").value});sampleRateDirty=false;}
  const result=await api("arm","POST");notice(result.message,true);
 }catch(err){notice("Cannot arm logger: "+err.message,false);}
 finally{busy=false;await poll();}
}

async function prepareAll(){
 if(busy)return;
 busy=true;
 notice("Starting full preparation, calibration, and 10-second ground test...");
 try{
  const result=await api("prepare","POST",{
   kind:$("missionKind").value,
   motor:$("motorLabel").value,
   rate:$("sampleRate").value,
   limit:$("recordingLimit").value,
   epoch:Date.now(),
   offset:new Date().getTimezoneOffset()
  });
  missionDirty=false;sampleRateDirty=false;
  notice(result.message,true);
 }catch(err){notice("PREPARE: "+err.message,false);}
 finally{busy=false;await poll();}
}

async function armAndWatch(){
 if(busy)return;
 busy=true;notice("Arming recorder and starting automatic launch watch...");
 try{const result=await api("armwatch","POST");notice(result.message,true);}
 catch(err){notice("ARM & WATCH: "+err.message,false);}
 finally{busy=false;await poll();}
}

async function scanSelectedIntegrity(){
 if(!selectedFlight){$("integrityScanResult").textContent="Select a flight first.";return;}
 try{const result=await api("integrity?name="+encodeURIComponent(selectedFlight));$("integrityScanResult").textContent=result.result+" · "+result.completeRecords+" complete records · "+result.trailingBytes+" trailing bytes · "+result.implausibleRecords+" implausible records";}
 catch(err){$("integrityScanResult").textContent=err.message;}
}
function downloadSelectedPackage(){
 if(!selectedFlight){alert("Select a flight first.");return;}
 const urls=["/api/download?name=","/api/metadata?name="];
 urls.forEach((base,i)=>setTimeout(()=>{const a=document.createElement("a");a.href=base+encodeURIComponent(selectedFlight);a.download="";document.body.appendChild(a);a.click();a.remove();},i*700));
 setTimeout(()=>{location.href="/api/manifest";},2300);
}
async function rollbackFirmware(){
 if(!confirm("Restart using the previous valid OTA firmware image?"))return;
 try{const result=await api("rollback","POST");notice(result.message,true);}catch(err){notice(err.message,false);}
}
async function restoreConfiguration(){
 const file=$("configFile").files[0];if(!file){notice("Choose a GREENION-CONFIG.json backup.",false);return;}
 try{const config=JSON.parse(await file.text());const result=await api("config","POST",config);notice(result.message,true);await poll();}
 catch(err){notice("Configuration restore failed: "+err.message,false);}
}

async function takeControl() {
 if(busy)return;
 busy=true;
 try {
  const result=await api("control","POST");
  notice(result.message,true);
 }catch(err){
  notice(err.message,false);
 }finally{
  busy=false;
  await poll();
 }
}

async function releaseControl() {
 if(busy)return;
 busy=true;
 try {
  const result=await api("release","POST");
  notice(result.message,true);
 }catch(err){
  notice(err.message,false);
 }finally{
  busy=false;
  await poll();
 }
}

async function saveRate(){
 if(busy)return;
 busy=true;sampleRateSaving=true;
 const rate=$("sampleRate").value;
 const limit=$("recordingLimit").value;
 try{const result=await api("settings","POST",{rate,limit});notice(result.message,true);sampleRateDirty=false;}
 catch(err){notice("Rate change failed: "+err.message,false);sampleRateDirty=true;}
 finally{busy=false;sampleRateSaving=false;await poll();}
}

async function installFirmware() {
 if(busy)return;

 const file=$("firmwareFile").files[0];
 if(!file||!file.name.toLowerCase().endsWith(".ino.bin")) {
  $("updateFeedback").textContent=
   "Choose the main file ending in .ino.bin.";
  return;
 }

 if(!confirm(
  "Install "+file.name+"? Keep power connected. The board will restart."
 ))return;

 busy=true;
 $("firmwareProgress").style.width="0%";
 $("firmwarePercent").textContent="0%";
 $("updateFeedback").textContent=
  "Uploading "+file.name+" · do not disconnect power...";

 const form=new FormData();
 form.append("firmware",file,file.name);

 try {
  const result=await new Promise((resolve,reject)=>{
   const request=new XMLHttpRequest();
   request.open("POST","/api/update?token="+encodeURIComponent(clientToken));
   request.upload.onprogress=event=>{
    if(!event.lengthComputable)return;
    const percent=Math.min(100,Math.round(event.loaded/event.total*100));
    $("firmwareProgress").style.width=percent+"%";
    $("firmwarePercent").textContent=percent+"%";
    $("updateFeedback").textContent="Uploading "+file.name+" · "+percent+"% · do not disconnect power...";
   };
   request.onerror=()=>reject(Error("Network connection lost during upload"));
   request.onload=()=>{try{const body=JSON.parse(request.responseText);if(request.status<200||request.status>=300||!body.success)reject(Error(body.message||"Update failed"));else resolve(body);}catch(error){reject(Error("Invalid update response"));}};
   request.send(form);
  });
  $("firmwareProgress").style.width="100%";
  $("firmwarePercent").textContent="100%";

  $("updateFeedback").textContent=
   result.message+" Wait for GREEN-ION to reappear.";
  notice("Firmware installed. ESP32 is restarting.",true);
  $("connection").textContent="● RESTARTING";
 }catch(err){
  $("updateFeedback").textContent="Update failed: "+err.message;
  notice(err.message,false);
  busy=false;
 }
}

async function shutdown() {
 if(busy)return;

 const warning=
  "Put the Feather into deep sleep?\n\n"+
  "Wi-Fi will disconnect and the dashboard will close.\n"+
  "Active recording will be stopped and saved.\n\n"+
  "To turn it on again, PRESS THE PHYSICAL RESET BUTTON.\n\n"+
  "Continue?";

 if(!confirm(warning))return;

 busy=true;
 $("powerFeedback").textContent="Sending shutdown request...";

 try {
  const r=await api("shutdown","POST");
  $("powerFeedback").textContent=
   r.message+" Press RESET to wake.";
  notice("Shutting down. Press physical RESET to restart.",true);
  $("connection").textContent="● SHUTTING DOWN";
 }catch(err){
  $("powerFeedback").textContent="Shutdown failed: "+err.message;
  notice(err.message,false);
  busy=false;
 }
}

async function disconnect() {
 if(!confirm("Disconnect Wi-Fi while recording?"))return;
 try {
  await api("disconnect","POST");
 }catch(err){
  notice(err.message,false);
 }
}

function append(key,value) {
 if(value==null||!Number.isFinite(Number(value)))return;
 history[key].push(value);
 if(history[key].length>120)history[key].shift();
}

function draw(id,key,convert) {
 const canvas=$(id);
 const width=Math.max(canvas.clientWidth,100);
 const ratio=devicePixelRatio||1;
 canvas.width=width*ratio;
 canvas.height=190*ratio;
 const ctx=canvas.getContext("2d");
 ctx.scale(ratio,ratio);

 const a=history[key]
  .slice(-Number($("graphWindow").value))
  .map(convert).filter(Number.isFinite);

 let lo=a.length?Math.min(...a):0;
 let hi=a.length?Math.max(...a):1;
 if(hi-lo<.01){lo-=.5;hi+=.5}

 ctx.strokeStyle="#41627e";
 for(let i=0;i<4;i++) {
  const y=24+i*45;
  ctx.beginPath();
  ctx.moveTo(24,y);ctx.lineTo(width-10,y);ctx.stroke();
 }
 ctx.fillStyle=sunlight?"#264b65":"#aac2d7";
 ctx.font="11px system-ui";
 ctx.fillText(number(hi),24,13);
 ctx.fillText(number(lo),24,186);
 if(a.length<2)return;

 ctx.beginPath();
 ctx.strokeStyle="#7bdbff";
 ctx.lineWidth=2.5;
 a.forEach((v,i)=>{
  const x=24+i*(width-36)/(a.length-1);
  const y=165-(v-lo)/(hi-lo)*135;
  i?ctx.lineTo(x,y):ctx.moveTo(x,y);
 });
 ctx.stroke();
}

function drawAll() {
 if($("status").classList.contains("active")){
  draw("overviewGraph","altitude",distance);
  draw("altGraph","altitude",distance);
  draw("pressureGraph","pressure",pressure);
  draw("tempGraph","temperature",temp);
 }
}

function clearHistory() {
 history={altitude:[],pressure:[],temperature:[]};
 drawAll();
}

async function led(settings) {
 try {
  const result=await api("led","POST",settings);
  $("ledFeedback").textContent=result.message;
  await poll();
 }catch(err){
  $("ledFeedback").textContent=err.message;
 }
}

function applyLed() {
 led({
  manual:1,r:rgb[0],g:rgb[1],b:rgb[2],
  effect:$("effect").value,
  brightness:$("brightness").value
 });
}

const colors=[
 "#ff4444","#ff9933","#ffe750","#52e798",
 "#32d9ff","#3d80ff","#b768ff","#ffffff"
];

for(const hex of colors) {
 const b=document.createElement("button");
 b.className="swatch";
 b.style.background=hex;
 b.title=hex;
 b.addEventListener("click",()=>{
  rgb=[1,3,5].map(i=>parseInt(hex.slice(i,i+2),16));
  applyLed();
 });
 $("swatches").appendChild(b);
}

function bytes(n) {
 if(n>=1048576)return(n/1048576).toFixed(2)+" MB";
 if(n>=1024)return(n/1024).toFixed(1)+" KB";
 return n+" bytes";
}

let savedFlightSeries=[];
let loadedFlightName="";
let graphRequestId=0;
function extreme(values,minimum){let result=minimum?Infinity:-Infinity;for(const value of values){if(!Number.isFinite(value))continue;result=minimum?Math.min(result,value):Math.max(result,value);}return Number.isFinite(result)?result:NaN;}
let flightPhases=[];
let flightMeta={};
function decodeRkt4(buffer){
 const d=new DataView(buffer);if(d.byteLength<32||String.fromCharCode(...new Uint8Array(buffer,0,4))!=="RKT4"||d.getUint16(4,true)!==2||d.getUint16(6,true)!==32||d.getUint16(8,true)!==16)throw Error("Invalid RKT4 v2 file");
 const baseline=d.getFloat32(12,true),count=Math.floor((d.byteLength-32)/16),rows=[];let first=0,prevT=NaN,prevA=NaN;
 for(let i=0;i<count;i++){const o=32+i*16,t=d.getUint32(o,true),p=d.getFloat32(o+4,true),temp=d.getFloat32(o+8,true),v=d.getUint16(o+12,true)/1000,valid=d.getUint16(o+14,true)===1;if(i===0)first=t;const time=((t-first)>>>0)/1e6,alt=valid&&p>0?44330*(1-Math.pow(p/baseline,.190294957)):NaN,dt=time-prevT,vel=Number.isFinite(prevA)&&dt>0&&dt<.5?(alt-prevA)/dt:NaN;rows.push([time,p,temp,v,valid?1:0,alt,vel]);if(Number.isFinite(alt)){prevA=alt;prevT=time;}}
 return rows;
}
function rowsCsv(rows){return "time_s,pressure_pa,temperature_c,battery_v,valid,altitude_m,vertical_speed_mps,estimated_acceleration_mps2,estimated_vertical_apparent_g\r\n"+rows.map(r=>r.map(v=>Number.isFinite(v)?v:"").join(",")).join("\r\n");}
function downloadRowsCsv(name,rows){const a=document.createElement("a"),blob=new Blob([rowsCsv(rows)],{type:"text/csv"});a.href=URL.createObjectURL(blob);a.download=name.replace(/\.BIN$/i,".CSV");a.click();URL.revokeObjectURL(a.href);}
async function loadSavedGraph(name){
 const requestId=++graphRequestId;
 loadedFlightName="";
 const label=$("savedGraphStatus");
 label.textContent="Loading recorded samples...";
 try{
  const response=await fetch("/api/download?name="+encodeURIComponent(name),{cache:"no-store"});
  if(!response.ok)throw Error("Could not load recorded BIN ("+response.status+")");
  const parsed=decodeRkt4(await response.arrayBuffer());
  if(requestId!==graphRequestId||selectedFlight!==name)return;
  savedFlightSeries=parsed;
  loadedFlightName=name;
  label.textContent=savedFlightSeries.length+" samples loaded from "+name;
  enrichFlightSeries(savedFlightSeries);
  renderFlightPerformance(savedFlightSeries);
  computePhases(savedFlightSeries);
  drawSavedGraph();
 }catch(err){if(requestId!==graphRequestId)return;savedFlightSeries=[];loadedFlightName="";label.textContent=err.message;$("flightPerformance").textContent="Performance unavailable: "+err.message;drawSavedGraph();}
}
function enrichFlightSeries(rows){
 const valid=rows.filter(r=>r[4]===1&&Number.isFinite(r[0])&&Number.isFinite(r[5]));
 for(const r of rows){r[7]=NaN;r[8]=NaN;}
 const velocities=valid.map((r,i)=>{const a=valid[Math.max(0,i-3)],b=valid[Math.min(valid.length-1,i+3)],dt=b[0]-a[0];return dt>.05&&dt<3?(b[5]-a[5])/dt:NaN;});
 for(let i=0;i<valid.length;i++){const a=Math.max(0,i-3),b=Math.min(valid.length-1,i+3),dt=valid[b][0]-valid[a][0];if(dt>.05&&dt<3&&Number.isFinite(velocities[a])&&Number.isFinite(velocities[b])){valid[i][7]=(velocities[b]-velocities[a])/dt;valid[i][8]=1+valid[i][7]/9.80665;}}
 const battery=rows.filter(r=>Number.isFinite(r[3])&&r[3]>0),acc=valid.map(r=>r[7]).filter(Number.isFinite),g=valid.map(r=>r[8]).filter(Number.isFinite);
 const first=battery.length?battery[0][3]:NaN,last=battery.length?battery[battery.length-1][3]:NaN;
 const metrics=[["Starting battery",first,"V"],["Ending battery",last,"V"],["Minimum battery",battery.length?extreme(battery.map(r=>r[3]),true):NaN,"V"],["Battery voltage drop",first-last,"V"],["Maximum estimated upward acceleration",acc.length?extreme(acc,false):NaN,"m/s²"],["Maximum estimated downward acceleration",acc.length?extreme(acc,true):NaN,"m/s²"],["Peak estimated vertical apparent G",g.length?extreme(g,false):NaN,"g"]];
 const out=$("flightExtraMetrics");out.replaceChildren();
 for(const [name,value,unit] of metrics){const row=document.createElement("div");row.className="row";const label=document.createElement("span");label.textContent=name;const val=document.createElement("strong");val.textContent=Number.isFinite(value)?value.toFixed(3)+" "+unit:"Unavailable";row.append(label,val);out.appendChild(row);}
 const note=document.createElement("p");note.className="muted";note.textContent="Acceleration and vertical apparent G are noisy estimates from barometric altitude, not direct accelerometer readings. Apparent G assumes a vertical rocket and is not total 3D G-force.";out.appendChild(note);
}
function computePhases(rows){
 const pts=rows.filter(r=>r[4]===1&&Number.isFinite(r[0])&&Number.isFinite(r[5]));
 flightPhases=[];const out=$("flightPhases");out.replaceChildren();
 if(pts.length<6){out.textContent="Not enough valid samples.";return;}
 const li=pts.findIndex((r,i)=>i+2<pts.length&&r[5]>=5&&pts[i+1][5]>=5&&pts[i+2][5]>=5);
 if(li<0){out.textContent="No sustained 5 m launch detected.";return;}
 const launch=pts[li],peak=pts.slice(li).reduce((a,b)=>a[5]>b[5]?a:b);
 const pi=pts.indexOf(peak);
 const di=pts.findIndex((r,i)=>i>pi+2&&i+2<pts.length&&Math.abs(r[5])<=5&&Math.abs(pts[i+1][5])<=5&&Math.abs(pts[i+2][5])<=5);
 flightPhases=[["Launch detected",launch[0],launch[5]],["Ascent / coast",launch[0],launch[5]],["Apogee",peak[0],peak[5]],["Descent",peak[0],peak[5]]];
 if(di>=0)flightPhases.push(["Landing detected",pts[di][0],pts[di][5]]);
 for(const [name,t,alt] of flightPhases){const row=document.createElement("div");row.className="row";const l=document.createElement("span");l.textContent=name;const v=document.createElement("strong");v.textContent=t.toFixed(2)+" s · "+alt.toFixed(1)+" m";row.append(l,v);out.appendChild(row);}
 const note=document.createElement("p");note.className="muted";note.textContent="Estimated from barometric altitude; launch and landing use sustained 5 m thresholds. Motor burnout, powered ascent, and parachute deployment cannot be determined reliably.";out.appendChild(note);
}
function renderFlightPerformance(rows){
 const target=$("flightPerformance");target.replaceChildren();
 // RKT4 CSV: time, pressure, temperature, battery, valid, altitude, velocity.
 const pts=rows.filter(r=>r[4]===1&&Number.isFinite(r[0])&&Number.isFinite(r[5]));
 if(pts.length<3){target.textContent="Insufficient valid samples for flight analysis.";return;}
 const peak=pts.reduce((best,p)=>p[5]>best[5]?p:best,pts[0]);
 // Threshold-based launch detection; not a motor ignition timestamp.
 const launch=pts.find(p=>p[5]>=5);
 const afterPeak=pts.filter(p=>p[0]>peak[0]);
 const landed=afterPeak.find(p=>Math.abs(p[5])<=5);
 const ascent=launch?pts.filter(p=>p[0]>=launch[0]&&p[0]<=peak[0]):[];
 const descent=landed?pts.filter(p=>p[0]>=peak[0]&&p[0]<=landed[0]):[];
 // Use 5-sample moving slopes to reduce barometer noise. Still estimates.
 const velocities=[];
 for(let i=2;i<pts.length-2;i++){
  const left=pts[i-2],right=pts[i+2],dt=right[0]-left[0];
  if(dt>.03&&dt<2&&Number.isFinite(dt))velocities.push({t:pts[i][0],v:(right[5]-left[5])/dt});
 }
 const rise=velocities.filter(p=>launch&&p.t>=launch[0]&&p.t<=peak[0]);
 const fall=velocities.filter(p=>p.t>=peak[0]&&(!landed||p.t<=landed[0]));
 const maxRise=rise.length?extreme(rise.map(p=>p.v),false):NaN;
 const maxFall=fall.length?extreme(fall.map(p=>p.v),true):NaN;
 const rate=Number(window.currentFlightSampleRate)||25;
 const recorded=Number(window.currentFlightSamples)||rows.length;
 const missed=Number(window.currentFlightMissed)||0;
 const invalid=Number(window.currentFlightInvalid)||0;
 const expected=recorded+missed;
 const completeness=expected>0?100*Math.max(0,recorded-invalid)/expected:NaN;
 const ascentTime=launch?peak[0]-launch[0]:NaN;
 const descentTime=landed?landed[0]-peak[0]:NaN;
 const metrics=[
  ["Maximum ascent speed",maxRise,"m/s"],
  ["Maximum descent speed",Number.isFinite(maxFall)?Math.abs(maxFall):NaN,"m/s"],
  ["Time to apogee",ascentTime,"s"],
  ["Ascent duration",ascentTime,"s"],
  ["Descent duration",descentTime,"s"],
  ["Average ascent speed",ascentTime>0?(peak[5]-launch[5])/ascentTime:NaN,"m/s"],
  ["Average descent speed",descentTime>0?(peak[5]-landed[5])/descentTime:NaN,"m/s"],
  ["Recording completeness",completeness,"%"]
 ];
 for(const [name,value,unit] of metrics){
  const row=document.createElement("div");row.className="row";
  const label=document.createElement("span");label.textContent=name;
  const val=document.createElement("strong");val.textContent=Number.isFinite(value)?value.toFixed(unit==="%"?1:2)+" "+unit:"Not determined";
  row.append(label,val);target.appendChild(row);
 }
 const note=document.createElement("p");note.className="muted";
 note.textContent="Speed is estimated from smoothed barometric altitude. Launch uses a 5 m threshold; landing uses a return within 5 m of ground. No detected launch/landing means related times are unavailable. Completeness counts invalid and missed samples; it is not a guarantee of accuracy.";
 target.appendChild(note);
}
function drawSavedGraph(){
 const canvas=$("savedFlightGraph"),rect=canvas.getBoundingClientRect();
 const width=Math.max(250,Math.round(rect.width||300)),height=220;
 canvas.width=width*devicePixelRatio;canvas.height=height*devicePixelRatio;
 const ctx=canvas.getContext("2d");ctx.setTransform(devicePixelRatio,0,0,devicePixelRatio,0,0);
 ctx.clearRect(0,0,width,height);
 const column=Number($("savedGraphMetric").value);
 const pts=savedFlightSeries.filter(r=>Number.isFinite(r[column]));
 if(pts.length<2)return;
 const x0=pts[0][0],x1=pts[pts.length-1][0],values=pts.map(r=>r[column]);
 let lo=Math.min(...values.slice(0,1000)),hi=Math.max(...values.slice(0,1000));
 for(const v of values){if(v<lo)lo=v;if(v>hi)hi=v;}
 if(hi===lo)hi=lo+1;
 ctx.strokeStyle="#35536e";ctx.strokeRect(44,12,width-58,height-43);
 ctx.fillStyle=sunlight?"#223f56":"#eff8ff";ctx.font="11px system-ui";
 ctx.fillText(hi.toFixed(1),3,19);ctx.fillText(lo.toFixed(1),3,height-31);
 ctx.fillText((x1-x0).toFixed(1)+" s",width-65,height-8);
 ctx.strokeStyle="#7bdbff";ctx.lineWidth=1.7;ctx.beginPath();
 const step=Math.max(1,Math.ceil(pts.length/1400));
 for(let i=0;i<pts.length;i+=step){const r=pts[i];
  const x=44+(r[0]-x0)/Math.max(.001,x1-x0)*(width-58);
  const y=12+(hi-r[column])/(hi-lo)*(height-43);
  if(i===0)ctx.moveTo(x,y);else ctx.lineTo(x,y);
 }
 ctx.stroke();
 ctx.save();ctx.font="10px system-ui";ctx.setLineDash([4,4]);
 for(const [name,t] of flightPhases.filter(p=>["Launch detected","Apogee","Landing detected"].includes(p[0]))){
  const x=44+(t-x0)/Math.max(.001,x1-x0)*(width-58);ctx.strokeStyle=name==="Apogee"?"#ffdd88":"#72e5ab";ctx.beginPath();ctx.moveTo(x,12);ctx.lineTo(x,height-31);ctx.stroke();ctx.fillStyle=sunlight?"#223f56":"#eff8ff";ctx.fillText(name.split(" ")[0],Math.min(width-90,Math.max(44,x+2)),22);
 }ctx.restore();
}
async function readFlightMetadata(name){
 const response=await fetch("/api/flightstats?name="+encodeURIComponent(name),{cache:"no-store"});
 if(!response.ok)throw Error(name+": summary missing");
 const result={};for(const line of (await response.text()).split(/\r?\n/)){
  const ix=line.indexOf("=");if(ix>0)result[line.slice(0,ix).trim()]=line.slice(ix+1).trim();
 }return result;
}
async function showFlightStats(name,scrollToReport=true) {
 selectedFlight=name;
 savedFlightSeries=[];
 loadedFlightName="";
 const status=$("flightSummaryStatus");
 const details=$("flightSummaryDetails");
 details.replaceChildren();
 status.textContent="Loading "+name+"...";
 try {
  const response=await fetch("/api/flightstats?name="+encodeURIComponent(name),
   {cache:"no-store"});
  const content=await response.text();
  if(!response.ok)throw Error(content||"Cannot read flight summary");
  const values={};
  for(const line of content.split(/\r?\n/)) {
   const ix=line.indexOf("=");
   if(ix>0)values[line.slice(0,ix).trim()]=line.slice(ix+1).trim();
  }
  flightMeta=values;
  const timeline=$("flightConfidenceTimeline");timeline.replaceChildren();
  const events=Object.entries(values).filter(([key])=>key.startsWith("confidence_event_"))
   .sort((a,b)=>Number(a[0].split("_").pop())-Number(b[0].split("_").pop()));
  if(!events.length)timeline.textContent="No firmware confidence events were recorded.";
  for(const [,value] of events){const [ms,label,eventValue]=value.split("|");const row=document.createElement("div");row.className="row";const left=document.createElement("span");left.textContent=label||"Event";const right=document.createElement("strong");right.textContent=(Number(ms)/1000).toFixed(2)+" s"+(eventValue&&eventValue!=="0"?" · "+eventValue:"");row.append(left,right);timeline.appendChild(row);}
  window.currentFlightSamples=values.samples;
  window.currentFlightMissed=values.missed_samples;
  window.currentFlightInvalid=values.invalid_samples;
  window.currentFlightSampleRate=values.sample_rate_hz;
  if(values.samples && values.invalid_samples){
   const total=Number(values.samples),invalid=Number(values.invalid_samples);
   if(total>0 && Number.isFinite(invalid))
    values.valid_percent=(100*(total-invalid)/total).toFixed(1)+"%";
  }
  if(values.peak_altitude_m && imperial)
   values.peak_altitude_ft=(Number(values.peak_altitude_m)*3.280839895).toFixed(1);
  const fields=[
   ["Flight","label"],["Motor","motor"],["Status","status"],
   ["Finish reason","finish_reason"],["Duration (s)","duration_s"],
   ["Peak altitude (m)","peak_altitude_m"],
   ["Peak altitude (ft)","peak_altitude_ft"],
   ["Samples recorded","samples"],["Valid samples (%)","valid_percent"],
   ["Invalid samples","invalid_samples"],
   ["Missed samples","missed_samples"],["Sampling rate (Hz)","sample_rate_hz"],
   ["Minimum pressure (Pa)","minimum_pressure_pa"],
   ["Minimum temperature (°C)","temperature_min_c"],
   ["Maximum temperature (°C)","temperature_max_c"],
   ["Starting battery (V)","starting_battery_v"]
  ];
  for(const [label,key] of fields){
   if(!(key in values))continue;
   const row=document.createElement("div");row.className="row";
   const title=document.createElement("span");title.textContent=label;
   const value=document.createElement("strong");value.textContent=values[key];
   row.append(title,value);details.appendChild(row);
  }
  if(!details.childElementCount)throw Error("Summary has no readable statistics");
  status.textContent="Showing saved statistics for "+name;
  loadSavedGraph(name);
  if(scrollToReport)$("flightSummaryCard").scrollIntoView({behavior:"smooth",block:"start"});
 }catch(err){status.textContent=err.message;}
}

function safeHTML(v){return String(v??"—").replace(/[&<>"']/g,c=>({"&":"&amp;","<":"&lt;",">":"&gt;",'"':"&quot;","'":"&#39;"}[c]));}
function pdfRows(rows){return "<table>"+rows.map(([k,v])=>"<tr><th>"+safeHTML(k)+"</th><td>"+safeHTML(v)+"</td></tr>").join("")+"</table>";}
function printableReport(title,body){
 const w=window.open("","_blank");if(!w){alert("Enable pop-ups for the printable report.");return;}
 w.document.open();w.document.write('<!doctype html><html><head><meta name="viewport" content="width=device-width,initial-scale=1"><title>'+safeHTML(title)+'</title><style>body{font:14px/1.5 system-ui,Arial;color:#172b40;max-width:800px;margin:25px auto;padding:0 16px}h1{font-size:24px}h2{font-size:18px;margin-top:24px}table{width:100%;border-collapse:collapse}td,th{padding:8px;border-bottom:1px solid #ddd;text-align:left}th{width:48%}img{max-width:100%;height:auto}.toolbar{background:#eef4f8;padding:12px}@media print{.toolbar{display:none}body{margin:0}table,img{break-inside:avoid}}</style></head><body><div class="toolbar"><button onclick="window.print()">PRINT / SAVE PDF</button> · On iPhone: Print, then share the preview to Files.</div><h1>'+safeHTML(title)+'</h1>'+body+'<p>Experimental barometric estimates. Not for flight-critical decisions.</p></body></html>');w.document.close();
}
function reportGraph(rows,column,title){
 const valid=rows.filter(r=>r[4]===1&&Number.isFinite(r[0])&&Number.isFinite(r[column]));
 if(valid.length<2)return "<p>"+safeHTML(title)+": insufficient data.</p>";
 const c=document.createElement("canvas");c.width=1000;c.height=310;const ctx=c.getContext("2d");ctx.fillStyle="#fff";ctx.fillRect(0,0,1000,310);
 let low=Infinity,high=-Infinity;for(const r of valid){low=Math.min(low,r[column]);high=Math.max(high,r[column]);}if(low===high)high=low+1;
 const t0=valid[0][0],duration=Math.max(.001,valid[valid.length-1][0]-t0);
 ctx.fillStyle="#152a40";ctx.font="16px Arial";ctx.fillText(title,65,22);ctx.font="12px Arial";ctx.fillText(high.toFixed(2),5,43);ctx.fillText(low.toFixed(2),5,270);ctx.fillText(duration.toFixed(2)+" seconds",850,298);
 ctx.strokeStyle="#c4d0dd";ctx.strokeRect(65,35,910,235);ctx.strokeStyle="#176da0";ctx.lineWidth=2;ctx.beginPath();let started=false;
 const step=Math.max(1,Math.ceil(valid.length/2000));for(let i=0;i<valid.length;i+=step){const r=valid[i],x=65+(r[0]-t0)/duration*910,y=35+(high-r[column])/(high-low)*235;if(!started){ctx.moveTo(x,y);started=true;}else ctx.lineTo(x,y);}ctx.stroke();
 if(column===5){for(const [name,time] of flightPhases.filter(p=>["Launch detected","Apogee","Landing detected"].includes(p[0]))){const x=65+(time-t0)/duration*910;ctx.setLineDash([4,4]);ctx.strokeStyle="#c88c27";ctx.beginPath();ctx.moveTo(x,35);ctx.lineTo(x,270);ctx.stroke();ctx.setLineDash([]);ctx.fillStyle="#152a40";ctx.fillText(name.split(" ")[0],Math.min(900,x+3),50);}}
 return "<h2>"+safeHTML(title)+"</h2><img alt='"+safeHTML(title)+"' src='"+c.toDataURL("image/png")+"'>";
}
function exportFlightPDF(){
 if(!selectedFlight||loadedFlightName!==selectedFlight||!savedFlightSeries.length){alert("Select a saved flight and wait for the graph to finish loading.");return;}
 const phases=flightPhases.map(([n,t,a])=>[n,t.toFixed(2)+" s · "+a.toFixed(1)+" m"]);
 const perf=[...$("flightPerformance").querySelectorAll(".row")].map(r=>[r.querySelector("span")?.textContent,r.querySelector("strong")?.textContent]);
 const extra=[...$("flightExtraMetrics").querySelectorAll(".row")].map(r=>[r.querySelector("span")?.textContent,r.querySelector("strong")?.textContent]);
 const charts=[[5,"Altitude (m)"],[6,"Vertical speed (m/s)"],[7,"Estimated acceleration (m/s²)"],[8,"Estimated vertical apparent G"],[3,"Battery voltage (V)"],[1,"Pressure (Pa)"],[2,"Temperature (°C)"]].map(([column,title])=>reportGraph(savedFlightSeries,column,title)).join("");
 printableReport("GREEN-ION flight — "+selectedFlight,"<h2>Flight metadata</h2>"+pdfRows(Object.entries(flightMeta))+"<h2>Estimated phases</h2>"+pdfRows(phases)+"<h2>Flight performance</h2>"+pdfRows(perf)+"<h2>Battery and acceleration</h2>"+pdfRows(extra)+charts);
}
async function loadFiles() {
 try {
  const result=await api("files");
  $("storageUsed").textContent=
   bytes(result.used)+" / "+bytes(result.total);
  $("sdFreeSpace").textContent=bytes(Math.max(0,result.total-result.used));
  $("fileCount").textContent=result.files.length;
  $("sdTestResult").textContent=state?.sdVerified?"PASS":"NOT VERIFIED";
  const list=$("fileList");
  list.replaceChildren();

  if(!result.files.length) {
   list.textContent="No flight files yet. Save a test recording to see it here.";
   return;
  }

  const sort=$("fileSort").value;
  const files=result.files.slice().sort((a,b)=>sort==="old"?a.name.localeCompare(b.name):sort==="size"?b.size-a.size:b.name.localeCompare(a.name));
  for(const f of files) {
   const row=document.createElement("div");
   row.className="file";
   const text=document.createElement("span");
   text.textContent=f.name+" · "+bytes(f.size);

   const actions=document.createElement("span");
   const dl=document.createElement("button");
   dl.textContent="BIN";
   dl.onclick=()=>{
    location.href="/api/download?name="+
     encodeURIComponent(f.name);
   };

   const csv=document.createElement("button");
   csv.textContent="CSV";
   csv.onclick=async()=>{await loadSavedGraph(f.name);if(loadedFlightName===f.name)downloadRowsCsv(f.name,savedFlightSeries);};

   const summary=document.createElement("button");
   summary.textContent="View stats";
   summary.onclick=()=>showFlightStats(f.name);

   const txt=document.createElement("button");
   txt.textContent="TXT";
   txt.onclick=()=>{
    location.href="/api/metadata?name="+
     encodeURIComponent(f.name);
   };

   const rm=document.createElement("button");
   rm.textContent="Delete from SD";
   rm.className="danger";
   const deleteBlocked=!!state?.recording||!!state?.home_test_active||
    !!state?.maintenance_mode||!!state?.update_in_progress;
   rm.disabled=deleteBlocked;
   rm.title=deleteBlocked?"Finish recording/testing/maintenance before deleting":"Delete this flight package from Feather SD only";
   rm.onclick=async()=>{
    const warning="Delete "+f.name+" from the Feather SD card?\n\n"+
     "This removes its BIN plus matching SEN/TXT/recovery files from the Feather only. "+
     "Browser, Windows, and cloud backups are not deleted.\n\n"+
     "If launch watching is active, GREEN ION will restart stationary calibration.";
    if(!confirm(warning))return;
    rm.disabled=true;
    $("sdDeleteFeedback").textContent="Deleting "+f.name+" from Feather SD...";
    try {
     const result=await api("delete?name="+encodeURIComponent(f.name)+
      "&confirm="+encodeURIComponent(f.name),"POST");
     if(!result.success||result.verified!==true)throw Error(result.message||"Delete was not verified");
     $("sdDeleteFeedback").textContent=result.message+" · "+result.deleted_files+" file(s) removed and verified absent.";
     if(selectedFlight===f.name){
      selectedFlight="";loadedFlightName="";savedFlightSeries=[];flightPhases=[];flightMeta={};graphRequestId++;
      $("flightSummaryDetails").replaceChildren();
      $("flightSummaryStatus").textContent="Flight removed from Feather SD. Local backups, if any, remain available.";
      $("flightPhases").textContent="No Feather flight selected.";
      $("flightConfidenceTimeline").textContent="No Feather flight selected.";
      $("flightPerformance").textContent="No Feather flight selected.";
      $("savedGraphStatus").textContent="No Feather flight selected.";
     }
     await loadFiles();
     await poll();
    }catch(err){
     $("sdDeleteFeedback").textContent="Delete failed safely: "+err.message;
     rm.disabled=false;
    }
   };

   actions.append(summary,dl,csv,txt,rm);
   row.append(text,actions);
   list.appendChild(row);
  }
  if(!selectedFlight && files.length)await showFlightStats(files[0].name,false);
 }catch(err){
  $("fileList").textContent="Cannot list recordings: "+err.message+(state?.recording?" · Wait until recording finishes.":" · Check the SD card and retry.");
 }
}

$("metric").onclick=()=>setUnits(false);
$("imperial").onclick=()=>setUnits(true);
$("sunlightMode").onclick=()=>setSunlight(!sunlight);
$("sunlightMode").textContent=sunlight?"Dark mode":"Sunlight mode";
$("sunlightMode").setAttribute("aria-pressed",sunlight?"true":"false");

document.querySelectorAll("[data-tab]").forEach(b=>{
 b.onclick=()=>openTab(b.dataset.tab,b);
});

$("calibrate").onclick=()=>command("calibrate");
$("testCalibrate").onclick=()=>command("calibrate");
$("prepareAll").onclick=prepareAll;
$("fieldPrepare").onclick=prepareAll;
$("armWatch").onclick=armAndWatch;
$("fieldArm").onclick=armAndWatch;
$("arm").onclick=()=>command("arm");
$("start").onclick=()=>command("start");
$("cancelWatch").onclick=()=>command("cancel");
$("saveMission").onclick=saveMission;
$("saveRate").onclick=saveRate;
$("sampleRate").addEventListener("change",()=>{sampleRateDirty=true;});
$("recordingLimit").addEventListener("change",()=>{sampleRateDirty=true;});
$("missionKind").addEventListener("change",()=>{missionDirty=true;});
$("motorLabel").addEventListener("input",()=>{missionDirty=true;});
$("takeControl").onclick=takeControl;
$("releaseControl").onclick=releaseControl;
$("installFirmware").onclick=installFirmware;
$("stop").onclick=()=>{
 if(confirm("Stop and save flight?"))command("stop");
};
$("simulate").onclick=()=>command("simulate");
$("simstop").onclick=()=>command("simstop");
$("sdtest").onclick=testSDCard;
$("groundtest").onclick=()=>command("groundtest");
$("shutdown").onclick=shutdown;
$("disconnect").onclick=disconnect;
$("fieldStop").onclick=()=>{if(confirm("Stop and save flight?"))command("stop");};
$("scanIntegrity").onclick=scanSelectedIntegrity;
$("downloadEverything").onclick=downloadSelectedPackage;
$("rollbackFirmware").onclick=rollbackFirmware;
$("restoreConfig").onclick=restoreConfiguration;
$("downloadPreflight").onclick=()=>{if(state?.preflightReport)location.href="/api/preflight";};

$("pause").onclick=()=>{
 paused=!paused;
 $("pause").textContent=paused?
  "Resume graphs":"Pause graphs";
};
$("clear").onclick=clearHistory;
$("graphWindow").onchange=drawAll;
$("refresh").onclick=loadFiles;
$("fileSort").onchange=loadFiles;
$("savedGraphMetric").onchange=drawSavedGraph;
$("exportFlightPDF").onclick=exportFlightPDF;
$("openFlightStats").onclick=()=>openTab("storage",document.querySelector('[data-tab="storage"]'));

$("effect").onchange=applyLed;
$("brightness").oninput=()=>{
 $("brightnessLabel").textContent=
  $("brightness").value+"%";
};
$("brightness").onchange=applyLed;
$("ledoff").onclick=()=>{
 $("effect").value="4";
 led({manual:1,effect:4});
};
$("ledauto").onclick=()=>led({manual:0});

window.addEventListener("error",e=>{
 notice("Dashboard error: "+e.message,false);
});
window.addEventListener("resize",()=>{drawAll();drawSavedGraph();});

notice("V11 dashboard restored. Starting V12 compatibility layer...");
</script>
<script>
"use strict";
const v12Unsupported=[
 "takeControl","releaseControl","saveMission","saveRate","arm","start","cancelWatch",
 "simulate","simstop","ledoff","ledauto",
 "restoreConfig","shutdown","disconnect","downloadPreflight","scanIntegrity","downloadEverything"
];
const v12HardwareEndpoint='/api/hardware-test';
const v12Set=(id,value)=>{const node=$(id);if(node)node.textContent=value;};
const v12Finite=value=>value!==null&&value!==''&&Number.isFinite(Number(value));
const v12Number=(value,digits=1,suffix="--")=>v12Finite(value)?Number(value).toFixed(digits):suffix;
const v12FlightNames=["CALIBRATING","WATCHING FOR LAUNCH","RECORDING","LANDED / COMPLETE","RECORDER FAULT","BENCH MAINTENANCE — LAUNCH DETECTION OFF","HOME VERIFICATION — LAUNCH DETECTION OFF"];
let v12Busy=false;


const v12PreflightLabels=[
 "SD card unavailable","BMP388 missing or unhealthy","Measured battery below 3.60 V",
 "Calibration incomplete or invalid","Not watching for launch","Home test / OTA maintenance active",
 "Interrupted flight recovery evidence needs review","Temporary home test cleanup or interrupted session",
 "IMU missing or wrong ±32 g range","No current GPS fix","LoRa receiver link not verified (or radio unavailable)",
 "Sampling periods were missed","Companion or metadata/checkpoint errors",
 "Calibration older than 30 minutes","Flight profile unset"];
function v12PreflightDetails(d){
 const flags=Number(d.preflight_flags||0)>>>0;
 const warnings=v12PreflightLabels.filter((label,i)=>(flags&(1<<i))!==0);
 const first=warnings.slice(0,5).join(" · ");
 return warnings.length?first+(warnings.length>5?" · +"+(warnings.length-5)+" more":""):
    "All software checks currently satisfied. Physical mounting, range and flight readiness remain unverified.";
}
function v12SavePreflightReport(){
 if(!state)return;
 const d=state,report={type:"GREEN ION V14 preflight advisory",
   captured:new Date().toISOString(),rocket:d.selected_rocket,motor:d.selected_motor,
   advisory:d.preflight_state,flags:d.preflight_flags,warnings:v12PreflightDetails(d),
   battery_v:d.battery_v,sd_ok:d.sd_ok,barometer_ok:d.barometer_ready,
   imu_ok:d.imu_ok,gps_status:d.gps_status,radio_ok:d.radio_ok,
   calibration_baseline_pa:d.calibration_baseline_pa,
   calibration_age_s:d.calibration_age_s,missed_periods:d.missed_sample_periods,
   hardware_approved:false,
   disclaimer:"This is software evidence, not permission to launch; verify physical assembly and range safety."};
 const blob=new Blob([JSON.stringify(report,null,2)],{type:"application/json"});
 const url=URL.createObjectURL(blob),link=document.createElement("a");
 link.href=url;link.download="green-ion-v12-preflight.json";link.click();
 setTimeout(()=>URL.revokeObjectURL(url),1000);
}
function v12UpdateProfileUi(d){
 if(!document.activeElement || !["rocketPreset","rocketCustom","profileMotor"].includes(document.activeElement.id)){
  const rocket=String(d.selected_rocket||"Green Eggs");
  const standard=rocket==="Green Eggs"||rocket==="So Long";
  $("rocketPreset").value=standard?rocket:"Custom";
  $("rocketCustom").value=standard?"":rocket;
  $("profileMotor").value=d.selected_motor||"";
 }
 $("rocketCustom").disabled=$("rocketPreset").value!=="Custom";
 $("saveFlightProfile").disabled=!!d.recording||!!d.home_test_active||
     !!d.maintenance_mode||!!d.update_in_progress;
 v12Set("flightProfileFeedback","Saved on Feather: "+(d.selected_rocket||"Unknown rocket")+
     (d.selected_motor?" · "+d.selected_motor:" · motor not set")+
     ". Profile changes restart calibration.");
}
async function v12SaveFlightProfile(){
 const rocket=$("rocketPreset").value==="Custom"?
    $("rocketCustom").value.trim():$("rocketPreset").value;
 const motor=$("profileMotor").value.trim();
 if(!rocket||rocket.length>24){v12Set("flightProfileFeedback","Enter a rocket name of 1–24 characters.");return;}
 if(!confirm("Save profile for "+rocket+"? This restarts stationary calibration. Do not change profiles during launch preparation."))return;
 $("saveFlightProfile").disabled=true;
 try{
  const result=await v12Fetch("profile","POST",{rocket,motor});
  v12Set("flightProfileFeedback",result.message);
  await v12Poll();
 }catch(error){v12Set("flightProfileFeedback","Profile not saved: "+error.message);}
 finally{$("saveFlightProfile").disabled=false;}
}

function v12ConfigureUi(){
 $('flight').append($('prepareRecordCard'),$('recorderHealthCard'));
 $('utilities').prepend($('controlAccessCard'));
 $('status').append($('graphOptionsCard'),$('altGraphCard'),$('pressureGraphCard'),$('tempGraphCard'));
 document.querySelectorAll('[data-tab]').forEach(button=>button.onclick=()=>openTab(button.dataset.tab,button));
 v12Unsupported.forEach(id=>{const node=$(id);if(node){node.disabled=true;node.title="Not yet implemented by the V12 firmware backend";}});
 ['missionKind','motorLabel','sampleRate','recordingLimit'].forEach(id=>{const node=$(id);if(node)node.disabled=true;});
 v12Set('controlState','V12 LOCAL');v12Set('controlLease','No remote-control lease');
 v12Set('settingsSaveState','V12 sample rate and recording policy are fixed in firmware for deterministic logging.');
 v12Set('testFeedback','Use the real IMU, GPS and all-sensor tests above. Simulation is intentionally disabled.');
 v12Set('ledFeedback','V12 automatic status indication is firmware-controlled. Manual RGB effects are not yet exposed.');
 v12Set('updateFeedback','Bench OTA ready. Upload only a verified Feather .ino.bin image while on stable USB power; the recorder must not be in flight.');
 v12Set('maintenanceStatus','OTA/rollback are available in bench maintenance; configuration restore remains unavailable.');
 v12Set('powerFeedback','Deep sleep control is not exposed in V12. Recorder operation remains autonomous.');
 v12Set('groundTestDetail','Runs the live IMU/GPS diagnostic and, when idle, a real temporary SD write/read/delete test.');
 $('prepareAll').textContent='RESTART STATIONARY CALIBRATION';
 $('fieldPrepare').textContent='RESTART STATIONARY CALIBRATION';
 $('armWatch').textContent='AUTOMATIC WATCH IS FIRMWARE CONTROLLED';$('armWatch').disabled=true;
 $('fieldArm').textContent='AUTOMATIC WATCH IS FIRMWARE CONTROLLED';$('fieldArm').disabled=true;
 $('prepareAll').onclick=$('fieldPrepare').onclick=$('calibrate').onclick=$('testCalibrate').onclick=v12Calibrate;
 $('stop').onclick=$('fieldStop').onclick=v12Stop;
 $('testImu').onclick=()=>v12HardwareTest('imu');
 $('testGps').onclick=()=>v12HardwareTest('gps');
 $('testAllSensors').onclick=()=>v12HardwareTest('all');
 $('groundtest').onclick=v12GroundTest;
 $('sdtest').onclick=v12SdTest;
 $('installFirmware').onclick=v12InstallFirmware;
 $('rollbackFirmware').onclick=v12RollbackFirmware;
 $('exitMaintenance').onclick=v12ExitMaintenance;
 $('rocketPreset').onchange=()=>{$("rocketCustom").disabled=$("rocketPreset").value!=="Custom";};
 $('saveFlightProfile').onclick=v12SaveFlightProfile;
 $('downloadPreflight').disabled=false;
 $('downloadPreflight').onclick=v12SavePreflightReport;
 $('refresh').onclick=v12LoadFiles;
 $('fileSort').onchange=v12LoadFiles;
 $('savedGraphMetric').onchange=drawSavedGraph;
 $('exportFlightPDF').onclick=exportFlightPDF;
 $('openFlightStats').onclick=()=>openTab('storage',document.querySelector('[data-tab="storage"]'));
 $('metric').onclick=()=>v12SetUnits(false);$('imperial').onclick=()=>v12SetUnits(true);
 $('sunlightMode').onclick=()=>setSunlight(!sunlight);
 $('pause').onclick=()=>{paused=!paused;$('pause').textContent=paused?'Resume graphs':'Pause graphs';};
 $('clear').onclick=clearHistory;$('graphWindow').onchange=drawAll;
 if('serviceWorker' in navigator)navigator.serviceWorker.register('/service-worker.js').catch(()=>{});
}

async function v12Fetch(path,method='GET',body=null){
 const options={method,cache:'no-store'};
 if(body){options.headers={'Content-Type':'application/x-www-form-urlencoded'};options.body=new URLSearchParams(body);}
 const response=await fetch('/api/'+path,options);
 const text=await response.text();let data={};try{data=JSON.parse(text);}catch(e){data={message:text};}
 if(!response.ok||data.success===false)throw Error(data.message||data.error||('HTTP '+response.status));
 return data;
}

function v12Render(d){
 state=d;connectedOnce=true;
 v12Set('connection',d.v13_source==='lora'?'● LORA TRACKING':d.v13_source==='demo'?'● DEMO DATA':'● V14 WI-FI ONLINE');$('connection').style.color=d.v13_source==='lora'?'#ffdd88':'#72e5ab';
 const altitude=d.relative_altitude_m,pressure=d.pressure_pa,battery=Number(d.battery_v);
 const batteryPct=Math.max(0,Math.min(100,(battery-3.3)/(4.2-3.3)*100));
 const v13RadioPhases=['BOOT','IDLE','ARMED','ASCENT','COAST','DESCENT','LANDED','RECOVERY'];
 const phase=(d.v13_source==='lora'||d.v13_source==='demo')?'LoRa: '+(v13RadioPhases[Number(d.flight_state)]||'UNKNOWN'):(v12FlightNames[Number(d.recorder_mode)]||'UNKNOWN');
  v12Set('altitude',v12Number(imperial&&v12Finite(altitude)?Number(altitude)*3.28084:altitude,2)+(imperial?' ft':' m'));v12Set('altitudeNote','Barometric altitude from calibrated BMP388');
  v12Set('pressure',v12Number(imperial&&v12Finite(pressure)?Number(pressure)/3386.389:pressure,imperial?3:0)+(imperial?' inHg':' Pa'));v12Set('psi',v12Finite(pressure)?(Number(pressure)/6894.757).toFixed(3)+' PSI absolute':'--');
  v12Set('temperature',v12Number(imperial&&v12Finite(d.imu_temperature_c)?Number(d.imu_temperature_c)*9/5+32:d.imu_temperature_c,1)+(imperial?' °F IMU':' °C IMU'));
 v12Set('batteryPercent',batteryPct.toFixed(0)+'%');v12Set('batteryVoltage',v12Number(battery,3)+' V');
  v12Set('speed',v12Number(imperial&&v12Finite(d.vertical_speed_mps)?Number(d.vertical_speed_mps)*3.28084:d.vertical_speed_mps,2)+(imperial?' ft/s':' m/s'));v12Set('peak',v12Number(imperial&&v12Finite(d.landing_peak_m)?Number(d.landing_peak_m)*3.28084:d.landing_peak_m,1)+(imperial?' ft':' m'));
 v12Set('flightState',phase);v12Set('flightPhase',phase);v12Set('missionIdentity',(d.selected_rocket||'V14 recorder')+(d.selected_motor?' · '+d.selected_motor:''));
 v12Set('prebufferStatus',String(d.prebuffer_records||0)+' buffered records');
 v12Set('prepareStatus',d.home_test_active?'Home verification: launch watching is OFF until end and recalibration.':d.maintenance_mode?'Firmware maintenance: launch watching is disabled until exit and calibration.':d.recording?'Flight logging is active. Dashboard loss cannot stop recording.':'V14 calibrates and watches for launch automatically after boot.');
 v12Set('feedback',d.v13_source==='lora'?'Received LoRa telemetry only; SD recorder status is unknown until Wi-Fi returns.':d.v13_source==='demo'?'SIMULATED telemetry; not real hardware.':d.recording?'Recording '+(d.log_file||'RKT4 file'):'Recorder is '+phase.toLowerCase()+'.');
 v12Set('preflightScore',d.preflight_state||'CHECK');
 if(d.home_interrupted)v12Set('homeInstructions','Previous home session was interrupted. Start a fresh session to clean temporary data and recheck.');
 v12Set('preflightDetail',v12PreflightDetails(d));
 v12UpdateProfileUi(d);
 v12Set('overviewSD',d.sd_ok?'MOUNTED':'UNAVAILABLE');v12Set('overviewSamples',String(d.prebuffer_records||0)+' prebuffered');v12Set('overviewFile',d.log_file||'Next V12D###.BIN');
 v12Set('recovery',d.recovery_status||'Not scanned');v12Set('missed',String(d.missed_sample_periods||0));v12Set('metadata',d.metadata_file||'--');
 v12Set('fieldBattery',batteryPct.toFixed(0)+'% · '+v12Number(battery,3)+' V');v12Set('fieldReadiness',d.recording?'RECORDING — SD has priority over dashboard':'AUTONOMOUS '+phase);
 v12Set('sdMounted',d.sd_ok?'YES':'NO');v12Set('sdReady',d.sd_ok?'READY':'NOT READY');v12Set('sdStatus',d.sd_ok?'SD mounted; recording path available.':'SD unavailable; inspect card and wiring.');
 v12Set('storageWriteErrors',String((d.checkpoint_failures||0)+(d.metadata_failures||0)+(d.finalization_failures||0)));
 v12Set('integrityStatus',d.recovery_status||'Not scanned');v12Set('recoveredFlights',String(d.recovered_logs||0));v12Set('recoveredSamples',String(d.recovered_records||0));
 v12Set('storageMissed',String(d.missed_sample_periods||0));v12Set('flushHeartbeat',d.recording?'ACTIVE':'IDLE');v12Set('maximumFlush','Bounded periodic flush');
 v12Set('firmwareBuild','V14 · '+(d.firmware_build||'unknown build'));v12Set('recordingLimitStatus','Automatic landing / firmware policy');
 v12Set('maintenanceStatus',d.maintenance_mode?'MAINTENANCE ACTIVE: launch detection OFF. Reboot or exit and recalibrate before flight.':d.ota_available?'OTA slot available; enter maintenance from this dashboard to install or roll back.':'OTA partition unavailable. Use USB to install firmware with OTA partitions.');
 $('exitMaintenance').disabled=!d.maintenance_mode;
 $('installFirmware').disabled=!!d.recording||!!d.update_in_progress||!d.ota_available||!!d.maintenance_mode||!!d.home_test_active;
 $('rollbackFirmware').disabled=!!d.recording||!!d.update_in_progress||!d.ota_available||!!d.maintenance_mode||!!d.home_test_active;
 v12Set('samples',String(d.prebuffer_records||0));v12Set('sampleRateStatus',String(d.sample_rate_hz||50)+' Hz configured');v12Set('sampleErrors',String(d.missed_sample_periods||0));
 v12Set('sdErrors',String((d.checkpoint_failures||0)+(d.sensor_log_failures||0)));v12Set('duration',d.recording?'Recording':'--');v12Set('filename',d.log_file||'--');
 v12Set('savedFlightLabel',d.selected_rocket||'Unknown');v12Set('savedMotorLabel',d.selected_motor||'Not selected');v12Set('minimumPressure',v12Number(pressure,0)+' Pa');
 v12Set('temperatureRange',v12Number(d.imu_temperature_c,1)+' °C current');v12Set('launched',Number(d.flight_state)>0?'YES / WATCHING':'NO');v12Set('finish',phase);
 v12Set('uptime',String(d.uptime_s||0)+' s');v12Set('error',Number(d.health)?'Health flags '+d.health:'None');
 v12Set('sysPct',batteryPct.toFixed(0)+'%');v12Set('sysV',v12Number(battery,3)+' V');v12Set('sysWarning',battery<3.5?'LOW':'NORMAL');
 v12Set('freshness',d.imu_ok&&d.barometer_ready?'LIVE':'CHECK SENSOR');v12Set('reads','Runtime loss '+String(d.barometer_runtime_losses||0));
 v12Set('sensorFrozen',d.barometer_ready?'NOT DETECTED':'UNAVAILABLE');v12Set('calCount',v12Finite(d.calibration_baseline_pa)?'COMPLETE':'IN PROGRESS');
 v12Set('baseline',v12Number(d.calibration_baseline_pa,1)+' Pa');v12Set('calResult',phase);v12Set('calNoise',v12Number(d.calibration_noise_m,3)+' m');
 v12Set('calDrift','Monitored by launch watcher');v12Set('calAge','Current boot');
 v12Set('imuTestResult',String(d.imu_test_result||'NOT RUN').toUpperCase());v12Set('imuTestMessage',d.imu_test_message||'No result');
 v12Set('imuAxes',[d.acceleration_x_g,d.acceleration_y_g,d.acceleration_z_g].map(v=>v12Number(v,3)).join(' / ')+' g');
 v12Set('imuTotal',v12Number(d.acceleration_g,3)+' g');v12Set('gyroAxes',[d.rotation_x_dps,d.rotation_y_dps,d.rotation_z_dps].map(v=>v12Number(v,2)).join(' / ')+' °/s');
 v12Set('gyroBias',[d.imu_test_bias_x_dps,d.imu_test_bias_y_dps,d.imu_test_bias_z_dps].map(v=>v12Number(v,3)).join(' / ')+' °/s');
 v12Set('imuTempRange',v12Number(d.imu_test_temperature_c??d.imu_temperature_c,1)+' °C · ±32 g '+(d.imu_range_32g?'verified':'not verified'));
 v12Set('gpsTestResult',String(d.gps_test_result||'NOT RUN').toUpperCase());v12Set('gpsTestMessage',d.gps_test_message||'No result');
 v12Set('gpsCommunication',(d.gps_communication||'waiting')+' · '+String(d.gps_nmea_sentences||0)+' NMEA');
 v12Set('gpsFix',(d.gps_status||'missing')+' · '+String(d.satellites||0)+' satellites · '+String(d.gps_fix_age_ms||0)+' ms');
 v12Set('gpsCoordinates',d.latitude==null?'No position':Number(d.latitude).toFixed(7)+', '+Number(d.longitude).toFixed(7));
 v12Set('gpsMotion',v12Number(d.gps_altitude_m,1)+' m · '+v12Number(d.gps_speed_mps,2)+' m/s · '+String(d.gps_utc_hour||0).padStart(2,'0')+':'+String(d.gps_utc_minute||0).padStart(2,'0')+':'+String(d.gps_utc_second||0).padStart(2,'0')+' UTC');
 ['testImu','testGps','testAllSensors'].forEach(id=>$(id).disabled=!d.hardware_tests_allowed);
  if(!paused){if(v12Finite(altitude))append('altitude',Number(altitude));if(v12Finite(pressure))append('pressure',Number(pressure));if(v12Finite(d.imu_temperature_c))append('temperature',Number(d.imu_temperature_c));drawAll();}
}

async function v12Poll(){if(v12Busy)return;try{v12Render(await v12Fetch('status'));}catch(error){v12Set('connection','● OFFLINE');$('connection').style.color='#ff91a0';v12Set('feedback','Dashboard offline; autonomous recording continues on the Feather.');}}
function v12SetUnits(useImperial){imperial=useImperial;$('metric').classList.toggle('active',!useImperial);$('imperial').classList.toggle('active',useImperial);if(state)v12Render(state);drawAll();drawSavedGraph();}
async function v12Calibrate(){try{notice((await v12Fetch('calibrate','POST')).message,true);}catch(error){notice(error.message,false);}finally{v12Poll();}}
async function v12Stop(){if(!confirm('Stop, flush and finalize the active V12 recording?'))return;try{notice((await v12Fetch('stop','POST')).message,true);}catch(error){notice(error.message,false);}finally{v12Poll();}}
async function v12HardwareTest(sensor){try{const result=await v12Fetch('hardware-test','POST',{sensor});notice(result.accepted?'Hardware test started. Keep the board stationary for the IMU bias test.':'Test not accepted',!!result.accepted);}catch(error){notice(error.message,false);}finally{v12Poll();}}
async function v12SdTest(){try{v12Set('sdTestFeedback','Running real write/read/delete verification...');const result=await v12Fetch('sdtest','POST');v12Set('sdTestFeedback',result.message);v12Set('sdTestResult','PASS');await v12LoadFiles();}catch(error){v12Set('sdTestFeedback',error.message);v12Set('sdTestResult','FAIL');}}
async function v12GroundTest(){
 if(state?.recording||state?.maintenance_mode){v12Set('groundTestResult','BLOCKED');v12Set('groundTestDetail','Cannot test while recording or in firmware maintenance.');return;}
 $('groundtest').disabled=true;
 v12Set('groundTestResult','RUNNING');
 v12Set('groundTestDetail','Testing IMU (5 s), GPS (12 s), and verifying the SD card. Keep Feather stationary; outdoor GPS fix may take longer.');
 try{
  await v12Fetch('hardware-test','POST',{sensor:'all'});
  const sd=await v12Fetch('sdtest','POST');
  await new Promise(resolve=>setTimeout(resolve,13500));
  const d=await v12Fetch('status');
  const results=[String(d.imu_test_result||'NOT RUN').toUpperCase(),String(d.gps_test_result||'NOT RUN').toUpperCase()];
  const fail=results.some(x=>x==='FAIL');
  const pass=results.every(x=>x==='PASS')&&sd.success;
  v12Set('groundTestResult',fail?'FAIL':pass?'PASS':'INCONCLUSIVE');
  v12Set('groundTestDetail','IMU: '+results[0]+' · GPS: '+results[1]+' · SD: '+(sd.success?'PASS':'FAIL')+'. This check does not verify radio range, power interruption, vibration, or flight readiness.');
 }catch(error){
  v12Set('groundTestResult','INCONCLUSIVE');
  v12Set('groundTestDetail',error.message);
 }finally{$('groundtest').disabled=false;v12Poll();}
}


let v12MaintenanceToken='';
async function v12EnterMaintenance() {
 const result=await v12Fetch('maintenance','POST');
 v12MaintenanceToken=result.token;
 v12Set('maintenanceStatus',result.message);
 return v12MaintenanceToken;
}
async function v12ExitMaintenance() {
 if(!v12MaintenanceToken) {
   notice('Maintenance token lost after a reload. Restart the Feather to leave maintenance and recalibrate.',false);
   return;
 }
 try{
   const result=await v12Fetch('maintenance/exit','POST',{token:v12MaintenanceToken});
   v12MaintenanceToken='';
   v12Set('updateFeedback',result.message);
   notice(result.message,true);
   await v12Poll();
 }catch(error){notice(error.message,false);}
}
async function v12RollbackFirmware() {
 if(!confirm('ROLLBACK ON BENCH ONLY: finish all flights, keep USB power connected, and disable launch detection to select the previous valid OTA image. Continue?'))return;
 try{
   const token=await v12EnterMaintenance();
   const result=await v12Fetch('rollback','POST',{token});
   v12Set('updateFeedback',result.message+' Reconnect after reboot, then rerun hardware checks.');
   v12Set('connection','RESTARTING');
 }catch(error){
   v12Set('updateFeedback','Rollback not completed: '+error.message+'. If maintenance is active, exit maintenance before flight.');
   notice(error.message,false);
 }
}
async function v12InstallFirmware() {
 const file=$('firmwareFile').files[0];
 if(!file||!file.name.toLowerCase().endsWith('.ino.bin')){
   v12Set('updateFeedback','Choose the main Feather file ending in .ino.bin from a successful build.');
   return;
 }
 if(!confirm('INSTALL ON BENCH ONLY: no flight may be underway. Keep charged LiPo and USB power connected. Launch watching turns OFF until reboot. Install '+file.name+'?'))return;
 v12Busy=true;
 $('firmwareProgress').style.width='0%';
 v12Set('firmwarePercent','0%');
 try{
   const token=await v12EnterMaintenance();
   const form=new FormData();
   form.append('firmware',file,file.name);
   const result=await new Promise((resolve,reject)=>{
     const request=new XMLHttpRequest();
     request.open('POST','/api/update?token='+encodeURIComponent(token));
     request.timeout=120000;
     request.upload.onprogress=event=>{
       if(!event.lengthComputable)return;
       const pct=Math.min(100,Math.round(event.loaded/event.total*100));
       $('firmwareProgress').style.width=pct+'%';
       v12Set('firmwarePercent',pct+'%');
       v12Set('updateFeedback','Uploading '+file.name+' · '+pct+'%. Do not unplug power.');
     };
     request.onerror=()=>reject(Error('Connection lost during firmware upload; inspect current firmware before retrying'));
     request.ontimeout=()=>reject(Error('Upload timed out; inspect Feather Serial Monitor before retrying'));
     request.onload=()=>{
       let data;
       try{data=JSON.parse(request.responseText);}catch{reject(Error('Invalid server response: '+request.status));return;}
       if(request.status<200||request.status>=300||!data.success)reject(Error(data.message||'Firmware rejected'));
       else resolve(data);
     };
     request.send(form);
   });
   $('firmwareProgress').style.width='100%';
   v12Set('firmwarePercent','100%');
   v12Set('updateFeedback',result.message+'. Wait for GREEN-ION-V14 Wi-Fi, verify build, then rerun all bench tests.');
   v12Set('connection','RESTARTING');
 }catch(error){
   v12Set('updateFeedback','Update failed: '+error.message+'. Launch detection remains OFF in maintenance; exit or restart before flight.');
   notice(error.message,false);
 }finally{v12Busy=false;}
}

async function v12ShowFlight(name){selectedFlight=name;flightMeta={file:name};v12Set('flightSummaryStatus','Loading and validating '+name+'...');$('flightSummaryDetails').replaceChildren();await loadSavedGraph(name);if(loadedFlightName===name){const valid=savedFlightSeries.filter(r=>r[4]===1),peak=valid.length?Math.max(...valid.map(r=>r[5]).filter(Number.isFinite)):NaN,duration=savedFlightSeries.length?savedFlightSeries[savedFlightSeries.length-1][0]:0;v12Set('flightSummaryStatus',name+' · RKT4 v2 decoded in this browser');for(const [label,value] of [['Samples',savedFlightSeries.length],['Valid samples',valid.length],['Duration',duration.toFixed(2)+' s'],['Peak barometric altitude',v12Number(peak,2)+' m']]){const row=document.createElement('div');row.className='row';row.innerHTML='<span>'+label+'</span><strong>'+value+'</strong>';$('flightSummaryDetails').appendChild(row);}}}
async function v12LoadFiles(){try{const result=await v12Fetch('files');v12Set('storageUsed',bytes(result.used)+' / '+bytes(result.total));v12Set('sdFreeSpace',bytes(Math.max(0,result.total-result.used)));v12Set('fileCount',String(result.files.length));const list=$('fileList');list.replaceChildren();let files=result.files.slice();const sort=$('fileSort').value;files.sort((a,b)=>sort==='old'?a.name.localeCompare(b.name):sort==='size'?b.size-a.size:b.name.localeCompare(a.name));if(!files.length){list.textContent='No completed V12 RKT4 recordings yet.';return;}for(const file of files){const row=document.createElement('div');row.className='file';const label=document.createElement('span');label.textContent=file.name+' · '+bytes(file.size);const actions=document.createElement('span');const view=document.createElement('button');view.textContent='VIEW / GRAPH';view.onclick=()=>v12ShowFlight(file.name);const bin=document.createElement('button');bin.textContent='BIN';bin.onclick=()=>location.href='/api/download?name='+encodeURIComponent(file.name);const csv=document.createElement('button');csv.textContent='CSV';csv.onclick=async()=>{await v12ShowFlight(file.name);if(loadedFlightName===file.name)downloadRowsCsv(file.name,savedFlightSeries);};const sen=document.createElement('button');sen.textContent='SEN';sen.title='Full-resolution IMU/GPS companion log';sen.onclick=()=>location.href='/api/sensors?name='+encodeURIComponent(file.name);const txt=document.createElement('button');txt.textContent='TXT';txt.onclick=()=>location.href='/api/metadata?name='+encodeURIComponent(file.name);actions.append(view,bin,sen,csv,txt);row.append(label,actions);list.appendChild(row);}if(!selectedFlight)await v12ShowFlight(files[0].name);}catch(error){v12Set('fileList','Cannot list recordings: '+error.message);}}

// The historical rocket/motor identity belongs to each downloaded TXT sidecar,
// not to whichever rocket is currently selected on the Feather.
const v12OriginalShowFlight=v12ShowFlight;
v12ShowFlight=async function(name){
 await v12OriginalShowFlight(name);
 try{
  const response=await fetch("/api/metadata?name="+encodeURIComponent(name),{cache:"no-store"});
  if(!response.ok)return;
  const metadata=await response.text();
  const entries={};
  for(const line of metadata.split(/\r?\n/)){
   const equals=line.indexOf("=");
   if(equals>0)entries[line.slice(0,equals).trim()]=line.slice(equals+1).trim();
  }
  v12Set("savedFlightLabel",entries.rocket||"Rocket not recorded in this flight");
  v12Set("savedMotorLabel",entries.motor||"Motor not recorded in this flight");
 }catch(error){
  v12Set("savedFlightLabel","Metadata unavailable");
 }
};
loadFiles=v12LoadFiles;
v12ConfigureUi();
notice('V11 blue dashboard restored with live V14 recorder integration. Connecting...');
setInterval(v12Poll,1200);v12Poll();
</script>

<script>
/* GREEN ION V14 guided home verification; actual evidence, no simulated rocket flight. */
(function(){
"use strict";
const node=id=>document.getElementById(id);
const steps=[
 {title:"1. Inspect wiring and safe setup",guide:"Remove all rocket motors. Check battery polarity, solder joints, insulated wires, secured SD card, mounting and antennas. Place the payload on a stable bench.",confirm:"I inspected the wiring, battery and both radio antennas.",kind:"manual"},
 {title:"2. Battery, barometer and calibration",guide:"Keep the Feather still. Press Run to check real measured battery voltage, BMP388 pressure and stationary calibration baseline.",kind:"power"},
 {title:"3. IMU acceleration and gyro test",guide:"Press Run. Keep the Feather motionless for five seconds while it measures the ±32 g IMU and gyro bias.",kind:"imu"},
 {title:"4. GPS communication and current fix",guide:"Take GPS outdoors with a clear sky. Press Run for the 12-second GPS test. Communication without a fresh fix is INCONCLUSIVE, not a passing GPS position.",kind:"gps"},
 {title:"5. Temporary SD read/write/cleanup",guide:"Press Run to write, read, compare and delete a test pattern. Existing real flight recordings are left alone.",kind:"sd"},
 {title:"6. LoRa transmitter and Uno receiver",guide:"Connect both 900/915 MHz antennas, Uno USB and Windows ground station. Press Run to transmit five bench packets. Confirm five valid packets arrived at the actual Uno.",kind:"radio",confirm:"I saw five valid bench packets arrive on my Uno/Windows receiver."},
 {title:"7. LiPo operation without USB",guide:"With a correctly wired protected LiPo, briefly unplug USB without disconnecting the battery or moving wiring. Verify the Feather remains powered, then reconnect USB.",kind:"manual",confirm:"Feather stayed running on LiPo and USB is safely reconnected."},
 {title:"8. Temporary 50 Hz recording + Wi-Fi disconnect",guide:"Press Run for 30 seconds of real pressure samples. Briefly disconnect PHONE Wi-Fi for at least five seconds, rejoin GREEN-ION-V14 and watch the result. Do not remove Feather power. The test verifies the RKT4 data and deletes /GIHOME.BIN.",kind:"record"},
 {title:"9. Final physical readiness review",guide:"Check the payload's secure mount, balance and recovery clearance for Green Eggs and So Long separately. RF range, hardware power-loss recovery, OTA/rollback, launch and landing detection, and flight conditions are NOT tested by this routine.",kind:"manual",confirm:"I reviewed remaining physical and range tests; I understand bench PASS is not flight approval."}
];
let active=false,step=-1,busy=false,pending="",results={},evidence=[],radioStarted=false,message="";
const here=()=>steps[step];
const setMessage=text=>{message=text;node("homeInstructions").textContent=text;};
async function request(path,method="GET"){
 const r=await fetch("/api/"+path,{method,cache:"no-store"});
 let data;try{data=await r.json();}catch{throw Error("Invalid response or connection lost");}
 if(!r.ok||data.success===false)throw Error(data.message||data.error||("HTTP "+r.status));
 return data;
}
function paint(){
 const item=here();
 node("homeStart").disabled=active||busy;
 node("homeRun").disabled=!active||busy||!item||pending==="imu"||pending==="gps"||pending==="record";
 node("homeNext").disabled=!active||busy||!Object.prototype.hasOwnProperty.call(results,step);
 node("homeSkip").disabled=!active||busy||!item||pending==="record";
 node("homeFinish").disabled=!active;
 node("homeReport").disabled=evidence.length===0;
 node("homeConfirmLabel").style.display=active&&item&&item.confirm?"block":"none";
 if(item&&item.confirm)node("homeConfirmText").textContent=item.confirm;
 node("homeStepCount").textContent=active?(step+1)+" of "+steps.length:(evidence.length?"Session ended":"Not started");
 node("homeStepTitle").textContent=item?item.title:(evidence.length?"Home test ended":"Ready to start");
 node("homeProgressFill").style.width=(active?Math.floor((step+1)*100/steps.length):(evidence.length?100:0))+"%";
 node("homeStepStatus").textContent=active?(results[step]||(busy?"RUNNING":pending==="radio"?"WAITING FOR RECEIVER":"NOT RUN")):"IDLE";
 const vals=Object.values(results);
 node("homeSummary").textContent=vals.filter(x=>x==="PASS").length+" measured PASS · "+
    vals.filter(x=>x==="CONFIRMED").length+" manually confirmed · "+
    vals.filter(x=>x==="FAIL").length+" FAIL · "+
    vals.filter(x=>x==="INCONCLUSIVE"||x==="NOT VERIFIED").length+
    " inconclusive/not verified. Flight readiness is NEVER automatically approved.";
}
async function save(result,note,data={}){
 if(step<0||!active)return;
 busy=true;pending="saving";paint();
 const action=result==="NOT VERIFIED"?"skip":result==="CONFIRMED"?"confirm":"measure";
 try {
  const response=await fetch("/api/home/step",{method:"POST",cache:"no-store",
    headers:{"Content-Type":"application/x-www-form-urlencoded"},
    body:new URLSearchParams({step:String(step+1),action})});
  const ack=await response.json();
  if(!response.ok||!ack.success)throw Error(ack.message||"Device refused result");
  result=String(ack.result);
  note+=" Feather evidence: "+result+".";
  data.device_verified=result==="PASS";
  data.device_result=result;
 } catch(error) {
  result="INCONCLUSIVE";
  note+=" Device could not persist this step ("+error.message+"). Re-run before relying on the result.";
  data.device_verified=false;
 }
 results[step]=result;
 evidence.push({step:step+1,title:here().title,result,note,details:data,checked_at:new Date().toISOString()});
 busy=false;pending="";
 setMessage(note+" Choose NEXT STEP, or RUN again to retry.");paint();
}
async function start(){
 busy=true;paint();
 try{
  const r=await request("home/start","POST");
  active=true;step=0;results={};evidence=[];pending="";radioStarted=false;
  node("homeConfirm").checked=false;
  setMessage(r.message+". "+here().guide);
 }catch(err){setMessage("Could not start: "+err.message);}
 busy=false;paint();
}
async function run(){
 if(!active||busy||!here())return;
 const item=here(),kind=item.kind;
 if(item.confirm&&!node("homeConfirm").checked){
  setMessage("Complete the instructed physical check and tick the confirmation box first.");return;
 }
 busy=true;paint();
 try{
  if(kind==="manual"){
   save("CONFIRMED","Manual inspection confirmed by user; not electronically measured.");return;
  }
  if(kind==="power"){
   const s=await request("status");
   const v=Number(s.battery_v),pressure=Number(s.pressure_pa),baseline=Number(s.calibration_baseline_pa);
   const ok=Number.isFinite(v)&&v>=3.5&&Number.isFinite(pressure)&&pressure>=30000&&pressure<=120000&&
     Number.isFinite(baseline)&&baseline>=30000&&baseline<=120000&&s.barometer_ready;
   save(ok?"PASS":"FAIL","Measured battery "+v.toFixed(2)+" V, pressure "+pressure.toFixed(0)+
     " Pa, calibration baseline "+(Number.isFinite(baseline)?baseline.toFixed(0):"missing")+" Pa.",
     {battery_v:v,pressure_pa:pressure,calibration_baseline_pa:baseline});
   return;
  }
  if(kind==="sd"){
   const r=await request("sdtest","POST");
   save("PASS",r.message);return;
  }
  if(kind==="imu"||kind==="gps"){
   const r=await fetch("/api/hardware-test",{method:"POST",cache:"no-store",
     headers:{"Content-Type":"application/x-www-form-urlencoded"},body:"sensor="+kind});
   const data=await r.json();
   if(!r.ok||!data.accepted)throw Error(data.error||"Hardware test rejected");
   pending=kind;setMessage("Collecting actual "+kind.toUpperCase()+" data. "+item.guide);
   return;
  }
  if(kind==="radio"){
   if(!radioStarted){
    await request("home/radio","POST");radioStarted=true;pending="radio";
    setMessage("Five bench packets queued. Watch the Uno receiver. After reception, tick confirmation and press RUN again.");
    return;
   }
   const s=await request("status");
   if(Number(s.home_radio_sent)<5||Number(s.home_radio_remaining)>0){
    busy=false;pending="radio";
    setMessage("Only "+s.home_radio_sent+" packets sent; wait until all five have transmitted.");paint();return;
   }
   const errors=Number(s.home_radio_failures);
   save(errors?"INCONCLUSIVE":"CONFIRMED",
     "Feather transmitted "+s.home_radio_sent+" packets; user confirmed actual Uno reception. TX errors: "+errors+
     ". Radio range has NOT been validated.",
     {feather_tx:s.home_radio_sent,tx_errors:errors,uno_confirmed:true});
   return;
  }
  if(kind==="record"){
   const r=await request("home/record","POST");pending="record";
   setMessage(r.message+". Briefly disconnect and reconnect your phone Wi-Fi. The Feather keeps writing independently.");
   return;
  }
 }catch(err){save("FAIL","Test failed: "+err.message);}
 finally{if(pending&&pending!=="saving"){busy=false;paint();}}
}
async function poll(){
 if(!active||!pending)return;
 try{
  const s=await request("status");
  if(!s.home_test_active){
   active=false;pending="";busy=false;
   setMessage("Home session ended unexpectedly. Check calibration and re-run a fresh test.");
   paint();return;
  }
  if(pending==="imu"||pending==="gps"){
   const kind=pending,res=String(s[kind+"_test_result"]||"IDLE").toUpperCase();
   if(["PASS","FAIL","INCONCLUSIVE","DEFERRED"].includes(res)){
    save(res==="DEFERRED"?"INCONCLUSIVE":res,kind.toUpperCase()+" "+res+
       ": "+(s[kind+"_test_message"]||"Completed"),{result:res,sensor_present:s[kind+"_ok"]});
   }
  }else if(pending==="record"){
   const result=String(s.home_record_result||"IDLE").toUpperCase();
   if(result==="PASS"||result==="FAIL"){
    save(result,(result==="PASS"?"RKT4 recording verified and deleted. ":"Temporary RKT4 verification FAILED. ")+
      "Samples: "+s.home_record_samples+", valid: "+s.home_record_valid_samples+
      ", missed periods: "+s.home_record_missed+", Wi-Fi absence: "+
      (s.home_record_wifi_absent?"OBSERVED":"NOT OBSERVED (rerun for Wi-Fi check)")+
      ", temporary file removed: "+(s.home_record_cleanup_ok?"YES":"NO"),
      {samples:s.home_record_samples,valid:s.home_record_valid_samples,missed:s.home_record_missed,
       wifi_absence_observed:s.home_record_wifi_absent,deleted:s.home_record_cleanup_ok});
   }else setMessage("Recording "+(s.home_record_elapsed_s||0)+"/30 seconds; "+
     (s.home_record_samples||0)+" sensor records. Disconnect and reconnect PHONE Wi-Fi, not Feather power.");
  }else if(pending==="radio"){
   setMessage("Radio bench TX: "+s.home_radio_sent+" / 5 packets; remaining "+s.home_radio_remaining+
     ". Confirm five valid packets arrived at the UNO, then press RUN.");
  }
 }catch(e){
  setMessage("Dashboard connection interrupted. Reconnect to GREEN-ION-V14 Wi-Fi. The Feather's temporary recording continues without the phone.");
 }
}
function next(){
 if(!Object.prototype.hasOwnProperty.call(results,step))return;
 if(step===steps.length-1){
  setMessage("All steps reviewed. Press END & DELETE TEMP DATA, then wait for automatic stationary calibration.");
  node("homeNext").disabled=true;return;
 }
 step++;pending="";radioStarted=false;node("homeConfirm").checked=false;setMessage(here().guide);paint();
}
function skip(){if(!active||busy||pending==="record")return;save("NOT VERIFIED","This check was skipped. It has not passed.");}
async function finish(){
 if(!active)return;
 busy=true;paint();
 try{
  const r=await request("home/finish","POST");
  active=false;step=-1;pending="";
  evidence.push({event:"session ended",cleanup:r.message,at:new Date().toISOString()});
  setMessage(r.message+". Hold Feather still while recalibrating. Verify WATCHING FOR LAUNCH before flight.");
 }catch(e){
  active=false;step=-1;pending="";
  evidence.push({event:"cleanup not verified",error:e.message});
  setMessage("Cleanup could not be confirmed: "+e.message+". Inspect GIHOME.BIN manually. Do not launch until resolved.");
 }
 busy=false;paint();
}
function report(){
 const payload={project:"GREEN ION V14",test:"guided bench verification",flight_ready:false,
  limits:"No real launch/landing, RF range, vibration, OTA rollback, power interruption or mechanical recovery verification.",
  created:new Date().toISOString(),checks:evidence};
 const blob=new Blob([JSON.stringify(payload,null,2)],{type:"application/json"});
 const url=URL.createObjectURL(blob),a=document.createElement("a");
 a.href=url;a.download="green-ion-home-verification.json";a.click();
 setTimeout(()=>URL.revokeObjectURL(url),1000);
}
node("homeStart").onclick=start;
node("homeRun").onclick=run;
node("homeNext").onclick=next;
node("homeSkip").onclick=skip;
node("homeFinish").onclick=finish;
node("homeReport").onclick=report;
// Restore only results persisted and classified by the Feather. Following an
// actual Feather reboot, the session is not resumed; user starts a new one.
request("status").then(s=>{
 if(active||busy)return;
 if(s.home_test_active){
  active=true;results={};evidence=[];
  const labels=["NOT RUN","PASS","FAIL","INCONCLUSIVE","CONFIRMED","NOT VERIFIED"];
  const saved=String(s.home_steps||"").split("");
  saved.slice(0,steps.length).forEach((digit,i)=>{
   const code=Number(digit);
   if(code>=1&&code<labels.length){
    results[i]=labels[code];
    evidence.push({step:i+1,title:steps[i].title,result:labels[code],
      restored:true,note:"Restored from Feather NVS; detailed browser measurements were not retained."});
   }
  });
  const missing=steps.findIndex((item,i)=>!Object.prototype.hasOwnProperty.call(results,i));
  step=missing>=0?missing:steps.length-1;
  if(String(s.home_record_result)==="RUNNING"){step=7;pending="record";}
  if(step===5&&Number(s.home_radio_remaining)>0){pending="radio";radioStarted=true;}
  setMessage("Reconnected to a Feather home session. Saved step result codes were recovered; "+
   "missing checks must be repeated. "+(pending==="record"?
    "The temporary SD recording continues. Reconnect and check its result.":here().guide));
  paint();
 }else if(s.home_interrupted) {
  setMessage("Previous Feather home verification was interrupted by restart/power loss. "+
    "Start a NEW safe bench session to clean reserved data and repeat all measurements.");
 }
}).catch(()=>{});
setInterval(poll,1200);
paint();
})();
</script>
</body>
</html>
)HTML";
