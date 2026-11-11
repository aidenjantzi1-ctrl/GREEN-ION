/* GIS1 v1, 32-byte header and 68-byte CRC32-guarded sensor records.
 * Implements libraries/GreenIonRecorder/src/GreenIonSensorLog.h without modification. */
export const GIS1_HEADER_BYTES=32,GIS1_RECORD_BYTES=68,MAX_GIS1_SAMPLES=125000;
export function crc32(bytes){
 const raw=bytes instanceof Uint8Array?bytes:new Uint8Array(bytes);
 let crc=0xffffffff;
 for(let i=0;i<raw.length;i++){
  crc=(crc^raw[i])>>>0;
  for(let bit=0;bit<8;bit++)crc=((crc>>>1)^((crc&1)?0xedb88320:0))>>>0;
 }
 return (~crc)>>>0;
}
export function parseGis1(input){
 const raw=input instanceof Uint8Array?input:new Uint8Array(input);
 if(raw.byteLength<GIS1_HEADER_BYTES)throw Error("GIS1 file is shorter than 32-byte header");
 const v=new DataView(raw.buffer,raw.byteOffset,raw.byteLength);
 if(String.fromCharCode(...raw.subarray(0,4))!=="GIS1")throw Error("Invalid GIS1 magic");
 const version=v.getUint16(4,true),headerBytes=v.getUint16(6,true);
 const recordBytes=v.getUint16(8,true),rateHz=v.getUint16(10,true),flightSequence=v.getUint16(12,true);
 if(version!==1||headerBytes!==GIS1_HEADER_BYTES||recordBytes!==GIS1_RECORD_BYTES)
  throw Error("Unsupported GIS1 v1 format");
 if(![10,25,50].includes(rateHz)||flightSequence>999)throw Error("Invalid GIS1 sample rate or flight number");
 if(v.getUint32(28,true)!==crc32(raw.subarray(0,28)))throw Error("GIS1 header CRC32 mismatch");
 const count=Math.floor((raw.length-GIS1_HEADER_BYTES)/GIS1_RECORD_BYTES);
 const trailingBytes=(raw.length-GIS1_HEADER_BYTES)%GIS1_RECORD_BYTES;
 const warnings=[];
 if(trailingBytes)warnings.push(trailingBytes+" truncated trailing bytes");
 if(count>MAX_GIS1_SAMPLES)warnings.push("Only first "+MAX_GIS1_SAMPLES+" records decoded (bounded memory)");
 const samples=[];let invalidIndex=null,validImu=0,currentGps=0,peakTotalG=0,maxSpeedMps=0,maxSatellites=0;
 const limit=Math.min(count,MAX_GIS1_SAMPLES);
 const toDeg=180/Math.PI,gravity=9.80665;
 for(let index=0;index<limit;index++){
  const o=GIS1_HEADER_BYTES+index*GIS1_RECORD_BYTES;
  if(v.getUint32(o+64,true)!==crc32(raw.subarray(o,o+64))){invalidIndex=index;warnings.push("CRC32 mismatch at sensor record "+index+"; only verified prefix decoded");break;}
  const flags=v.getUint16(o+56,true),sat=v.getUint8(o+58),hour=v.getUint8(o+59),minute=v.getUint8(o+60),second=v.getUint8(o+61);
  const imu=(flags&1)!==0,position=(flags&2)!==0,gpsCurrent=(flags&4)!==0;
  const latE7=v.getInt32(o+32,true),lonE7=v.getInt32(o+36,true);
  const ax=v.getFloat32(o+8,true),ay=v.getFloat32(o+12,true),az=v.getFloat32(o+16,true);
  const gx=v.getFloat32(o+20,true),gy=v.getFloat32(o+24,true),gz=v.getFloat32(o+28,true);
  const gpsAlt=v.getFloat32(o+40,true),gpsSpeed=v.getFloat32(o+44,true);
  if(flags&~7||sat>64||hour>23||minute>59||second>60||gpsCurrent&&!position||
    imu&&![ax,ay,az,gx,gy,gz].every(Number.isFinite)||
    position&&(latE7<-900000000||latE7>900000000||lonE7<-1800000000||lonE7>1800000000||
      !Number.isFinite(gpsAlt)||!Number.isFinite(gpsSpeed))||
    !position&&(latE7!==0||lonE7!==0||gpsAlt!==0||gpsSpeed!==0||
      v.getUint32(o+52,true)!==0||v.getUint32(o+48,true)!==0xffffffff)){
   invalidIndex=index;warnings.push("Invalid sensor values or flags at record "+index+"; only verified prefix decoded");break;
  }
  const magnitudeG=imu?Math.hypot(ax,ay,az)/gravity:null;
  const sample={
   index,timeS:v.getUint32(o,true)/1e6,sequence:v.getUint32(o+4,true),
   imuValid:imu,gpsHasPosition:position,gpsCurrent,
   accelerationXG:imu?ax/gravity:null,accelerationYG:imu?ay/gravity:null,
   accelerationZG:imu?az/gravity:null,accelerationG:magnitudeG,
   gyroXDps:imu?gx*toDeg:null,gyroYDps:imu?gy*toDeg:null,gyroZDps:imu?gz*toDeg:null,
   latitude:position?latE7/1e7:null,longitude:position?lonE7/1e7:null,
   gpsAltitudeM:position?gpsAlt:null,gpsSpeedMps:position?gpsSpeed:null,
   gpsFixAgeMs:position?v.getUint32(o+48,true):null,
   gpsFixUptimeMs:position?v.getUint32(o+52,true):null,
   satellites:sat,utcHour:hour,utcMinute:minute,utcSecond:second
  };
  if(imu){validImu++;peakTotalG=Math.max(peakTotalG,magnitudeG);}
  if(gpsCurrent){currentGps++;maxSpeedMps=Math.max(maxSpeedMps,gpsSpeed);}
  maxSatellites=Math.max(maxSatellites,sat);
  samples.push(sample);
 }
 return {format:"GIS1",version,rateHz,flightSequence,headerCrc32:v.getUint32(28,true),
  rkt4HeaderCrc32:v.getUint32(20,true),startTimestampUs:v.getUint32(16,true),
  count,decoded:samples.length,trailingBytes,invalidIndex,warnings,samples,
  summary:{validImu,currentGps,peakTotalG,maxSpeedMps,maxSatellites}};
}
export const SENSOR_GRAPH_METRICS={
 accelerationXG:["Acceleration X","g"],accelerationYG:["Acceleration Y","g"],
 accelerationZG:["Acceleration Z","g"],accelerationG:["Acceleration magnitude","g"],
 gyroXDps:["Gyroscope X","°/s"],gyroYDps:["Gyroscope Y","°/s"],gyroZDps:["Gyroscope Z","°/s"],
 gpsAltitudeM:["GPS altitude","m"],gpsSpeedMps:["GPS ground speed","m/s"],
 satellites:["Satellites","count"],gpsFixAgeMs:["GPS age","ms"],
 latitude:["GPS latitude","°"],longitude:["GPS longitude","°"]
};
export function sensorCsv(report){
 const keys=["timeS","sequence","imuValid","accelerationXG","accelerationYG","accelerationZG",
 "accelerationG","gyroXDps","gyroYDps","gyroZDps","gpsCurrent","latitude","longitude",
 "gpsAltitudeM","gpsSpeedMps","gpsFixAgeMs","satellites","utcHour","utcMinute","utcSecond"];
 const quote=value=>value===null||value===undefined?"":String(value);
 return [keys.join(","),...report.samples.map(s=>keys.map(k=>quote(s[k])).join(","))].join("\r\n");
}
export function gpsTrackGeoJson(report){
 const coordinates=[];let lastT=-Infinity;
 for(const s of report.samples){
  if(!s.gpsCurrent||s.latitude===null||s.longitude===null||s.timeS-lastT<1)continue;
  coordinates.push([s.longitude,s.latitude,s.gpsAltitudeM??0]);
  lastT=s.timeS;
 }
 return {type:"FeatureCollection",features:coordinates.length>=2?[{
  type:"Feature",properties:{source:"GREEN ION verified current GPS only",points:coordinates.length},
  geometry:{type:"LineString",coordinates}
 }]:[]};
}
