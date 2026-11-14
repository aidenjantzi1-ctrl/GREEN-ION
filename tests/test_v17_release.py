from pathlib import Path
import json
import unittest

ROOT=Path(__file__).resolve().parents[1]

class V17ReleaseContractTests(unittest.TestCase):
    def test_current_v17_identities(self):
        fw=(ROOT/"Green_Ion_V17/Green_Ion_V17.ino").read_text()
        cfg=(ROOT/"Green_Ion_V17/Config.h").read_text()
        rx=(ROOT/"Ground_Receiver_V17/Ground_Receiver_V17.ino").read_text()
        app=json.loads((ROOT/"Ground_Station/version.json").read_text())
        web=(ROOT/"web/v14_utils.mjs").read_text()
        self.assertIn('FIRMWARE_NAME[] = "GREEN ION Feather V17"',fw)
        self.assertIn('FIRMWARE_VERSION[] = "17.0.0"',fw)
        self.assertIn('DASHBOARD_SSID[] = "GREEN-ION-V17"',cfg)
        self.assertIn('SENSOR_RATE_HZ = 50',cfg)
        self.assertIn('PREBUFFER_RECORDS == 150',cfg)
        self.assertIn('RECEIVER_FIRMWARE_VERSION[] = "17.0.0"',rx)
        self.assertEqual(app["version"],"17.0.0")
        self.assertIn('APP_VERSION = "17.0.0"',web)

    def test_hardware_and_file_contracts_are_preserved(self):
        cfg=(ROOT/"Green_Ion_V17/Config.h").read_text()
        fw=(ROOT/"Green_Ion_V17/Green_Ion_V17.ino").read_text()
        proto=(ROOT/"libraries/GreenIonProtocol/src/GreenIonProtocol.h").read_text()
        for name,value in {
            "PIN_SPI_SCK":"5","PIN_SPI_MOSI":"19","PIN_SPI_MISO":"21",
            "PIN_SD_CS":"33","PIN_LORA_CS":"27","PIN_LORA_RST":"15","PIN_LORA_DIO0":"32"
        }.items():
            self.assertRegex(cfg,rf"{name}\s*=\s*{value}\s*;")
        self.assertIn("#include <GreenIonRkt4.h>",fw)
        self.assertIn("kProtocolVersion = 2",proto)
        self.assertIn("sizeof(TelemetryPacket) == 70",proto)
        self.assertNotIn("STATE_IGNITION",proto)

    def test_v17_release_pipeline_builds_v17_sources(self):
        workflow=(ROOT/".github/workflows/build-v17.yml").read_text()
        tagged=(ROOT/".github/workflows/release-v17.yml").read_text()
        assembler=(ROOT/"tools/assemble_v17_release.py").read_text()
        for token in (
            "Green_Ion_V17","Ground_Receiver_V17","tools/build_v17_windows_package.py",
            "tools/assemble_v17_release.py","publish-v17:","v17.0.0",
            "GREEN-ION-V17-RELEASE-1"
        ):
            self.assertIn(token,workflow)
        self.assertIn('tags: ["v17.*"]',tagged)
        for token in (
            "Green_Ion_V17.ino.bin","Ground_Receiver_V17.ino.hex",
            "GREEN_ION_V17_SOURCE.zip",'"firmwareGeneration": "V17"',
            '"receiverGeneration": "V17"'
        ):
            self.assertIn(token,assembler)

    def test_v17_updater_support(self):
        utils=(ROOT/"web/v14_utils.mjs").read_text()
        updates=(ROOT/"web/v14_updates.mjs").read_text()
        relay=(ROOT/"web/api/firmware.js").read_text()
        self.assertIn('if(version.major===17)return "Green_Ion_V17.ino.bin"',utils)
        self.assertIn('parsed?.major===17 ? "V17"',utils)
        self.assertIn('/^v(?:17|16)\\./.test',updates)
        self.assertIn('Green_Ion_V17.ino.bin',relay)

    def test_v17_docs_keep_hardware_gate_explicit(self):
        notes=(ROOT/"docs/V17_RELEASE_NOTES.md").read_text()
        hw=(ROOT/"docs/V17_HARDWARE_VALIDATION.md").read_text()
        self.assertIn("do **not** prove",notes)
        self.assertIn("Do not convert CI PASS into physical PASS",hw)

if __name__=="__main__":
    unittest.main()
