"""Regression contracts for V9 API state transitions (host-side; no hardware)."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
INO = (ROOT / "Green_Ion_V9/Green_Ion_V9.ino").read_text(encoding="utf-8")
DASH = (ROOT / "Green_Ion_V9/Dashboard.h").read_text(encoding="utf-8")


def section(start, end):
    first = INO.index(start)
    return INO[first:INO.index(end, first + len(start))]


def route(name, next_name):
    return section('web.on("/api/' + name + '"', 'web.on("/api/' + next_name + '"')


class StateGuardTests(unittest.TestCase):
    def test_shared_busy_guard_covers_active_workflows(self):
        guard = section("bool flightSettingsLocked()", "bool setSampleRate(")
        for flag in ("prepareActive", "armed", "recording", "launchWatch",
                     "calibrating", "groundTestActive", "simulating",
                     "shutdownPending", "updateInProgress"):
            self.assertIn(flag, guard)

    def test_rejected_settings_cannot_partially_change_sample_rate(self):
        body = route("settings", "time")
        self.assertLess(body.index("if (flightSettingsLocked())"),
                        body.index("setSampleRate(requested)"))
        self.assertLess(body.index("if (!validRate || !validLimit)"),
                        body.index("setSampleRate(requested)"))
        self.assertIn("prepareReady = false;", body)

    def test_prepare_checks_busy_state_and_all_fields_before_commit(self):
        body = route("prepare", "arm")
        self.assertLess(body.index("if (flightSettingsLocked())"),
                        body.index("requestedKind = web.arg"))
        self.assertLess(body.index('requestedRate != 10'),
                        body.index("if (!startPrepareAll())"))
        self.assertLess(body.index('requestedLimit != 2'),
                        body.index("if (!startPrepareAll())"))
        self.assertLess(body.index("if (!startPrepareAll())"),
                        body.index("missionKind = requestedKind;"))
        self.assertLess(body.index("missionKind = requestedKind;"),
                        body.index("setSampleRate(requestedRate)"))
        self.assertLess(body.index("if (!startPrepareAll())"),
                        body.index('nv.putUChar("limitmin"'))
        self.assertIn('respond(false, "Invalid phone time")', body)

    def test_maintenance_cannot_interrupt_active_data_capture(self):
        sd = section("bool verifySD()", "return sdVerified;")
        self.assertIn("groundTestActive", sd)
        self.assertIn("updateInProgress", sd)
        self.assertIn("flightSettingsLocked()", route("simulate", "simstop"))
        shutdown = INO[INO.index('"/api/shutdown"'):]
        self.assertLess(shutdown.index("if (flightSettingsLocked())"),
                        shutdown.index("shutdownPending = true;"))
        ota = section("void handleFirmwareUpload()", "void finishFirmwareUpload()")
        self.assertIn("flightSettingsLocked() || criticalBattery()", ota)

    def test_dashboard_explains_sd_busy_state(self):
        test_sd = DASH[DASH.index("async function testSDCard()"):
                       DASH.index("async function command(", DASH.index("async function testSDCard()"))]
        self.assertIn("state.groundTestActive", test_sd)
        self.assertIn("state.prepareActive", test_sd)
        self.assertIn("10-second ground test", test_sd)

    def test_mission_and_preparation_share_state_guard(self):
        self.assertIn("flightSettingsLocked()", route("mission", "settings"))
        self.assertIn("flightSettingsLocked()", section("bool startPrepareAll()", "void finishPrepareReview("))


if __name__ == "__main__":
    unittest.main()
