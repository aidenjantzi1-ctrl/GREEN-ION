/* Explicit local-vs-hardware actions and accessible tab navigation. */
export const FEATHER_CONTROLS = new Set([
 "prepareAll","fieldPrepare","fieldArm","fieldStop","armWatch","stop","calibrate","testCalibrate",
 "homeAuto","homeStart","homeRun","homeNext","homeSkip","homeFinish","testImu","testGps",
 "testAllSensors","groundtest","sdtest","refresh","downloadEverything",
 "installFirmware","v14DownloadUpdate","rollbackFirmware","exitMaintenance","restoreConfig","shutdown",
 "saveFlightProfile","ledoff","ledauto","takeControl","releaseControl","control",
 "start","testCalibrate","reconnect","testRadio"
]);
export function requiresFeather(id){return FEATHER_CONTROLS.has(String(id||""));}
export function initTabs(nav){
 if(!nav)return;
 const buttons=Array.from(nav.querySelectorAll("button[data-tab]"));
 function sync(){
  for(const button of buttons){
   const active=button.classList.contains("active");
   button.setAttribute("role","tab");
   button.setAttribute("aria-controls",button.dataset.tab);
   button.setAttribute("aria-selected",String(active));
   button.tabIndex=active?0:-1;
  }
 }
 nav.setAttribute("role","tablist");
 nav.addEventListener("click",sync);
 nav.addEventListener("keydown",event=>{
  if(!["ArrowRight","ArrowLeft","Home","End"].includes(event.key))return;
  const current=buttons.indexOf(document.activeElement);
  if(current<0)return;
  const next=event.key==="Home"?0:event.key==="End"?buttons.length-1:
    (current+(event.key==="ArrowRight"?1:-1)+buttons.length)%buttons.length;
  event.preventDefault();buttons[next].click();buttons[next].focus();
  buttons[next].scrollIntoView({block:"nearest",inline:"nearest"});
  sync();
 });
 const observer=new MutationObserver(sync);
 for(const button of buttons)observer.observe(button,{attributes:true,attributeFilter:["class"]});
 sync();
}
