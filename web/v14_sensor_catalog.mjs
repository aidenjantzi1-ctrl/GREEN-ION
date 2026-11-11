/* V14 sensor contract: values are taken only from real status/radio fields.
   Missing fields stay missing. All radio/phase estimates are labeled as such. */
export const SENSOR_GROUPS=Object.freeze([
 {name:"Barometer & flight",fields:[
  ["pressure_pa","Air pressure","Pa",0],
  ["temperature_c","Barometer temperature","°C",2],
  ["altitude_m","Pressure altitude (estimated)","m",2],
  ["relative_altitude_m","Relative altitude (estimated)","m",2],
  ["vertical_speed_mps","Pressure vertical speed (estimated)","m/s",2],
  ["landing_peak_m","Peak altitude observed","m",2],
  ["calibration_baseline_pa","Calibration baseline","Pa",1],
  ["calibration_noise_m","Calibration noise","m",3],
  ["calibration_age_s","Calibration age","s",0],
  ["barometer_runtime_losses","Barometer runtime failures","count",0],
  ["barometer_recoveries","Barometer recoveries","count",0],
  ["barometer_init_failures","Barometer initialization failures","count",0],
  ["barometer_ready","Barometer ready","bool"]
 ]},
 {name:"IMU · LSM6DSO32",fields:[
  ["acceleration_x_g","Acceleration X","g",3],
  ["acceleration_y_g","Acceleration Y","g",3],
  ["acceleration_z_g","Acceleration Z","g",3],
  ["acceleration_g","Acceleration magnitude","g",3],
  ["rotation_x_dps","Gyroscope X","°/s",2],
  ["rotation_y_dps","Gyroscope Y","°/s",2],
  ["rotation_z_dps","Gyroscope Z","°/s",2],
  ["imu_temperature_c","IMU temperature","°C",2],
  ["imu_ok","IMU connected","bool"],
  ["imu_data_valid","IMU current sample valid","bool"],
  ["imu_read_failures","IMU read failures","count",0],
  ["imu_consecutive_read_failures","IMU consecutive failures","count",0],
  ["imu_range_32g","32 g mode verified","bool"],
  ["imu_test_result","IMU bench-test result","text"],
  ["imu_test_samples","IMU test sample count","count",0],
  ["imu_test_invalid","IMU test invalid samples","count",0],
  ["imu_test_mean_g","IMU test mean acceleration","g",3],
  ["imu_test_bias_x_dps","Gyro bias X","°/s",3],
  ["imu_test_bias_y_dps","Gyro bias Y","°/s",3],
  ["imu_test_bias_z_dps","Gyro bias Z","°/s",3],
  ["imu_test_temperature_c","IMU test temperature","°C",2]
 ]},
 {name:"GPS · PA1010D",fields:[
  ["gps_status","GPS position","text"],
  ["latitude","Latitude","°",7],
  ["longitude","Longitude","°",7],
  ["gps_altitude_m","GPS altitude","m",2],
  ["gps_speed_mps","GPS ground speed","m/s",2],
  ["gps_course_deg","Ground-track course (moving only)","°",1],
  ["gps_fix_quality","GPS fix quality code","count",0],
  ["gps_hdop","Horizontal dilution of precision","count",2],
  ["gps_geoid_separation_m","Geoid/WGS84 separation","m",2],
  ["satellites","Satellites reported","count",0],
  ["gps_fix_age_ms","GPS fix age","ms",0],
  ["gps_utc_hour","GPS UTC hour","count",0],
  ["gps_utc_minute","GPS UTC minute","count",0],
  ["gps_utc_second","GPS UTC second","count",0],
  ["gps_communication","GPS serial link","text"],
  ["gps_nmea_sentences","NMEA sentences received","count",0],
  ["gps_parsed_sentences","NMEA sentences parsed","count",0],
  ["gps_parse_failures","NMEA parse failures","count",0],
  ["gps_nmea_age_ms","Latest NMEA sentence age","ms",0],
  ["gps_ok","GPS module connected","bool"],
  ["gps_test_result","GPS bench-test result","text"],
  ["gps_test_nmea","GPS test NMEA count","count",0],
  ["gps_test_parsed","GPS test parsed count","count",0]
 ]},
 {name:"Power, storage & recorder",fields:[
  ["battery_v","Battery voltage","V",3],
  ["battery_mv","Battery voltage (raw)","mV",0],
  ["sd_ok","microSD mounted","bool"],
  ["recording","Recording now","bool"],
  ["recorder_mode","Recorder mode code","count",0],
  ["recorder_status","Recorder status code","count",0],
  ["sample_rate_hz","Sample rate","Hz",0],
  ["prebuffer_records","Prelaunch buffer","count",0],
  ["missed_sample_periods","Missed sample periods","count",0],
  ["sensor_log_pending","Pending sensor records","count",0],
  ["sensor_log_dropped","Dropped sensor records","count",0],
  ["sensor_log_failures","Sensor log failures","count",0],
  ["metadata_failures","Metadata write failures","count",0],
  ["checkpoint_generation","Checkpoint generation","count",0],
  ["checkpoint_failures","Checkpoint failures","count",0],
  ["finalization_failures","Finalization failures","count",0],
  ["recovered_logs","Recovered flight logs","count",0],
  ["recovered_records","Recovered RKT4 samples","count",0],
  ["recovery_pending","Recovery operations pending","count",0],
  ["recovery_status","Recovery status","text"],
  ["uptime_s","Feather uptime","s",0],
  ["flight_state","Flight phase code","count",0],
  ["preflight_state","Preflight state","text"],
  ["preflight_flags","Preflight flags","count",0]
 ]},
 {name:"LoRa radio & receiver",fields:[
  ["radio_ok","Feather radio initialized","bool"],
  ["radio_rssi_dbm","Received signal strength","dBm",0],
  ["radio_packet_age_ms","Last radio packet age","ms",0],
  ["radio_send_failures","Feather radio TX failures","count",0],
  ["radio_bench_packets_sent","Feather bench packets sent","count",0],
  ["home_radio_sent","Home test packets sent","count",0],
  ["home_radio_failures","Home test transmit failures","count",0],
  ["estimated_missing_packets","Estimated radio packets missed","count",0],
  ["valid_packets","Valid receiver packets","count",0],
  ["invalid_packets","Invalid receiver packets","count",0],
  ["duplicates","Duplicate receiver packets","count",0],
  ["out_of_order","Out-of-order receiver packets","count",0],
  ["sender_restarts","Detected sender restarts","count",0],
  ["bench_received","Receiver bench packets","count",0],
  ["sequence","Last radio sequence","count",0],
  ["rssi_dbm","Uno receiver RSSI","dBm",0],
  ["packet_age_ms","Receiver-reported packet age","ms",0]
 ]},
 {name:"System & diagnostics",fields:[
  ["health","Health bitfield","count",0],
  ["hardware_tests_allowed","Hardware tests permitted","bool"],
  ["ota_available","OTA partition available","bool"],
  ["maintenance_mode","Maintenance mode","bool"],
  ["update_in_progress","OTA in progress","bool"],
  ["home_test_active","Home test running","bool"],
  ["home_interrupted","Interrupted previous test","bool"],
  ["home_record_samples","Home test sensor samples","count",0],
  ["home_record_valid_samples","Home test valid samples","count",0],
  ["home_record_missed","Home test missed periods","count",0],
  ["home_record_wifi_absent","Wi-Fi loss observed","bool"],
  ["home_record_cleanup_ok","Temporary data cleaned","bool"],
  ["imu_test_message","IMU diagnostic message","text"],
  ["gps_test_message","GPS diagnostic message","text"],
  ["firmware_build","Firmware build timestamp","text"],
  ["selected_rocket","Rocket profile","text"],
  ["selected_motor","Motor profile","text"]
 ]}
]);
export const SENSOR_KEYS=new Set(SENSOR_GROUPS.flatMap(g=>g.fields.map(f=>f[0])));
export function safeReading(value,unit="text",precision=2,imperial=false){
 if(value===null||value===undefined||value==="")return "—";
 if(unit==="bool")return typeof value==="boolean"?(value?"YES":"NO"):"—";
 if(unit==="text")return typeof value==="string" ? value.slice(0,120) : "—";
 if(typeof value!=="number" || !Number.isFinite(value))return "—";
 let n=value,u=unit;
 if(imperial){
  if(unit==="m"){n=value*3.28084;u="ft";}
  if(unit==="m/s"){n=value*2.236936;u="mph";}
  if(unit==="°C"){n=value*9/5+32;u="°F";}
 }
 return n.toFixed(precision)+" "+u;
}
export function sensorRows(status={},radio=null,imperial=false){
 const data={...(status||{})};
 if(radio&&typeof radio==="object"){
  for(const key of ["rssi_dbm","estimated_missing_packets","valid_packets","invalid_packets",
   "duplicates","out_of_order","sender_restarts","bench_received","sequence","packet_age_ms"]){
    if(Object.prototype.hasOwnProperty.call(radio,key))data[key]=radio[key];
  }
 }
 return SENSOR_GROUPS.map(group=>({
  name:group.name,
  rows:group.fields.map(([key,label,unit,precision=2])=>({
   key,label,unit,present:Object.prototype.hasOwnProperty.call(data,key)&&data[key]!==null&&data[key]!==undefined,
   raw:data[key]??null,
   formatted:safeReading(data[key],unit,precision,imperial)
  }))
 }));
}
