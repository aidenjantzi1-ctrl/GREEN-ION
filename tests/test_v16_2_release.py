from pathlib import Path
import json
import re
import unittest

ROOT=Path(__file__).resolve().parents[1]

class V162ReleaseContractTests(unittest.TestCase):
    def test_windows_identity_is_stable_v162(self):
        v=json.loads((ROOT/"Ground_Station/version.json").read_text())
        self.assertEqual(v["version"],"16.2.0")
        self.assertEqual(v["channel"],"stable")
        self.assertEqual(v["data_dir"],r"%USERPROFILE%\GREEN_ION_Data")

    def test_release_assembler_uses_v162_hardware_and_source_assets(self):
        s=(ROOT/"tools/assemble_v16_release.py").read_text()
        for token in (
            "Green_Ion_V16_2.ino.bin",
            "Ground_Receiver_V16_2.ino.hex",
            "GREEN_ION_V16_2_SOURCE.zip",
            '"firmwareGeneration": "V16.2"',
            '"receiverGeneration": "V16.2"',
            '"radioProtocolVersion": 2',
            '"radioPacketBytes": 70',
            '"Green_Ion_V16_2"',
            '"Ground_Receiver_V16_2"',
            '"libraries/GreenIonProtocol"',
            '"libraries/GreenIonRecorder"',
        ):
            self.assertIn(token,s)

    def test_main_and_tag_workflows_compile_current_targets(self):
        main=(ROOT/".github/workflows/build-v14.yml").read_text()
        release=(ROOT/".github/workflows/release-v16.yml").read_text()
        for text in (main,release):
            self.assertIn("Green_Ion_V16_2",text)
            self.assertIn("Ground_Receiver_V16_2",text)
            self.assertIn("v16_2_render_browser.mjs",text)
        self.assertNotIn("GREEN_ION_V14_COMPLETE.zip",main)
        self.assertIn("publish-v16-2:",main)
        self.assertIn("needs: [app-tests, firmware-build, windows-package, v16-release-smoke]",main)
        self.assertIn("gh release create v16.2.0",main)
        self.assertIn('--target "$GITHUB_SHA"',main)

    def test_no_google_key_is_committed(self):
        paths=[
            ROOT/"web/v16_2_map.mjs",
            ROOT/"web/build.mjs",
            ROOT/"Ground_Station/ground_station.py",
            ROOT/"docs/V16_2_GOOGLE_MAPS.md",
        ]
        joined="\n".join(p.read_text() for p in paths)
        keys=re.findall(r"AIza[0-9A-Za-z_-]{20,}",joined)
        self.assertEqual(keys,[])

    def test_release_notes_and_guides_exist(self):
        for path in (
            "docs/V16_RELEASE_NOTES.md",
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

if __name__=="__main__":
    unittest.main()
