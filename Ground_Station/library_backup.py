"""Portable, checksum-verified backup of laptop flight and session files."""

from __future__ import annotations

from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import stat
import tempfile
from typing import BinaryIO
import zipfile

from flight_library import FlightLibrary
from session_library import SessionLibrary, inspect_session


FORMAT = "green-ion-v12-library-backup-v1"
MAX_ARCHIVE_BYTES = 512 * 1024 * 1024
MAX_FILES = 1000
SAFE_PATH = re.compile(
    r"(?:flights/raw/[0-9a-f]{64}\.(?:bin|sen)|"
    r"flights/metadata/[0-9a-f]{64}\.json|"
    r"flights/trash/[0-9a-f]{64}/raw/[0-9a-f]{64}\.(?:bin|sen)|"
    r"flights/trash/[0-9a-f]{64}/metadata/[0-9a-f]{64}\.json|"
    r"session_trash/session-[A-Za-z0-9_.-]{1,100}\.jsonl/session-[A-Za-z0-9_.-]{1,100}\.jsonl|"
    r"session_trash/session-import-[0-9a-f]{64}\.jsonl/[0-9a-f]{64}\.json|"
    r"session-[A-Za-z0-9_.-]{1,100}\.jsonl|"
    r"session_imports/[0-9a-f]{64}\.json)\Z"
)


class BackupError(ValueError):
    pass


def file_digest(path: Path) -> tuple[int, str]:
    digest = hashlib.sha256()
    size = 0
    with path.open("rb") as handle:
        while chunk := handle.read(65536):
            size += len(chunk)
            digest.update(chunk)
    return size, digest.hexdigest()


def files_for_backup(root: Path) -> list[tuple[str, Path]]:
    patterns = ("flights/raw/*.bin", "flights/raw/*.sen", "flights/metadata/*.json",
                "flights/trash/*/raw/*.bin", "flights/trash/*/raw/*.sen",
                "flights/trash/*/metadata/*.json",
                "session_trash/session-*.jsonl/*.jsonl", "session_trash/session-*.jsonl/*.json",
                "session-*.jsonl", "session_imports/*.json")
    files = []
    for pattern in patterns:
        for path in root.glob(pattern):
            name = path.relative_to(root).as_posix()
            if not SAFE_PATH.fullmatch(name) or path.is_symlink() or not path.is_file():
                raise BackupError(f"unsafe library file: {name}")
            files.append((name, path))
    if len(files) > MAX_FILES:
        raise BackupError("library contains too many files for one backup")
    return sorted(files)


def create_backup(root: Path, destination: Path) -> dict:
    """Write a ZIP to a temporary destination; callers publish it only after success."""
    files = files_for_backup(root)
    entries = []
    total = 0
    with zipfile.ZipFile(destination, "w", compression=zipfile.ZIP_STORED, allowZip64=True) as archive:
        for name, path in files:
            before = path.stat()
            size, sha256 = file_digest(path)
            total += size
            if total > MAX_ARCHIVE_BYTES:
                raise BackupError("library exceeds the 512 MiB backup limit")
            archive.write(path, name)
            after = path.stat()
            if before.st_size != size or after.st_size != size or before.st_mtime_ns != after.st_mtime_ns:
                raise BackupError(f"library file changed during backup: {name}")
            if file_digest(path) != (size, sha256):
                raise BackupError(f"library file changed during backup: {name}")
            entries.append({"path": name, "bytes": size, "sha256": sha256})
        manifest = {"format": FORMAT, "created_utc": datetime.now(timezone.utc).isoformat(),
                    "entries": entries}
        archive.writestr("manifest.json", json.dumps(manifest, separators=(",", ":"), allow_nan=False))
    if destination.stat().st_size > MAX_ARCHIVE_BYTES:
        raise BackupError("ZIP exceeds the 512 MiB backup limit")
    with zipfile.ZipFile(destination) as archive:
        for entry in entries:
            digest = hashlib.sha256()
            size = 0
            with archive.open(entry["path"]) as member:
                while chunk := member.read(65536):
                    size += len(chunk)
                    digest.update(chunk)
            if size != entry["bytes"] or digest.hexdigest() != entry["sha256"]:
                raise BackupError(f"backup ZIP member changed during creation: {entry['path']}")
    _validate_staged_library(root, manifest)
    return manifest


