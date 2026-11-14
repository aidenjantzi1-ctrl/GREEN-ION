import test from "node:test";
import assert from "node:assert/strict";
import {readFileSync} from "node:fs";

const html=readFileSync(new URL("../web/index.html",import.meta.url),"utf8");
const embedded=readFileSync(new URL("../Green_Ion_V16_3/Dashboard.h",import.meta.url),"utf8");
const firmware=readFileSync(new URL("../Green_Ion_V16_3/Green_Ion_V16_3.ino",import.meta.url),"utf8");
const wifi=readFileSync(new URL("../Green_Ion_V16_3/HomeWifi.h",import.meta.url),"utf8");

for(const [name,source] of [["hosted",html],["embedded",embedded]]){
 test(name+" dashboard exposes complete AP+STA controls",()=>{
  for(const id of [
   "wifiSettingsCard","wifiMode","wifiHomeStatus","wifiDirectAddress","wifiHomeAddress",
   "wifiAuthorize","wifiScan","wifiNetworkList","wifiSsid","wifiPassword","wifiConnect",
   "wifiDisconnectHome","wifiForget","wifiApPassword","wifiSaveApPassword","wifiFinishSetup"
  ]) assert.match(source,new RegExp('id="'+id+'"'),id);
  assert.match(source,/2\.4 GHz/);
  assert.match(source,/5 GHz-only/);
  assert.match(source,/type="password"[^>]*id="wifiPassword"|id="wifiPassword"[^>]*type="password"/);
  assert.match(source,/Saved passwords are never returned/);
  assert.match(source,/greenIonWifiManagement/);
  assert.match(source,/X-Green-Ion-CSRF/);
  assert.match(source,/X-Green-Ion-Management/);
 });

 test(name+" dashboard supports both addresses and copy actions",()=>{
  assert.match(source,/http:\/\/192\.168\.4\.1/);
  assert.match(source,/wifiCopyDirect/);
  assert.match(source,/wifiCopyHome/);
  assert.match(source,/station_ip/);
 });
}

test("firmware restricts cross-origin management and permits same-origin router IP",()=>{
 assert.match(firmware,/origin == "http:\/\/" \+ host/);
 assert.match(firmware,/green-ion-beta\.vercel\.app/);
 assert.match(firmware,/X-Green-Ion-CSRF, X-Green-Ion-Management/);
 assert.match(firmware,/\/api\/wifi\/authorize/);
 assert.match(firmware,/\/api\/wifi\/connect/);
 assert.match(firmware,/\/api\/wifi\/forget/);
});

test("passwords remain write-only",()=>{
 const status=wifi.slice(wifi.indexOf("static void sendHomeWifiStatus()"),wifi.indexOf("static void startHomeWifiScan()"));
 assert.doesNotMatch(status,/homeWifiPassword/);
 assert.doesNotMatch(status,/homeWifiPendingPassword/);
 assert.match(wifi,/persistHomeWifiCredentials/);
 assert.match(wifi,/prefs\.putString\("pass", password\)/);
});
