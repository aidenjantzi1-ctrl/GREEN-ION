"""V11 Stage B contract tests for safe, bounded RKT4 transfer behavior."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
INO = (ROOT / "Green_Ion_V10/Green_Ion_V10.ino").read_text(encoding="utf-8")
SPEC = (ROOT / "docs/RKT4_V2_FORMAT.md").read_text(encoding="utf-8")


class TransferContractTests(unittest.TestCase):
    def test_format_spec_matches_firmware_layout(self):
        for token in ("RKT4", "Header: 32 bytes", "Records: 16 bytes each", "uint32", "float32", "micros()"):
            self.assertIn(token, SPEC)
        for token in ('w16(hdr + 6, 32)', 'w16(hdr + 8, 16)', 'w32(r, ts)', 'wf(r + 4, p)', 'wf(r + 8, t)'):
            self.assertIn(token, INO)

    def test_download_is_not_allowed_during_recording(self):
        start = INO.index("void downloadAPI()")
        body = INO[start:INO.index("void preflightAPI()", start)]
        self.assertIn("!sdOK || recording", body)
        self.assertIn('web.send(409, "text/plain", "SD busy")', body)

    def test_download_has_length_and_version_evidence(self):
        start = INO.index("void downloadAPI()")
        body = INO[start:INO.index("void preflightAPI()", start)]
        self.assertIn('"X-Green-Ion-Bytes"', body)
        self.assertIn('"X-Green-Ion-RKT4-Version", "2"', body)
        self.assertIn('web.streamFile(f, "application/octet-stream")', body)


if __name__ == "__main__":
    unittest.main()