def _read_archive(source: BinaryIO, stage: Path, expected_bytes: int | None) -> tuple[dict, Path]:
    archive_path = stage / "upload.zip"
    received = 0
    with archive_path.open("wb") as output:
        while chunk := source.read(65536):
            received += len(chunk)
            if received > MAX_ARCHIVE_BYTES:
                raise BackupError("backup upload exceeds 512 MiB")
            output.write(chunk)
    if expected_bytes is not None and received != expected_bytes:
        raise BackupError(f"incomplete backup transfer: expected {expected_bytes}, received {received}")
    try:
        archive = zipfile.ZipFile(archive_path)
    except zipfile.BadZipFile as error:
        raise BackupError("invalid backup ZIP") from error
    with archive:
        info = archive.infolist()
        if len(info) < 1 or len(info) > MAX_FILES + 1:
            raise BackupError("backup file count is outside supported limits")
        names = [item.filename for item in info]
        if names.count("manifest.json") != 1 or len(set(names)) != len(names):
            raise BackupError("backup has missing or duplicate manifest/member names")
        if any(name != "manifest.json" and not SAFE_PATH.fullmatch(name) for name in names):
            raise BackupError("backup contains an unsafe path")
        if any(item.is_dir() or stat.S_IFMT(item.external_attr >> 16) == stat.S_IFLNK for item in info):
            raise BackupError("backup contains a directory or symlink member")
        if sum(item.file_size for item in info) > MAX_ARCHIVE_BYTES or archive.getinfo("manifest.json").file_size > 1024 * 1024:
            raise BackupError("backup expands beyond its size limit")
        try:
            manifest = json.loads(archive.read("manifest.json"))
        except (json.JSONDecodeError, RuntimeError, zipfile.BadZipFile) as error:
            raise BackupError("backup manifest is unreadable") from error
        if not isinstance(manifest, dict) or manifest.get("format") != FORMAT or not isinstance(manifest.get("entries"), list):
            raise BackupError("unsupported backup manifest")
        entries = manifest["entries"]
        if any(not isinstance(item, dict) or not isinstance(item.get("path"), str) for item in entries):
            raise BackupError("backup manifest contains an invalid entry")
        if len(entries) != len(info) - 1 or {item["path"] for item in entries} != set(names) - {"manifest.json"}:
            raise BackupError("backup manifest does not match ZIP members")
        if len(entries) != len({item["path"] for item in entries}):
            raise BackupError("backup manifest has duplicate paths")
        for entry in entries:
            if not isinstance(entry, dict) or not isinstance(entry.get("path"), str) or not SAFE_PATH.fullmatch(entry["path"]):
                raise BackupError("backup manifest contains an unsafe path")
            name = entry["path"]
            member = archive.getinfo(name)
            if (type(entry.get("bytes")) is not int or entry["bytes"] != member.file_size or
                    not isinstance(entry.get("sha256"), str) or not re.fullmatch(r"[0-9a-f]{64}", entry["sha256"])):
                raise BackupError(f"backup manifest size/checksum is invalid: {name}")
            target = stage / name
            target.parent.mkdir(parents=True, exist_ok=True)
            digest = hashlib.sha256()
            size = 0
            try:
                with archive.open(member) as source_file, target.open("xb") as output:
                    while chunk := source_file.read(65536):
                        size += len(chunk)
                        if size > entry["bytes"]:
                            raise BackupError(f"backup member grew while extracting: {name}")
                        output.write(chunk)
                        digest.update(chunk)
            except (RuntimeError, zipfile.BadZipFile) as error:
                raise BackupError(f"backup member is corrupt: {name}") from error
            if size != entry["bytes"] or digest.hexdigest() != entry["sha256"]:
                raise BackupError(f"backup member checksum mismatch: {name}")
    return manifest, archive_path


