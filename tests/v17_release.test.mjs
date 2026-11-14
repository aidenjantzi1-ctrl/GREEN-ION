import test from "node:test";
import assert from "node:assert/strict";
import {APP_VERSION,assetForRelease,firmwareImageNameForTag,releaseFirmwarePackage,checkFirmwareManifest} from "../web/v14_utils.mjs";

const tag="v17.0.1";
const base="https://github.com/aidenjantzi1-ctrl/GREEN-ION/releases/download/"+tag+"/";
const image={name:"Green_Ion_V17.ino.bin",size:200100,browser_download_url:base+"Green_Ion_V17.ino.bin"};
const manifest={name:"SHA256_MANIFEST.json",browser_download_url:base+"SHA256_MANIFEST.json"};
const release={tag_name:tag,assets:[image,manifest]};

test("V17 is the primary software generation, retains older manifest readers",()=>{
  assert.equal(APP_VERSION,"17.0.1");
  assert.equal(firmwareImageNameForTag(tag),image.name);
  assert.equal(firmwareImageNameForTag("v16.3.0"),"Green_Ion_V16_3.ino.bin");
  assert.deepEqual(assetForRelease(release),image);
  assert.deepEqual(releaseFirmwarePackage(release),{image,manifest});
});

test("V17 direct OTA accepts only matching signed-format metadata and official URLs",()=>{
  const payload={tag,version:tag,board:"Adafruit Feather ESP32 V2",
    firmwareFormat:"ESP32 .ino.bin app image",firmwareGeneration:"V17.0",
    radioProtocolVersion:2,radioPacketBytes:70,
    artifacts:{[image.name]:{size:image.size,sha256:"a".repeat(64)}}};
  assert.deepEqual(checkFirmwareManifest(release,payload,image),
    {size:image.size,sha256:"a".repeat(64)});
  assert.throws(()=>checkFirmwareManifest(release,{...payload,firmwareGeneration:"V16.3"},image),/invalid/);
  assert.throws(()=>checkFirmwareManifest(release,{...payload,board:"other"},image),/invalid/);
  assert.equal(assetForRelease({...release,assets:[{...image,browser_download_url:"https://evil.example/fake"},manifest]}),null);
});
