import test from "node:test";
import assert from "node:assert/strict";
import {readFileSync} from "node:fs";
import {runInNewContext} from "node:vm";

const boot=readFileSync(new URL("../web/v14_boot.js",import.meta.url),"utf8");

function sandboxFor(nativeFetch){
 const win={fetch:nativeFetch,addEventListener:()=>{},dispatchEvent:()=>{}};
 const sandbox={
  window:win,
  location:{hostname:"127.0.0.1",port:"8765",protocol:"http:"},
  Response,Headers,Event,Date,console,setTimeout,clearTimeout
 };
 runInNewContext(boot,sandbox);
 return win;
}

test("V16 Windows transport coalesces simultaneous status polls",async()=>{
 let calls=0,release;
 const gate=new Promise(resolve=>{release=resolve;});
 const win=sandboxFor(async()=>{
  calls++;
  await gate;
  return new Response(JSON.stringify({
   version:14,v13_source:"wifi",v13_wifi_connected:true,
   relative_altitude_m:12,recorder_mode:1,pressure_pa:101000
  }),{status:200,headers:{"Content-Type":"application/json"}});
 });
 const a=win.fetch("/api/status",{cache:"no-store"});
 const b=win.fetch("/api/status",{cache:"no-store"});
 await Promise.resolve();
 assert.equal(calls,1,"overlapping dashboard polls must share one upstream request");
 release();
 const [ra,rb]=await Promise.all([a,b]);
 assert.equal((await ra.json()).v13_source,"wifi");
 assert.equal((await rb.json()).v13_source,"wifi");
 assert.equal(win.GREEN_ION_TRANSPORT.wifiFresh(),true);
 assert.equal(win.GREEN_ION_TRANSPORT.state.statusInFlight,false);
});

test("V16 stale bridge status stays visible without becoming command-authorized Wi-Fi",async()=>{
 const win=sandboxFor(async()=>new Response(JSON.stringify({
  v13_source:"wifi_stale",v13_wifi_connected:false,v16_status_stale:true,
  v16_connection_state:"delayed",v16_status_age_ms:7200,
  relative_altitude_m:42,recorder_mode:3,pressure_pa:100000
 }),{status:200,headers:{"Content-Type":"application/json"}}));
 const response=await win.fetch("/api/status",{cache:"no-store"});
 const data=await response.json();
 assert.equal(data.v13_source,"wifi_stale");
 assert.equal(data.relative_altitude_m,42);
 assert.equal(win.GREEN_ION_TRANSPORT.wifiFresh(),false);
 assert.equal(win.GREEN_ION_TRANSPORT.source(),"none");
});

test("V16 diagnostics show explicit Wi-Fi state and age",()=>{
 const link=readFileSync(new URL("../web/v15_link.mjs",import.meta.url),"utf8");
 const html=readFileSync(new URL("../web/index.html",import.meta.url),"utf8");
 assert.match(link,/\/api\/v16\/telemetry/);
 assert.match(link,/connection_state/);
 assert.match(link,/active_operation/);
 assert.match(html,/id="v15LinkWifi"/);
 assert.match(html,/GREEN ION V16/);
});

test("V16.2 direct browser keeps brief Wi-Fi dropout visible but blocks commands",async()=>{
 let fail=false;
 const win=sandboxFor(async()=>{
  if(fail)throw Error("brief timeout");
  return new Response(JSON.stringify({
   version:16,v13_source:"wifi",v13_wifi_connected:true,
   relative_altitude_m:21,recorder_mode:1,pressure_pa:100900
  }),{status:200,headers:{"Content-Type":"application/json"}});
 });
 await win.fetch("/api/status",{cache:"no-store"});
 assert.equal(win.GREEN_ION_TRANSPORT.wifiFresh(),true);
 assert.equal(win.GREEN_ION_TRANSPORT.controlsVerified(),true);
 fail=true;
 await assert.rejects(()=>win.fetch("/api/status",{cache:"no-store"}));
 assert.equal(win.GREEN_ION_TRANSPORT.wifiFresh(),true,
  "one failed poll must not erase the last visible observation");
 assert.equal(win.GREEN_ION_TRANSPORT.controlsVerified(),false,
  "a failed status poll must immediately pause hardware commands");
 const blocked=await win.fetch("/api/delete?name=V12D001.BIN",{method:"POST"});
 assert.equal(blocked.status,503);
});
