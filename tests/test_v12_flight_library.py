import io
from pathlib import Path
import struct
import sys
import tempfile
import unittest
import zlib

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "Ground_Station"))

from flight_library import FlightImportError, FlightLibrary
import sensor_log


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


def sensor_fixture(rkt4: bytes, *, start_us: int = 40000, rate: int = 25,
                   header_crc: int | None = None, torn: bytes = b"") -> bytes:
    header = bytearray(32)
    struct.pack_into("<4sHHHHH2xII4x", header, 0, b"GIS1", 1, 32, 68, rate, 7,
                     start_us, zlib.crc32(rkt4[:32]) & 0xFFFFFFFF if header_crc is None else header_crc)
    struct.pack_into("<I", header, 28, zlib.crc32(header[:28]) & 0xFFFFFFFF)
    record = bytearray(sensor_log.RECORD.pack(
        40000, 1, 1.0, 2.0, 3.0, 0.1, 0.2, 0.3,
        0, 0, 0.0, 0.0, 0xFFFFFFFF, 0,
        sensor_log.FLAG_IMU_VALID, 0, 12, 34, 56, 0,
    ))
    struct.pack_into("<I", record, 64, zlib.crc32(record[:64]) & 0xFFFFFFFF)
    return bytes(header) + bytes(record) + torn


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

    def test_sensor_companion_binding_preserves_raw_and_exports_valid_prefix(self):
        raw = fixture()
        imported = self.library.import_stream("V12D007.BIN", io.BytesIO(raw))
        sensor = sensor_fixture(raw, torn=b"x")
        attached = self.library.import_sensor_stream(imported["id"], "V12D007.SEN",
                                                     io.BytesIO(sensor), len(sensor))
        self.assertEqual("torn_tail", attached["inspection"]["status"])
        self.assertEqual(1, attached["inspection"]["valid_prefix_records"])
        saved, path = self.library.get_sensor(imported["id"])
        self.assertEqual(sensor, path.read_bytes())
        self.assertEqual(attached["sha256"], saved["sha256"])
        self.assertIn(b"acceleration_x_mps2", self.library.sensor_csv_export(imported["id"]))
        self.assertEqual(attached["sha256"], self.library.list()[0]["sensor"]["sha256"])
        duplicate = self.library.import_sensor_stream(imported["id"], "V12D007.SEN", io.BytesIO(sensor))
        self.assertTrue(duplicate["duplicate"])
        path.write_bytes(sensor[:-1] + b"y")
        with self.assertRaisesRegex(FlightImportError, "checksum"):
            self.library.get_sensor(imported["id"])

    def test_sensor_companion_rejects_unrelated_or_incomplete_data(self):
        raw = fixture()
        imported = self.library.import_stream("V12D007.BIN", io.BytesIO(raw))
        sensor = sensor_fixture(raw)
        cases = [
            ("V12D008.SEN", sensor, "filename"),
            ("V12D007.SEN", sensor_fixture(raw, header_crc=0), "bind"),
            ("V12D007.SEN", sensor_fixture(raw, rate=50), "bind"),
            ("V12D007.SEN", sensor_fixture(raw, start_us=10000000), "outside"),
        ]
        for name, data, message in cases:
            with self.subTest(message=message), self.assertRaisesRegex(FlightImportError, message):
                self.library.import_sensor_stream(imported["id"], name, io.BytesIO(data))
        with self.assertRaisesRegex(FlightImportError, "incomplete"):
            self.library.import_sensor_stream(imported["id"], "V12D007.SEN",
                                              io.BytesIO(sensor), len(sensor) + 1)
        with self.assertRaisesRegex(FlightImportError, "16 MB"):
            self.library.import_sensor_stream(imported["id"], "V12D007.SEN",
                                              io.BytesIO(sensor), 16 * 1024 * 1024 + 1)
        self.assertNotIn("sensor", self.library.get(imported["id"])[0])


if __name__ == "__main__":
    unittest.main()
