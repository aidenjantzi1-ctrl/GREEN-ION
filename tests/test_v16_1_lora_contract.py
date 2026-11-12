from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]


class V161LoRaContractTests(unittest.TestCase):
    def test_compact_packet_keeps_critical_recovery_fields(self):
        protocol = (ROOT / "libraries" / "GreenIonProtocol" / "src" / "GreenIonProtocol.h").read_text(encoding="utf-8")
        for token in (
            "static_assert(sizeof(TelemetryPacket) == 70",
            "latitudeE7", "longitudeE7", "altitudeCm", "pressurePa",
            "gpsAltitudeCm", "speedCms", "batteryMv", "gpsFixAgeMs",
            "satellites", "flightState", "flags", "health", "crc32",
        ):
            self.assertIn(token, protocol)

    def test_feather_populates_critical_lora_values(self):
        firmware = (ROOT / "Green_Ion_V14" / "Green_Ion_V14.ino").read_text(encoding="utf-8")
        for token in (
            "packet.latitudeE7", "packet.longitudeE7", "packet.altitudeCm",
            "packet.pressurePa", "packet.gpsAltitudeCm", "packet.speedCms",
            "packet.batteryMv", "packet.gpsFixAgeMs", "packet.satellites",
            "packet.flightState", "packet.flags", "packet.health",
            "green_ion::seal(packet)",
        ):
            self.assertIn(token, firmware)

    def test_uno_exports_critical_fields_plus_link_quality(self):
        receiver = (ROOT / "Ground_Receiver" / "Ground_Receiver.ino").read_text(encoding="utf-8")
        for token in (
            '"altitude_m"', '"pressure_pa"', '"gps_altitude_m"',
            '"battery_v"', '"gps_fix_age_ms"', '"gps_status"', '"satellites"',
            '"flight_state"', '"health"', '"latitude"', '"longitude"',
            '"rssi_dbm"', '"snr_db"',
        ):
            self.assertIn(token.replace('"', '\\"'), receiver)
        self.assertIn("hasCurrentPosition(packet)", receiver)
        self.assertIn("lastSNR()", receiver)

    def test_lora_remains_receive_only_on_ground(self):
        receiver = (ROOT / "Ground_Receiver" / "Ground_Receiver.ino").read_text(encoding="utf-8")
        # The ground receiver only calls recv; it must not send commands back.
        self.assertIn("radio.recv(", receiver)
        self.assertNotIn("radio.send(", receiver)


if __name__ == "__main__":
    unittest.main()
