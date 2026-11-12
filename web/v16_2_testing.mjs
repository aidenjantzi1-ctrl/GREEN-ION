/* GREEN ION V16.2 Testing front door.
 * Detailed tests remain available, but routine use starts with one measured home-test action. */
const $=id=>document.getElementById(id);
const t=window.GREEN_ION_TRANSPORT;
function row(label,id){const d=document.createElement("div");d.className="row";d.innerHTML="<span>"+label+"</span><strong id='"+id+"'>PENDING</strong>";return d;}
function normalize(value){const v=String(value??"").toUpperCase();return /PASS/.test(v)?"PASS":/FAIL|ERROR/.test(v)?"FAIL":/INCONCLUSIVE|NOT VERIFIED/.test(v)?"INCONCLUSIVE":/NOT RUN|NOT TESTED/.test(v)?"NOT TESTED":"PENDING";}
function evidence(){
 const w=t.state.lastWifiStatus||{};
 const set=(id,value,next)=>{const n=$(id);if(!n)return;n.textContent=normalize(value)+(next?" · "+next:"");};
 set("v162TestFeather",t.controlsVerified?.()?"PASS":"PENDING",t.controlsVerified?.()?"fresh Wi-Fi status":"connect Feather Wi-Fi");
 set("v162TestBattery",Number(w.battery_v)>0?"PASS":"PENDING",Number(w.battery_v)>0?Number(w.battery_v).toFixed(3)+" V measured":"battery voltage unavailable");
 set("v162TestSd",w.sd_ok===true?"PASS":w.sd_ok===false?"FAIL":"PENDING",w.sd_ok===false?"check card/wiring":"");
 set("v162TestBaro",w.barometer_ready===true?"PASS":w.barometer_ready===false?"FAIL":"PENDING",w.barometer_ready===false?"check BMP388/cable":"");
 set("v162TestImu",w.imu_test_result|| (w.imu_ok===true?"PENDING":w.imu_ok===false?"FAIL":"PENDING"),w.imu_ok===false?"check LSM6DSO32/cable":"");
 set("v162TestGps",w.gps_test_result|| (w.gps_ok===true?"PENDING":w.gps_ok===false?"FAIL":"PENDING"),w.gps_status==="missing"?"move outdoors for fix":"");
 set("v162TestRadio",w.radio_ok===true?(t.radioFresh()?"PASS":"INCONCLUSIVE"):w.radio_ok===false?"FAIL":"PENDING",t.radioFresh()?"receiver packet seen":"receiver confirmation still required");
 const home=String(w.home_record_result||"IDLE");
 set("v162TestRecording",home==="PASS"?"PASS":home==="FAIL"?"FAIL":"PENDING",home==="PASS"?"temporary recording verified":"run full test");
 set("v162TestCalibration",w.preflight_state==="READY"?"PASS":w.calibration_baseline_pa?"PENDING":"NOT TESTED",w.calibration_baseline_pa?"calibration exists; finish test":"keep Feather stationary");
}
function reorganize(){
 const testing=$("testing"),guide=$("v161GuidedTestCard");if(!testing||!guide)return;
 guide.querySelector("h2").textContent="Run Full Home Test";
 const p=guide.querySelector("p.muted");if(p)p.textContent="One guided workflow checks every measurable subsystem, temporary recording, receiver evidence and calibration. Physical inspections remain separate.";
 const start=$("v161GuideStart");if(start){start.textContent="RUN FULL HOME TEST";start.classList.add("green","wide");}
 if(!$("v162TestEvidence")){
  const ev=document.createElement("div");ev.id="v162TestEvidence";ev.className="v162-test-evidence";
  for(const [label,id] of [["Feather connection","v162TestFeather"],["Battery","v162TestBattery"],["SD","v162TestSd"],["Barometer","v162TestBaro"],["IMU","v162TestImu"],["GPS","v162TestGps"],["LoRa / receiver","v162TestRadio"],["Temporary recording","v162TestRecording"],["Calibration","v162TestCalibration"]])ev.append(row(label,id));
  guide.insertBefore(ev,guide.querySelector(".actions"));
 }
 if(!$("v162DetailedTests")){
  const details=document.createElement("details");details.id="v162DetailedTests";details.className="v16-section-group card";
  const summary=document.createElement("summary");summary.textContent="Detailed tests & manual tools";details.append(summary);
  const nodes=[...testing.children].filter(n=>n!==guide&&n!==details);
  for(const n of nodes)details.append(n);testing.append(details);
 }
 const button=$("v161GuideDetails");if(button)button.onclick=()=>{const d=$("v162DetailedTests");d.open=true;d.scrollIntoView({behavior:"smooth",block:"start"});};
 evidence();
}
export function initV162Testing(){
 reorganize();window.addEventListener("green-ion-link",evidence);window.addEventListener("green-ion-v16-preflight",evidence);
 setInterval(evidence,3000);
}
