"use strict";
const VERSION="green-ion-v18-offline-app-v7";
const APP="./v18-offline.html";
const ASSETS=[
 APP,
 "./v18_offline_app.mjs",
 "./v18_offline_core.mjs",
 "./v18.webmanifest",
 "./icon.svg",
 "./analysis/rkt4.mjs",
 "./v14_store.mjs",
 "./v14_utils.mjs"
];
self.addEventListener("install",event=>{
 event.waitUntil(caches.open(VERSION).then(cache=>cache.addAll(ASSETS)).then(()=>self.skipWaiting()));
});
self.addEventListener("activate",event=>{
 event.waitUntil(caches.keys().then(keys=>Promise.all(keys.filter(k=>k.startsWith("green-ion-v18-")&&k!==VERSION).map(k=>caches.delete(k)))).then(()=>self.clients.claim()));
});
self.addEventListener("fetch",event=>{
 const request=event.request,url=new URL(request.url);
 if(request.method!=="GET"||url.origin!==self.location.origin||url.pathname.startsWith("/api/"))return;
 if(request.mode==="navigate"){
  event.respondWith(fetch(request).then(response=>{
   if(response.ok)caches.open(VERSION).then(cache=>cache.put(APP,response.clone())).catch(()=>{});
   return response;
  }).catch(()=>caches.match(APP)));
  return;
 }
 event.respondWith(caches.match(request).then(cached=>cached||fetch(request).then(response=>{
  if(response.ok)caches.open(VERSION).then(cache=>cache.put(request,response.clone())).catch(()=>{});
  return response;
 })));
});
