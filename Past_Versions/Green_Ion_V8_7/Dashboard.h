#pragma once
// Served directly from flash. Standalone header avoids Arduino sketch preprocessing HTML/JS.
const char PAGE[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport"
 content="width=device-width, initial-scale=1">
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
@media(min-width:900px){#testing.page.active,#system.page.active,#storage.page.active{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:16px;align-items:start}#storage #flightSummaryCard{grid-column:1/-1}#storage .card,#testing .card,#system .card{margin-bottom:0}.metrics{grid-template-columns:repeat(3,minmax(0,1fr))}}
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
 <button class="active" data-tab="overview">Overview</button>
 <button data-tab="testing">Testing</button>
 <button data-tab="lights">Lights</button>
 <button data-tab="storage">Storage</button>
 <button data-tab="system">System</button>
</nav>

<section id="overview" class="page active">
<div class="card launch-guide" id="launchGuide">
 <div class="guide-top"><div><h2>Launch assistant</h2><p class="muted">Follow each stage from setup to flight report. This is a data logger, not an ignition controller.</p></div><strong id="guideStage">CONNECT</strong></div>
 <div class="progress" aria-hidden="true"><div id="guideProgress" style="width:0%"></div></div>
 <div class="guide-steps" id="guideSteps" aria-label="Launch workflow">
  <span>1 · Connect</span><span>2 · Prepare</span><span>3 · Calibrate</span><span>4 · Record</span><span>5 · Review</span>
 </div>
 <div class="guide-message" id="guideMessage" role="status">Connecting to the recorder...</div>
 <div class="guide-actions"><button id="guideNext" class="green">VIEW NEXT STEP</button><button id="guideStorage" class="secondary">SAVED FLIGHTS</button></div>
 <details class="checklist"><summary>Physical launch checklist</summary>
  <ul><li>Secure the Feather, BMP388, battery and SD wiring so connections cannot move during flight.</li>
  <li>Confirm the rocket and launch range are prepared according to the motor manufacturer's instructions and the applicable safety code.</li>
  <li>Check that the motor designation and sample rate are saved; test the SD card before arming.</li>
  <li>Calibrate with the rocket stationary on the pad; keep the pressure vent unobstructed.</li>
  <li>Start recording before launch. Do not depend on Wi-Fi remaining connected during flight.</li>
  <li>After recovery, stop and save if still recording; then verify the file and export the report.</li></ul>
  <p class="muted">The checklist is informational. GREEN-ION does not fire the motor or control deployment.</p>
 </details>
</div>

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

<div class="card">
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

<div class="card">
 <h2>Prepare and record</h2><p class="muted">Set the flight information, calibrate on the ground, then arm and start recording before launch.</p>
 <label for="missionKind">Recording type</label>
 <select id="missionKind"><option value="flight">Flight</option><option value="test">Test</option></select>
 <label for="flightLabel">Automatic label</label>
 <input id="flightLabel" type="text" readonly value="Flight 1">
 <label for="motorLabel">Motor designation</label>
 <input id="motorLabel" type="text" maxlength="24" list="motorList" placeholder="Search motor (or type your own)" autocomplete="off">
 <datalist id="motorList">
 <option value="A8-3"><option value="A8-5"><option value="B4-2"><option value="B4-4"><option value="B6-2"><option value="B6-4"><option value="B6-6"><option value="C6-3"><option value="C6-5"><option value="C6-7"><option value="D12-3"><option value="D12-5"><option value="D12-7"><option value="E12-4"><option value="E12-6"><option value="E9-4"><option value="F15-6">
 </datalist>
 <button id="saveMission" class="secondary"
  style="width:100%">SAVE FLIGHT DETAILS</button>
 <label for="sampleRate">SD samples per second</label>
 <select id="sampleRate">
  <option value="10">10 Hz · longest recordings</option>
  <option value="25" selected>25 Hz · recommended</option>
  <option value="50">50 Hz · fastest detail</option>
 </select>
 <button id="saveRate" class="secondary"
  style="width:100%">APPLY SAMPLE RATE</button>
 <div class="notice" id="settingsSaveState" aria-live="polite">Flight settings will appear here.</div>
 <p class="muted">Change the rate before arming. Higher rates
 capture fast motion with larger files and more SD writes.</p>
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
  <button id="calibrate">1 · CALIBRATE</button>
  <button id="arm" class="warning">2 · SAVE SETTINGS & ARM LOGGER</button>
  <button id="start" class="wide green">3 · START RECORDING</button>
  <button id="stop" class="danger">STOP &amp; SAVE</button>
  <button id="disconnect" class="secondary">DISCONNECT WI-FI</button>
 </div>
 <p class="muted">Calibration enables live altitude without
 requiring an SD card or starting a flight.</p>
</div>

<div class="card">
 <h2>Recorder health</h2>
 <div class="row"><span>Preflight readiness</span><strong id="preflightScore">--</strong></div>
 <div class="notice" id="preflightDetail">Waiting for checks.</div>
 <div class="row"><span>SD card</span><strong id="overviewSD">--</strong></div>
 <div class="row"><span>Recorded samples</span><strong id="overviewSamples">--</strong></div>
 <div class="row"><span>Current file</span><strong id="overviewFile">--</strong></div>
 <button id="openFlightStats" class="secondary" style="width:100%">OPEN SAVED FLIGHTS AND REPORTS</button>
 </div>

<div class="card">
 <h2>Live altitude history</h2>
 <canvas id="overviewGraph"></canvas>
</div>
</section>

<section id="testing" class="page">
 <div class="card">
  <h2>Sensor testing</h2>
  <button id="testCalibrate">CALIBRATE ALTITUDE</button>
  <div class="notice" id="testFeedback">Ready for testing.</div>
  <div class="actions">
   <button id="simulate">START SIMULATION</button>
   <button id="simstop" class="secondary">STOP SIMULATION</button>
  </div>
  <p class="muted">Simulation does not create a flight log.</p>
 </div>
 <div class="card">
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
 <div class="card"><h2>Altitude</h2>
  <canvas id="altGraph"></canvas></div>
 <div class="card"><h2>Pressure</h2>
  <canvas id="pressureGraph"></canvas></div>
 <div class="card"><h2>Temperature</h2>
  <canvas id="tempGraph"></canvas></div>
 <div class="card"><h2>Sensor diagnostics</h2>
  <div class="row"><span>Reading freshness</span>
   <strong id="freshness">--</strong></div>
  <div class="row"><span>Good / failed reads</span>
   <strong id="reads">--</strong></div>
  <div class="row"><span>Calibration count</span>
   <strong id="calCount">--</strong></div>
  <div class="row"><span>Calibration age</span>
   <strong id="calAge">--</strong></div>
  <div class="row"><span>Baseline pressure</span>
   <strong id="baseline">--</strong></div>
  <div class="row"><span>Calibration result</span>
   <strong id="calResult">--</strong></div>
 </div>
</section>

<section id="lights" class="page">
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
   <div class="dot" style="background:#a855f7"></div>
   <div>Purple blink — simulation</div>
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
  <button id="refresh" class="secondary">REFRESH FILES</button>
  <label for="fileSort">Sort recordings</label>
  <select id="fileSort"><option value="new">Newest first</option><option value="old">Oldest first</option><option value="size">Largest first</option></select>
  <div id="fileList"></div>
 </div>
 <div class="card" id="flightSummaryCard">
  <h2>Saved flight summary</h2>
  <p class="muted">Select View stats to read stored flight data directly. No download needed.</p>
  <div class="notice" id="flightSummaryStatus">Choose View stats on a saved flight.</div>
  <div id="flightSummaryDetails"></div>
  <h3>Battery / estimated acceleration</h3><div id="flightExtraMetrics" class="notice">Load a flight.</div><h3>Estimated flight phases</h3><div id="flightPhases" class="notice">Select a flight.</div><h3>Calculated flight performance</h3>
  <div id="flightPerformance" aria-live="polite">Select a recording to calculate performance.</div>
  <h3>Saved flight graph</h3>
  <label for="savedGraphMetric">Measurement</label>
  <select id="savedGraphMetric"><option value="5">Altitude (m)</option><option value="1">Pressure (Pa)</option><option value="2">Temperature (°C)</option><option value="6">Vertical speed (m/s)</option><option value="3">Battery (V)</option><option value="7">Estimated acceleration (m/s²)</option><option value="8">Estimated vertical apparent G</option></select>
  <canvas id="savedFlightGraph" style="width:100%;height:220px"></canvas>
  <div class="notice" id="savedGraphStatus">Select a flight to load its graph.</div><button id="exportFlightPDF" class="secondary" style="width:100%">PRINT / SAVE FLIGHT PDF</button>
 </div>
 <div class="card"><h2>Compare flights</h2>
  <p class="muted">Select two saved recordings to compare their summary statistics.</p>
  <select id="compareA"></select><select id="compareB"></select>
  <button id="compareFlights" class="secondary" style="width:100%">COMPARE FLIGHTS</button>
  <div id="comparisonResults" role="status"></div><button id="exportComparePDF" class="secondary" style="width:100%">PRINT / SAVE MOTOR COMPARISON PDF</button>
 </div>
</section>

<section id="system" class="page">
 <div class="card"><h2>Help and connection</h2>
  <details open><summary>Connect from a phone, iPad or computer</summary><p>Connect to <strong>GREEN-ION</strong> Wi-Fi and open <strong>http://192.168.4.1</strong>. This local Wi-Fi does not provide internet access; remain connected to use the recorder.</p></details>
  <details><summary>Buttons disabled or view-only?</summary><p>Use <strong>Take Control</strong> on Overview. Only one browser may change recorder settings at a time. Recording or arming temporarily disables some settings.</p></details>
  <details><summary>SD testing or download trouble?</summary><p>Finish recording and disarm, then run <strong>Detect &amp; Test SD Card</strong> under Storage. The check uses only a temporary file. Download saved recordings after logging ends.</p></details>
  <details><summary>Updating the firmware</summary><p>Install only the main <strong>.ino.bin</strong> file from a successful GitHub Actions build. Keep power connected, and perform a ground test after updating.</p></details>
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
  <div class="notice" id="updateFeedback">
   Updates are allowed only while disarmed and not recording.
  </div>
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

let guideTarget="takeControl";
let previousRecording=false;
let justFinishedRecording=false;
function jumpToControl(id,tab="overview"){
 const button=document.querySelector('[data-tab="'+tab+'"]');
 if(button)openTab(tab,button);
 const target=$(id);if(target){target.scrollIntoView({behavior:"smooth",block:"center"});target.focus({preventScroll:true});}
}
function renderGuide(d){
 if(previousRecording&&!d.recording){justFinishedRecording=true;}
 if(d.recording)justFinishedRecording=false;
 previousRecording=d.recording;
 let step=0,action="takeControl",tab="overview",message="Take control to set up the recorder.",stage="CONNECT";
 if(d.recording){step=3;action="stop";message="Recording "+(Number(d.duration)||0).toFixed(1)+" seconds · "+(d.samples||0)+" samples. The SD logger continues independently of the Wi-Fi dashboard. Stop and save after recovery if not already stopped.";stage="RECORDING";}
 else if(justFinishedRecording){step=4;tab="storage";action="refresh";stage="REVIEW";message="Recording has ended. Open Storage, confirm the saved file and inspect the flight PDF report.";}
 else if(d.simulation){step=1;tab="testing";action="simstop";stage="SIMULATING";message="Simulation is active. Stop it before performing real preflight checks.";}
 else if(d.armed){step=3;action="start";message="Recorder armed. Start recording before the physical launch and verify samples are increasing.";stage="ARMED";}
 else if(!d.controller){step=0;action="takeControl";message=d.controlAvailable?"Take control to calibrate and change settings.":"Another browser has control. You can still view live data and saved files.";stage="CONNECT";}
 else if(!d.sensor||!d.reading){step=1;tab="testing";action="testCalibrate";message="Check the BMP388 sensor and its I²C wiring before proceeding.";stage="SENSOR CHECK";}
 else if(!d.sdVerified||d.sdFault){step=1;tab="storage";action="sdtest";message="Test the microSD card. The test writes, reads, and removes a temporary file without deleting flight recordings.";stage="TEST SD";}
 else if(!d.calibrated){step=2;action="calibrate";message="Calibrate altitude while the rocket is stationary near its launch position.";stage="CALIBRATE";}
 else if(d.batteryCritical){step=2;action="sysV";tab="system";message="Battery critically low. Recharge or replace the battery before recording.";stage="BATTERY";}
 else{step=2;action="arm";message="Preflight checks passed. Confirm motor and sample rate, then choose SAVE SETTINGS & ARM LOGGER. Any unsaved changes will be saved automatically.";stage="READY";}
 guideTarget=action;window.guideTab=tab;
 $("guideStage").textContent=stage;
 $("guideMessage").textContent=message;
 $("guideProgress").style.width=(step===0?7:step===1?30:step===2?55:step===3?80:100)+"%";
 $("guideNext").textContent=action==="stop"?"GO TO STOP & SAVE":action==="start"?"GO TO START RECORDING":action==="sdtest"?"GO TO SD TEST":action==="refresh"?"GO TO SAVED FLIGHTS":"VIEW NEXT STEP";
 $("guideNext").disabled=!d.controlAvailable&&!d.controller&&step!==3;
 [...$("guideSteps").children].forEach((el,i)=>{el.classList.toggle("done",i<step);el.classList.toggle("current",i===step);});
}
function render(d,collect=true) {
 state=d;
 renderGuide(d);
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
  d.recording?(d.launched?"IN FLIGHT":"RECORDING"):
  d.armed?"ARMED":d.simulation?"SIMULATING":
  d.calibrated?"PREFLIGHT LIVE":"NOT CALIBRATED";

 $("controlState").textContent=
  d.controller?"ACTIVE CONTROLLER":"VIEW ONLY";
 $("controlState").style.color=
  d.controller?"#72e5ab":"#ffdd88";
 $("controlLease").textContent=
  d.controller?d.controlLeaseSeconds+" s (auto-renewing)":
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
  busy||d.calibrating||d.shutdownPending||d.updateInProgress;
 const lock=deviceLock||!d.controller;

 $("calibrate").disabled=
  lock||d.armed||d.recording||d.simulation;
 $("testCalibrate").disabled=$("calibrate").disabled;

 $("arm").disabled=
  lock||d.armed||d.recording||d.simulation||
  !d.sensor||!d.reading||!d.calibrated||
  !d.sdVerified||d.sdFault||d.batteryCritical;

 $("start").disabled=lock||!d.armed||d.recording;
 $("stop").disabled=lock||!d.recording;
 $("disconnect").disabled=lock||!d.recording;
 $("simulate").disabled=
  lock||d.recording||d.armed||d.simulation;
 $("simstop").disabled=
  lock||!d.simulation;
 // Keep the SD test clickable in view-only mode: it requests control
 // and reports errors instead of silently appearing unresponsive.
 $("sdtest").disabled=deviceLock||d.recording||d.armed;
 $("shutdown").disabled=lock;
 $("installFirmware").disabled=
  lock||d.armed||d.recording||d.simulation||d.batteryCritical;
 $("saveMission").disabled=lock||d.armed||d.recording;
 $("saveRate").disabled=lock||d.armed||d.recording;
 $("missionKind").disabled=lock||d.armed||d.recording;
 $("motorLabel").disabled=lock||d.armed||d.recording;
 $("sampleRate").disabled=lock||d.armed||d.recording;
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
  Math.min(100,d.calStep/35*100)+"%";
 $("calText").textContent=
  "Readings "+d.calStep+"/35 · Valid "+d.calValid;

 if(d.calibrating) {
  notice("Calibrating BMP388: "+d.calStep+"/35");
 }
 if(previousCalibrating&&!d.calibrating) {
  notice(d.calStatus,d.calibrated);
 }
 previousCalibrating=d.calibrating;

 $("freshness").textContent=
  d.freshness==null?"No readings":d.freshness+" ms";
 $("reads").textContent=d.good+" / "+d.bad;
 $("calCount").textContent=d.calCount;
 $("calAge").textContent=
  d.calAge==null?"Never":d.calAge+" s ago";
 $("baseline").textContent=number(d.baseline,2)+" Pa";
 $("calResult").textContent=d.calStatus;

 $("sdStatus").textContent=d.sdStatus;
 const checks=[["Sensor",d.sensor&&d.reading],["Calibration",d.calibrated&&!d.calibrating],["SD",d.sd&&d.sdVerified&&!d.sdFault],["Battery",!d.batteryCritical&&(!d.batteryPresent||!d.batteryLow)],["Idle",!d.recording&&!d.armed]];
 const passed=checks.filter(x=>x[1]).length;
 $("preflightScore").textContent=passed+"/"+checks.length+" checks";
 $("preflightDetail").textContent=checks.map(x=>(x[1]?"PASS ":"CHECK ")+x[0]).join(" · ")+(d.batteryPresent?"":" · Battery not detected");
 $("storageWriteErrors").textContent=d.sdErrors;
 $("storageMissed").textContent=d.missed;
 $("storageRate").textContent=d.sampleRate+" Hz";
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

 $("flightLabel").value=(d.armed||d.recording)?d.flightLabel:(d.missionKind==="test"?"Test ":"Flight ")+d.nextFlightNumber;
 if(!missionDirty&&document.activeElement!==$("missionKind"))$("missionKind").value=d.missionKind;
 if(!missionDirty&&document.activeElement!==$("motorLabel"))
  $("motorLabel").value=d.motorLabel;
 if(!sampleRateDirty && !sampleRateSaving)$("sampleRate").value=String(d.sampleRate);
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
  render(await api("status"));
 }catch(err) {
  $("connection").textContent="● OFFLINE";
  $("connection").style.color="#ff91a0";
  $("guideStage").textContent="OFFLINE";
  $("guideMessage").textContent="Recorder disconnected. Check that this device is connected to GREEN-ION Wi-Fi, then refresh.";
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
  if(sampleRateDirty){await api("settings","POST",{rate:$("sampleRate").value});sampleRateDirty=false;}
  const result=await api("arm","POST");notice(result.message,true);
 }catch(err){notice("Cannot arm logger: "+err.message,false);}
 finally{busy=false;await poll();}
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
 try{const result=await api("settings","POST",{rate});notice(result.message,true);sampleRateDirty=false;}
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
 $("updateFeedback").textContent=
  "Uploading "+file.name+" · do not disconnect power...";

 const form=new FormData();
 form.append("firmware",file,file.name);

 try {
  const response=await fetch(
   "/api/update?token="+encodeURIComponent(clientToken),{
   method:"POST",
   body:form
  });
  const result=await response.json();
  if(!response.ok||!result.success)
   throw Error(result.message||"Update failed");

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
 if($("overview").classList.contains("active"))draw("overviewGraph","altitude",distance);
 if($("testing").classList.contains("active")){
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
let comparisonMeta=null;
async function loadSavedGraph(name){
 const requestId=++graphRequestId;
 loadedFlightName="";
 const label=$("savedGraphStatus");
 label.textContent="Loading recorded samples...";
 try{
  const response=await fetch("/api/csv?name="+encodeURIComponent(name),{cache:"no-store"});
  if(!response.ok)throw Error("Could not load recorded CSV ("+response.status+")");
  const csv=await response.text();
  const lines=csv.trim().split(/\r?\n/);
  const parsed=lines.slice(1).map(line=>line.split(",").map(Number)).filter(r=>r.length>=7&&Number.isFinite(r[0]));
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
async function compareFlights(){
 const el=$("comparisonResults");el.replaceChildren();
 const a=$("compareA").value,b=$("compareB").value;
 if(!a||!b||a===b){el.textContent="Select two different flights.";return;}
 el.textContent="Loading comparison...";
 try{
  const [one,two]=await Promise.all([readFlightMetadata(a),readFlightMetadata(b)]);
  comparisonMeta={a,b,one,two};
  el.replaceChildren();
  for(const [key,title] of [["peak_altitude_m","Peak altitude (m)"],["duration_s","Duration (s)"],["samples","Samples"],["missed_samples","Missed samples"],["invalid_samples","Invalid samples"],["temperature_max_c","Max temperature (°C)"]]){
   if(!(key in one)&&!(key in two))continue;
   const row=document.createElement("div");row.className="row";
   const label=document.createElement("span");label.textContent=title;
   const value=document.createElement("strong");value.textContent=(one[key]||"—")+" / "+(two[key]||"—");
   row.append(label,value);el.appendChild(row);
  }
 }catch(err){el.textContent=err.message;}
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
function compareBarGraph(a,b,key,title,unit){
 const x=Number(a[key]),y=Number(b[key]);
 if(!Number.isFinite(x)||!Number.isFinite(y))return "<p>"+safeHTML(title)+": not enough data for a graph.</p>";
 const c=document.createElement("canvas");c.width=1000;c.height=230;
 const ctx=c.getContext("2d");ctx.fillStyle="#fff";ctx.fillRect(0,0,c.width,c.height);
 ctx.fillStyle="#172b40";ctx.font="18px Arial";ctx.fillText(title,34,28);
 const max=Math.max(1,x,y)*1.1;
 const vals=[[x,"First flight","#1976a4"],[y,"Second flight","#30a077"]];
 vals.forEach(([v,label,color],i)=>{const top=55+i*82;ctx.fillStyle="#2a3949";ctx.font="15px Arial";ctx.fillText(label,34,top);ctx.fillStyle="#e6edf3";ctx.fillRect(34,top+9,820,28);ctx.fillStyle=color;ctx.fillRect(34,top+9,Math.max(0,Math.min(820,v/max*820)),28);ctx.fillStyle="#172b40";ctx.fillText(v.toFixed(2)+" "+unit,870,top+28);});
 return "<h2>"+safeHTML(title)+"</h2><img alt='"+safeHTML(title)+"' src='"+c.toDataURL("image/png")+"'>";
}
function exportComparePDF(){
 const c=comparisonMeta;
 if(!c||c.a!==$("compareA").value||c.b!==$("compareB").value){alert("Compare the selected flights first.");return;}
 const fields=[["Motor","motor"],["Peak altitude (m)","peak_altitude_m"],["Duration (s)","duration_s"],["Samples","samples"],["Missed samples","missed_samples"],["Invalid samples","invalid_samples"],["Max temperature (°C)","temperature_max_c"]];
 const graphs=compareBarGraph(c.one,c.two,"peak_altitude_m","Peak altitude","m")+compareBarGraph(c.one,c.two,"duration_s","Recording duration","s")+compareBarGraph(c.one,c.two,"samples","Samples recorded","samples")+compareBarGraph(c.one,c.two,"starting_battery_v","Starting battery voltage","V");
 printableReport("GREEN-ION motor comparison", "<p>"+safeHTML(c.a)+" ("+safeHTML(c.one.motor||"No motor label")+") vs "+safeHTML(c.b)+" ("+safeHTML(c.two.motor||"No motor label")+") · Values in the table are first / second flight. Comparisons do not control for rocket mass, atmospheric conditions, or launch configuration.</p>"+pdfRows(fields.map(([label,key])=>[label,(c.one[key]??"—")+" / "+(c.two[key]??"—")]))+graphs);
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
  for(const id of ["compareA","compareB"]){
   const el=$(id),previous=el.value;el.replaceChildren();
   for(const f of files){const option=document.createElement("option");option.value=f.name;option.textContent=f.name;el.appendChild(option);}
   if(files.some(f=>f.name===previous))el.value=previous;
  }
  if(files.length>1&&$("compareA").value===$("compareB").value)$("compareB").selectedIndex=1;
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
   csv.onclick=()=>{
    location.href="/api/csv?name="+
     encodeURIComponent(f.name);
   };

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
   rm.textContent="Delete";
   rm.className="danger";
   rm.onclick=async()=>{
    if(!confirm("Delete "+f.name+"?"))return;
    try {
     await api("delete?name="+
      encodeURIComponent(f.name),"POST");
     if(selectedFlight===f.name)selectedFlight="";
     loadFiles();
    }catch(err){alert(err.message)}
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
$("arm").onclick=saveThenArm;
$("start").onclick=()=>command("start");
$("saveMission").onclick=saveMission;
$("saveRate").onclick=saveRate;
$("sampleRate").addEventListener("change",()=>{sampleRateDirty=true;});
$("missionKind").addEventListener("change",()=>{missionDirty=true;if(state && !state.armed && !state.recording)$("flightLabel").value=($("missionKind").value==="test"?"Test ":"Flight ")+state.nextFlightNumber;});
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
$("shutdown").onclick=shutdown;
$("disconnect").onclick=disconnect;

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
$("compareFlights").onclick=compareFlights;
$("exportFlightPDF").onclick=exportFlightPDF;
$("exportComparePDF").onclick=exportComparePDF;
$("openFlightStats").onclick=()=>openTab("storage",document.querySelector('[data-tab="storage"]'));
$("guideNext").onclick=()=>jumpToControl(guideTarget,window.guideTab||"overview");
$("guideStorage").onclick=()=>openTab("storage",document.querySelector('[data-tab="storage"]'));

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

notice("Dashboard loaded. Connecting to ESP32...");
setInterval(poll,1200);
poll();
</script>
</body>
</html>
)HTML";
