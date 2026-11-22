#!/usr/bin/env node
// Offline, read-only software contract probe. Never makes HTTP/serial requests.
import {existsSync, readFileSync} from "node:fs";
import {resolve, join} from "node:path";
import {pathToFileURL} from "node:url";
import {execFileSync} from "node:child_process";
import {createContext, runInContext} from "node:vm";

const args=process.argv.slice(2);
const sourceIndex=args.indexOf("--source-root");
const root=sourceIndex>=0&&args[sourceIndex+1]?resolve(args[sourceIndex+1]):process.cwd();
const corePath=join(root,"web","v18_offline_core.mjs");
const contractPath=join(root,"docs","V18","FIRMWARE_DEVTEST_CONTRACT.md");
const appPath=join(root,"web","v18_offline_app.mjs");
let sourceSha=null;
try{sourceSha=execFileSync("git",["-C",root,"rev-parse","HEAD"],{encoding:"utf8",stdio:["ignore","pipe","ignore"]}).trim();}catch{}
const report={schema:"green-ion-v18-contract-probe-1",sourceSha,results:[]};
const add=(id,state,evidence)=>report.results.push({id,state,evidence});
const check=(id,condition,good,bad)=>add(id,condition?"PASS":"FAIL",condition?good:bad);

if(!existsSync(corePath)){
 add("v18_core","NOT TESTED","V18 offline core is absent from this source tree.");
 console.log(JSON.stringify(report,null,2));
 process.exit(2);
}

const core=await import(pathToFileURL(corePath).href);
const {deriveFlightState,criticalFlightProblem,optionalFlightWarnings,canPrepare,
 fileSaveEvidence,parseReceiverLine,recoveryView,developmentStatusChecks}=core;
const now=100000;
const ready={sd_ok:true,imu_ok:true,barometer_ready:true,recorder_mode:0,battery_mv:3900};
const sequence=[
 [ready,"READY"],
 [{...ready,flight_prep_active:true},"TESTING"],
 [{...ready,armed:true,recorder_mode:1},"ARMED"],
 [{...ready,recording:true,recorder_mode:2},"FLYING"],
 [{...ready,recorder_mode:3},"SAVING"],
];
check("firmware_phase_sequence",
 sequence.every(([status,expected])=>deriveFlightState(status,{connected:true,saveVerified:false})===expected)&&
 deriveFlightState({...ready,recorder_mode:3},{connected:true,saveVerified:true})==="SAVED",
 "Display states follow supplied firmware evidence; SAVED requires explicit save evidence.",
 "A display state is missing or SAVED appears without supplied save evidence.");
check("disconnected_fails_closed",deriveFlightState(ready,{connected:false})==="OFFLINE",
 "Disconnected status is OFFLINE.","A disconnected status is shown as ready.");

const stale=developmentStatusChecks(ready,{connected:true,lastStatusAt:now-4000,now});
check("stale_status_no_pass",stale.every(x=>x.state!=="PASS"),
 "Stale Feather status gives no development-check PASS.",
 "A development check passed using stale Feather status.");
check("critical_sd_optional_radio",
 !!criticalFlightProblem({...ready,sd_ok:false})&&
 !canPrepare({...ready,sd_ok:false})&&
 canPrepare({...ready,gps_status:"missing",radio_ok:false})&&
 optionalFlightWarnings({...ready,gps_status:"missing",radio_ok:false}).length===2,
 "SD blocks preparation while GPS/LoRa absence remains advisory.",
 "Critical and optional readiness are not separated.");
const unknownBattery={...ready,battery_mv:null,battery_v:null};
const batteryCheck=developmentStatusChecks(unknownBattery,{connected:true,lastStatusAt:now-100,now})
 .find(x=>x.id==="battery");
check("unknown_battery_not_tested",batteryCheck?.state==="NOT TESTED",
 "Unknown battery measurement stays NOT TESTED.",
 "Unknown battery measurement is classified as PASS or FAIL.");

