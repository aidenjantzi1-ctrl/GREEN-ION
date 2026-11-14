"""GREEN ION V14 hosted app and compatible firmware contract checks."""
from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[1]
WEB=ROOT/"web"
PAGE=(WEB/"index.html").read_text(encoding="utf-8")
FIRMWARE=(ROOT/"Past_Versions/V14/Past_Versions/V14/Green_Ion_V14/Green_Ion_V14.ino").read_text(encoding="utf-8")
CONFIG=(ROOT/"Past_Versions/V14/Past_Versions/V14/Green_Ion_V14/Config.h").read_text(encoding="utf-8")
class V14ImplementationTests(unittest.TestCase):
 def test_blue_dashboard_and_all_historical_tabs_intact(self):
  self.assertIn("--bg:#081420",PAGE)
  for name in ("status","flight","field","testing","utilities","storage","system","history","cloud","connections","updates"):
   self.assertIn('data-tab="'+name+'"',PAGE,name)
  for id in ("firmwareFile","homeStart","firmwareProgress","flightSummaryDetails"):
   self.assertIn('id="'+id+'"',PAGE,id)
 def test_v14_installable_assets(self):
  for name in ("build.mjs","v14_boot.js","v14_app.mjs","v14_cloud.mjs","v14_serial.mjs",
    "v14_updates.mjs","v14_store.mjs","v14_analysis.mjs","v14_ui.mjs","v14_layout.css","v14_preferences.mjs","v14_sensor_catalog.mjs","v14_sensor_lab.mjs","v14_sensor_history.mjs","sensor-worker.mjs","v14_utils.mjs","v16_layout.mjs","v16_navigation.mjs","v16_mission.mjs","v16_telemetry.mjs","v16_data.mjs","service-worker.js","manifest.webmanifest"):
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
  self.assertIn('"Bearer "+access',cloud)
  self.assertIn('digest(bytes)',cloud)
  self.assertNotIn("service_role",cloud)
  updates=(WEB/"v14_updates.mjs").read_text()
  self.assertIn("v14DownloadUpdate",updates)
  self.assertIn("green-ion-v14-last-notified-release",updates)
  self.assertIn("v14UpdateLater",updates)
  self.assertNotIn("window.location.reload()",updates)
 def test_offline_flight_analysis_and_comparison(self):
  self.assertIn('id="v14OfflineAnalysisCard"',PAGE)
  self.assertIn('id="v14AnalysisCanvas"',PAGE)
  self.assertIn('id="v14CompareFlight"',PAGE)
  self.assertIn('id="v14ExportCsv"',PAGE)
  self.assertIn('id="v14PrintReport"',PAGE)
  module=(WEB/"v14_analysis.mjs").read_text(encoding="utf-8")
  self.assertIn("analysis-worker.mjs",module)
  self.assertIn("refreshHistoryAnalysis",module)
 def test_unique_html_ids_and_all_tab_panels(self):
  import re
  ids=re.findall(r'\bid="([^"]+)"',PAGE)
  self.assertEqual(len(ids),len(set(ids)),"Duplicate HTML ids can break tab or status rendering")
  for name in ("status","flight","field","testing","utilities","storage","system","connections","history","cloud","updates"):
   self.assertRegex(PAGE,rf'<section id="{name}" class="page(?: active)?"')
  self.assertIn('id="v14ConnectionNoticeDetail"',PAGE)
 def test_layout_has_no_grid_row_holes_and_tabs_are_accessible(self):
  css=(WEB/"v14_layout.css").read_text()
  self.assertIn("column-count:2",css)
  self.assertIn("break-inside:avoid-column",css)
  self.assertIn("scroll-snap-type:x proximity",css)
  self.assertIn('v14_layout.css',PAGE)
  ui=(WEB/"v14_ui.mjs").read_text()
  self.assertIn('aria-selected',ui)
  self.assertIn('FEATHER_CONTROLS',ui)
 def test_v14_feature_modularity_and_cloud_sync(self):
  for value in ("v14SensorLabCard","v14SensorGroups","v14PrefSensorLab","v14PrefCloudAuto","v14CloudProgress","v14CloudRecent"):
   self.assertIn('id="'+value+'"',PAGE)
  cloud=(WEB/"v14_cloud.mjs").read_text(encoding="utf-8")
  for term in ("PAGE_SIZE=250","refreshing=null","scheduleRetry","await digest(bytes)","preference(\"cloudAuto\")"):
   self.assertIn(term,cloud)
  fw=FIRMWARE
  for field in ("temperature_c","imu_data_valid","gps_parse_failures","radio_send_failures","battery_mv"):
   self.assertIn('\\\"'+field+'\\\"',fw)
 def test_sensor_history_and_advanced_device_features(self):
  for item in ("v14SensorArchiveCard","v14SenCanvas","v14SenCsv","v14SenGps","v14PrefSensorArchive"):
   self.assertIn('id="'+item+'"',PAGE)
  for path in ("analysis/gis1.mjs","v14_sensor_history.mjs","sensor-worker.mjs"):
   self.assertTrue((WEB/path).is_file(),path)
  history=(WEB/"v14_sensor_history.mjs").read_text()
  self.assertIn('sensor-worker.mjs',history)
  self.assertIn("crc32(prefix)",history)
  sw=(WEB/"service-worker.js").read_text()
  self.assertIn("analysis/gis1.mjs",sw)
  self.assertIn("v14_sensor_history.mjs",sw)
 def test_main_readme_is_a_navigation_hub(self):
  readme=(ROOT/"README.md").read_text()
  self.assertLess(len(readme),6000)
  for page in ("V14_WIRING_SOLDER.md","V14_SENSOR_CAPABILITIES.md",
      "V14_DEVICES.md","V14_CLOUD.md","V14_IMPROVEMENT_PLAN.md",
      "V14_DEVELOPMENT.md","V14_QUALITY_PLAN.md"):
   self.assertIn(page,readme)
   self.assertTrue((ROOT/"docs"/page).exists())
  self.assertTrue((ROOT/"Past_Versions/V13_README_ARCHIVE.md").exists())
 def test_fast_indexeddb_metadata_migration(self):
  store=(WEB/"v14_store.mjs").read_text()
  self.assertIn("indexedDB.open(DB,2)",store)
  self.assertIn("listLocalMetadata()",store)
  self.assertIn("table.openCursor()",store)
  self.assertIn("db.transaction([TABLE,META]",store)
  for name in ("v14_app.mjs","v14_analysis.mjs","v14_sensor_history.mjs","v14_cloud.mjs"):
   self.assertIn("listLocalMetadata", (WEB/name).read_text())
 def test_expanded_gps_without_claiming_compass(self):
  for key in ("gps_course_deg","gps_hdop","gps_fix_quality","gps_geoid_separation_m"):
   self.assertIn(key,FIRMWARE)
   self.assertIn(key,(WEB/"v14_sensor_catalog.mjs").read_text())
  self.assertIn("state.gpsSpeedMps >= 0.75f",FIRMWARE)
  self.assertIn("gps.fixquality",FIRMWARE)
  self.assertIn("gps.HDOP",FIRMWARE)
 def test_windows_usb_remains_receive_only(self):
  receiver=(WEB/"v14_serial.mjs").read_text()
  self.assertIn("navigator.serial.requestPort",receiver)
  self.assertIn("baudRate:115200",receiver)
  self.assertNotIn(".write(",receiver)
 def test_github_automatically_builds_current_firmware_and_verified_source_release(self):
  ci=(ROOT/".github/workflows/build-v14.yml").read_text()
  assembler=(ROOT/"tools/assemble_v16_release.py").read_text()
  self.assertIn("Green_Ion_V16_3",ci)
  self.assertIn("Ground_Receiver_V16_3",ci)
  self.assertIn("arduino-cli compile",ci)
  self.assertIn("GREEN_ION_V16_3_SOURCE.zip",ci)
  self.assertIn('"Green_Ion_V16_3"',assembler)
  self.assertIn('"Ground_Receiver_V16_3"',assembler)
  self.assertIn('"libraries/GreenIonRecorder"',assembler)
  self.assertIn('"libraries/GreenIonProtocol"',assembler)
 def test_complete_feather_binary_contains_local_dashboard_and_headers(self):
  for header in ("Config.h","SensorState.h","Dashboard.h","FirmwareMaintenance.h","HomeVerification.h"):
   self.assertIn('#include "'+header+'"',FIRMWARE)
  self.assertIn('dashboardServer.on("/api/update"',FIRMWARE)
  self.assertIn('id="v14ManualDownload"',PAGE)
  self.assertIn('greenIonInstallFirmwareFile',PAGE)
  self.assertTrue((WEB/"api/firmware.js").exists())

if __name__=="__main__":unittest.main()
