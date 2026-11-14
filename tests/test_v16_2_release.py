from pathlib import Path
import re
import unittest

ROOT=Path(__file__).resolve().parents[1]

class V162HistoricalReleaseTests(unittest.TestCase):
    def test_v162_versioned_source_remains_available_for_rollback(self):
        for path in (
            "Past_Versions/V16/Green_Ion_V16_2/Green_Ion_V16_2.ino",
            "Past_Versions/V16/Green_Ion_V16_2/Config.h",
            "Past_Versions/V16/Ground_Receiver_V16_2/Ground_Receiver_V16_2.ino",
            "docs/V16_2_IMPLEMENTATION.md",
        ):
            self.assertTrue((ROOT/path).is_file(),path)
        fw=(ROOT/"Past_Versions/V16/Green_Ion_V16_2/Green_Ion_V16_2.ino").read_text()
        uno=(ROOT/"Past_Versions/V16/Ground_Receiver_V16_2/Ground_Receiver_V16_2.ino").read_text()
        self.assertIn('FIRMWARE_VERSION[] = "16.2.0"',fw)
        self.assertIn('RECEIVER_FIRMWARE_VERSION[] = "16.2.0"',uno)

    def test_v162_guides_and_release_notes_remain_in_tree(self):
        for path in (
            "docs/V16_2_WINDOWS_SETUP.md",
            "docs/V16_2_FIRMWARE_UPDATE.md",
            "docs/V16_2_UNO_RECEIVER.md",
            "docs/V16_2_LORA_PROTOCOL.md",
            "docs/V16_2_SD_FILE_MANAGEMENT.md",
            "docs/V16_2_GOOGLE_MAPS.md",
            "docs/V16_2_3D_VIEWER.md",
            "docs/V16_2_LIVE_VISUALIZATION.md",
            "docs/V16_2_HARDWARE_VALIDATION.md",
        ):
            self.assertTrue((ROOT/path).is_file(),path)

    def test_no_google_key_was_added_while_advancing_versions(self):
        paths=[
            ROOT/"web/v16_2_map.mjs",
            ROOT/"web/build.mjs",
            ROOT/"Ground_Station/ground_station.py",
            ROOT/"docs/V16_2_GOOGLE_MAPS.md",
        ]
        joined="\n".join(p.read_text() for p in paths)
        self.assertEqual(re.findall(r"AIza[0-9A-Za-z_-]{20,}",joined),[])

if __name__=="__main__":
    unittest.main()
