"""GREEN ION V14 hosted app and compatible firmware contract checks."""
from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[1]
WEB=ROOT/"web"
PAGE=(WEB/"index.html").read_text(encoding="utf-8")
FIRMWARE=(ROOT/"Green_Ion_V14/Green_Ion_V14.ino").read_text(encoding="utf-8")
CONFIG=(ROOT/"Green_Ion_V14/Config.h").read_text(encoding="utf-8")
class V14ImplementationTests(unittest.TestCase):
 def test_blue_dashboard_and_all_historical_tabs_intact(self):
  self.assertIn("--bg:#081420",PAGE)
  for name in ("status","flight","field","testing","utilities","storage","system","history","cloud","connections","updates"):
   self.assertIn('data-tab="'+name+'"',PAGE,name)
  for id in ("firmwareFile","homeStart","firmwareProgress","flightSummaryDetails"):
   self.assertIn('id="'+id+'"',PAGE,id)
 def test_v14_installable_assets(self):
  for name in ("build.mjs","v14_boot.js","v14_app.mjs","v14_cloud.mjs","v14_serial.mjs",
    "v14_updates.mjs","v14_store.mjs","v14_utils.mjs","service-worker.js","manifest.webmanifest"):
   self.assertTrue((WEB/name).is_file(),name)
  worker=(WEB/"service-worker.js").read_text()
  self.assertIn('url.pathname.startsWith("/api/")',worker)
 def test_version_and_rkt4_backward_compatibility(self):
  self.assertIn('GREEN ION V14',FIRMWARE)
  self.assertIn('"/V12D%03u.BIN"',FIRMWARE)
  self.assertIn('body += "{\\"version\\":14,\\"recording\\":";',FIRMWARE)
  self.assertIn('"GREEN-ION-V14"',CONFIG)
  self.assertIn('"Dashboard.h"',FIRMWARE)
 def test_safe_cloud_api_and_local_history(self):
  cloud=(WEB/"v14_cloud.mjs").read_text()
  self.assertIn('"/auth/v1/"',cloud)
  self.assertIn('"Bearer "+token',cloud)
  self.assertIn('digest(bytes)',cloud)
  self.assertNotIn("service_role",cloud)
  updates=(WEB/"v14_updates.mjs").read_text()
  self.assertIn("v14DownloadUpdate",updates)
  self.assertIn("v14UpdateLater",updates)
  self.assertNotIn("window.location.reload()",updates)
 def test_windows_usb_remains_receive_only(self):
  receiver=(WEB/"v14_serial.mjs").read_text()
  self.assertIn("navigator.serial.requestPort",receiver)
  self.assertIn("baudRate:115200",receiver)
  self.assertNotIn(".write(",receiver)
 def test_github_automatically_builds_firmware(self):
  ci=(ROOT/".github/workflows/build-v14.yml").read_text()
  self.assertIn("Green_Ion_V14",ci)
  self.assertIn("arduino-cli compile",ci)
  self.assertIn("GREEN_ION_V14_COMPLETE.zip",ci)

if __name__=="__main__":unittest.main()
