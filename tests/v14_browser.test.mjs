import test from "node:test";
import assert from "node:assert/strict";
import {readFileSync} from "node:fs";
import {runInNewContext} from "node:vm";
import {APP_VERSION,compareVersions,parseVersion,assetForRelease,receiverPacket,radioStatus,safeFileName,validCoordinate} from "../web/v14_utils.mjs";

test("semantic version handling includes stable and beta precedence",()=>{
 assert.equal(APP_VERSION,"14.0.0-beta.1");
 assert.equal(compareVersions("v14.0.0-beta.1","v13.9.9"),1);
 assert.equal(compareVersions("v14.0.0","v14.0.0-beta.2"),1);
 assert.equal(compareVersions("v14.0.0-beta.2","v14.0.0-beta.1"),1);
 assert.equal(compareVersions("invalid","v13.0.0"),null);
 assert.equal(parseVersion("v14.0.0-beta.1").major,14);
});
test("only official firmware release assets may be offered",()=>{
 const good={name:"GREEN_ION_V14_COMPLETE.zip",browser_download_url:"https://github.com/aidenjantzi1-ctrl/GREEN-ION/releases/download/v14.0.0-beta.1/GREEN_ION_V14_COMPLETE.zip"};
 assert.deepEqual(assetForRelease({tag_name:"v14.0.0-beta.1",assets:[good]}),good);
 assert.equal(assetForRelease({tag_name:"v14.0.0-beta.1",assets:[{...good,browser_download_url:"https://evil.example/malware.zip"}]}),null);
});
test("serial radio packet validation and telemetry-only flags",()=>{
 assert.equal(receiverPacket({type:"control",altitude_m:1,battery_v:4}),null);
 assert.equal(receiverPacket({type:"telemetry",altitude_m:Infinity,battery_v:4}),null);
 const packet=receiverPacket({type:"telemetry",altitude_m:123.4,battery_v:4.1,latitude:34.1,longitude:-117.2,gps_status:"live"});
 assert.ok(packet);
 assert.ok(validCoordinate(packet.latitude,packet.longitude));
 const status=radioStatus(packet);
 assert.equal(status.v13_source,"lora");
 assert.equal(status.recording,false);
 assert.equal(status.ota_available,false);
 assert.equal(status.sd_ok,false);
 assert.equal(status.latitude,34.1);
 assert.equal(status.vertical_speed_mps,null);
});
test("file names reject traversal and unsupported extensions",()=>{
 assert.equal(safeFileName("V12D001.BIN"),"V12D001.BIN");
 assert.throws(()=>safeFileName("../../flight.bin"));
 assert.throws(()=>safeFileName("report.exe"));
});
test("browser adapter fails closed for controls and falls back to receive-only LoRa",async()=>{
 const script=readFileSync(new URL("../web/v14_boot.js",import.meta.url),"utf8");
 let requests=0;
 const mock={
  fetch:async url=>{
   requests++;
   if(String(url).includes("/api/status"))throw Error("Feather unreachable");
   throw Error("No Feather Wi-Fi");
  },
  addEventListener:()=>{},dispatchEvent:()=>{}
 };
 const sandbox={window:mock,location:{hostname:"green-ion.vercel.app",port:"",protocol:"https:"},
  Response,Event,Date,console,setTimeout,clearTimeout};
 runInNewContext(script,sandbox);
 const t=mock.GREEN_ION_TRANSPORT;
 assert.equal(t.source(),"none");
 assert.equal(t.feedRadio({type:"telemetry",altitude_m:25,battery_v:3.9,flight_state:3,gps_status:"live",latitude:34.1,longitude:-117.2}),true);
 assert.equal(t.source(),"lora");
 const res=await mock.fetch("/api/status");
 assert.equal(res.status,200);
 const data=await res.json();
 assert.equal(data.v13_source,"lora");
 assert.equal(data.ota_available,false);
 const before=requests;
 const blocked=await mock.fetch("/api/maintenance",{method:"POST"});
 assert.equal(blocked.status,503);
 assert.equal(requests,before,"must never proxy LoRa control");
});
