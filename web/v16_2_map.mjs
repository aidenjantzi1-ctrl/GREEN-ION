/* GREEN ION V16.2 optional embedded Google Maps recovery view.
 * Raw confirmed rocket fixes remain authoritative; offline recovery never depends on this module. */
const $=id=>document.getElementById(id);
let map=null,track=null,rocketCircle=null,userCircle=null,launchCircle=null,radioCircle=null,lastDetail=null,loading=null;
function config(){return window.GREEN_ION_CONFIG||{};}
function hasInternet(){return navigator.onLine!==false;}
function addScript(key){
 if(globalThis.google?.maps)return Promise.resolve();
 if(loading)return loading;
 loading=new Promise((resolve,reject)=>{
  const script=document.createElement("script");
  script.src="https://maps.googleapis.com/maps/api/js?key="+encodeURIComponent(key)+"&v=weekly&loading=async";
  script.async=true;script.defer=true;script.referrerPolicy="no-referrer-when-downgrade";
  script.onload=()=>resolve();script.onerror=()=>reject(Error("Google Maps JavaScript could not load"));
  document.head.append(script);
 });
 return loading;
}
async function ensureMap(){
 const key=String(config().googleMapsBrowserKey||"").trim(),status=$("v162MapStatus");
 if(!key){
  status.textContent="Embedded Google Map is not configured. Offline GPS plot, coordinates, distance/bearing and Open in Google Maps still work.";
  return false;
 }
 if(!hasInternet()){
  status.textContent="Internet is offline. Using the local recovery plot; confirmed coordinates remain available.";
  return false;
 }
 try{
  await addScript(key);
  const {Map}=await google.maps.importLibrary("maps");
  if(!map){
   map=new Map($("v162RecoveryMap"),{
    center:{lat:0,lng:0},zoom:18,mapTypeId:"satellite",
    mapTypeControl:true,mapTypeControlOptions:{
     mapTypeIds:["roadmap","terrain","satellite","hybrid"]
    },
    streetViewControl:false,fullscreenControl:true,gestureHandling:"greedy"
   });
  }
  status.textContent="Embedded Google Map ready. Google may process map and location requests according to its service terms.";
  return true;
 }catch(error){
  status.textContent="Embedded map unavailable: "+error.message+". Offline recovery remains active.";
  return false;
 }
}
function clearShape(shape){if(shape)shape.setMap(null);return null;}
async function render(detail){
 lastDetail=detail||lastDetail;if(!lastDetail)return;
 const latest=lastDetail.latest,status=$("v162MapEvidence");
 if(status){
  if(!latest)status.textContent="No confirmed rocket GPS fix received.";
  else{
   const age=Math.max(0,Number(latest.ageMs)||0),live=age<=5000;
   status.textContent=(live?"LIVE CONFIRMED FIX":"LAST KNOWN FIX")+" · "+latest.source+
    " · "+(age/1000).toFixed(1)+" s old"+
    (Number.isFinite(latest.satellites)?" · "+latest.satellites+" satellites":"")+
    (Number.isFinite(latest.hdop)?" · HDOP "+latest.hdop.toFixed(1)+" (dilution, not meter accuracy)":"");
  }
 }
 if(!latest||!(await ensureMap()))return;
 const path=(lastDetail.fixes||[]).filter(x=>Number.isFinite(x.lat)&&Number.isFinite(x.lon))
  .map(x=>({lat:x.lat,lng:x.lon}));
 if(track)track.setMap(null);
 track=new google.maps.Polyline({map,path,geodesic:true,strokeColor:"#7bdbff",strokeOpacity:.85,strokeWeight:3});
 launchCircle=clearShape(launchCircle);
 radioCircle=clearShape(radioCircle);
 const prelaunch=(lastDetail.fixes||[]).find(f=>f.flightState===2&&Number.isFinite(f.lat)&&Number.isFinite(f.lon));
 if(prelaunch)launchCircle=new google.maps.Circle({
  map,center:{lat:prelaunch.lat,lng:prelaunch.lon},radius:7,strokeColor:"#72e5ab",
  strokeOpacity:1,strokeWeight:2,fillColor:"#72e5ab",fillOpacity:.35
 });
 const radio=[...(lastDetail.fixes||[])].reverse().find(f=>f.source==="LoRa"&&Number.isFinite(f.lat)&&Number.isFinite(f.lon));
 if(radio)radioCircle=new google.maps.Circle({
  map,center:{lat:radio.lat,lng:radio.lon},radius:7,strokeColor:"#d2abff",
  strokeOpacity:1,strokeWeight:2,fillColor:"#d2abff",fillOpacity:.3
 });
 rocketCircle=clearShape(rocketCircle);
 rocketCircle=new google.maps.Circle({
  map,center:{lat:latest.lat,lng:latest.lon},radius:6,strokeColor:"#ffdd88",
  strokeOpacity:1,strokeWeight:3,fillColor:"#ffdd88",fillOpacity:.42
 });
 userCircle=clearShape(userCircle);
 if(lastDetail.userPosition&&Number.isFinite(lastDetail.userPosition.lat)){
  const u=lastDetail.userPosition,accuracy=Math.max(4,Math.min(1000,Number(u.accuracy)||8));
  userCircle=new google.maps.Circle({
   map,center:{lat:u.lat,lng:u.lon},radius:accuracy,strokeColor:"#72e5ab",
   strokeOpacity:.9,strokeWeight:2,fillColor:"#72e5ab",fillOpacity:.18
  });
  const bounds=new google.maps.LatLngBounds();
  bounds.extend({lat:latest.lat,lng:latest.lon});bounds.extend({lat:u.lat,lng:u.lon});map.fitBounds(bounds,70);
 }else map.setCenter({lat:latest.lat,lng:latest.lon});
}
function inject(){
 const card=$("v15RecoveryCard");if(!card||$("v162RecoveryMapCard"))return;
 const block=document.createElement("div");block.id="v162RecoveryMapCard";block.className="v162-map-block";
 block.innerHTML='<h3>Embedded recovery map</h3>'+
  '<p class="muted">Uses only confirmed received rocket GPS fixes. A reported landing phase does not create or move a GPS marker.</p>'+
  '<div id="v162RecoveryMap" class="v162-map" role="application" aria-label="Interactive Google map showing confirmed rocket GPS trail and optional user location"></div>'+
  '<div class="row"><span>Map evidence</span><strong id="v162MapEvidence">No confirmed rocket GPS fix received.</strong></div>'+
  '<p class="muted">Yellow: latest confirmed GPS; green: recorded prelaunch fix (when available); purple: last valid LoRa position, which may be stale.</p>'+
  '<p id="v162MapStatus" class="muted" role="status">Checking optional Google Maps configuration...</p>'+
  '<details><summary>Map privacy and fallback</summary><p class="muted">Your current location is requested only after you choose Use my current location and is not added to flight history. If the embedded map is enabled, Google Maps receives map requests and may process location information needed to display/navigation features. If no key or internet is available, GREEN ION keeps the local offline plot, raw coordinates, distance/bearing and external directions link.</p></details>';
 const offline=$("v15RecoveryTrack");if(offline)offline.after(block);else card.append(block);
}
export function initV162Map(){
 inject();
 window.addEventListener("green-ion-v16-recovery",e=>render(e.detail));
 window.addEventListener("online",()=>render(lastDetail));
 window.addEventListener("offline",()=>{if($("v162MapStatus"))$("v162MapStatus").textContent="Internet is offline. Offline GPS recovery remains active.";});
 ensureMap().catch(()=>{});
}
