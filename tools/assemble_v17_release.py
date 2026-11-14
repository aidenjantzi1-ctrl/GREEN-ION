#!/usr/bin/env python3
"""Assemble checksum-verified GREEN ION V17 software release assets.

Historical sketch and Windows package *input* names are deliberately retained
for Arduino/rollback compatibility; public release artifacts use V17 names.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import shutil
from zipfile import ZipFile, ZIP_DEFLATED

ROOT = Path(__file__).resolve().parents[1]
WINDOWS_INPUTS = {
    "GREEN_ION_V16_WINDOWS.zip": "GREEN_ION_V17_WINDOWS.zip",
    "GREEN_ION_V16_WINDOWS.sha256": "GREEN_ION_V17_WINDOWS.sha256",
    "GREEN_ION_V16_WINDOWS_MANIFEST.json": "GREEN_ION_V17_WINDOWS_MANIFEST.json",
}
SOURCE_ROOTS = (
    "Green_Ion_V16_3",
    "Ground_Receiver_V16_3",
    "libraries/GreenIonProtocol",
    "libraries/GreenIonRecorder",
)

def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for data in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(data)
    return digest.hexdigest()

def require_one(root: Path, name: str) -> Path:
    matches = list(root.rglob(name))
    if len(matches) != 1:
        raise SystemExit(f"Expected one compiled {name}; got {len(matches)}")
    return matches[0]

def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--web", required=True, type=Path)
    parser.add_argument("--firmware", required=True, type=Path)
    parser.add_argument("--windows", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--tag", default="ci-smoke")
    parser.add_argument("--commit", default="unknown")
    args = parser.parse_args()

    if not (args.web / "index.html").is_file():
        raise SystemExit("Missing built V17 web index.html")
    for original in WINDOWS_INPUTS:
        if not (args.windows / original).is_file():
            raise SystemExit(f"Missing Windows input: {original}")
    supplied_checksum = (args.windows / "GREEN_ION_V16_WINDOWS.sha256").read_text(encoding="ascii").split()[0].lower()
    windows_zip = args.windows / "GREEN_ION_V16_WINDOWS.zip"
    if supplied_checksum != sha256(windows_zip):
        raise SystemExit("Windows ZIP SHA-256 mismatch")

    metadata = json.loads((ROOT / "Ground_Station/version.json").read_text(encoding="utf-8"))
    if metadata["version"] != "17.0.1" or metadata["channel"] != "stable":
        raise SystemExit("V17 release requires version.json 17.0.1 stable software channel")

    feather = require_one(args.firmware, "Green_Ion_V16_3.ino.bin")
    uno = require_one(args.firmware, "Ground_Receiver_V16_3.ino.hex")
    for item in SOURCE_ROOTS:
        if not (ROOT / item).exists():
            raise SystemExit(f"Missing V17 active source {item}")

    output = args.output
    if output.exists():
        shutil.rmtree(output)
    output.mkdir(parents=True)

    for original, final in WINDOWS_INPUTS.items():
        if original.endswith(".sha256"):
            (output / final).write_text(f"{supplied_checksum}  GREEN_ION_V17_WINDOWS.zip\n", encoding="ascii")
        else:
            shutil.copy2(args.windows / original, output / final)

    shutil.copy2(feather, output / "Green_Ion_V17.ino.bin")
    shutil.copy2(uno, output / "Ground_Receiver_V17.ino.hex")

    with ZipFile(output / "GREEN_ION_V17_WEB.zip", "w", ZIP_DEFLATED, compresslevel=7) as archive:
        for path in sorted(args.web.rglob("*")):
            if path.is_file():
                archive.write(path, Path("GREEN_ION_V17_WEB") / path.relative_to(args.web))

    with ZipFile(output / "GREEN_ION_V17_SOURCE.zip", "w", ZIP_DEFLATED, compresslevel=7) as archive:
        for relative in SOURCE_ROOTS:
            for path in sorted((ROOT / relative).rglob("*")):
                if path.is_file():
                    archive.write(path, Path("GREEN_ION_V17_SOURCE") / path.relative_to(ROOT))
        for relative in ("README.md", "CHANGELOG.md", "VERSIONS.md"):
            archive.write(ROOT / relative, Path("GREEN_ION_V17_SOURCE") / relative)
        for path in sorted((ROOT / "docs").glob("V17_*.md")):
            archive.write(path, Path("GREEN_ION_V17_SOURCE/docs") / path.name)

    assets = {
        p.name: {"sha256": sha256(p), "size": p.stat().st_size}
        for p in sorted(output.iterdir()) if p.is_file() and p.name != "SHA256_MANIFEST.json"
    }
    manifest = {
        "schema": "GREEN-ION-V17-RELEASE-1",
        "tag": args.tag,
        "version": args.tag,
        "commit": args.commit,
        "board": "Adafruit Feather ESP32 V2",
        "firmwareFormat": "ESP32 .ino.bin app image",
        "firmwareGeneration": "V17.0",
        "receiverGeneration": "V17.0",
        "radioProtocolVersion": 2,
        "radioPacketBytes": 70,
        "windowsApp": "V17.0.1",
        "artifacts": assets,
        "safety": {
            "loraReceiveOnly": True,
            "autonomousSdRecorderPreserved": True,
            "sdFirstRecorder": True,
            "optionalRadioDoesNotGateRecorder": True,
            "hardwareValidationRequired": True,
            "flightCertified": False,
        },
        "compatibility": {
            "featherSourceDirectory": "Green_Ion_V16_3",
            "receiverSourceDirectory": "Ground_Receiver_V16_3",
            "windowsInnerDirectory": "GREEN_ION_V16_WINDOWS",
        },
    }
    (output / "SHA256_MANIFEST.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"tag": args.tag, "assets": len(assets), "commit": args.commit}))

if __name__ == "__main__":
    main()
