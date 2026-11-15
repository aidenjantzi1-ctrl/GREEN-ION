"""Synthetic byte-level RKT4 integrity regressions; no hardware claims."""
import struct
import subprocess
import sys
import tempfile
import unittest
import json
import hashlib
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from v18_rkt4_audit import audit_bytes, audit_file

AUDIT = Path(__file__).resolve().parents[1] / "tools/v18_rkt4_audit.py"


def fixture(stamps=(0, 20_000, 40_000), valid=1):
    header = struct.pack("<4sHHHHfH14x", b"RKT4", 2, 32, 16, 50, 101325.0, 3900)
    return header + b"".join(struct.pack("<IffHH", t, 101325.0, 22.0, 3900, valid)
                             for t in stamps)


def completion(data, name="FLT0001.BIN"):
    return ("GREEN-ION-COMPLETION-1\nstatus=complete\n"
            f"file={name}\nfile_bytes={len(data)}\n"
            f"complete_records={(len(data) - 32) // 16}\n")


class Rkt4AuditTests(unittest.TestCase):
    def test_complete_file_and_original_bytes(self):
        data = fixture()
        before = data[:]
        report = audit_bytes(data, metadata=completion(data))
        self.assertEqual(report["status"], "structurally_complete")
        self.assertEqual(report["complete_records"], 3)
        self.assertEqual(data, before)

    def test_short_header_and_corrupt_layout(self):
        self.assertEqual(audit_bytes(b"RKT4")["issues"], ["short_header"])
        data = bytearray(fixture())
        data[8] = 32
        self.assertEqual(audit_bytes(data)["status"], "invalid")

    def test_torn_record_and_missing_metadata(self):
        self.assertIn("truncated_record_tail", audit_bytes(fixture() + b"abc")["issues"])
        self.assertEqual(audit_bytes(fixture())["status"], "unverified")
        self.assertIn("no_complete_records", audit_bytes(fixture(()))["issues"])

    def test_complete_record_truncation_detected_by_metadata(self):
        full = fixture()
        cut = full[:-16]
        result = audit_bytes(cut, metadata=completion(full))
        self.assertEqual(result["trailing_bytes"], 0)
        self.assertIn("invalid_completion_metadata", result["issues"])

    def test_malformed_and_wrong_flight_metadata(self):
        data = fixture()
        self.assertIn("invalid_completion_metadata", audit_bytes(data, metadata="done")["issues"])
        result = audit_bytes(data, metadata=completion(data), expected_name="FLT0002.BIN")
        self.assertIn("invalid_completion_metadata", result["issues"])
        good = completion(data)
        for bad in (
            good.replace("GREEN-ION-COMPLETION-1", "GREEN-ION-METADATA-1"),
            good.replace("status=complete", "status=recording"),
            good.replace(f"file_bytes={len(data)}", "file_bytes=999"),
            good.replace("complete_records=3", "complete_records=2"),
        ):
            with self.subTest(bad=bad):
                self.assertIn("invalid_completion_metadata", audit_bytes(data, metadata=bad)["issues"])

    def test_timestamp_gap_and_wrap(self):
        gap_data = fixture((0, 20_000, 500_000))
        gap = audit_bytes(gap_data, metadata=completion(gap_data))
        self.assertEqual(gap["timestamp_gaps"], 1)
        wrap_data = fixture((0xFFFFD000, 0x1E20))
        wrap = audit_bytes(wrap_data, metadata=completion(wrap_data))
        self.assertEqual(wrap["nonmonotonic_timestamps"], 0)

    def test_invalid_sensor_read_and_bad_validity(self):
        data = fixture(valid=0)
        self.assertEqual(audit_bytes(data, metadata=completion(data))["invalid_sensor_reads"], 3)
        self.assertEqual(audit_bytes(fixture(valid=7))["status"], "invalid")
        data = bytearray(fixture())
        struct.pack_into("<H", data, 10, 99)
        self.assertEqual(audit_bytes(data)["status"], "invalid")

    def test_recovery_and_journal_prevent_complete_claim(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "FLT0001.BIN"
            path.write_bytes(fixture())
            path.with_suffix(".TXT").write_text(completion(path.read_bytes()))
            path.with_suffix(".JRA").write_bytes(b"journal")
            self.assertIn("pending_journal", audit_file(path)["issues"])
            path.with_suffix(".JRA").unlink()
            path.with_suffix(".RCV").write_text("status=interrupted")
            self.assertIn("interrupted_recovery_report", audit_file(path)["issues"])

    def test_two_flights_keep_distinct_hashes(self):
        with tempfile.TemporaryDirectory() as directory:
            a, b = Path(directory) / "FLT0001.BIN", Path(directory) / "FLT0002.BIN"
            a.write_bytes(fixture())
            b.write_bytes(fixture((0, 20_000, 60_000)))
            self.assertNotEqual(audit_file(a)["sha256"], audit_file(b)["sha256"])
            self.assertEqual(a.read_bytes(), fixture())

    def test_cli_empty_directory_is_explicit_failure(self):
        with tempfile.TemporaryDirectory() as directory:
            run = subprocess.run([sys.executable, str(AUDIT), directory],
                                 capture_output=True, text=True, check=False)
            self.assertNotEqual(run.returncode, 0)
            self.assertIn("No RKT4 .BIN files found", run.stderr)
            self.assertEqual(run.stdout, "")

    def test_cli_lowercase_and_batch_preserve_files(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            first = root / "FLT0001.bin"
            second = root / "FLT0002.BIN"
            first.write_bytes(fixture())
            second.write_bytes(fixture((0, 20_000, 60_000)))
            (root / "FLT0001.txt").write_text(completion(first.read_bytes(), "FLT0001.BIN"))
            (root / "FLT0002.TXT").write_text(completion(second.read_bytes(), second.name))
            before = {p.name: hashlib.sha256(p.read_bytes()).hexdigest()
                      for p in root.iterdir()}
            run = subprocess.run([sys.executable, str(AUDIT), directory],
                                 capture_output=True, text=True, check=False)
            self.assertEqual(run.returncode, 0, run.stderr)
            reports = json.loads(run.stdout)
            self.assertEqual({r["file"] for r in reports}, {first.name, second.name})
            self.assertTrue(all(r["status"] == "structurally_complete" for r in reports))
            self.assertEqual(before, {p.name: hashlib.sha256(p.read_bytes()).hexdigest()
                                      for p in root.iterdir()})

    def test_cli_unverified_file_is_nonzero(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "FLT0003.bin"
            path.write_bytes(fixture())
            run = subprocess.run([sys.executable, str(AUDIT), str(path)],
                                 capture_output=True, text=True, check=False)
            self.assertEqual(run.returncode, 1)
            self.assertEqual(json.loads(run.stdout)[0]["status"], "unverified")


if __name__ == "__main__":
    unittest.main()
