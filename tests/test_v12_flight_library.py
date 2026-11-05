import io
from pathlib import Path
import struct
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "Ground_Station"))

from flight_library import FlightImportError, FlightLibrary


def fixture(trailing=b""):
    header = bytearray(32)
    header[:4] = b"RKT4"
    struct.pack_into("<HHHHfH", header, 4, 2, 32, 16, 25, 101325.0, 4100)
    records = b"".join([
        struct.pack("<IffHH", 0, 101325.0, 20.0, 4100, 1),
        struct.pack("<IffHH", 40000, 100900.0, 20.1, 4050, 1),
        struct.pack("<IffHH", 80000, 100700.0, 20.2, 4000, 1),
    ])
    return bytes(header) + records + trailing


class FlightLibraryTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.library = FlightLibrary(Path(self.temporary.name))

    def tearDown(self):
        self.temporary.cleanup()

    def test_import_preserves_original_and_deduplicates_by_checksum(self):
        raw = fixture()
        first = self.library.import_stream("V12D007.BIN", io.BytesIO(raw), len(raw))
        second = self.library.import_stream("copy.BIN", io.BytesIO(raw), len(raw))
        self.assertFalse(first["duplicate"])
        self.assertTrue(second["duplicate"])
        self.assertEqual(first["id"], second["id"])
        metadata, path = self.library.get(first["id"])
        self.assertEqual(raw, path.read_bytes())
        self.assertEqual("V12D007.BIN", metadata["original_name"])

    def test_summary_and_exports_are_generated_from_valid_rkt4(self):
        imported = self.library.import_stream("V12D001.BIN", io.BytesIO(fixture()))
        summary = imported["summary"]
        self.assertEqual(3, summary["records"])
        self.assertEqual(25, summary["sample_rate_hz"])
        self.assertGreater(summary["peak_altitude_m_estimated"], 0)
        self.assertIn(b"altitude_m_estimated", self.library.csv_export(imported["id"]))
        self.assertIn(b'"analysis_version": 1', self.library.json_export(imported["id"]))
        self.assertIn(b"Print / Save PDF", self.library.report_html(imported["id"]))

    def test_trailing_bytes_are_retained_and_reported(self):
        imported = self.library.import_stream("AIRP001.BIN", io.BytesIO(fixture(b"xyz")))
        self.assertEqual(3, imported["summary"]["trailing_bytes"])
        self.assertRegex(imported["summary"]["warnings"][0], "truncated")

    def test_incomplete_transfer_is_rejected_without_library_entry(self):
        raw = fixture()
        with self.assertRaisesRegex(FlightImportError, "incomplete transfer"):
            self.library.import_stream("V12D002.BIN", io.BytesIO(raw), len(raw) + 1)
        self.assertEqual([], self.library.list())

    def test_corrupt_header_and_unsafe_filename_are_rejected(self):
        with self.assertRaisesRegex(FlightImportError, "magic"):
            self.library.import_stream("bad.BIN", io.BytesIO(b"x" * 64))
        with self.assertRaisesRegex(FlightImportError, "filename"):
            self.library.import_stream("../bad.BIN", io.BytesIO(fixture()))


if __name__ == "__main__":
    unittest.main()
