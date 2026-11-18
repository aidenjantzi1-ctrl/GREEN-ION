#!/usr/bin/env python3
"""Build an opt-in V18 DEVELOPMENT TEST PREVIEW ZIP, never a release."""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import shutil
from zipfile import ZipFile, ZIP_DEFLATED

ROOT = Path(__file__).resolve().parents[1]
REQUIRED_WEB = (
    "v18-offline.html", "v18_offline_app.mjs", "v18_offline_core.mjs",
    "v18.webmanifest",
)
REQUIRED_GROUND = (
    "START_V18_PREVIEW.cmd", "START_GREEN_ION.cmd",
    "Start-GreenIon.ps1", "ground_station.py", "version.json",
)

def build(website: Path, output: Path, commit: str) -> Path:
    for name in REQUIRED_WEB:
        if not (website / name).is_file():
            raise ValueError(f"Missing V18 web asset: {name}")
    for name in REQUIRED_GROUND:
        if not (ROOT / "Ground_Station" / name).is_file():
            raise ValueError(f"Missing V18 launcher/bridge asset: {name}")
    if not (ROOT / "docs/V18/TESTING_GUIDE.md").is_file():
        raise ValueError("Missing V18 testing guide")
    if not (website / "analysis/rkt4.mjs").is_file():
        raise ValueError("Missing RKT4 offline analyzer")
    output.mkdir(parents=True, exist_ok=True)
    name = "GREEN_ION_V18_DEV_TEST_PREVIEW"
    stage = output / "_package" / name
    if stage.parent.exists():
        shutil.rmtree(stage.parent)
    stage.mkdir(parents=True)
    shutil.copytree(ROOT / "Ground_Station", stage / "Ground_Station",
                    ignore=shutil.ignore_patterns("__pycache__", "*.pyc", "logs", ".DS_Store"))
    shutil.copytree(website, stage / "web")
    shutil.copytree(ROOT / "docs/V18", stage / "docs/V18")
    (stage / "START_V18_PREVIEW.cmd").write_text(
        "@echo off\r\ncall \"%~dp0Ground_Station\\START_V18_PREVIEW.cmd\" %*\r\nexit /b %ERRORLEVEL%\r\n",
        encoding="utf-8")
    manifest = {
        "schema": "GREEN-ION-V18-DEVELOPMENT-TEST-PREVIEW-1",
        "commit": commit,
        "status": "DEVELOPMENT TEST ONLY - NOT A RELEASE OR FLIGHT APPROVAL",
        "entrypoint": "START_V18_PREVIEW.cmd",
        "interface": ["Flight", "Recovery", "Data", "Settings"],
        "pythonMinimum": "3.10",
        "requiresInternetAtRuntime": False,
        "firmwareFlashing": False,
        "preservesOriginalFlightFiles": True,
        "hardwareTestsVerified": False,
    }
    (stage / "PREVIEW_MANIFEST.json").write_text(
        json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    zip_path = output / (name + ".zip")
    with ZipFile(zip_path, "w", ZIP_DEFLATED, compresslevel=7) as archive:
        for path in sorted(stage.rglob("*")):
            if path.is_file():
                archive.write(path, path.relative_to(stage.parent))
    digest = hashlib.sha256(zip_path.read_bytes()).hexdigest()
    (output / (name + ".sha256")).write_text(
        f"{digest}  {zip_path.name}\n", encoding="ascii")
    with ZipFile(zip_path) as archive:
        files = set(archive.namelist())
        expected = {
            f"{name}/START_V18_PREVIEW.cmd",
            f"{name}/Ground_Station/START_V18_PREVIEW.cmd",
            f"{name}/Ground_Station/ground_station.py",
            f"{name}/web/v18-offline.html",
            f"{name}/web/analysis/rkt4.mjs",
            f"{name}/docs/V18/TESTING_GUIDE.md",
            f"{name}/PREVIEW_MANIFEST.json",
        }
        missing = expected - files
        if missing:
            raise ValueError(f"ZIP missing required entries: {sorted(missing)}")
    print(json.dumps({"zip": str(zip_path), "sha256": digest, "commit": commit}))
    return zip_path

def main() -> None:
    p = argparse.ArgumentParser()
    p.add_argument("--website", required=True, type=Path)
    p.add_argument("--output", required=True, type=Path)
    p.add_argument("--commit", required=True)
    args = p.parse_args()
    build(args.website, args.output, args.commit)

if __name__ == "__main__":
    main()
