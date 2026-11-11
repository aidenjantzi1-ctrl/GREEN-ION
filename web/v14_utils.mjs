export const APP_VERSION = "14.0.0-beta.3";
export const REPOSITORY = "aidenjantzi1-ctrl/GREEN-ION";
export const FEATHER_ADDRESS = "http://192.168.4.1";
export function parseVersion(input) {
  const match = /^v?(\d+)\.(\d+)\.(\d+)(?:-([0-9A-Za-z.-]+))?$/.exec(String(input || "").trim());
  return match ? {major:+match[1], minor:+match[2], patch:+match[3], pre:match[4] || ""} : null;
}
export function compareVersions(a,b) {
  const x=parseVersion(a), y=parseVersion(b);
  if (!x || !y) return null;
  for (const key of ["major","minor","patch"]) if (x[key]!==y[key]) return x[key]>y[key]?1:-1;
  if (!x.pre && !y.pre) return 0;
  if (!x.pre) return 1;
  if (!y.pre) return -1;
  const l=x.pre.split("."),r=y.pre.split(".");
  for (let i=0;i<Math.max(l.length,r.length);i++) {
    if (l[i]===undefined) return -1;
    if (r[i]===undefined) return 1;
    const aNum=/^\d+$/.test(l[i]), bNum=/^\d+$/.test(r[i]);
    if(aNum && bNum && +l[i]!==+r[i])return +l[i]>+r[i]?1:-1;
    if(aNum!==bNum)return aNum?-1:1;
    if(l[i]!==r[i])return l[i]>r[i]?1:-1;
  }
  return 0;
}
export function safeFirmwareAsset(asset, releaseTag) {
  if(!asset || typeof asset.browser_download_url!=="string")return false;
  if(!/^v\d+\.\d+\.\d+(?:-[0-9A-Za-z.-]+)?$/.test(releaseTag))return false;
  const prefix="https://github.com/"+REPOSITORY+"/releases/download/"+releaseTag+"/";
  return asset.browser_download_url.startsWith(prefix) &&
    /^[A-Za-z0-9_.-]{1,110}$/.test(asset.name || "") &&
    asset.browser_download_url === prefix + asset.name;
}
export function assetForRelease(release) {
  if(!release || !Array.isArray(release.assets) || !parseVersion(release.tag_name))return null;
  const approved=release.assets.filter(a=>safeFirmwareAsset(a,release.tag_name));
  return approved.find(a=>a.name==="GREEN_ION_V14_COMPLETE.zip") ||
    approved.find(a=>a.name==="Green_Ion_V14.ino.bin") || null;
}
export function fileKind(name) {
  const match=/\.([A-Za-z]{3})$/.exec(String(name||""));
  const kind=match?match[1].toUpperCase():"";
  return ["BIN","SEN","TXT"].includes(kind)?kind:null;
}
export function safeFileName(name) {
  const base=String(name||"").trim();
  if(base.includes("/")||base.includes("\\"))throw Error("Path separators are not allowed in recording names");
  if(!fileKind(base) || !/^[A-Za-z0-9_.-]{1,180}$/.test(base))throw Error("File name must be a simple BIN, SEN or TXT name");
  return base;
}
export function validCoordinate(lat,lon) {
  return Number.isFinite(+lat)&&Number.isFinite(+lon) &&
    +lat>=-90 && +lat<=90 && +lon>=-180 && +lon<=180 &&
    !(+lat===0 && +lon===0);
}
export function receiverPacket(packet) {
  if(!packet || typeof packet!=="object" ||
    !["telemetry","radio_bench"].includes(packet.type))return null;
  const alt=+packet.altitude_m;
  const batt=+packet.battery_v;
  if(!Number.isFinite(alt) || Math.abs(alt)>100000 || !Number.isFinite(batt) || batt<0 || batt>8)return null;
  return {
    ...packet,
    altitude_m:alt,
    battery_v:batt,
    latitude:validCoordinate(packet.latitude,packet.longitude)?+packet.latitude:null,
    longitude:validCoordinate(packet.latitude,packet.longitude)?+packet.longitude:null,
    receivedAt:Date.now()
  };
}
export function radioStatus(packet,lastPosition=null) {
  const gpsCurrent=packet.gps_status==="live" && validCoordinate(packet.latitude,packet.longitude);
  const lat=gpsCurrent?packet.latitude:lastPosition?.[0]??null;
  const lon=gpsCurrent?packet.longitude:lastPosition?.[1]??null;
  return {
    v13_source:"lora",v13_wifi_connected:false,
    recording:false,recorder_mode:-1,flight_state:packet.flight_state,
    relative_altitude_m:packet.altitude_m,pressure_pa:packet.pressure_pa??null,
    battery_v:packet.battery_v,acceleration_g:packet.acceleration_g??null,
    acceleration_x_g:packet.acceleration_x_g??null,acceleration_y_g:packet.acceleration_y_g??null,
    acceleration_z_g:packet.acceleration_z_g??null,
    rotation_x_dps:packet.rotation_x_dps??null,rotation_y_dps:packet.rotation_y_dps??null,
    rotation_z_dps:packet.rotation_z_dps??null,
    vertical_speed_mps:null,gps_speed_mps:packet.speed_mps??null,
    gps_altitude_m:packet.gps_altitude_m??null,
    gps_status:gpsCurrent?"live":validCoordinate(lat,lon)?"stale":"missing",
    latitude:lat,longitude:lon,satellites:packet.satellites??null,
    gps_fix_age_ms:packet.gps_fix_age_ms??null,
    preflight_state:"WIFI_UNAVAILABLE",preflight_flags:0,
    hardware_tests_allowed:false,ota_available:false,
    sd_ok:false,imu_ok:false,barometer_ready:false,
    radio_ok:true,selected_rocket:"LoRa telemetry (profile unavailable)",
    firmware_build:"Feather Wi-Fi unavailable",
    radio_packet_age_ms:Date.now()-(packet.receivedAt||Date.now()),
    radio_rssi_dbm:packet.rssi_dbm??null
  };
}
