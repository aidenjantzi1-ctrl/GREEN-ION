import test from "node:test";
import assert from "node:assert/strict";
import {SENSOR_GROUPS,SENSOR_KEYS,sensorRows,safeReading} from "../web/v14_sensor_catalog.mjs";
import {DEFAULTS,getPreferences,setPreference,preference} from "../web/v14_preferences.mjs";
test("existing Feather and Uno fields are mapped without inventing unavailable values",()=>{
 const all=[...SENSOR_KEYS];
 for(const name of ["pressure_pa","temperature_c","acceleration_x_g","acceleration_y_g",
  "acceleration_z_g","rotation_x_dps","rotation_y_dps","rotation_z_dps","gps_speed_mps",
  "gps_fix_age_ms","gps_nmea_sentences","satellites","battery_v","missed_sample_periods",
  "radio_rssi_dbm","radio_send_failures","imu_read_failures"])assert.ok(all.includes(name),name);
 assert.ok(SENSOR_GROUPS.length>=6);
 const groups=sensorRows({battery_v:0,pressure_pa:null,imu_data_valid:false},null,false);
 const rows=groups.flatMap(g=>g.rows);
 assert.equal(rows.find(r=>r.key==="battery_v").formatted,"0.000 V");
 assert.equal(rows.find(r=>r.key==="imu_data_valid").formatted,"NO");
 assert.equal(rows.find(r=>r.key==="pressure_pa").formatted,"—");
 assert.equal(rows.find(r=>r.key==="gps_speed_mps").present,false);
 const radio=sensorRows({}, {rssi_dbm:-89,estimated_missing_packets:3},false).flatMap(g=>g.rows);
 assert.equal(radio.find(r=>r.key==="rssi_dbm").formatted,"-89 dBm");
});
test("imperial conversion is optional and does not modify raw readings",()=>{
 assert.equal(safeReading(100,"m",1,true),"328.1 ft");
 assert.equal(safeReading(10,"m/s",2,true),"22.37 mph");
 assert.equal(safeReading(20,"°C",1,true),"68.0 °F");
 assert.equal(safeReading(null,"V",2,true),"—");
 assert.equal(safeReading(Infinity,"V",2,true),"—");
});
test("feature flags can be turned off independently with default preserved",()=>{
 assert.equal(DEFAULTS.sensorLab,true);
 assert.equal(DEFAULTS.cloudAuto,true);
 assert.equal(DEFAULTS.sensorArchive,true);
 const before=getPreferences();
 try{
   setPreference("sensorLab",false);
   assert.equal(preference("sensorLab"),false);
   assert.equal(preference("cloudAuto"),before.cloudAuto);
   assert.throws(()=>setPreference("deleteFiles",true));
 }finally{setPreference("sensorLab",before.sensorLab);}
});