const validFiles={files:[{name:"V12D001.BIN",size:80,recording_complete:true},
 {name:"V12D002.BIN",size:64,recording_complete:false}]};
check("flight_identity_and_completion",
 fileSaveEvidence(validFiles,"V12D001.BIN")?.name==="V12D001.BIN"&&
 fileSaveEvidence(validFiles,"V12D002.BIN")===null&&
 fileSaveEvidence(validFiles,"V12D003.BIN")===null,
 "Distinct flight names and incomplete file-list entries do not cross-authorize SAVED.",
 "A mismatched or incomplete file-list entry authorizes save evidence.");

// Execute the app's actual save-verification body with local response doubles.
// The only accepted positive is matching structural completion evidence; this
// does not certify unchecksummed sensor record contents.
if(!existsSync(appPath))add("save_metadata_binding","NOT TESTED","V18 app source is absent.");
else{
 const app=readFileSync(appPath,"utf8");
 const start=app.indexOf("async function verifySave()");
 const end=start<0?-1:app.indexOf("async function pollFlight()",start);
 const functionSource=end>start?app.slice(start,end).trim():null;
 if(!functionSource)add("save_metadata_binding","FAIL","Save-verification function could not be isolated from the reviewed app source.");
 else{
  const metadata=(name="V12D001.BIN",bytes=80,records=3)=>
   `GREEN-ION-COMPLETION-1\nstatus=complete\nfile=${name}\nfile_bytes=${bytes}\ncomplete_records=${records}\nduration_ms=1200\n`;
  const makeContext=({name="V12D001.BIN",txt=metadata(name),files,
                     metadataError=false,onMetadata}={})=>{
   const context=createContext({
    ...core,
    currentStatus:{...ready,recorder_mode:3,log_file:name,recording:false,armed:false},
    connected:true,lastStatusAt:Date.now(),lastLogFile:name,lastFiles:null,
    saveVerified:false,lastSaved:null,metaText:txt,
    files:files??{files:[{name,size:80,recording_complete:true}]},
    json:async()=>context.files,
    api:async()=>{
     if(metadataError)throw Error("metadata unavailable");
     return {text:async()=>{onMetadata?.(context);return context.metaText;}};
    },
    parseMeta:t=>Object.fromEntries(String(t).split(/\r?\n/)
     .filter(x=>x.includes("=")).map(x=>{const i=x.indexOf("=");return [x.slice(0,i).trim(),x.slice(i+1).trim()];})),
    paintFlight:()=>{},encodeURIComponent,Date,
   });
   runInContext(functionSource,context);
   return context;
  };
  const verified=c=>c.saveVerified===true&&c.lastSaved?.name===c.currentStatus.log_file&&
   deriveFlightState(c.currentStatus,{connected:c.connected,saveVerified:c.saveVerified})==="SAVED"&&
   canPrepare(c.currentStatus,{saveVerified:true});
  const rejected=c=>c.saveVerified===false&&c.lastSaved===null&&
   deriveFlightState(c.currentStatus,{connected:c.connected,saveVerified:c.saveVerified})!=="SAVED";
  const evaluate=async(id,options,expectValid)=>{
   const context=makeContext(options);
   await runInContext("verifySave()",context);
   check(id,expectValid?verified(context):rejected(context),
    expectValid?"Matching BIN/TXT completion permits SAVED and FLY AGAIN.":"Unverified completion keeps SAVED and FLY AGAIN closed.",
    expectValid?"Matching completion failed to authorize the selected flight.":"Invalid or stale evidence authorized SAVED or retained prior save evidence.");
   return context;
  };
  const first=await evaluate("save_valid_completion",{},true);
  first.currentStatus={...first.currentStatus,log_file:"V12D002.BIN"};
  first.lastLogFile="V12D002.BIN";first.saveVerified=false;first.lastSaved=null;
  first.files={files:[{name:"V12D001.BIN",size:80,recording_complete:true},
   {name:"V12D002.BIN",size:80,recording_complete:true}]};
  first.metaText=metadata("V12D002.BIN");
  await runInContext("verifySave()",first);
  check("save_second_flight",verified(first),
   "A later flight needs its own matching completion evidence.",
   "The second flight was not independently verified or inherited prior flight evidence.");
  await evaluate("save_wrong_flight",{txt:metadata("OTHER.BIN")},false);
  await evaluate("save_blank_metadata",{txt:""},false);
  await evaluate("save_missing_marker",{txt:metadata().replace("GREEN-ION-COMPLETION-1\n","")},false);
  await evaluate("save_incomplete",{txt:metadata().replace("status=complete","status=incomplete")},false);
  await evaluate("save_partial_records",{txt:metadata("V12D001.BIN",64,2)},false);
  await evaluate("save_byte_mismatch",{txt:metadata("V12D001.BIN",96,3)},false);
  await evaluate("save_record_mismatch",{txt:metadata("V12D001.BIN",80,2)},false);
  await evaluate("save_zero_records",{txt:metadata("V12D001.BIN",32,0)},false);
  await evaluate("save_contradictory_fields",{txt:metadata()+"file=OTHER.BIN\n"},false);
  await evaluate("save_metadata_error",{metadataError:true},false);
  await evaluate("save_delayed_new_flight",{onMetadata:c=>{
   c.currentStatus={...c.currentStatus,log_file:"V12D002.BIN"};
   c.lastLogFile="V12D002.BIN";
  }},false);
  await evaluate("save_delayed_disconnection",{onMetadata:c=>{c.connected=false;}},false);
  await evaluate("save_delayed_stale_status",{onMetadata:c=>{c.lastStatusAt=Date.now()-5000;}},false);
 }
}

