import {parseGis1,sensorCsv,gpsTrackGeoJson} from "./analysis/gis1.mjs";
self.onmessage=event=>{
 try{
  const {id,bytes,name}=event.data;
  const parsed=parseGis1(new Uint8Array(bytes));
  self.postMessage({id,name,parsed,csv:sensorCsv(parsed),geojson:gpsTrackGeoJson(parsed)});
 }catch(e){self.postMessage({id:event.data?.id,error:e.message});}
};
