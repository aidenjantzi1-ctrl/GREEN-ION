import io
import json
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
                   header_crc: int | None = None, torn: bytes = b"",
                   gps_current: bool = False,
                   gps_altitude_current: bool | None = None) -> bytes:
    header = bytearray(32)
    struct.pack_into("<4sHHHHH2xII4x", header, 0, b"GIS1", 1, 32, 68, rate, 7,
                     start_us, zlib.crc32(rkt4[:32]) & 0xFFFFFFFF if header_crc is None else header_crc)
    struct.pack_into("<I", header, 28, zlib.crc32(header[:28]) & 0xFFFFFFFF)
    record = bytearray(sensor_log.RECORD.pack(
        40000, 1, 1.0, 2.0, 3.0, 0.1, 0.2, 0.3,
        371000000 if gps_current else 0, -1219000000 if gps_current else 0,
        123.5 if gps_current else 0.0, 5.0 if gps_current else 0.0,
        0 if gps_current else 0xFFFFFFFF, 40000 if gps_current else 0,
        sensor_log.FLAG_IMU_VALID | sensor_log.FLAG_GPS_HAS_POSITION | sensor_log.FLAG_GPS_CURRENT if gps_current else sensor_log.FLAG_IMU_VALID,
        7 if gps_current else 0, 12, 34, 56, 0,
    ))
    if gps_altitude_current is not None:
        record[62] = sensor_log.GPS_QUALITY_KNOWN | (
            sensor_log.GPS_ALTITUDE_CURRENT if gps_altitude_current else 0)
        record[63] = sensor_log.GPS_QUALITY_MARKER
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
        self.assertEqual("manual_upload", metadata["source"])
        self.assertIsNone(metadata["flight_date"])
        self.assertTrue(metadata["imported_utc"].endswith("+00:00"))

    def test_user_details_are_separate_from_recorded_facts_and_survive_duplicate(self):
        raw = fixture()
        first = self.library.import_stream("V12D007.BIN", io.BytesIO(raw), source_kind="feather_sd")
        edited = self.library.update_details(first["id"], "  Field test  ", "Estes So Long", "2026-11-05")
        self.assertEqual(("Field test", "Estes So Long", "2026-11-05"),
                         (edited["label"], edited["rocket_profile"], edited["flight_date"]))
        duplicate = self.library.import_stream("V12D007.BIN", io.BytesIO(raw))
        self.assertEqual("feather_sd", duplicate["source"])
        self.assertEqual("Field test", duplicate["label"])
        for label, profile, date_value in (("bad\nname", "", None),
                                           ("", "X" * 61, None),
                                           ("", "", "yesterday")):
            with self.assertRaises(FlightImportError):
                self.library.update_details(first["id"], label, profile, date_value)
        self.assertEqual("Field test", self.library.get(first["id"])[0]["label"])

    def test_summary_and_exports_are_generated_from_valid_rkt4(self):
        imported = self.library.import_stream("V12D001.BIN", io.BytesIO(fixture()))
        summary = imported["summary"]
        self.assertEqual(3, summary["records"])
        self.assertEqual(25, summary["sample_rate_hz"])
        self.assertGreater(summary["peak_altitude_m_estimated"], 0)
        self.assertIn(b"altitude_m_estimated", self.library.csv_export(imported["id"]))
        self.assertEqual(1, json.loads(self.library.json_export(imported["id"]))["summary"]["analysis_version"])
        self.assertIn(b"Print / Save PDF", self.library.report_html(imported["id"]))
        self.library.update_details(imported["id"], "Flight <A>", "Estes So Long", "2026-11-05")
        report = self.library.report_html(imported["id"])
        self.assertIn(b"Flight &lt;A&gt;", report)
        self.assertNotIn(b"<h2>Flight <A></h2>", report)
        self.assertIn(b"Estimated barometric altitude", report)
        self.assertIn(b"Peak IMU acceleration magnitude", report)
        self.assertIn(b"does not generate a native PDF", report)

    def test_trailing_bytes_are_retained_and_reported(self):
        imported = self.library.import_stream("AIRP001.BIN", io.BytesIO(fixture(b"xyz")))
        self.assertEqual(3, imported["summary"]["trailing_bytes"])
        self.assertRegex(imported["summary"]["warnings"][0], "truncated")

    def test_nonfinite_record_is_invalid_and_json_safe(self):
        raw = fixture() + struct.pack("<IffHH", 120000, float("nan"), float("inf"), 3990, 1)
        imported = self.library.import_stream("V12D007.BIN", io.BytesIO(raw))
        self.assertEqual(1, imported["summary"]["invalid_records"])
        exported = json.loads(self.library.json_export(imported["id"]))
        self.assertFalse(exported["records"][-1]["valid"])
        self.assertIsNone(exported["records"][-1]["pressure_pa"])
        self.assertIsNone(exported["records"][-1]["temperature_c"])
        damaged = bytearray(fixture())
        struct.pack_into("<f", damaged, 12, float("nan"))
        with self.assertRaisesRegex(FlightImportError, "baseline"):
            self.library.import_stream("V12D008.BIN", io.BytesIO(damaged))

    def test_incomplete_transfer_is_rejected_without_library_entry(self):
        raw = fixture()
        with self.assertRaisesRegex(FlightImportError, "incomplete transfer"):
            self.library.import_stream("V12D002.BIN", io.BytesIO(raw), len(raw) + 1)
        self.assertEqual([], self.library.list())

    def test_changed_library_raw_is_not_reused_as_a_duplicate_or_replayed(self):
        raw = fixture()
        imported = self.library.import_stream("V12D007.BIN", io.BytesIO(raw))
        _metadata, path = self.library.get(imported["id"])
        path.write_bytes(raw[:-1] + b"x")
        with self.assertRaisesRegex(FlightImportError, "checksum"):
            self.library.get(imported["id"])
        with self.assertRaisesRegex(FlightImportError, "checksum"):
            self.library.analysis_timeline(imported["id"])
        with self.assertRaisesRegex(FlightImportError, "checksum"):
            self.library.import_stream("V12D007.BIN", io.BytesIO(raw))
        self.assertFalse(list(self.library.root.glob("*.part")))

    def test_laptop_flight_trash_is_reversible_and_keeps_raw_bytes(self):
        raw = fixture()
        imported = self.library.import_stream("V12D007.BIN", io.BytesIO(raw))
        sensor = sensor_fixture(raw)
        self.library.import_sensor_stream(imported["id"], "V12D007.SEN", io.BytesIO(sensor))
        moved = self.library.trash_flight(imported["id"])
        self.assertTrue(moved["feather_sd_unchanged"])
        self.assertEqual([], self.library.list())
        self.assertEqual(1, len(self.library.list_trash()))
        trashed = FlightLibrary(self.library.root / "trash" / imported["id"])
        self.assertEqual(raw, trashed.get(imported["id"])[1].read_bytes())
        self.assertEqual(sensor, trashed.get_sensor(imported["id"])[1].read_bytes())
        with self.assertRaisesRegex(FlightImportError, "Trash copy"):
            self.library.trash_flight(imported["id"])
        self.library.restore_trash(imported["id"])
        self.assertEqual(raw, self.library.get(imported["id"])[1].read_bytes())
        self.assertEqual(sensor, self.library.get_sensor(imported["id"])[1].read_bytes())
        self.assertEqual([], self.library.list_trash())

    def test_trash_integrity_failure_does_not_restore_damaged_raw_file(self):
        imported = self.library.import_stream("V12D007.BIN", io.BytesIO(fixture()))
        self.library.trash_flight(imported["id"])
        trashed_raw = self.library.root / "trash" / imported["id"] / "raw" / f"{imported['id']}.bin"
        trashed_raw.write_bytes(b"damaged")
        self.assertEqual("invalid", self.library.list_trash()[0]["integrity"])
        with self.assertRaisesRegex(FlightImportError, "checksum"):
            self.library.restore_trash(imported["id"])
        self.assertEqual([], self.library.list())

    def test_corrupt_flight_metadata_is_not_exposed_as_a_valid_library_row(self):
        raw = fixture()
        imported = self.library.import_stream("V12D007.BIN", io.BytesIO(raw))
        metadata_path = self.library.meta / f"{imported['id']}.json"
        metadata_path.write_text(json.dumps({"id": imported["id"], "raw_file": "other.bin"}))
        self.assertEqual([], self.library.list())
        with self.assertRaisesRegex(FlightImportError, "metadata no longer matches"):
            self.library.get(imported["id"])
        self.assertEqual(raw, (self.library.raw / f"{imported['id']}.bin").read_bytes())

    def test_plausible_but_wrong_summary_size_is_rejected_on_verified_access(self):
        raw = fixture()
        imported = self.library.import_stream("V12D007.BIN", io.BytesIO(raw))
        metadata_path = self.library.meta / f"{imported['id']}.json"
        damaged = json.loads(metadata_path.read_text(encoding="utf-8"))
        damaged["summary"]["bytes"] += 16
        damaged["summary"]["records"] += 1
        metadata_path.write_text(json.dumps(damaged), encoding="utf-8")
        self.assertEqual([], self.library.list())
        with self.assertRaisesRegex(FlightImportError, "summary size"):
            self.library.get(imported["id"])
        with self.assertRaisesRegex(FlightImportError, "metadata no longer matches"):
            self.library.import_stream("V12D007.BIN", io.BytesIO(raw))
        self.assertEqual(raw, (self.library.raw / f"{imported['id']}.bin").read_bytes())

    def test_damaged_summary_or_sensor_metadata_cannot_break_library_or_duplicate_import(self):
        raw = fixture()
        imported = self.library.import_stream("V12D007.BIN", io.BytesIO(raw))
        metadata_path = self.library.meta / f"{imported['id']}.json"
        for field, value in (("duration_s", "bad"), ("peak_altitude_m_estimated", float("nan")),
                             ("warnings", ["ok", 3])):
            with self.subTest(field=field):
                damaged = json.loads(json.dumps(imported))
                damaged["summary"][field] = value
                metadata_path.write_text(json.dumps(damaged), encoding="utf-8")
                self.assertEqual([], self.library.list())
                with self.assertRaisesRegex(FlightImportError, "metadata no longer matches"):
                    self.library.get(imported["id"])
                with self.assertRaisesRegex(FlightImportError, "metadata no longer matches"):
                    self.library.import_stream("V12D007.BIN", io.BytesIO(raw))
                self.assertEqual(raw, (self.library.raw / f"{imported['id']}.bin").read_bytes())
        metadata_path.write_text(json.dumps(imported), encoding="utf-8")
        self.library.import_sensor_stream(imported["id"], "V12D007.SEN",
                                          io.BytesIO(sensor_fixture(raw)))
        damaged = json.loads(metadata_path.read_text(encoding="utf-8"))
        damaged["sensor"]["inspection"]["status"] = None
        metadata_path.write_text(json.dumps(damaged), encoding="utf-8")
        self.assertEqual([], self.library.list())
        with self.assertRaisesRegex(FlightImportError, "metadata no longer matches"):
            self.library.get(imported["id"])

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

    def test_analysis_timeline_aligns_sensor_sample_without_calling_it_live(self):
        raw = fixture()
        imported = self.library.import_stream("V12D007.BIN", io.BytesIO(raw))
        self.library.import_sensor_stream(imported["id"], "V12D007.SEN",
                                          io.BytesIO(sensor_fixture(raw)))
        timeline = self.library.analysis_timeline(imported["id"])
        self.assertEqual((3, 3, 1), (timeline["total_records"], timeline["displayed_records"],
                                    timeline["display_stride"]))
        self.assertEqual("onboard_rkt4", timeline["source"])
        self.assertEqual("valid", timeline["sensor_status"])
        self.assertEqual(0.04, timeline["points"][1]["elapsed_s"])
        self.assertEqual(0, timeline["points"][1]["sensor"]["time_delta_ms"])
        self.assertIsNone(timeline["points"][0]["sensor"])
        self.assertEqual("missing", timeline["points"][1]["sensor"]["gps_status"])
        self.assertGreater(timeline["peak_vertical_speed_mps_estimated"], 0)
        self.assertIsNotNone(timeline["peak_acceleration_g"])
        json.dumps(timeline, allow_nan=False)
        with self.assertRaisesRegex(FlightImportError, "point limit"):
            self.library.analysis_timeline(imported["id"], 10)

    def test_analysis_reports_current_gps_peak_and_elapsed_apogee(self):
        raw = fixture()
        imported = self.library.import_stream("V12D007.BIN", io.BytesIO(raw))
        self.library.import_sensor_stream(imported["id"], "V12D007.SEN",
                                          io.BytesIO(sensor_fixture(raw, gps_current=True)))
        timeline = self.library.analysis_timeline(imported["id"])
        self.assertEqual(123.5, timeline["peak_current_gps_altitude_m"])
        self.assertIsNone(timeline["points"][1]["sensor"]["gps_altitude_current"])
        self.assertEqual([37.1, -121.9], timeline["last_current_gps_fix"])
        self.assertIsNotNone(timeline["estimated_apogee_elapsed_s"])
        self.assertIn(b"Peak GPS altitude (quality unverified)", self.library.report_html(imported["id"]))

    def test_extended_gis1_stale_altitude_is_excluded_without_losing_position(self):
        raw = fixture()
        imported = self.library.import_stream("V12D007.BIN", io.BytesIO(raw))
        extended = sensor_fixture(raw, gps_current=True, gps_altitude_current=False)
        self.library.import_sensor_stream(imported["id"], "V12D007.SEN", io.BytesIO(extended))
        timeline = self.library.analysis_timeline(imported["id"])
        self.assertIsNone(timeline["peak_current_gps_altitude_m"])
        self.assertEqual([37.1, -121.9], timeline["last_current_gps_fix"])
        self.assertIs(timeline["points"][1]["sensor"]["gps_altitude_current"], False)
        self.assertIs(sensor_log.decode_record(extended[32:])["gps_altitude_current"], False)
        self.assertEqual("false", sensor_log.csv_row(sensor_log.decode_record(extended[32:]))["gps_altitude_current"])
        current = sensor_fixture(raw, gps_current=True, gps_altitude_current=True)
        self.assertIs(sensor_log.decode_record(current[32:])["gps_altitude_current"], True)
        self.assertEqual("true", sensor_log.csv_row(sensor_log.decode_record(current[32:]))["gps_altitude_current"])
        malformed = bytearray(current[32:])
        malformed[62] = sensor_log.GPS_ALTITUDE_CURRENT
        struct.pack_into("<I", malformed, 64, zlib.crc32(malformed[:64]) & 0xFFFFFFFF)
        with self.assertRaisesRegex(sensor_log.SensorLogError, "bad_gps_quality_flags"):
            sensor_log.decode_record(bytes(malformed))

    def test_large_timeline_is_bounded_and_retains_peak_and_final_sample(self):
        header = fixture()[:32]
        records = b"".join(struct.pack("<IffHH", index * 40000,
                                        101325 - (5000 if index == 123 else index), 20.0, 4000, 1)
                           for index in range(1001))
        imported = self.library.import_stream("V12D007.BIN", io.BytesIO(header + records))
        timeline = self.library.analysis_timeline(imported["id"], 100)
        self.assertLessEqual(timeline["displayed_records"], 102)
        self.assertEqual(1000, timeline["points"][-1]["index"])
        self.assertEqual(123, timeline["estimated_peak_index"])
        self.assertIn(123, [point["index"] for point in timeline["points"]])

    def test_full_size_export_generators_keep_chunks_bounded_and_complete(self):
        header = fixture()[:32]
        records = b"".join(struct.pack("<IffHH", index * 40000, 101325 - index / 10,
                                       20.0, 4000, 1) for index in range(3000))
        imported = self.library.import_stream("V12D007.BIN", io.BytesIO(header + records))
        json_chunks = list(self.library.iter_json_export(imported["id"]))
        csv_chunks = list(self.library.iter_csv_export(imported["id"]))
        self.assertGreater(len(json_chunks), 2)
        self.assertGreater(len(csv_chunks), 1)
        self.assertLess(max(map(len, json_chunks + csv_chunks)), 70000)
        self.assertEqual(3000, len(json.loads(b"".join(json_chunks))["records"]))
        self.assertEqual(3001, len(b"".join(csv_chunks).splitlines()))

    def test_sensor_csv_streams_only_inspected_valid_prefix_in_bounded_chunks(self):
        raw = fixture()
        imported = self.library.import_stream("V12D007.BIN", io.BytesIO(raw))
        example = sensor_fixture(raw)
        records = []
        for sequence in range(3000):
            record = bytearray(example[32:])
            struct.pack_into("<I", record, 4, sequence)
            struct.pack_into("<I", record, 64, zlib.crc32(record[:64]) & 0xFFFFFFFF)
            records.append(bytes(record))
        sensor = example[:32] + b"".join(records) + b"torn"
        attached = self.library.import_sensor_stream(imported["id"], "V12D007.SEN", io.BytesIO(sensor))
        self.assertEqual("torn_tail", attached["inspection"]["status"])
        chunks = list(self.library.iter_sensor_csv_export(imported["id"]))
        self.assertGreater(len(chunks), 1)
        self.assertLess(max(map(len, chunks)), 70000)
        self.assertEqual(3001, len(b"".join(chunks).splitlines()))
        self.assertEqual(b"".join(chunks), self.library.sensor_csv_export(imported["id"]))
        timeline = self.library.analysis_timeline(imported["id"])
        self.assertEqual("torn_tail", timeline["sensor_status"])
        self.assertIsNotNone(timeline["points"][1]["sensor"])

    def test_timeline_chooses_nearest_sensor_even_when_sensor_timestamps_are_out_of_order(self):
        raw = fixture()
        imported = self.library.import_stream("V12D007.BIN", io.BytesIO(raw))
        example = sensor_fixture(raw)
        rows = []
        for sequence, timestamp in ((1, 50000), (2, 30000)):
            record = bytearray(example[32:])
            struct.pack_into("<I", record, 0, timestamp)
            struct.pack_into("<I", record, 4, sequence)
            struct.pack_into("<I", record, 64, zlib.crc32(record[:64]) & 0xFFFFFFFF)
            rows.append(bytes(record))
        self.library.import_sensor_stream(imported["id"], "V12D007.SEN",
                                          io.BytesIO(example[:32] + b"".join(rows)))
        timeline = self.library.analysis_timeline(imported["id"])
        self.assertEqual(-10.0, timeline["points"][1]["sensor"]["time_delta_ms"])

    def test_timeline_rejects_plausible_but_wrong_sensor_inspection(self):
        raw = fixture()
        imported = self.library.import_stream("V12D007.BIN", io.BytesIO(raw))
        self.library.import_sensor_stream(imported["id"], "V12D007.SEN",
                                          io.BytesIO(sensor_fixture(raw)))
        metadata_path = self.library.meta / f"{imported['id']}.json"
        metadata = json.loads(metadata_path.read_text(encoding="utf-8"))
        metadata["sensor"]["inspection"]["sequence_gaps"] = 1
        metadata_path.write_text(json.dumps(metadata), encoding="utf-8")
        with self.assertRaisesRegex(FlightImportError, "sensor inspection"):
            self.library.analysis_timeline(imported["id"])
        self.assertEqual(sensor_fixture(raw),
                         (self.library.raw / f"{metadata['sensor']['sha256']}.sen").read_bytes())

    def test_empty_or_invalid_flight_does_not_claim_a_peak_speed(self):
        imported = self.library.import_stream("V12D007.BIN", io.BytesIO(fixture()[:32]))
        timeline = self.library.analysis_timeline(imported["id"])
        self.assertEqual([], timeline["points"])
        self.assertIsNone(timeline["peak_vertical_speed_mps_estimated"])
        self.assertIn(b"Unavailable", self.library.report_html(imported["id"]))

    def test_comparison_uses_recorded_evidence_and_unknowns(self):
        first_raw = fixture()
        first = self.library.import_stream("V12D007.BIN", io.BytesIO(first_raw))
        second = self.library.import_stream("V12D008.BIN", io.BytesIO(fixture(b"partial")))
        self.library.update_details(first["id"], "Green Eggs", "Estes Green Eggs", "2026-11-05")
        self.library.import_sensor_stream(first["id"], "V12D007.SEN", io.BytesIO(sensor_fixture(first_raw)))
        compared = self.library.compare([first["id"], second["id"]])
        self.assertEqual("onboard_recordings", compared["source"])
        self.assertEqual("Green Eggs", compared["flights"][0]["label"])
        self.assertEqual("Estes Green Eggs", compared["flights"][0]["rocket_profile"])
        self.assertEqual("valid", compared["flights"][0]["sensor_status"])
        self.assertIsNone(compared["flights"][0]["last_current_gps_fix"])
        self.assertIsNone(compared["flights"][1]["rocket_profile"])
        self.assertEqual("missing", compared["flights"][1]["sensor_status"])
        self.assertTrue(compared["flights"][1]["rkt4_warnings"])
        self.assertIn("not a confirmed landing", compared["gps_note"])
        with self.assertRaisesRegex(FlightImportError, "distinct"):
            self.library.compare([first["id"], first["id"]])


if __name__ == "__main__":
    unittest.main()
