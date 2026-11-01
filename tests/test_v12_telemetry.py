from pathlib import Path
import importlib.util
import struct
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location("green_ion_telemetry", ROOT / "Ground_Station" / "telemetry.py")
telemetry = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = telemetry
SPEC.loader.exec_module(telemetry)


class V12TelemetryTests(unittest.TestCase):
    def make(self, **updates):
        values = dict(
            sequence=42, uptime_ms=123456, latitude=37.4219999,
            longitude=-122.0840575, altitude_m=152.34, speed_mps=41.25,
            pressure_pa=98765, gps_altitude_m=149.25,
            acceleration_g=5.125, acceleration_x_g=-1.25,
            acceleration_y_g=0.5, acceleration_z_g=4.94,
            rotation_x_dps=-120.4, rotation_y_dps=15.2,
            rotation_z_dps=830.0, battery_v=3.91, gps_fix_age_ms=500,
            satellites=11, flight_state=3,
            flags=telemetry.FLAG_GPS_HAS_POSITION | telemetry.FLAG_GPS_CURRENT,
            health=0,
        )
        values.update(updates)
        return telemetry.Telemetry(**values)

    def test_packet_layout_is_fixed_70_bytes(self):
        self.assertEqual(telemetry.PACKET.size, 70)
        self.assertEqual(len(telemetry.encode_packet(self.make())), 70)

    def test_round_trip_preserves_scaled_values(self):
        decoded = telemetry.decode_packet(telemetry.encode_packet(self.make()))
        self.assertEqual(decoded.sequence, 42)
        self.assertAlmostEqual(decoded.latitude, 37.4219999, places=7)
        self.assertAlmostEqual(decoded.longitude, -122.0840575, places=7)
        self.assertAlmostEqual(decoded.altitude_m, 152.34, places=2)
        self.assertEqual(decoded.pressure_pa, 98765)
        self.assertAlmostEqual(decoded.gps_altitude_m, 149.25, places=2)
        self.assertAlmostEqual(decoded.acceleration_x_g, -1.25, places=3)
        self.assertAlmostEqual(decoded.rotation_z_dps, 830.0, places=1)
        self.assertEqual(decoded.gps_status, "live")

    def test_corruption_is_rejected_by_crc(self):
        packet = bytearray(telemetry.encode_packet(self.make()))
        packet[20] ^= 0x80
        with self.assertRaisesRegex(ValueError, "crc"):
            telemetry.decode_packet(bytes(packet))

    def test_version_and_size_are_rejected(self):
        packet = bytearray(telemetry.encode_packet(self.make()))
        packet[4] = 99
        with self.assertRaisesRegex(ValueError, "version"):
            telemetry.decode_packet(bytes(packet))
        with self.assertRaisesRegex(ValueError, "size"):
            telemetry.decode_packet(bytes(packet[:-1]))

    def test_stale_position_is_not_current(self):
        item = self.make(gps_fix_age_ms=15000, flags=telemetry.FLAG_GPS_HAS_POSITION)
        decoded = telemetry.decode_packet(telemetry.encode_packet(item))
        self.assertEqual(decoded.gps_status, "stale")
        self.assertIsNotNone(decoded.latitude)

    def test_missing_fix_carries_no_coordinates(self):
        item = self.make(latitude=None, longitude=None, gps_fix_age_ms=0xFFFFFFFF)
        decoded = telemetry.decode_packet(telemetry.encode_packet(item))
        self.assertEqual(decoded.gps_status, "missing")
        self.assertIsNone(decoded.latitude)
        self.assertIsNone(decoded.longitude)

    def test_cpp_and_python_contract_constants_match(self):
        header = (ROOT / "libraries" / "GreenIonProtocol" / "src" / "GreenIonProtocol.h").read_text(encoding="utf-8")
        self.assertIn('memcpy(packet.magic, "GI12", 4)', header)
        self.assertIn("kProtocolVersion = 2", header)
        self.assertIn("kPayloadBytes = 62", header)
        self.assertIn("sizeof(TelemetryPacket) == 70", header)

    def test_shared_radio_contract_is_explicit(self):
        radio = (ROOT / "libraries" / "GreenIonProtocol" / "src" / "GreenIonRadioConfig.h").read_text(encoding="utf-8")
        self.assertIn("kFrequencyMhz = 920.5f", radio)
        self.assertIn("kSignalBandwidthHz = 125000L", radio)
        self.assertIn("kSpreadingFactor = 7", radio)
        self.assertIn("kCodingRateDenominator = 5", radio)


if __name__ == "__main__":
    unittest.main()

