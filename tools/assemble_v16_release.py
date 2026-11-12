#!/usr/bin/env python3
"""Assemble verified GREEN ION V16 release assets using only stdlib."""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import shutil
from zipfile import ZipFile, ZIP_DEFLATED


WINDOWS_FILES = (
    "GREEN_ION_V16_WINDOWS.zip",
    "GREEN_ION_V16_WINDOWS.sha256",
    "GREEN_ION_V16_WINDOWS_MANIFEST.json",
)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def one(root: Path, pattern: str, label: str) -> Path:
    matches = list(root.rglob(pattern))
    if len(matches) != 1:
        raise SystemExit(f"Expected exactly one {label}; found {len(matches)}")
    return matches[0]


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--web", type=Path, required=True)
    parser.add_argument("--firmware", type=Path, required=True)
    parser.add_argument("--windows", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--tag", default="workflow-dispatch")
    parser.add_argument("--commit", default="unknown")
    args = parser.parse_args()

    if not (args.web / "index.html").is_file():
        raise SystemExit("Built V16 website is missing index.html")
    for name in WINDOWS_FILES:
        if not (args.windows / name).is_file():
            raise SystemExit(f"Windows release input missing {name}")

    # Re-verify the Windows ZIP independently of its build job.
    windows_zip = args.windows / "GREEN_ION_V16_WINDOWS.zip"
    expected_windows_sha = (
        args.windows / "GREEN_ION_V16_WINDOWS.sha256"
    ).read_text(encoding="ascii").split()[0].lower()
    if sha256(windows_zip) != expected_windows_sha:
        raise SystemExit("Windows ZIP SHA-256 mismatch during release assembly")

    feather = one(args.firmware, "Green_Ion_V14.ino.bin", "Feather app image")
    uno = one(args.firmware, "Ground_Receiver.ino.hex", "Uno HEX")

    out = args.output
    if out.exists():
        shutil.rmtree(out)
    out.mkdir(parents=True)

    for name in WINDOWS_FILES:
        shutil.copy2(args.windows / name, out / name)
    shutil.copy2(feather, out / "Green_Ion_V14.ino.bin")
    shutil.copy2(uno, out / "Ground_Receiver.ino.hex")

    web_zip = out / "GREEN_ION_V16_WEB.zip"
    with ZipFile(web_zip, "w", ZIP_DEFLATED, compresslevel=7) as archive:
        for path in sorted(args.web.rglob("*")):
            if path.is_file():
                archive.write(path, Path("GREEN_ION_V16_WEB") / path.relative_to(args.web))

    assets = {}
    for path in sorted(out.iterdir()):
        if path.is_file() and path.name != "SHA256_MANIFEST.json":
            assets[path.name] = {
                "sha256": sha256(path),
                "size": path.stat().st_size,
            }

    manifest = {
        "schema": "GREEN-ION-V16-RELEASE-1",
        "tag": args.tag,
        "commit": args.commit,
        "firmwareGeneration": "V14",
        "windowsApp": "V16",
        "artifacts": assets,
        "safety": {
            "loraReceiveOnly": True,
            "autonomousSdRecorderPreserved": True,
            "hardwareValidationRequired": True,
        },
    }
    (out / "SHA256_MANIFEST.json").write_text(
        json.dumps(manifest, indent=2) + "\n", encoding="utf-8"
    )
    print(json.dumps({
        "output": str(out),
        "assets": len(assets),
        "tag": args.tag,
        "commit": args.commit,
    }))


if __name__ == "__main__":
    main()
