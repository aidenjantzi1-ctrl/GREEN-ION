"""Adversarial synthetic audit corpus and CLI behavior; no hardware access."""
from __future__ import annotations

import base64
import hashlib
import json
import os
from pathlib import Path
import shutil
import stat
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
sys.path.insert(0, str(ROOT / "tests/fixtures"))
from v18_rkt4_audit import audit_bytes
from v18_rkt4_synthetic import cases, completion_text, make_log

AUDIT = ROOT / "tools/v18_rkt4_audit.py"


def run_cli(*args: object):
    return subprocess.run([sys.executable, str(AUDIT), *(str(a) for a in args)],
                          text=True, capture_output=True, check=False)


class IndependentAuditTests(unittest.TestCase):
    def test_fixture_hash_is_stable(self):
        self.assertEqual(hashlib.sha256(make_log()).hexdigest(),
                         "40c0d2cd3d937ad8b203dc6cad6c217bf6042b8e04e8e05e702899da8713a36a")

    def test_corpus_has_expected_structural_states_and_hashes(self):
        for name, (data, metadata, expected) in cases().items():
            with self.subTest(case=name):
                before = bytes(data)
                report = audit_bytes(data, metadata=metadata, expected_name="V12D001.BIN")
                self.assertEqual(report["status"], expected)
                self.assertEqual(report["sha256"], hashlib.sha256(before).hexdigest())
                self.assertEqual(bytes(data), before)

    def test_clock_and_sensor_evidence_never_creates_samples(self):
        corpus = cases()
        self.assertEqual(audit_bytes(corpus["duplicate_time"][0])["nonmonotonic_timestamps"], 1)
        self.assertEqual(audit_bytes(corpus["backward_time"][0])["nonmonotonic_timestamps"], 1)
        self.assertEqual(audit_bytes(corpus["clock_wrap"][0])["nonmonotonic_timestamps"], 0)
        self.assertEqual(audit_bytes(corpus["timestamp_gap"][0])["timestamp_gaps"], 1)
        self.assertEqual(audit_bytes(corpus["missing_sensor"][0])["invalid_sensor_reads"], 3)
        self.assertEqual(audit_bytes(corpus["partial_tail"][0])["complete_records"], 3)

    def test_recovery_evidence_blocks_structural_complete(self):
        data = make_log()
        metadata = completion_text(data)
        self.assertIn("pending_journal", audit_bytes(data, metadata=metadata,
                                                     journal_present=True)["issues"])
        self.assertIn("interrupted_recovery_report", audit_bytes(data, metadata=metadata,
                                                                 recovery="status=interrupted")["issues"])

    def test_summary_counts_mixed_batch_and_default_list_stays_compatible(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            good = root / "V12D001.BIN"
            bad = root / "V12D002.bin"
            data = make_log()
            good.write_bytes(data)
            good.with_suffix(".TXT").write_text(completion_text(data), encoding="ascii")
            bad.write_bytes(data[:-3])
            original_hashes = {p.name: hashlib.sha256(p.read_bytes()).hexdigest()
                               for p in root.iterdir()}
            result = run_cli("--summary", root)
            self.assertEqual(result.returncode, 1, result.stderr)
            payload = json.loads(result.stdout)
            self.assertEqual(payload["schema"], "green-ion-rkt4-audit-1")
            self.assertEqual(payload["summary"], {
                "total": 2, "structurally_complete": 1,
                "incomplete": 1, "invalid": 0, "unverified": 0,
            })
            self.assertEqual([entry["file"] for entry in payload["files"]],
                             ["V12D001.BIN", "V12D002.bin"])
            self.assertNotIn(directory, result.stdout)
            legacy = run_cli(root)
            self.assertIsInstance(json.loads(legacy.stdout), list)
            self.assertEqual(original_hashes, {p.name: hashlib.sha256(p.read_bytes()).hexdigest()
                                               for p in root.iterdir()})

    def test_summary_empty_and_missing_paths_fail_closed_without_path_leak(self):
        with tempfile.TemporaryDirectory() as directory:
            for path in (directory, Path(directory) / "private-location"):
                with self.subTest(path=path):
                    result = run_cli("--summary", path)
                    self.assertEqual(result.returncode, 2)
                    payload = json.loads(result.stdout)
                    self.assertEqual(payload["summary"]["total"], 0)
                    self.assertEqual(payload["errors"], ["no_bin_files"])
                    self.assertNotIn(str(path), result.stdout + result.stderr)

    def test_read_only_file_is_not_mutated(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "V12D001.BIN"
            data = make_log()
            path.write_bytes(data)
            path.with_suffix(".TXT").write_text(completion_text(data), encoding="ascii")
            before = hashlib.sha256(path.read_bytes()).hexdigest()
            os.chmod(path, stat.S_IREAD)
            try:
                result = run_cli(path)
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertEqual(hashlib.sha256(path.read_bytes()).hexdigest(), before)
            finally:
                os.chmod(path, stat.S_IWRITE | stat.S_IREAD)

    @unittest.skipUnless(shutil.which("node"), "Node unavailable; JS parity NOT TESTED")
    def test_python_and_js_parser_agree_on_layout_but_not_integrity(self):
        script = (
            'import {parseRkt4} from "./web/analysis/rkt4.mjs";'
            'const b=Buffer.from(process.argv[1],"base64");'
            'try{const x=parseRkt4(new Uint8Array(b));'
            'console.log(JSON.stringify({count:x.samples.length,tail:x.trailingBytes,rate:x.rateHz,warnings:x.warnings}));}'
            'catch(e){console.log(JSON.stringify({error:e.message}));}'
        )
        for name in ("valid", "partial_tail", "bad_magic", "bad_version", "bad_rate"):
            with self.subTest(case=name):
                data = cases()[name][0]
                encoded = base64.b64encode(data).decode("ascii")
                run = subprocess.run(["node", "--input-type=module", "-e", script, encoded],
                                     cwd=ROOT, text=True, capture_output=True, check=True)
                parsed = json.loads(run.stdout)
                if name in ("bad_magic", "bad_version"):
                    self.assertIn("error", parsed)
                else:
                    self.assertEqual(parsed["count"], audit_bytes(data)["complete_records"])
                    self.assertEqual(parsed["tail"], audit_bytes(data)["trailing_bytes"])
                if name == "bad_rate":
                    self.assertIn("Unexpected sample rate", parsed["warnings"])
                    self.assertEqual(audit_bytes(data)["status"], "invalid")


if __name__ == "__main__":
    unittest.main()
