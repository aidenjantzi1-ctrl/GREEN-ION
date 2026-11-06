import io
import hashlib
import json
import os
from pathlib import Path
import struct
import sys
import tempfile
import unittest
from unittest.mock import patch
import zipfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "Ground_Station"))

from flight_library import FlightLibrary
import library_backup
from library_backup import BackupError, create_backup, restore_backup
from session_library import SessionLibrary
from telemetry_manager import SessionWriter


def flight_bytes():
    header = bytearray(32)
    header[:4] = b"RKT4"
    struct.pack_into("<HHHHfH", header, 4, 2, 32, 16, 25, 101325.0, 4100)
    return bytes(header) + struct.pack("<IffHH", 40000, 100900.0, 20.0, 4000, 1)


def session_bytes():
    return (json.dumps({"received_utc": "2026-11-06T03:30:00+00:00", "source": "wifi",
                        "data": {"sequence": 1}}, separators=(",", ":")) + "\n").encode()


class LibraryBackupTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.base = Path(self.temp.name)
        self.source = self.base / "source"
        self.source.mkdir()
        self.flights = FlightLibrary(self.source / "flights")
        self.flight = self.flights.import_stream("V12D007.BIN", io.BytesIO(flight_bytes()))
        self.sessions = SessionLibrary(self.source)
        self.session = self.sessions.import_stream("field.jsonl", io.BytesIO(session_bytes()))
        writer = SessionWriter(self.source, "live-test")
        writer.append("radio", {"sequence": 2})
        (self.source / "session-legacy-v11.jsonl").write_bytes(b"legacy session format preserved\n")
        self.archive = self.base / "backup.zip"

    def tearDown(self):
        self.temp.cleanup()

    def test_backup_restores_raw_bytes_metadata_and_sessions_without_overwriting(self):
        manifest = create_backup(self.source, self.archive)
        self.assertEqual("green-ion-v12-library-backup-v1", manifest["format"])
        self.assertTrue(any(item["path"].endswith(".bin") for item in manifest["entries"]))
        target = self.base / "restored"
        result = restore_backup(target, io.BytesIO(self.archive.read_bytes()), self.archive.stat().st_size)
        self.assertEqual(len(manifest["entries"]), result["files_added"])
        self.assertEqual(flight_bytes(), FlightLibrary(target / "flights").get(self.flight["id"])[1].read_bytes())
        self.assertEqual(session_bytes(), SessionLibrary(target).imported_file(self.session["name"]).read_bytes())
        self.assertTrue((target / "session-live-test.jsonl").is_file())
        self.assertEqual(b"legacy session format preserved\n",
                         (target / "session-legacy-v11.jsonl").read_bytes())
        repeated = restore_backup(target, io.BytesIO(self.archive.read_bytes()))
        self.assertEqual(0, repeated["files_added"])

    def test_manifest_tamper_and_unsafe_member_are_rejected_before_restore(self):
        create_backup(self.source, self.archive)
        with zipfile.ZipFile(self.archive) as source_zip:
            members = {name: source_zip.read(name) for name in source_zip.namelist()}
        manifest = json.loads(members["manifest.json"])
        manifest["entries"][0]["sha256"] = "0" * 64
        members["manifest.json"] = json.dumps(manifest).encode()
        damaged = self.base / "damaged.zip"
        with zipfile.ZipFile(damaged, "w") as output:
            for name, payload in members.items():
                output.writestr(name, payload)
        target = self.base / "bad-restore"
        with self.assertRaisesRegex(BackupError, "checksum mismatch"):
            restore_backup(target, io.BytesIO(damaged.read_bytes()))
        self.assertFalse((target / "flights" / "raw").exists())
        unsafe = self.base / "unsafe.zip"
        with zipfile.ZipFile(unsafe, "w") as output:
            output.writestr("manifest.json", json.dumps({"format": manifest["format"], "entries": []}))
            output.writestr("../escape", b"unsafe")
        with self.assertRaisesRegex(BackupError, "unsafe path"):
            restore_backup(target, io.BytesIO(unsafe.read_bytes()))
        self.assertFalse((self.base / "escape").exists())

    def test_conflicting_existing_raw_file_is_preserved(self):
        manifest = create_backup(self.source, self.archive)
        target = self.base / "conflict"
        raw_name = next(item["path"] for item in manifest["entries"] if item["path"].endswith(".bin"))
        destination = target / raw_name
        destination.parent.mkdir(parents=True)
        destination.write_bytes(b"existing data")
        with self.assertRaisesRegex(BackupError, "conflicts"):
            restore_backup(target, io.BytesIO(self.archive.read_bytes()))
        self.assertEqual(b"existing data", destination.read_bytes())
        self.assertFalse((target / "session-live-test.jsonl").exists())

    def test_backup_includes_trashed_flight_and_restore_keeps_it_recoverable(self):
        self.flights.trash_flight(self.flight["id"])
        manifest = create_backup(self.source, self.archive)
        self.assertTrue(any(item["path"].startswith("flights/trash/") for item in manifest["entries"]))
        target = self.base / "restore-trash"
        restore_backup(target, io.BytesIO(self.archive.read_bytes()))
        restored_library = FlightLibrary(target / "flights")
        self.assertEqual(1, len(restored_library.list_trash()))
        restored_library.restore_trash(self.flight["id"])
        self.assertEqual(flight_bytes(), restored_library.get(self.flight["id"])[1].read_bytes())

    def test_backup_includes_trashed_imported_and_legacy_sessions(self):
        imported_name = self.session["name"]
        self.sessions.trash_session(imported_name)
        self.sessions.trash_session("session-legacy-v11.jsonl")
        manifest = create_backup(self.source, self.archive)
        self.assertTrue(any(item["path"].startswith("session_trash/") for item in manifest["entries"]))
        target = self.base / "restore-session-trash"
        restore_backup(target, io.BytesIO(self.archive.read_bytes()))
        restored = SessionLibrary(target)
        self.assertEqual(2, len(restored.list_trash()))
        restored.restore_trash(imported_name)
        restored.restore_trash("session-legacy-v11.jsonl")
        self.assertEqual(session_bytes(), restored.imported_file(imported_name).read_bytes())
        self.assertEqual(b"legacy session format preserved\n",
                         (target / "session-legacy-v11.jsonl").read_bytes())

    def test_backup_rejects_incomplete_imported_session_trash(self):
        name = self.session["name"]
        self.sessions.trash_session(name)
        metadata = self.source / "session_trash" / name / f"{self.session['sha256']}.json"
        metadata.unlink()
        with self.assertRaisesRegex(BackupError, "metadata missing"):
            create_backup(self.source, self.archive)
        self.assertTrue((self.source / "session_trash" / name / name).is_file())

    def test_backup_rejects_imported_session_inspection_mismatch(self):
        metadata_path = self.source / "session_imports" / f"{self.session['sha256']}.json"
        metadata = json.loads(metadata_path.read_text())
        metadata["inspection"]["records"] += 1
        metadata_path.write_text(json.dumps(metadata))
        with self.assertRaisesRegex(BackupError, "inspection mismatch"):
            create_backup(self.source, self.archive)
        self.assertEqual(session_bytes(),
                         (self.source / self.session["name"]).read_bytes())

    def test_restore_rejects_checksum_consistent_but_invalid_flight_metadata_atomically(self):
        create_backup(self.source, self.archive)
        with zipfile.ZipFile(self.archive) as source_zip:
            members = {name: source_zip.read(name) for name in source_zip.namelist()}
        metadata_name = f"flights/metadata/{self.flight['id']}.json"
        metadata = json.loads(members[metadata_name])
        metadata["summary"]["duration_s"] = float("nan")
        members[metadata_name] = json.dumps(metadata).encode()
        manifest = json.loads(members["manifest.json"])
        entry = next(item for item in manifest["entries"] if item["path"] == metadata_name)
        entry["bytes"] = len(members[metadata_name])
        entry["sha256"] = hashlib.sha256(members[metadata_name]).hexdigest()
        members["manifest.json"] = json.dumps(manifest).encode()
        damaged = self.base / "invalid-flight.zip"
        with zipfile.ZipFile(damaged, "w") as output:
            for name, payload in members.items():
                output.writestr(name, payload)
        target = self.base / "invalid-flight-restore"
        with self.assertRaisesRegex(BackupError, "flight metadata/recording invalid"):
            restore_backup(target, io.BytesIO(damaged.read_bytes()))
        self.assertFalse((target / "flights" / "raw").exists())
        self.assertFalse((target / "session-live-test.jsonl").exists())

    @unittest.skipIf(os.name == "nt", "symlink creation may require Windows developer mode")
    def test_restore_rejects_existing_symlink_parent_before_writing_any_member(self):
        create_backup(self.source, self.archive)
        target = self.base / "symlink-restore"
        outside = self.base / "outside"
        outside.mkdir()
        target.mkdir()
        (target / "flights").symlink_to(outside, target_is_directory=True)
        with self.assertRaisesRegex(BackupError, "symlink parent"):
            restore_backup(target, io.BytesIO(self.archive.read_bytes()))
        self.assertEqual([], list(outside.iterdir()))
        self.assertFalse((target / "session-live-test.jsonl").exists())

    @unittest.skipIf(os.name == "nt", "symlink creation may require Windows developer mode")
    def test_backup_rejects_symlinked_flight_directory(self):
        raw = self.source / "flights" / "raw"
        outside = self.base / "outside-raw"
        raw.rename(outside)
        raw.symlink_to(outside, target_is_directory=True)
        with self.assertRaisesRegex(BackupError, "unsafe library file"):
            create_backup(self.source, self.archive)

    def test_backup_rejects_session_append_during_archive_creation(self):
        active = self.source / "session-live-test.jsonl"
        original_digest = library_backup.file_digest
        changed = False

        def digest_with_concurrent_append(path):
            nonlocal changed
            result = original_digest(path)
            if path == active and not changed:
                changed = True
                with active.open("ab") as output:
                    output.write(session_bytes())
            return result

        with patch.object(library_backup, "file_digest", side_effect=digest_with_concurrent_append):
            with self.assertRaisesRegex(BackupError, "changed during backup"):
                create_backup(self.source, self.archive)
        self.assertTrue(changed)
        self.assertTrue(active.read_bytes().endswith(session_bytes()))


if __name__ == "__main__":
    unittest.main()
