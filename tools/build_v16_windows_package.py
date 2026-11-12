#!/usr/bin/env python3
"""Build the GREEN ION V16 Windows package using only stdlib."""
from __future__ import annotations
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
from zipfile import ZipFile, ZIP_DEFLATED

ROOT = Path(__file__).resolve().parents[1]

def copytree(src: Path, dst: Path) -> None:
    shutil.copytree(
        src, dst, dirs_exist_ok=True,
        ignore=shutil.ignore_patterns("__pycache__", "*.pyc", "logs", ".DS_Store"),
    )

def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()

def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--website", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--commit", default=os.environ.get("GITHUB_SHA", "unknown"))
    args = parser.parse_args()
    version = json.loads((ROOT / "Ground_Station/version.json").read_text(encoding="utf-8"))
    if not (args.website / "index.html").is_file():
        raise SystemExit("built website index.html missing")
    args.output.mkdir(parents=True, exist_ok=True)
    stage = args.output / "_package" / "GREEN_ION_V16_WINDOWS"
    if stage.parent.exists():
        shutil.rmtree(stage.parent)
    stage.mkdir(parents=True)

    copytree(ROOT / "Ground_Station", stage / "Ground_Station")
    copytree(args.website, stage / "web")
    for name in ("START_GREEN_ION.cmd", "UPDATE_GREEN_ION.cmd", "ROLLBACK_GREEN_ION.cmd", "README.md", "VERSIONS.md", "CHANGELOG.md"):
        shutil.copy2(ROOT / name, stage / name)
    docs = stage / "docs"
    docs.mkdir()
    for path in sorted((ROOT / "docs").glob("V16_*.md")):
        shutil.copy2(path, docs / path.name)

    package_manifest = {
        "schema": "GREEN-ION-V16-WINDOWS-PACKAGE-1",
        "version": version["version"],
        "channel": version["channel"],
        "commit": args.commit,
        "dataLocation": "%USERPROFILE%\\GREEN_ION_Data",
        "updateInstallLocation": "%LOCALAPPDATA%\\GREEN_ION\\App\\<tag>-<sha12>",
        "requirements": {
            "python": ">=3.10",
            "pyserial": "3.5 (optional; launcher attempts installation and falls back to Wi-Fi-only)",
        },
        "safety": {
            "firmwareFlashing": False,
            "loraCommands": False,
            "preservesUserData": True,
            "rollbackRetainsPriorApp": True,
        },
    }
    (stage / "PACKAGE_MANIFEST.json").write_text(json.dumps(package_manifest, indent=2) + "\n", encoding="utf-8")

    zip_path = args.output / "GREEN_ION_V16_WINDOWS.zip"
    with ZipFile(zip_path, "w", ZIP_DEFLATED, compresslevel=7) as archive:
        for path in sorted(stage.rglob("*")):
            if path.is_file():
                archive.write(path, path.relative_to(stage.parent))
    digest = sha256(zip_path)
    (args.output / "GREEN_ION_V16_WINDOWS.sha256").write_text(
        f"{digest}  {zip_path.name}\n", encoding="ascii"
    )
    package_manifest["zipSha256"] = digest
    package_manifest["zipBytes"] = zip_path.stat().st_size
    (args.output / "GREEN_ION_V16_WINDOWS_MANIFEST.json").write_text(
        json.dumps(package_manifest, indent=2) + "\n", encoding="utf-8"
    )
    print(json.dumps({"zip": str(zip_path), "sha256": digest, "bytes": zip_path.stat().st_size}))

if __name__ == "__main__":
    main()
