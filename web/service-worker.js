/* Only cache same-origin app assets, never sensor endpoints, auth or cloud data. */
const VERSION="green-ion-v16-1-reliability-simplicity-1";
const ASSETS=["./","./index.html","./config.js","./manifest.webmanifest","./icon.svg",
 "./v14_boot.js","./v14_layout.css","./v14_ui.mjs","./v14_app.mjs","./v14_utils.mjs","./v14_serial.mjs",
 "./v14_store.mjs","./v14_preferences.mjs","./v14_sensor_catalog.mjs","./v14_sensor_lab.mjs","./v14_sensor_history.mjs","./sensor-worker.mjs","./analysis/gis1.mjs","./v14_analysis.mjs","./v14_cloud.mjs","./v14_updates.mjs",
 "./history-db.mjs","./analysis-worker.mjs","./analysis/rkt4.mjs",
 "./v15_recovery.mjs","./v15_advanced.mjs","./v15_analysis_core.mjs","./v15_report_worker.mjs",
 "./v15_local.mjs","./v15_battery.mjs","./v15_link.mjs","./v15_help.mjs","./v16_telemetry.mjs","./v16_mission.mjs","./v16_layout.mjs","./v16_navigation.mjs","./v16_data.mjs","./v16_runtime.mjs","./v16_visualization.mjs","./v16_visualization_worker.mjs","./v16_validation.mjs","./v16_1_diagnostics.mjs","./v16_1_guided_test.mjs","./v16_1_analysis_core.mjs"];
self.addEventListener("install",event=>{
 event.waitUntil(caches.open(VERSION).then(cache=>cache.addAll(ASSETS)).then(()=>self.skipWaiting()));
});
self.addEventListener("activate",event=>{
 event.waitUntil(caches.keys().then(names=>Promise.all(names.filter(name=>name.startsWith("green-ion-")&&name!==VERSION).map(name=>caches.delete(name))))
  .then(()=>self.clients.claim()));
});
self.addEventListener("fetch",event=>{
 const request=event.request,url=new URL(request.url);
 if(request.method!=="GET"||url.origin!==self.location.origin||url.pathname.startsWith("/api/"))return;
 event.respondWith(fetch(request).then(async response=>{
  if(response.ok){const cache=await caches.open(VERSION);cache.put(request,response.clone()).catch(()=>{});}
  return response;
 }).catch(async()=>{
  const cache=await caches.open(VERSION);
  return await cache.match(request)||((request.mode==="navigate")?await cache.match("./index.html"):Response.error());
 }));
});
