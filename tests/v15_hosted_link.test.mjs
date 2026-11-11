import test from "node:test";
import assert from "node:assert/strict";
import {readFileSync} from "node:fs";
import {runInNewContext} from "node:vm";

const boot=readFileSync(new URL("../web/v14_boot.js",import.meta.url),"utf8");
const firmware=readFileSync(new URL("../Green_Ion_V14/Green_Ion_V14.ino",import.meta.url),"utf8");
const page=readFileSync(new URL("../web/index.html",import.meta.url),"utf8");

test("hosted dashboard directs only Feather API requests to private network with LNA opt-in",async()=>{
 const calls=[];
 const native=async(url,options)=>{
  calls.push({url,options});
  return new Response(JSON.stringify({
   version:14,v13_source:"wifi",v13_wifi_connected:true,
   relative_altitude_m:0,recorder_mode:1
  }),{status:200,headers:{"Content-Type":"application/json"}});
 };
 const window={fetch:native,dispatchEvent(){}};
 const location={hostname:"green-ion-beta.vercel.app",protocol:"https:",port:""};
 runInNewContext(boot,{window,location,Response,Event:class{constructor(type){this.type=type;}},Date});
 const response=await window.fetch("/api/status",{cache:"no-store"});
 assert.equal(response.status,200);
 assert.equal(calls[0].url,"http://192.168.4.1/api/status");
 assert.equal(calls[0].options.targetAddressSpace,"local");
 assert.equal(window.GREEN_ION_TRANSPORT.wifiFresh(),true);
 await window.fetch("https://bqvvtbnjthbiqyljmzfx.supabase.co/auth/v1/token");
 assert.equal(calls[1].url,"https://bqvvtbnjthbiqyljmzfx.supabase.co/auth/v1/token");
 assert.equal(calls[1].options,undefined);
});

test("unverified Feather control requests remain blocked",async()=>{
 const calls=[];
 const window={fetch:async(...args)=>{calls.push(args);return new Response("{}",{status:200});},dispatchEvent(){}};
 runInNewContext(boot,{window,location:{hostname:"green-ion-beta.vercel.app",port:""},Response,Event:class{},Date});
 const response=await window.fetch("/api/stop",{method:"POST"});
 assert.equal(response.status,503);
 assert.equal(calls.length,0);
});

test("Feather cross-origin access is restricted to production domain and implements preflight",()=>{
 assert.match(firmware,/origin != "https:\/\/green-ion-beta\.vercel\.app"/);
 assert.match(firmware,/server\.method\(\) == HTTP_OPTIONS/);
 assert.match(firmware,/Access-Control-Allow-Private-Network/);
 assert.doesNotMatch(firmware,/dashboardServer\.enableCORS\(true\)/);
});

test("hosting presents an explicit user action and truthful phone fallback",()=>{
 assert.match(page,/id="v15ConnectFeather"/);
 assert.match(page,/id="v15ConnectGuide"/);
 assert.match(page,/Safari may block/);
 assert.match(page,/reconnect to internet/);
});