const packet=(overrides={})=>JSON.stringify({type:"telemetry",protocol:2,altitude_m:10,
 battery_v:3.9,gps_status:"live",latitude:34.1,longitude:-117.2,...overrides});
const good=parseReceiverLine(packet(),now);
check("receiver_protocol_rejection",
 !!good&&parseReceiverLine("{bad",now)===null&&
 parseReceiverLine(packet({protocol:3}),now)===null&&
 parseReceiverLine(packet({type:"control"}),now)===null,
 "Malformed, unknown-version and control packets are rejected.",
 "Malformed, unknown-version or control packet was accepted.");
check("receiver_stale_fix",recoveryView(good,null,now+4000).gps!=="live"&&
 parseReceiverLine(packet({gps_fix_age_ms:16000}),now)?.gpsStatus!=="live",
 "Old packets and old GPS fixes are not shown LIVE.",
 "Old receiver or GPS evidence is shown LIVE.");
check("receiver_missing_coordinate",
 parseReceiverLine(packet({latitude:null}),now)?.gpsStatus!=="live"&&
 parseReceiverLine(packet({longitude:null}),now)?.gpsStatus!=="live",
 "A missing latitude or longitude never creates a live fix.",
 "A null coordinate was treated as a live GPS fix.");

if(!existsSync(contractPath))add("firmware_contract","NOT TESTED","Firmware development-test contract is absent.");
else{
 const contract=readFileSync(contractPath,"utf8");
 check("firmware_contract_documented",
 ["GET /api/status","POST /api/sdtest","GET /api/wifi/status",
  "POST /api/hardware-test","V12SD.TMP"].every(x=>contract.includes(x)),
 "Read-only and reserved temporary-file endpoint contracts are documented; hardware behavior is NOT TESTED.",
 "One or more documented firmware endpoint contracts are missing.");
}

console.log(JSON.stringify(report,null,2));
process.exit(report.results.some(x=>x.state==="FAIL")?1:
 report.results.some(x=>x.state==="NOT TESTED")?2:0);