def _validate_staged_library(stage: Path, manifest: dict) -> None:
    flights = FlightLibrary(stage / "flights")
    sessions = SessionLibrary(stage)
    for entry in manifest["entries"]:
        name = entry["path"]
        path = stage / name
        if "/raw/" in name and path.stem != entry["sha256"]:
            raise BackupError(f"raw flight identity mismatch: {name}")
        if name.startswith("flights/metadata/") or name.startswith("flights/trash/") and "/metadata/" in name:
            identifier = path.stem
            try:
                library = FlightLibrary(stage / "flights" / "trash" / name.split("/")[2]) if name.startswith("flights/trash/") else flights
                if name.startswith("flights/trash/") and name.split("/")[2] != identifier:
                    raise BackupError(f"Trash directory and flight identity differ: {name}")
                metadata, _raw = library.get(identifier)
                if metadata.get("id") != identifier or metadata.get("sha256") != identifier:
                    raise BackupError(f"flight metadata identity mismatch: {name}")
                library.analysis_timeline(identifier)
            except (OSError, ValueError, KeyError, TypeError) as error:
                raise BackupError(f"flight metadata/recording invalid: {name}: {error}") from error
        elif name.startswith("session_imports/"):
            try:
                metadata = json.loads(path.read_text(encoding="utf-8"))
                if (metadata.get("sha256") != path.stem or metadata.get("id") != path.stem or
                        metadata.get("name") != f"session-import-{path.stem}.jsonl"):
                    raise BackupError(f"session metadata identity mismatch: {name}")
                imported = sessions.imported_file(metadata["name"])
                if metadata.get("bytes") != imported.stat().st_size or metadata.get("inspection") != inspect_session(imported):
                    raise BackupError(f"session metadata/inspection mismatch: {name}")
            except (OSError, ValueError, KeyError, TypeError) as error:
                raise BackupError(f"imported session invalid: {name}: {error}") from error
        elif name.startswith("session_trash/"):
            parts = name.split("/")
            if len(parts) != 3:
                raise BackupError(f"invalid session Trash path: {name}")
            session_name = parts[1]
            if parts[2].endswith(".jsonl") and parts[2] != session_name:
                raise BackupError(f"session Trash directory/name mismatch: {name}")
            match = re.fullmatch(r"session-import-([0-9a-f]{64})\.jsonl", session_name)
            if match and parts[2].endswith(".jsonl") and file_digest(path)[1] != match.group(1):
                raise BackupError(f"trashed imported session checksum mismatch: {name}")
            if match and parts[2].endswith(".jsonl"):
                if not (path.parent / f"{match.group(1)}.json").is_file():
                    raise BackupError(f"trashed imported session metadata missing: {name}")
                try:
                    inspect_session(path)
                except (OSError, ValueError) as error:
                    raise BackupError(f"trashed imported session invalid: {name}: {error}") from error
            if parts[2].endswith(".json") and (not match or parts[2] != f"{match.group(1)}.json"):
                raise BackupError(f"trashed imported session metadata mismatch: {name}")
            if parts[2].endswith(".json"):
                try:
                    metadata = json.loads(path.read_text(encoding="utf-8"))
                    if (metadata.get("id") != match.group(1) or metadata.get("sha256") != match.group(1) or
                            metadata.get("name") != session_name or not (path.parent / session_name).is_file()):
                        raise BackupError(f"trashed imported session metadata invalid: {name}")
                except (OSError, ValueError, AttributeError) as error:
                    raise BackupError(f"trashed imported session metadata invalid: {name}: {error}") from error


def restore_backup(root: Path, source: BinaryIO, expected_bytes: int | None = None) -> dict:
    if expected_bytes is not None and (expected_bytes <= 0 or expected_bytes > MAX_ARCHIVE_BYTES):
        raise BackupError("backup upload must be between 1 byte and 512 MiB")
    root.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(dir=root, prefix="restore-stage-") as directory:
        stage = Path(directory)
        manifest, _archive = _read_archive(source, stage, expected_bytes)
        _validate_staged_library(stage, manifest)
        additions = []
        for entry in manifest["entries"]:
            destination = root / entry["path"]
            if destination.exists():
                if destination.is_symlink() or file_digest(destination) != (entry["bytes"], entry["sha256"]):
                    raise BackupError(f"existing library file conflicts with backup: {entry['path']}")
            else:
                additions.append((stage / entry["path"], destination))
        created = []
        try:
            for staged, destination in additions:
                destination.parent.mkdir(parents=True, exist_ok=True)
                with staged.open("rb") as input_file, destination.open("xb") as output:
                    created.append(destination)
                    shutil.copyfileobj(input_file, output, length=65536)
                    output.flush()
                    os.fsync(output.fileno())
        except Exception:
            for path in created:
                path.unlink(missing_ok=True)
            raise
        return {"format": FORMAT, "files_added": len(created),
                "files_already_present": len(manifest["entries"]) - len(created),
                "entries": len(manifest["entries"])}
