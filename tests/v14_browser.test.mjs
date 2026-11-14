import test from "node:test";
import assert from "node:assert/strict";
import {readFileSync} from "node:fs";
import {createHash,webcrypto} from "node:crypto";
import {createRequire} from "node:module";
import {runInNewContext} from "node:vm";
import {APP_VERSION,compareVersions,parseVersion,assetForRelease,releaseFirmwarePackage,checkFirmwareManifest,verifiedFirmwareBytes,receiverPacket,radioStatus,safeFileName,validCoordinate} from "../web/v14_utils.mjs";

test("semantic version handling includes stable and beta precedence",()=>{
 assert.equal(APP_VERSION,"17.0.1");
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

test("direct Feather update uses only the compiled app and matching SHA-256 release manifest",async()=>{
 const tag="v16.3.0";
 const base="https://github.com/aidenjantzi1-ctrl/GREEN-ION/releases/download/"+tag+"/";
 const raw=new Uint8Array(200100);raw[0]=0xE9;raw[1]=0x06;
 const hash=createHash("sha256").update(raw).digest("hex");
 const image={name:"Green_Ion_V16_3.ino.bin",size:raw.byteLength,browser_download_url:base+"Green_Ion_V16_3.ino.bin"};
 const sum={name:"SHA256_MANIFEST.json",browser_download_url:base+"SHA256_MANIFEST.json"};
 const zip={name:"GREEN_ION_V16_3_SOURCE.zip",browser_download_url:base+"GREEN_ION_V16_3_SOURCE.zip"};
 const release={tag_name:tag,assets:[zip,sum,image]};
 const manifest={version:tag,tag,board:"Adafruit Feather ESP32 V2",firmwareFormat:"ESP32 .ino.bin app image",
  firmwareGeneration:"V16.3",artifacts:{[image.name]:{size:raw.byteLength,sha256:hash}}};
 assert.deepEqual(releaseFirmwarePackage(release),{image,manifest:sum});
 assert.equal(assetForRelease(release),image,"never upload the ZIP");
 assert.deepEqual(checkFirmwareManifest(release,manifest,image),{size:raw.byteLength,sha256:hash});
 const called=[];
 const fetchFile=async url=>{
  called.push(url);
  return url===sum.browser_download_url?
   {ok:true,headers:new Headers(),text:async()=>JSON.stringify(manifest)}:
   {ok:true,headers:new Headers({"content-length":String(raw.byteLength)}),
    arrayBuffer:async()=>raw.buffer.slice(0)};
 };
 const result=await verifiedFirmwareBytes(release,fetchFile,webcrypto.subtle);
 assert.equal(result.byteLength,raw.byteLength);
 assert.deepEqual(called,[sum.browser_download_url,image.browser_download_url]);
 await assert.rejects(()=>verifiedFirmwareBytes({...release,assets:[zip,image]},fetchFile,webcrypto.subtle),/manifest/);
 await assert.rejects(()=>verifiedFirmwareBytes(release,async url=>
  url===sum.browser_download_url?{ok:true,headers:new Headers(),
   text:async()=>JSON.stringify({...manifest,board:"wrong ESP32 board"})}:
  {ok:true,headers:new Headers(),arrayBuffer:async()=>raw.buffer},webcrypto.subtle),/invalid/);
 await assert.rejects(()=>verifiedFirmwareBytes(release,async url=>
  url===sum.browser_download_url?{ok:true,headers:new Headers(),
   text:async()=>JSON.stringify(manifest)}:
  {ok:true,headers:new Headers(),arrayBuffer:async()=>new Uint8Array(raw.byteLength).buffer},
  webcrypto.subtle),/header/);
 const mutated=raw.slice();mutated[2]=1;
 await assert.rejects(()=>verifiedFirmwareBytes(release,async url=>
  url===sum.browser_download_url?{ok:true,headers:new Headers(),
   text:async()=>JSON.stringify(manifest)}:
  {ok:true,headers:new Headers(),arrayBuffer:async()=>mutated.buffer},
  webcrypto.subtle),/checksum/);
});

test("firmware release relay refuses unknown assets and unsafe HTTP methods",async()=>{
 const require=createRequire(import.meta.url);
 const handler=require("../web/api/firmware.js");
 const reply=()=>{
  const r={statusCode:200,headers:{},body:""};
  return {
   state:r,
   setHeader:(key,value)=>{r.headers[key]=value;},
   status:(code)=>{r.statusCode=code;return res;},
  };
 };
 const invoke=async req=>{
  const state={statusCode:200};
  const res={
   setHeader:()=>{},
   status(code){state.statusCode=code;return this;},
   end(text){state.body=text;return this;}
  };
  await handler(req,res);
  return state;
 };
 assert.equal((await invoke({method:"POST",query:{tag:"v16.3.0",name:"Green_Ion_V16_3.ino.bin"}})).statusCode,405);
 for(const name of ["../../secret","GREEN_ION_V16_3_SOURCE.zip","Green_Ion_V14.ino.bin","Feather_merged.bin"]){
  assert.equal((await invoke({method:"GET",query:{tag:"v16.3.0",name}})).statusCode,400);
 }
 assert.equal((await invoke({method:"GET",query:{tag:"v12.0.0-beta.1",name:"Green_Ion_V16_3.ino.bin"}})).statusCode,400);
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
