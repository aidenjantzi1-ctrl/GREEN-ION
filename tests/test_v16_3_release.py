from pathlib import Path
import json
import unittest

ROOT=Path(__file__).resolve().parents[1]

class V163ReleaseContractTests(unittest.TestCase):
    def test_windows_identity_is_v163_release_candidate(self):
        v=json.loads((ROOT/"Ground_Station/version.json").read_text())
        self.assertEqual(v["version"],"16.3.0-rc.1")
        self.assertEqual(v["channel"],"release-candidate")
        self.assertEqual(v["data_dir"],r"%USERPROFILE%\GREEN_ION_Data")

    def test_release_assembler_uses_v163_hardware_and_source_assets(self):
        s=(ROOT/"tools/assemble_v16_release.py").read_text()
        for token in (
            "Green_Ion_V16_3.ino.bin",
            "Ground_Receiver_V16_3.ino.hex",
            "GREEN_ION_V16_3_SOURCE.zip",
            '"firmwareGeneration": "V16.3"',
            '"receiverGeneration": "V16.3"',
            '"radioProtocolVersion": 2',
            '"radioPacketBytes": 70',
            '"Green_Ion_V16_3"',
            '"Ground_Receiver_V16_3"',
            '"libraries/GreenIonProtocol"',
            '"libraries/GreenIonRecorder"',
        ):
            self.assertIn(token,s)

    def test_main_workflow_compiles_v163_and_gates_stable_publication(self):
        main=(ROOT/".github/workflows/build-v14.yml").read_text()
        self.assertIn("Green_Ion_V16_3",main)
        self.assertIn("Ground_Receiver_V16_3",main)
        self.assertIn("test_v16_3_shared_spi.cpp",main)
        self.assertIn("publish-v16-3:",main)
        self.assertIn("needs: [app-tests, firmware-build, windows-package, v16-release-smoke]",main)
        self.assertIn("gh release create v16.3.0",main)
        self.assertIn('--target "$GITHUB_SHA"',main)
        self.assertNotIn("GREEN_ION_V14_COMPLETE.zip",main)

    def test_protocol_and_user_data_contract_are_preserved(self):
        p=(ROOT/"libraries/GreenIonProtocol/src/GreenIonProtocol.h").read_text()
        self.assertIn("kProtocolVersion = 2",p)
        self.assertIn("sizeof(TelemetryPacket) == 70",p)
        package=(ROOT/"tools/build_v16_windows_package.py").read_text()
        self.assertIn(r"%USERPROFILE%\\GREEN_ION_Data",package)
        self.assertIn('"preservesUserData": True',package)

if __name__=="__main__":
    unittest.main()
