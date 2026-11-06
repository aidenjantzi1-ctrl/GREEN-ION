import io
import json
from pathlib import Path
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "Ground_Station"))

from session_library import SessionImportError, SessionLibrary


def event(sequence: int, *, source: str = "wifi", stamp: str | None = None) -> bytes:
    value = {"received_utc": stamp or f"2026-11-06T01:30:0{sequence}+00:00",
             "source": source, "data": {"sequence": sequence, "latitude": 0.0}}
    return (json.dumps(value, separators=(",", ":")) + "\n").encode()


class SessionLibraryTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.library = SessionLibrary(Path(self.temporary.name))

    def tearDown(self):
        self.temporary.cleanup()

    def test_import_preserves_original_events_and_checksum_deduplicates(self):
        raw = event(1) + event(2, source="radio")
        first = self.library.import_stream("ground-session.jsonl", io.BytesIO(raw), len(raw))
        second = self.library.import_stream("copy.jsonl", io.BytesIO(raw), len(raw))
        self.assertFalse(first["duplicate"])
        self.assertTrue(second["duplicate"])
        self.assertEqual(first["sha256"], second["sha256"])
        self.assertEqual("ground-session.jsonl", second["original_name"])
        self.assertEqual(raw, self.library.imported_file(first["name"]).read_bytes())
        self.assertEqual(2, first["inspection"]["records"])
        self.assertEqual(1, first["inspection"]["source_counts"]["radio"])
        listed = self.library.list()[0]
        self.assertEqual("imported_jsonl", listed["source_kind"])
        self.assertEqual(first["sha256"], listed["sha256"])

    def test_invalid_event_or_transfer_never_enters_library(self):
        valid = event(1)
        bad_rows = [b"", b"not json\n", b"{}\n",
                    b'{"source":[],"data":{},"received_utc":"2026-11-06T01:30:01Z"}\n',
                    b'{"source":"wifi","data":{},"received_utc":"2026-11-06T01:30:01"}\n',
                    b'{"source":"wifi","data":{"v":NaN},"received_utc":"2026-11-06T01:30:01Z"}\n']
        for raw in bad_rows:
            with self.subTest(raw=raw), self.assertRaises(SessionImportError):
                self.library.import_stream("invalid.jsonl", io.BytesIO(raw))
        with self.assertRaisesRegex(SessionImportError, "incomplete"):
            self.library.import_stream("missing.jsonl", io.BytesIO(valid), len(valid) + 1)
        with self.assertRaisesRegex(SessionImportError, "filename"):
            self.library.import_stream("../escaped.jsonl", io.BytesIO(valid))
        self.assertEqual([], self.library.list())
        self.assertFalse(list(self.library.root.glob("*.part")))

    def test_time_order_and_tail_are_reported_without_rewriting(self):
        raw = event(2) + event(1).rstrip(b"\n")
        imported = self.library.import_stream("order.jsonl", io.BytesIO(raw))
        self.assertEqual(1, imported["inspection"]["out_of_order_receive_times"])
        self.assertFalse(imported["inspection"]["final_newline"])
        self.assertEqual(raw, self.library.imported_file(imported["name"]).read_bytes())

    def test_tampered_import_cannot_be_downloaded_or_reused(self):
        raw = event(1)
        imported = self.library.import_stream("original.jsonl", io.BytesIO(raw))
        path = self.library.imported_file(imported["name"])
        path.write_bytes(raw + b"x")
        with self.assertRaisesRegex(SessionImportError, "checksum"):
            self.library.imported_file(imported["name"])
        with self.assertRaisesRegex(SessionImportError, "checksum"):
            self.library.import_stream("original.jsonl", io.BytesIO(raw))

    def test_imported_session_trash_preserves_bytes_and_blocks_active_session(self):
        raw = event(1)
        imported = self.library.import_stream("original.jsonl", io.BytesIO(raw))
        name = imported["name"]
        with self.assertRaisesRegex(SessionImportError, "current writable"):
            self.library.trash_session(name, active_name=name)
        self.library.trash_session(name, active_name="session-current.jsonl")
        self.assertEqual([], self.library.list())
        self.assertEqual("valid", self.library.list_trash()[0]["integrity"])
        self.assertEqual(raw, (self.library.root / "session_trash" / name / name).read_bytes())
        self.library.restore_trash(name)
        self.assertEqual(raw, self.library.imported_file(name).read_bytes())
        self.assertEqual([], self.library.list_trash())

    def test_legacy_session_trash_is_reversible_without_rewriting(self):
        name = "session-legacy.jsonl"
        raw = b'{"old_format":true}\n'
        (self.library.root / name).write_bytes(raw)
        self.library.trash_session(name)
        self.library.restore_trash(name)
        self.assertEqual(raw, (self.library.root / name).read_bytes())

    def test_damaged_imported_trash_metadata_blocks_restore(self):
        imported = self.library.import_stream("original.jsonl", io.BytesIO(event(1)))
        name = imported["name"]
        self.library.trash_session(name)
        (self.library.root / "session_trash" / name / f"{imported['sha256']}.json").write_text("{}")
        self.assertEqual("invalid", self.library.list_trash()[0]["integrity"])
        with self.assertRaisesRegex(SessionImportError, "metadata"):
            self.library.restore_trash(name)
        self.assertFalse((self.library.root / name).exists())


if __name__ == "__main__":
    unittest.main()
