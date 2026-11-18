#!/usr/bin/env python3
"""Validate and run the *extracted* V18 preview ZIP, without hardware or cloud.

Software-only localhost smoke: this deliberately does not connect to Feather/Uno,
arm a flight, flash devices, transmit RF, or claim Windows desktop qualification.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import socket
import subprocess
import sys
import tempfile
import time
from urllib.error import URLError
from urllib.request import urlopen
from zipfile import ZipFile

NAME = "GREEN_ION_V18_DEV_TEST_PREVIEW"
NEEDED = (
    "START_V18_PREVIEW.cmd",
    "Ground_Station/START_V18_PREVIEW.cmd",
    "Ground_Station/Start-GreenIon.ps1",
    "Ground_Station/ground_station.py",
    "Ground_Station/version.json",
    "web/index.html",
    "web/v18-offline.html",
    "web/v18_offline_app.mjs",
    "web/v18_offline_core.mjs",
    "web/v18_service_worker.js",
    "web/v18.webmanifest",
    "web/analysis/rkt4.mjs",
    "docs/V18/TESTING_GUIDE.md",
    "PREVIEW_MANIFEST.json",
)


def smoke(zip_path: Path, sha_file: Path, expected_commit: str) -> None:
    digest = hashlib.sha256(zip_path.read_bytes()).hexdigest()
    expected_hash = sha_file.read_text(encoding="ascii").split()[0]
    if expected_hash != digest:
        raise AssertionError("ZIP SHA-256 does not match companion .sha256")

    with tempfile.TemporaryDirectory(prefix="green-ion-v18-preview-") as tmp:
        root = Path(tmp)
        with ZipFile(zip_path) as z:
            names = set(z.namelist())
            missing = [part for part in NEEDED if f"{NAME}/{part}" not in names]
            if missing:
                raise AssertionError(f"ZIP missing required files: {missing}")
            manifest = json.loads(z.read(f"{NAME}/PREVIEW_MANIFEST.json"))
            if manifest.get("commit") != expected_commit:
                raise AssertionError("ZIP manifest does not identify the exact CI commit")
            if manifest.get("hardwareTestsVerified") is not False:
                raise AssertionError("ZIP incorrectly claims physical hardware verification")
            if manifest.get("interface") != ["Flight", "Recovery", "Data", "Settings"]:
                raise AssertionError("ZIP does not declare the locked four-tab V18 UI")
            z.extractall(root)

        package = root / NAME
        launcher = (package / "Ground_Station/START_V18_PREVIEW.cmd").read_text(encoding="utf-8")
        if "-NoRedirect --v18-offline" not in launcher:
            raise AssertionError("Preview might redirect into installed V17")
        if "--v18-offline" not in launcher:
            raise AssertionError("V18 preview launcher is not opt-in")
        if not (package / "START_V18_PREVIEW.cmd").is_file():
            raise AssertionError("Root one-click entrypoint is missing")
        page = (package / "web/v18-offline.html").read_text(encoding="utf-8")
        tabs = re.findall(r'<button[^>]*data-tab="([^"]+)"', page)
        if tabs != ["flight", "recovery", "data", "settings"]:
            raise AssertionError(f"Unexpected V18 navigation tabs: {tabs}")
        if "--bg:#081420" not in page:
            raise AssertionError("Blue/navy V18 background changed")
        if "TEST / CHECK EVERYTHING" not in page:
            raise AssertionError("Guided dev-test entry is missing")

        # Start only the extracted local bridge, without COM ownership or browser.
        with socket.socket() as sock:
            sock.bind(("127.0.0.1", 0))
            port = sock.getsockname()[1]
        env = dict(os.environ, GREEN_ION_DATA_DIR=str(root / "isolated-test-data"), PYTHONUTF8="1")
        proc = subprocess.Popen(
            [sys.executable, "-u", "ground_station.py", "--v18-offline",
             "--no-browser", "--http-port", str(port)],
            cwd=package / "Ground_Station",
            env=env,
            stdin=subprocess.DEVNULL,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
        try:
            last_error: Exception | None = None
            for _ in range(60):
                if proc.poll() is not None:
                    raise AssertionError(f"Extracted V18 bridge exited early: {proc.returncode}")
                try:
                    with urlopen(f"http://127.0.0.1:{port}/v18-offline.html", timeout=2) as response:
                        hosted_page = response.read().decode("utf-8")
                    with urlopen(f"http://127.0.0.1:{port}/v18_offline_app.mjs", timeout=2) as response:
                        hosted_app = response.read().decode("utf-8")
                    with urlopen(f"http://127.0.0.1:{port}/v18_service_worker.js", timeout=2) as response:
                        hosted_cache = response.read().decode("utf-8")
                    if "TEST / CHECK EVERYTHING" not in hosted_page:
                        raise AssertionError("Extracted served HTML lost the guided-test entry")
                    if "runDevTests" not in hosted_app or "service-worker" not in hosted_page.lower():
                        # Registration can be a module import; verify real JS and cache separately.
                        if "runDevTests" not in hosted_app:
                            raise AssertionError("Extracted served V18 JavaScript is incomplete")
                    if "caches" not in hosted_cache:
                        raise AssertionError("Offline cache worker not present")
                    break
                except (URLError, ConnectionError, TimeoutError, OSError) as exc:
                    last_error = exc
                    time.sleep(0.25)
            else:
                raise AssertionError(f"Extracted local bridge failed to serve V18: {last_error}")
        finally:
            proc.terminate()
            try:
                proc.wait(timeout=8)
            except subprocess.TimeoutExpired:
                proc.kill()
                proc.wait(timeout=8)

        print(json.dumps({
            "result": "PASS",
            "type": "software-only extracted-ZIP localhost smoke",
            "commit": expected_commit,
            "sha256": digest,
            "four_tabs": tabs,
            "hardware": "NOT TESTED",
            "manual_windows_double_click": "NOT TESTED",
        }))


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--zip", dest="zip_path", required=True, type=Path)
    parser.add_argument("--sha256", required=True, type=Path)
    parser.add_argument("--commit", required=True)
    args = parser.parse_args()
    smoke(args.zip_path, args.sha256, args.commit)


if __name__ == "__main__":
    main()
