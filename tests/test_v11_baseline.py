"""V11 Stage A: host-side baseline checks for the unchanged V10 RKT4 contract."""
import math
import struct
import unittest

HEADER_BYTES, SAMPLE_BYTES = 32, 16


def make_rkt4(rows, rate=25, baseline=101325.0):
    header = bytearray(HEADER_BYTES)
    struct.pack_into("<4sHHHHfH", header, 0, b"RKT4", 2, HEADER_BYTES, SAMPLE_BYTES,
                     rate, baseline, 4000)
    return bytes(header) + b"".join(struct.pack("<IffHH", *row) for row in rows)


def decode_rkt4(data):
    if len(data) < HEADER_BYTES:
        raise ValueError("short header")
    magic, version, header, sample, rate = struct.unpack_from("<4sHHHH", data)
    if (magic, version, header, sample) != (b"RKT4", 2, 32, 16):
        raise ValueError("invalid header")
    count, trailing = divmod(len(data) - HEADER_BYTES, SAMPLE_BYTES)
    rows = [struct.unpack_from("<IffHH", data, HEADER_BYTES + i * SAMPLE_BYTES)
            for i in range(count)]
    return rate, rows, trailing


def altitude_m(pressure_pa, baseline_pa):
    return 44330.0 * (1.0 - math.pow(pressure_pa / baseline_pa, 0.190294957))


class V11BaselineTests(unittest.TestCase):
    def setUp(self):
        self.rows = [
            (0, 101325.0, 20.0, 4000, 1),
            (40000, 101300.0, 20.1, 3998, 1),
            (80000, 101275.0, 20.2, 3996, 1),
        ]

    def test_v10_header_and_sample_layout_round_trip(self):
        rate, rows, trailing = decode_rkt4(make_rkt4(self.rows, 25))
        self.assertEqual((rate, trailing), (25, 0))
        for actual, expected in zip(rows, self.rows):
            self.assertEqual((actual[0], actual[3], actual[4]), (expected[0], expected[3], expected[4]))
            self.assertAlmostEqual(actual[1], expected[1], places=3)
            self.assertAlmostEqual(actual[2], expected[2], places=5)

    def test_allowed_recording_rates_remain_exact(self):
        for rate in (10, 25, 50):
            self.assertEqual(decode_rkt4(make_rkt4(self.rows, rate))[0], rate)

    def test_interrupted_log_keeps_complete_records(self):
        raw = make_rkt4(self.rows) + b"\x00\x01\x02"
        rate, rows, trailing = decode_rkt4(raw)
        self.assertEqual(rate, 25)
        self.assertEqual(len(rows), len(self.rows))
        for actual, expected in zip(rows, self.rows):
            self.assertEqual((actual[0], actual[3], actual[4]), (expected[0], expected[3], expected[4]))
            self.assertAlmostEqual(actual[1], expected[1], places=3)
            self.assertAlmostEqual(actual[2], expected[2], places=5)
        self.assertEqual(trailing, 3)

    def test_timestamp_scale_is_microseconds(self):
        _, rows, _ = decode_rkt4(make_rkt4(self.rows))
        self.assertAlmostEqual((rows[2][0] - rows[0][0]) / 1_000_000, 0.08)

    def test_barometric_altitude_baseline_and_ascent(self):
        self.assertAlmostEqual(altitude_m(101325.0, 101325.0), 0.0, places=5)
        self.assertGreater(altitude_m(100000.0, 101325.0), 100.0)

    def test_bad_headers_are_rejected(self):
        with self.assertRaises(ValueError):
            decode_rkt4(b"RKT4")
        bad = bytearray(make_rkt4(self.rows)); bad[:4] = b"NOPE"
        with self.assertRaises(ValueError):
            decode_rkt4(bytes(bad))


if __name__ == "__main__":
    unittest.main()
