/* GREEN ION V16.1 guided test wrapper.
 * Uses the existing real home/preflight test; it does not invent simulated PASS results. */
const $=id=>document.getElementById(id);
const t=window.GREEN_ION_TRANSPORT;
let lastPreflight=null;

function set(id,value){const n=$(id);if(n)n.textContent=value;}
function step(number,title,detail,state="WAITING"){
 set("v161GuideStep","Step "+number+" · "+title);
 set("v161GuideDetail",detail);
 set("v161GuideState",state);
}
function mirrorExistingStatus(){
 const source=$("homeAutoStatus");
 if(source&&source.textContent?.trim())set("v161GuideExisting",source.textContent.trim());
}
function startGuided(){
 if(!$("v161GuideSafe")?.checked){
  step(1,"Make the setup safe","Remove all motors, secure the payload, attach the intended radio antennas, and use safe bench power. Then check the confirmation box.","ACTION REQUIRED");
  return;
 }
 if(!t.wifiFresh()){
  step(2,"Connect the Feather","Join the GREEN ION Wi-Fi and wait for Feather Wi-Fi to show LIVE. The Windows app will keep retrying automatically.","WAITING FOR FEATHER");
  return;
 }
 const button=$("homeAuto");
 if(!button||button.disabled){
  step(3,"Wait for the existing test","The full preflight button is currently unavailable. Finish any recording, update, or maintenance session, then retry.","BLOCKED");
  return;
 }
 lastPreflight=null;
 step(3,"Run measured checks","GREEN ION is running the existing real one-button preflight. Keep the payload still when instructed. Do not disconnect power.","RUNNING");
 button.click();
}
function renderResult(detail){
 lastPreflight=detail||{};
 const fails=Math.max(0,Number(detail?.fails)||0);
 const blocking=Math.max(0,Number(detail?.blocking_unverified)||0);
 const manual=Math.max(0,Number(detail?.manual_unverified)||0);
 if(detail?.error||fails){
  step(4,"Fix the measured problem",
   (detail?.error||fails+" measured check(s) failed.")+" Review the detailed test below, correct the issue, then run the guided check again.","NEEDS ATTENTION");
 }else if(!detail?.complete){
  step(4,"Measured test incomplete","The measurable test did not finish. Review the detailed test output and retry before relying on the result.","INCOMPLETE");
 }else if(blocking){
  step(4,"Receiver/evidence still missing",blocking+" blocking item(s) are still unverified. These cannot be manually overridden; restore the missing receiver/evidence and rerun.","NOT VERIFIED");
 }else{
  step(5,"Review physical checks",
   "Measured checks completed. "+manual+" physical/manual item(s) still require your inspection. GREEN ION does not auto-approve those checks or authorize launch.","MEASURED CHECKS COMPLETE");
 }
}
export function initV161GuidedTest(){
 const testing=$("testing");if(!testing)return;
 if(!$("v161GuidedTestCard")){
  const card=document.createElement("div");card.className="card";card.id="v161GuidedTestCard";
  card.innerHTML='<h2>Simple guided system test</h2>'+
   '<p class="muted">One front door to the existing full hardware/preflight test. GREEN ION tells you what to do; detailed evidence remains below.</p>'+
   '<label><input id="v161GuideSafe" type="checkbox"> Motors removed, payload secured, intended radio antennas attached, safe bench power connected</label>'+
   '<div class="row"><span id="v161GuideStep">Step 1 · Make the setup safe</span><strong id="v161GuideState">ACTION REQUIRED</strong></div>'+
   '<p id="v161GuideDetail" class="muted">Confirm the safe bench setup, then start the guided check.</p>'+
   '<div class="actions"><button id="v161GuideStart">START GUIDED CHECK</button>'+
   '<button id="v161GuideDetails" class="secondary">Open detailed test</button>'+
   '<button id="v161GuideReport" class="secondary">Download test report</button></div>'+
   '<p id="v161GuideExisting" class="muted"></p>';
  testing.prepend(card);
 }
 $("v161GuideStart").onclick=startGuided;
 $("v161GuideDetails").onclick=()=>$("homeVerificationCard")?.scrollIntoView({behavior:"smooth",block:"start"});
 $("v161GuideReport").onclick=()=>{
  const button=$("homeReport");
  if(button&&!button.disabled)button.click();
  else set("v161GuideExisting","A downloadable report becomes available after the full test produces evidence.");
 };
 window.addEventListener("green-ion-v16-preflight",e=>renderResult(e.detail));
 window.addEventListener("green-ion-link",()=>{
  if($("v161GuideState")?.textContent==="WAITING FOR FEATHER"&&t.wifiFresh())
   step(2,"Feather connected","Feather Wi-Fi is live. Start the guided check when the payload is still and safe.","READY");
 });
 const observed=$("homeAutoStatus");
 if(observed){
  new MutationObserver(mirrorExistingStatus).observe(observed,{childList:true,subtree:true,characterData:true});
  mirrorExistingStatus();
 }
}
