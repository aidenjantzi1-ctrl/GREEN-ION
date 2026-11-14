"""V17 repository release/archival invariants, independent of historical test names."""
from pathlib import Path
import json
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]

class V17ReleaseAndArchiveTests(unittest.TestCase):
    def test_current_v17_runtime_and_deployment_sources(self):
        readme = (ROOT / "README.md").read_text()
        self.assertIn("CURRENT SOFTWARE VERSION: GREEN ION V17.0", readme)
        for path in (
            "Green_Ion_V16_3/Green_Ion_V16_3.ino",
            "Ground_Receiver_V16_3/Ground_Receiver_V16_3.ino",
            "Ground_Station/version.json",
            "web/index.html", "web/service-worker.js",
            "tools/assemble_v17_release.py",
            "docs/V17_INSTALL_UPDATE.md", "docs/V17_RELEASE_NOTES.md",
            "docs/V17_RELEASE_CANDIDATE.md",
        ):
            self.assertTrue((ROOT / path).is_file(), path)
        firmware = (ROOT / "Green_Ion_V16_3/Green_Ion_V16_3.ino").read_text()
        uno = (ROOT / "Ground_Receiver_V16_3/Ground_Receiver_V16_3.ino").read_text()
        self.assertIn('FIRMWARE_VERSION[] = "17.0.0"', firmware)
        self.assertIn('RECEIVER_FIRMWARE_VERSION[] = "17.0.0"', uno)
        self.assertIn('APP_VERSION = "17.0.0"', (ROOT / "web/v14_utils.mjs").read_text())
        metadata = json.loads((ROOT / "Ground_Station/version.json").read_text())
        self.assertEqual(metadata["version"], "17.0.0")
        self.assertEqual(metadata["channel"], "stable")
        self.assertEqual(metadata["data_dir"], r"%USERPROFILE%\GREEN_ION_Data")

    def test_historical_source_archives_without_fabrication(self):
        old_to_new = {
            "Green_Ion_V12": "Past_Versions/V12/Green_Ion_V12/Green_Ion_V12.ino",
            "Green_Ion_V13": "Past_Versions/V13/Green_Ion_V13/Green_Ion_V13.ino",
            "Green_Ion_V14": "Past_Versions/V14/Green_Ion_V14/Green_Ion_V14.ino",
            "Green_Ion_V16_2": "Past_Versions/V16/Green_Ion_V16_2/Green_Ion_V16_2.ino",
            "Ground_Receiver": "Past_Versions/V12/Ground_Receiver/Ground_Receiver.ino",
            "Ground_Receiver_V16_2": "Past_Versions/V16/Ground_Receiver_V16_2/Ground_Receiver_V16_2.ino",
        }
        for old, new in old_to_new.items():
            self.assertFalse((ROOT / old).exists(), old)
            self.assertTrue((ROOT / new).is_file(), new)
        for path in (
            "Past_Versions/V15/docs/V15_3D_TWIN_PLAN.md",
            "Past_Versions/V15/docs/V15_HOSTED_PHONE_CONNECTION.md",
            "Past_Versions/V15/docs/V15_LOCAL_FIRST.md",
            "Past_Versions/V11/docs/V11_MINIMAL_ARCHITECTURE.md",
            "Past_Versions/V12/CHATGPT_TRANSFER.txt",
        ):
            self.assertTrue((ROOT / path).is_file(), path)

    def test_active_ci_builds_current_sources_and_release_gated(self):
        workflow = (ROOT / ".github/workflows/build-v14.yml").read_text()
        for token in (
            "arduino-cli compile", "Green_Ion_V16_3",
            "Ground_Receiver_V16_3", "publish-v17:",
            "v17-release-smoke:", "needs: [app-tests, firmware-build, windows-package, v17-release-smoke]",
            "gh release create v17.0.0", "--notes-file docs/V17_RELEASE_NOTES.md",
        ):
            self.assertIn(token, workflow)
        assembler = (ROOT / "tools/assemble_v17_release.py").read_text()
        for token in ("Green_Ion_V17.ino.bin", "Ground_Receiver_V17.ino.hex",
                      "GREEN_ION_V17_WINDOWS.zip", "GREEN_ION_V17_SOURCE.zip",
                      '"radioProtocolVersion": 2', '"radioPacketBytes": 70',
                      '"flightCertified": False'):
            self.assertIn(token, assembler)
        self.assertIn("GreenIonSharedSpi", (ROOT / "Green_Ion_V16_3/Green_Ion_V16_3.ino").read_text())

    def test_current_readme_has_no_missing_local_markdown_links(self):
        page = ROOT / "README.md"
        content = page.read_text()
        for link in re.findall(r"\]\(([^)]+)\)", content):
            if link.startswith(("http://", "https://", "#", "mailto:")):
                continue
            target = link.split("#", 1)[0]
            if target:
                self.assertTrue((page.parent / target).exists(), link)

if __name__ == "__main__":
    unittest.main()
