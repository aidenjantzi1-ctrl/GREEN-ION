"""Validated import of Ground Station JSONL sessions without replaying them as live telemetry."""

from __future__ import annotations

from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import re
import tempfile
import threading
from typing import BinaryIO


MAX_SESSION_BYTES = 32 * 1024 * 1024
SAFE_SESSION_NAME = re.compile(r"[A-Za-z0-9][A-Za-z0-9_.-]{0,95}\.jsonl\Z", re.IGNORECASE)
IMPORTED_NAME = re.compile(r"session-import-([0-9a-f]{64})\.jsonl\Z")
SOURCES = {"wifi", "radio", "simulation"}


class SessionImportError(ValueError):
    pass


def file_sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        while chunk := handle.read(65536):
            digest.update(chunk)
    return digest.hexdigest()


def reject_constant(value: str) -> None:
    raise ValueError(f"nonfinite JSON number {value}")


def inspect_session(path: Path) -> dict:
    count = 0
    first = last = None
    source_counts = {name: 0 for name in sorted(SOURCES)}
    previous_time = None
    out_of_order_times = 0
    with path.open("rb") as handle:
        for line_number, line in enumerate(handle, 1):
            if len(line) > 256 * 1024:
                raise SessionImportError(f"session line {line_number} exceeds 256 KiB")
            try:
                event = json.loads(line.decode("utf-8"), parse_constant=reject_constant)
            except (UnicodeDecodeError, json.JSONDecodeError, ValueError, RecursionError) as error:
                raise SessionImportError(f"invalid JSONL event at line {line_number}: {error}") from error
            if not isinstance(event, dict) or not isinstance(event.get("data"), dict):
                raise SessionImportError(f"line {line_number} is not a Ground Station event")
            source = event.get("source")
            if not isinstance(source, str) or source not in SOURCES:
                raise SessionImportError(f"line {line_number} has an unsupported telemetry source")
            stamp = event.get("received_utc")
            if not isinstance(stamp, str):
                raise SessionImportError(f"line {line_number} has no received UTC timestamp")
            try:
                parsed = datetime.fromisoformat(stamp.replace("Z", "+00:00"))
            except ValueError as error:
                raise SessionImportError(f"line {line_number} has an invalid received UTC timestamp") from error
            if parsed.tzinfo is None or parsed.utcoffset() is None:
                raise SessionImportError(f"line {line_number} has a timezone-free received timestamp")
            utc = parsed.astimezone(timezone.utc)
            if previous_time is not None and utc < previous_time:
                out_of_order_times += 1
            previous_time = utc
            first = first or utc.isoformat()
            last = utc.isoformat()
            source_counts[source] += 1
            count += 1
    if count == 0:
        raise SessionImportError("session contains no events")
    with path.open("rb") as handle:
        handle.seek(-1, os.SEEK_END)
        final_newline = handle.read(1) == b"\n"
    return {"records": count, "first_received_utc": first, "last_received_utc": last,
            "source_counts": source_counts, "out_of_order_receive_times": out_of_order_times,
            "final_newline": final_newline}


class SessionLibrary:
    def __init__(self, root: Path) -> None:
        self.root = root
        self.metadata = root / "session_imports"
        self.root.mkdir(parents=True, exist_ok=True)
        self.metadata.mkdir(parents=True, exist_ok=True)
        self.lock = threading.RLock()

    def _metadata_path(self, digest: str) -> Path:
        if not re.fullmatch(r"[0-9a-f]{64}", digest):
            raise SessionImportError("invalid session checksum")
        return self.metadata / f"{digest}.json"

    def import_stream(self, filename: str, source: BinaryIO,
                      expected_bytes: int | None = None) -> dict:
        if not isinstance(filename, str) or not SAFE_SESSION_NAME.fullmatch(filename) or Path(filename).name != filename:
            raise SessionImportError("filename must be a simple .jsonl name")
        if expected_bytes is not None and (expected_bytes <= 0 or expected_bytes > MAX_SESSION_BYTES):
            raise SessionImportError("JSONL upload must be between 1 byte and 32 MiB")
        with self.lock:
            with tempfile.NamedTemporaryFile(dir=self.root, prefix="session-import-", suffix=".part",
                                             delete=False) as output:
                temporary = Path(output.name)
                digest = hashlib.sha256()
                received = 0
                try:
                    while chunk := source.read(65536):
                        received += len(chunk)
                        if received > MAX_SESSION_BYTES:
                            raise SessionImportError("JSONL upload exceeds 32 MiB")
                        output.write(chunk)
                        digest.update(chunk)
                    output.flush()
                    os.fsync(output.fileno())
                except Exception:
                    temporary.unlink(missing_ok=True)
                    raise
            try:
                if expected_bytes is not None and received != expected_bytes:
                    raise SessionImportError(f"incomplete JSONL transfer: expected {expected_bytes}, received {received}")
                inspection = inspect_session(temporary)
                identifier = digest.hexdigest()
                destination = self.root / f"session-import-{identifier}.jsonl"
                metadata_path = self._metadata_path(identifier)
                duplicate = destination.exists()
                if duplicate:
                    if destination.is_symlink():
                        raise SessionImportError("stored imported session is a symlink")
                    if file_sha256(destination) != identifier:
                        raise SessionImportError("stored imported session checksum mismatch")
                    temporary.unlink()
                else:
                    temporary.replace(destination)
                if metadata_path.exists():
                    metadata = json.loads(metadata_path.read_text(encoding="utf-8"))
                else:
                    metadata = {"id": identifier, "name": destination.name,
                                "original_name": filename, "sha256": identifier,
                                "bytes": received, "imported_utc": datetime.now(timezone.utc).isoformat(),
                                "inspection": inspection}
                    with tempfile.NamedTemporaryFile(mode="w", encoding="utf-8", dir=self.metadata,
                                                     prefix="metadata-", suffix=".part", delete=False) as output:
                        metadata_temp = Path(output.name)
                        json.dump(metadata, output, indent=2, allow_nan=False)
                        output.flush()
                        os.fsync(output.fileno())
                    metadata_temp.replace(metadata_path)
                return {**metadata, "duplicate": duplicate}
            except Exception:
                temporary.unlink(missing_ok=True)
                raise

    def imported_file(self, name: str) -> Path:
        match = IMPORTED_NAME.fullmatch(name)
        if not match:
            raise SessionImportError("invalid imported session filename")
        path = self.root / name
        if path.is_symlink() or not path.is_file() or file_sha256(path) != match.group(1):
            raise SessionImportError("imported session is missing or its checksum changed")
        return path

    def list(self) -> list[dict]:
        result = []
        for path in sorted(self.root.glob("session-*.jsonl"), reverse=True):
            if path.is_symlink() or not path.is_file():
                continue
            item = {"name": path.name, "bytes": path.stat().st_size,
                    "modified": datetime.fromtimestamp(path.stat().st_mtime, timezone.utc).isoformat(),
                    "source_kind": "ground_station_session"}
            match = IMPORTED_NAME.fullmatch(path.name)
            if match:
                try:
                    metadata = json.loads(self._metadata_path(match.group(1)).read_text(encoding="utf-8"))
                    item.update(source_kind="imported_jsonl", original_name=metadata["original_name"],
                                sha256=metadata["sha256"], inspection=metadata["inspection"])
                except (OSError, KeyError, json.JSONDecodeError):
                    item["source_kind"] = "imported_metadata_missing"
            result.append(item)
        return result
