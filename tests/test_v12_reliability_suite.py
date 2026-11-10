"""GREEN ION V12 reliability upgrade: actual protocol, persistence and UI contracts.

These software tests do not substitute for hardware bench/flight validation.
"""
from __future__ import annotations

from pathlib import Path
import subprocess
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "Ground_Station"))
from ground_station import Store  # noqa: E402


class GroundStationLinkTests(unittest.TestCase):
    def test_sequence_loss_duplicate_and_sender_reboot(self) -> None:
        store = Store()
        store.receiver("ready", info={"frequency_mhz": 920.5})
        store.update({"type": "telemetry", "sequence": 100, "uptime_ms": 1000,
                      "latitude": 33.0, "longitude": -117.0, "gps_status": "live",
                      "rssi_dbm": -68})
        last_fix = store.last_position_monotonic
        store.update({"type": "telemetry", "sequence": 103, "uptime_ms": 3000,
                      "latitude": 34.0, "longitude": -118.0, "gps_status": "stale",
                      "rssi_dbm": -74})
        self.assertEqual(store.last_position, (33.0, -117.0))
        self.assertEqual(store.last_position_monotonic, last_fix)
        self.assertEqual(store.snapshot()["packets_missed_estimated"], 2)
        store.update({"type": "telemetry", "sequence": 103, "uptime_ms": 3100,
                      "gps_status": "missing"})
        self.assertEqual(store.snapshot()["packets_repeated"], 1)
        store.update({"type": "telemetry", "sequence": 0, "uptime_ms": 200,
                      "gps_status": "missing"})
        self.assertEqual(store.snapshot()["sender_resets"], 1)
        self.assertEqual(store.snapshot()["packets_missed_estimated"], 2)
        self.assertEqual(store.snapshot()["receiver_state"], "ready")

    def test_link_stale_demotes_gps_live_and_bench_is_separate(self) -> None:
        store = Store()
        store.update({"type": "telemetry", "sequence": 1, "uptime_ms": 1000,
                      "latitude": 33.7, "longitude": -117.7, "gps_status": "live"})
        store.bench({"type": "radio_bench", "sequence": 2, "uptime_ms": 2000})
        self.assertEqual(store.snapshot()["bench_packets"], 1)
        self.assertEqual(store.snapshot()["packets_received"], 2)
        self.assertEqual(store.snapshot()["telemetry_packets"], 1)
        store.received_monotonic -= 4
        snap = store.snapshot()
        self.assertEqual(snap["link_status"], "stale")
        self.assertEqual(snap["gps_status"], "stale")
        self.assertEqual(snap["last_position"], (33.7, -117.7))
        self.assertEqual(store.latest["gps_status"], "live")
        missing = Store().snapshot()
        self.assertEqual(missing["link_status"], "missing")

    def test_bench_packets_do_not_create_fake_sequence_loss(self) -> None:
        store = Store()
        store.update({"type": "telemetry", "sequence": 11, "uptime_ms": 11000})
        store.bench({"type": "radio_bench", "sequence": 12, "uptime_ms": 12000})
        store.bench({"type": "radio_bench", "sequence": 13, "uptime_ms": 13000})
        store.update({"type": "telemetry", "sequence": 14, "uptime_ms": 14000})
        snap = store.snapshot()
        self.assertEqual(snap["packets_received"], 4)
        self.assertEqual(snap["telemetry_packets"], 2)
        self.assertEqual(snap["bench_packets"], 2)
        self.assertEqual(snap["packets_missed_estimated"], 0)

    def test_store_instances_have_independent_locks_and_state(self) -> None:
        first, second = Store(), Store()
        self.assertIsNot(first.lock, second.lock)
        first.bench({"type": "radio_bench"})
        self.assertEqual(second.snapshot()["bench_packets"], 0)


class FirmwareSourceContracts(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.fw = (ROOT / "Green_Ion_V12/Green_Ion_V12.ino").read_text()
        cls.home = (ROOT / "Green_Ion_V12/HomeVerification.h").read_text()
        cls.ui = (ROOT / "Green_Ion_V12/Dashboard.h").read_text()
        cls.uno = (ROOT / "Ground_Receiver/Ground_Receiver.ino").read_text()
        cls.ground = (ROOT / "Ground_Station/ground_station.py").read_text()

    def test_profiles_and_flight_file_collision_protection(self) -> None:
        for token in ('"/api/profile"', '"gioflight"', '"rocket="',
                      '"motor="', 'selectedRocket', 'selectedMotor'):
            self.assertIn(token, self.fw)
        naming = (ROOT / "libraries/GreenIonRecorder/src/GreenIonFileNaming.h").read_text()
        for extension in (".BIN", ".SEN", ".TXT", ".JRA", ".JRB", ".RCV"):
            self.assertIn(f'"{extension}"', naming)
        self.assertIn("flightSequenceOccupied(", self.fw)
        self.assertIn('if (!occupied)', self.fw)
        self.assertIn('rocketPreset', self.ui)

    def test_preflight_evidence_not_flight_authorization(self) -> None:
        self.assertIn("GreenIonPreflight.h", self.fw)
        self.assertIn("currentPreflight()", self.fw)
        self.assertIn("preflight_flags", self.fw)
        self.assertIn("preflight_state", self.fw)
        self.assertIn("v12PreflightDetails", self.ui)
        self.assertIn("hardware_approved:false", self.ui)

    def test_home_test_persistence_and_uncertainty(self) -> None:
        for token in ('"giohome"', 'getBool("active", false)', 'putBool("active", true)',
                      '"/api/home/step"', 'homeStepStatuses', 'homeStepName',
                      'homeWifiAbsent', 'homeCleanupOk'):
            self.assertIn(token, self.home + self.fw)
        self.assertIn("homeStoreStepResult", self.fw)
        self.assertIn('homeResult == 3 || !homeCleanupOk ? 2 : homeWifiAbsent ? 1 : 3', self.home)
        self.assertIn('"NOT VERIFIED"', self.home)
        self.assertIn('recorderMode == RECORDER_HOME_TEST', self.fw)

    def test_receiver_and_ground_station_cover_actual_link(self) -> None:
        for token in ("duplicatePackets", "receivedBenchPackets",
                      "senderRestarts", "printReceiverHeartbeat"):
            self.assertIn(token, self.uno)
        for token in ("/api/export", "serial.Serial", "STORE.bench",
                      "STORE.receiver", "last_position_monotonic",
                      "packets_missed_estimated"):
            self.assertIn(token, self.ground)

    def test_ground_station_browser_js_syntax(self) -> None:
        html = (ROOT / "Ground_Station/index.html").read_text()
        start = html.index("<script>") + len("<script>")
        end = html.index("</script>", start)
        result = subprocess.run(["node", "--check", "-"], input=html[start:end],
                                text=True, capture_output=True)
        self.assertEqual(result.returncode, 0, result.stderr)


if __name__ == "__main__":
    unittest.main()
