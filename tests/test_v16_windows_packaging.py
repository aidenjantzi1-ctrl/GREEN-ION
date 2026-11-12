from __future__ import annotations
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from zipfile import ZipFile

ROOT = Path(__file__).resolve().parents[1]


class V16WindowsPackagingTests(unittest.TestCase):
    def test_one_click_launcher_has_no_com_prompt(self):
        launcher = (ROOT / "Ground_Station" / "START_GREEN_ION.cmd").read_text(encoding="utf-8")
        legacy = (ROOT / "Ground_Station" / "OPEN_GROUND_STATION.cmd").read_text(encoding="utf-8")
        ps = (ROOT / "Ground_Station" / "Start-GreenIon.ps1").read_text(encoding="utf-8")
        self.assertIn("Start-GreenIon.ps1", launcher)
        self.assertNotIn("set /p", launcher.lower())
        self.assertIn("START_GREEN_ION.cmd", legacy)
        self.assertIn("keeps watching usb", ps.lower())
        self.assertIn("--wifi-only", ps)
        self.assertIn("GREEN_ION_DATA_DIR", ps)
        self.assertIn('$LaunchArgs += $GroundStationArgs', ps)
        self.assertIn('& $PythonExe @PythonPrefix @LaunchArgs', ps)
        self.assertNotIn('& $PythonExe @PythonPrefix @Args', ps)

    def test_updater_is_side_by_side_and_checksum_gated(self):
        ps = (ROOT / "Ground_Station" / "Update-GreenIon.ps1").read_text(encoding="utf-8")
        self.assertIn("Get-FileHash -Algorithm SHA256", ps)
        self.assertIn("GREEN_ION_V16_WINDOWS.sha256", ps)
        self.assertIn("LOCALAPPDATA", ps)
        self.assertIn("current.txt", ps)
        self.assertIn("Move-Item", ps)
        self.assertIn("$Actual.Substring(0,12)", ps)
        target_delete = [line for line in ps.splitlines()
                         if "Remove-Item" in line and "$Target" in line]
        self.assertEqual(target_delete, [], "installed app directories must be immutable")
        self.assertNotIn("GREEN_ION_Data", "\n".join(
            line for line in ps.splitlines() if "Remove-Item" in line
        ))
        self.assertIn("Your GREEN_ION_Data folder was not changed", ps)
        self.assertIn("previous.txt", ps)

    def test_rollback_swaps_only_verified_app_pointer(self):
        ps = (ROOT / "Ground_Station" / "Rollback-GreenIon.ps1").read_text(encoding="utf-8")
        cmd = (ROOT / "Ground_Station" / "ROLLBACK_GREEN_ION.cmd").read_text(encoding="utf-8")
        self.assertIn("previous.txt", ps)
        self.assertIn("current.txt", ps)
        self.assertIn("Valid-GreenIonPackage", ps)
        self.assertIn("GREEN_ION_Data was not changed", ps)
        self.assertNotIn("Remove-Item -Recurse", ps)
        self.assertIn("Rollback-GreenIon.ps1", cmd)

    def test_ground_station_continuously_rediscovers_uno(self):
        source = (ROOT / "Ground_Station" / "ground_station.py").read_text(encoding="utf-8")
        self.assertIn("wifi+lora-searching", source)
        self.assertIn("USB reconnects can come back on a different COM number", source)
        self.assertIn('"/api/app/diagnostics"', source)
        self.assertIn("receiver_last_connected_utc", source)

    def test_runtime_health_endpoint_reports_separation(self):
        source = (ROOT / "Ground_Station" / "ground_station.py").read_text(encoding="utf-8")
        self.assertIn('"/api/app/health"', source)
        self.assertIn('"user_data_outside_app": True', source)
        self.assertIn("APP_INFO", source)
        self.assertIn("receiver_port", source)

    def test_package_builder_produces_self_verified_zip(self):
        with tempfile.TemporaryDirectory() as temp:
            temp = Path(temp)
            web = temp / "web"
            web.mkdir()
            (web / "index.html").write_text("<!doctype html><title>GREEN ION V16</title>", encoding="utf-8")
            out = temp / "out"
            subprocess.run(
                [sys.executable, str(ROOT / "tools" / "build_v16_windows_package.py"),
                 "--website", str(web), "--output", str(out), "--commit", "test-sha"],
                check=True,
                cwd=ROOT,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
            )
            archive = out / "GREEN_ION_V16_WINDOWS.zip"
            expected = (out / "GREEN_ION_V16_WINDOWS.sha256").read_text(encoding="ascii").split()[0]
            self.assertEqual(hashlib.sha256(archive.read_bytes()).hexdigest(), expected)
            manifest = json.loads((out / "GREEN_ION_V16_WINDOWS_MANIFEST.json").read_text(encoding="utf-8"))
            self.assertEqual(manifest["zipSha256"], expected)
            self.assertTrue(manifest["safety"]["preservesUserData"])
            with ZipFile(archive) as zf:
                names = set(zf.namelist())
            self.assertIn("GREEN_ION_V16_WINDOWS/START_GREEN_ION.cmd", names)
            self.assertIn("GREEN_ION_V16_WINDOWS/UPDATE_GREEN_ION.cmd", names)
            self.assertIn("GREEN_ION_V16_WINDOWS/ROLLBACK_GREEN_ION.cmd", names)
            self.assertIn("GREEN_ION_V16_WINDOWS/Ground_Station/Rollback-GreenIon.ps1", names)
            self.assertIn("GREEN_ION_V16_WINDOWS/Ground_Station/ground_station.py", names)
            self.assertIn("GREEN_ION_V16_WINDOWS/web/index.html", names)


if __name__ == "__main__":
    unittest.main()
