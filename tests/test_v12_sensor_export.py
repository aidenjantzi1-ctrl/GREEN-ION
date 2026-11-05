from pathlib import Path
import csv
import importlib.util
import struct
import sys
import tempfile
import unittest
import zlib

ROOT = Path(__file__).resolve().parents[1]
MODULE_PATH = ROOT / "Ground_Station" / "sensor_log.py"
SPEC = importlib.util.spec_from_file_location("v12_sensor_log", MODULE_PATH)
sensor_log = importlib.util.module_from_spec(SPEC)
assert SPEC and SPEC.loader
sys.modules[SPEC.name] = sensor_log
SPEC.loader.exec_module(sensor_log)


def rkt4_header() -> bytes:
    return struct.pack("<4sHHHHfH14x", b"RKT4", 2, 32, 16, 50, 101325.0, 4000)


def header(sequence: int = 7) -> bytes:
    data = bytearray(32)
    struct.pack_into("<4sHHHHH2xII4x", data, 0, b"GIS1", 1, 32, 68, 50,
                     sequence, 1234, zlib.crc32(rkt4_header()) & 0xFFFFFFFF)
    struct.pack_into("<I", data, 28, zlib.crc32(data[:28]) & 0xFFFFFFFF)
    return bytes(data)


def record(sequence: int, flags: int, *, corrupt: bool = False) -> bytes:
    has_position = bool(flags & sensor_log.FLAG_GPS_HAS_POSITION)
    data = bytearray(sensor_log.RECORD.pack(
        1000 + sequence * 20000, sequence,
        1.0, 2.0, 3.0, 0.1, 0.2, 0.3,
        374219999 if has_position else 0,
        -1220840575 if has_position else 0,
        12.5 if has_position else 0.0,
        3.25 if has_position else 0.0,
        500 if has_position else 0xFFFFFFFF,
        900 if has_position else 0,
        flags, 9 if has_position else 0, 12, 34, 56, 0,
    ))
    struct.pack_into("<I", data, 64, zlib.crc32(data[:64]) & 0xFFFFFFFF)
    if corrupt:
        data[20] ^= 0x40
    return bytes(data)


class SensorExportTests(unittest.TestCase):
    def test_clean_log_preserves_live_stale_and_missing_gps(self):
        live = sensor_log.FLAG_IMU_VALID | sensor_log.FLAG_GPS_HAS_POSITION | sensor_log.FLAG_GPS_CURRENT
        stale = sensor_log.FLAG_IMU_VALID | sensor_log.FLAG_GPS_HAS_POSITION
        data = header() + record(1, live) + record(2, stale) + record(3, 0)
        decoded_header, rows, inspection = sensor_log.parse_sensor_log(data, 7)
        self.assertEqual(inspection.status, "valid")
        self.assertEqual(inspection.valid_prefix_records, 3)
        self.assertTrue(sensor_log.validate_rkt4_binding(decoded_header, rkt4_header()))
        exported = [sensor_log.csv_row(row) for row in rows]
        self.assertEqual([row["gps_status"] for row in exported], ["live", "stale", "missing"])
        self.assertEqual(exported[1]["latitude_deg"], "37.4219999")
        self.assertEqual(exported[2]["latitude_deg"], "")
        self.assertEqual(exported[2]["imu_status"], "missing")

    def test_binding_and_filename_sequence_mismatch_are_rejected(self):
        decoded_header, _, _ = sensor_log.parse_sensor_log(header(), 7)
        changed = bytearray(rkt4_header())
        changed[16] ^= 1
        self.assertFalse(sensor_log.validate_rkt4_binding(decoded_header, changed))
        changed_rate = bytearray(rkt4_header())
        struct.pack_into("<H", changed_rate, 10, 25)
        changed_header = bytearray(header())
        struct.pack_into("<I", changed_header, 20, zlib.crc32(changed_rate) & 0xFFFFFFFF)
        struct.pack_into("<I", changed_header, 28, zlib.crc32(changed_header[:28]) & 0xFFFFFFFF)
        mismatched, _, _ = sensor_log.parse_sensor_log(bytes(changed_header), 7)
        self.assertFalse(sensor_log.validate_rkt4_binding(mismatched, changed_rate))
        self.assertEqual(sensor_log._rkt4_sequence_from_name(Path("V12D007.BIN")), 7)
        self.assertEqual(sensor_log._sequence_from_name(Path("TEST007.SEN")), 7)
        self.assertNotEqual(sensor_log._rkt4_sequence_from_name(Path("V12D008.BIN")),
                            decoded_header.flight_sequence)
        with self.assertRaisesRegex(sensor_log.SensorLogError, "filename_sequence_mismatch"):
            sensor_log.parse_sensor_log(header(), 8)

    def test_corrupt_record_stops_at_valid_prefix_and_torn_tail_is_reported(self):
        flags = sensor_log.FLAG_IMU_VALID
        _, rows, corrupt = sensor_log.parse_sensor_log(
            header() + record(1, flags) + record(2, flags, corrupt=True)
        )
        self.assertEqual(len(rows), 1)
        self.assertEqual(corrupt.status, "corrupt")
        self.assertEqual(corrupt.first_invalid_record, 1)
        self.assertEqual(corrupt.first_invalid_reason, "bad_record_crc")
        _, rows, torn = sensor_log.parse_sensor_log(header() + record(1, flags) + b"torn")
        self.assertEqual(len(rows), 1)
        self.assertEqual((torn.status, torn.trailing_bytes), ("torn_tail", 4))
        _, _, gap = sensor_log.parse_sensor_log(
            header() + record(1, flags) + record(3, flags)
        )
        self.assertEqual(gap.sequence_gaps, 1)

    def test_csv_output_is_deterministic(self):
        flags = sensor_log.FLAG_IMU_VALID | sensor_log.FLAG_GPS_HAS_POSITION
        _, rows, _ = sensor_log.parse_sensor_log(header() + record(1, flags))
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "flight.csv"
            sensor_log.write_csv(rows, output)
            with output.open(newline="", encoding="utf-8") as handle:
                exported = list(csv.DictReader(handle))
        self.assertEqual(len(exported), 1)
        self.assertEqual(tuple(exported[0]), sensor_log.CSV_FIELDS)
        self.assertEqual(exported[0]["gps_status"], "stale")
        self.assertEqual(exported[0]["gps_fix_age_ms"], "500")

    def test_bad_header_crc_and_unlabeled_position_are_rejected(self):
        damaged = bytearray(header())
        damaged[12] ^= 1
        with self.assertRaisesRegex(sensor_log.SensorLogError, "bad_header_crc"):
            sensor_log.parse_sensor_log(damaged)
        invalid = bytearray(record(1, 0))
        struct.pack_into("<i", invalid, 32, 1)
        struct.pack_into("<I", invalid, 64, zlib.crc32(invalid[:64]) & 0xFFFFFFFF)
        _, rows, inspection = sensor_log.parse_sensor_log(header() + invalid)
        self.assertEqual(rows, [])
        self.assertEqual(inspection.first_invalid_reason, "unlabeled_last_position")


if __name__ == "__main__":
    unittest.main()
