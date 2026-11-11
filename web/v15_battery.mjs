/* Conservative planning helper, not a LiPo fuel gauge or flight clearance.
 * A single voltage reading is insufficient to infer actual remaining capacity. */
const KEY="green-ion-v15-battery-plan";
const $=id=>document.getElementById(id),t=window.GREEN_ION_TRANSPORT;
export function estimateBattery({voltage,capacityMah,drawMa,requiredMinutes,reserveFactor=1.5}){
 if(!Number.isFinite(voltage)||voltage<=0)
  return {state:"NOT VERIFIED",reason:"No live battery voltage received"};
 if(voltage<3.6)
  return {state:"NOT READY",reason:"Battery below the firmware's 3.60 V OTA/preflight threshold; recharge and bench-test"};
 if(![capacityMah,drawMa,requiredMinutes].every(v=>Number.isFinite(v)&&v>0))
  return {state:"NOT VERIFIED",reason:"Enter tested battery capacity, measured operating current and required operating duration"};
 // Voltage-derived charge fraction is an intentionally coarse planning range.
 // Load, cell age, temperature and discharge curve can overwhelm this approximation.
 const fraction=Math.max(0,Math.min(1,(voltage-3.5)/0.7));
 const approxMinutes=60*capacityMah*fraction/drawMa;
 if(approxMinutes<requiredMinutes*reserveFactor)
  return {state:"CAUTION",approxMinutes,reason:"Estimated endurance is shorter than the requested duration plus reserve; recharge or use a tested battery"};
 return {state:"ESTIMATE ONLY",approxMinutes,reason:"Rough runtime estimate meets entered duration and reserve. This is NOT flight approval: verify real loaded runtime, voltage sag, sensors and physical safety"};
}
function render(){
 const wifi=t.wifiFresh(),raw=wifi?t.state.lastWifiStatus:t.radioFresh()?t.state.radio:null;
 const voltage=Number(raw?.battery_v);
 const capacityMah=Number($("v15BatteryCapacity").value);
 const drawMa=Number($("v15BatteryDraw").value);
 const requiredMinutes=Number($("v15BatteryMinutes").value);
 const result=estimateBattery({voltage,capacityMah,drawMa,requiredMinutes});
 $("v15BatteryVoltage").textContent=Number.isFinite(voltage)&&voltage>0?voltage.toFixed(3)+" V · "+(wifi?"Wi-Fi":"LoRa"):"No live battery reading";
 $("v15BatteryResult").textContent=result.state+" · "+result.reason;
 $("v15BatteryRuntime").textContent=result.approxMinutes===undefined?
  "Runtime estimate unavailable":Math.floor(result.approxMinutes)+" min rough estimate (before safety reserve)";
 $("v15FieldBattery").textContent=result.state+" — "+(result.approxMinutes===undefined?
  "recording endurance unverified":Math.floor(result.approxMinutes)+" min rough estimate; NOT flight approval");
}
export function initBattery(){
 try{
  const settings=JSON.parse(localStorage.getItem(KEY)||"{}");
  for(const [id,k] of [["v15BatteryCapacity","capacity"],["v15BatteryDraw","draw"],["v15BatteryMinutes","minutes"]]){
   if(Number.isFinite(Number(settings[k]))&&Number(settings[k])>0)$(id).value=String(settings[k]);
  }
 }catch{}
 for(const id of ["v15BatteryCapacity","v15BatteryDraw","v15BatteryMinutes"]){
  $(id).addEventListener("input",()=>{
   try{localStorage.setItem(KEY,JSON.stringify({
    capacity:$("v15BatteryCapacity").value,draw:$("v15BatteryDraw").value,
    minutes:$("v15BatteryMinutes").value}));}catch{}
   render();
  });
 }
 window.addEventListener("green-ion-link",render);
 setInterval(render,2500);render();
}
