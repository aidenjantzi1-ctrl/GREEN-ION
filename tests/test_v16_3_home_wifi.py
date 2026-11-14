import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
FW = (ROOT / "Green_Ion_V16_3" / "Green_Ion_V16_3.ino").read_text(encoding="utf-8")
WIFI = (ROOT / "Green_Ion_V16_3" / "HomeWifi.h").read_text(encoding="utf-8")
CFG = (ROOT / "Green_Ion_V16_3" / "Config.h").read_text(encoding="utf-8")
DASH = (ROOT / "Green_Ion_V16_3" / "Dashboard.h").read_text(encoding="utf-8")
WEB = (ROOT / "web" / "index.html").read_text(encoding="utf-8")


class V163HomeWifiTests(unittest.TestCase):
    def test_01_ap_only_without_saved_home_network(self):
        self.assertIn("shouldUseApSta(homeWifiConfigured, false)", WIFI)
        self.assertIn("WiFi.mode(useSta ? WIFI_AP_STA : WIFI_AP)", WIFI)
        self.assertIn("WiFi.softAP(config::DASHBOARD_SSID", WIFI)
        self.assertIn("192, 168, 4, 1", WIFI)

    def test_02_successful_connection_only_saves_after_connected(self):
        connected = WIFI.index("if (WiFi.status() == WL_CONNECTED)")
        saved = WIFI.index("persistHomeWifiCredentials", connected)
        self.assertGreater(saved, connected)
        self.assertIn('"connected_and_saved"', WIFI)
        self.assertIn('prefs.putString("ssid", ssid)', WIFI)
        self.assertIn('prefs.putString("pass", password)', WIFI)

    def test_03_failed_password_does_not_save_pending_credentials(self):
        self.assertIn('"connection_failed_not_saved"', WIFI)
        failure = WIFI.index('"connection_failed_not_saved"')
        self.assertIn("homeWifiPendingSave = false", WIFI[:failure + 500])
        self.assertNotIn("persistHomeWifiCredentials", WIFI[failure:failure + 700])

    def test_04_router_unavailable_uses_bounded_retry(self):
        self.assertIn("WIFI_CONNECT_TIMEOUT_MS", CFG)
        self.assertIn("WIFI_RETRY_INTERVAL_MS", CFG)
        self.assertIn('"router_unavailable"', WIFI)
        self.assertIn("homeWifiRetryAtMs = nowMs + config::WIFI_RETRY_INTERVAL_MS", WIFI)

    def test_05_disconnect_and_reconnect_preserve_ap(self):
        self.assertIn("disconnectHomeWifi", WIFI)
        self.assertIn("WiFi.disconnect(false, false)", WIFI)
        self.assertIn("homeWifiPaused = true", WIFI)
        self.assertIn("useApOnlyMode()", WIFI)
        self.assertIn("shouldRetryStation", WIFI)

    def test_06_forget_removes_saved_credentials(self):
        self.assertIn('prefs.remove("ssid")', WIFI)
        self.assertIn('prefs.remove("pass")', WIFI)
        self.assertIn("WiFi.disconnect(false, true)", WIFI)
        self.assertIn("homeWifiConfigured = false", WIFI)

    def test_07_dashboard_is_available_on_ap_and_router_ip(self):
        self.assertIn("WiFi.softAPIP().toString()", WIFI)
        self.assertIn("WiFi.localIP().toString()", WIFI)
        self.assertIn('origin == "http://" + host', FW)
        self.assertIn('id="wifiDirectAddress"', DASH)
        self.assertIn('id="wifiHomeAddress"', DASH)
        self.assertIn('id="wifiDirectAddress"', WEB)
        self.assertIn('id="wifiHomeAddress"', WEB)

    def test_08_credential_changes_require_physical_and_csrf_authorization(self):
        self.assertIn("PIN_MANAGEMENT_BUTTON = 38", CFG)
        self.assertIn("wifiPhysicalAuthorizationUntilMs", WIFI)
        self.assertIn("X-Green-Ion-CSRF", FW)
        self.assertIn("X-Green-Ion-Management", FW)
        self.assertIn("requireWifiManagement()", WIFI)
        for handler in ("connectHomeWifi", "disconnectHomeWifi", "forgetHomeWifi", "saveDirectApPassword"):
            start = WIFI.index("static void " + handler)
            block = WIFI[start:start + 500]
            self.assertIn("requireWifiManagement()", block)

    def test_09_recording_and_launch_watch_block_wifi_reconfiguration(self):
        self.assertIn("recorderMode == RECORDER_WATCHING", WIFI)
        self.assertIn("recorderMode == RECORDER_RECORDING", WIFI)
        self.assertIn("if (homeWifiNetworkChangeBlocked()) return;", WIFI)
        self.assertIn("serviceHomeWifi(nowMs);", FW)
        self.assertLess(FW.index("appendLog(nowUs, nowMs"), FW.index("serviceHomeWifi(nowMs);"))

    def test_10_reboot_loads_saved_network_without_exposing_password(self):
        self.assertIn('prefs.getString("ssid", "")', WIFI)
        self.assertIn('prefs.getString("pass", "")', WIFI)
        self.assertIn("initializeHomeWifi();", FW)
        status_start = WIFI.index("static void sendHomeWifiStatus()")
        status_end = WIFI.index("static void startHomeWifiScan()", status_start)
        status = WIFI[status_start:status_end]
        self.assertNotIn("homeWifiPassword", status)
        self.assertNotIn("homeWifiPendingPassword", status)
        self.assertNotIn('password\":', status)

    def test_no_home_credentials_are_hardcoded_or_logged(self):
        self.assertNotIn("HOME_WIFI_SSID", CFG)
        self.assertNotIn("HOME_WIFI_PASSWORD", CFG)
        self.assertNotIn("Serial.print(homeWifiPassword", WIFI)
        self.assertNotIn("Serial.printf(homeWifiPassword", WIFI)
        self.assertIn("Home Wi-Fi: %s", FW)

    def test_scan_is_async_and_forbidden_during_critical_states(self):
        self.assertIn("WiFi.scanNetworks(\n      true, true, false", WIFI)
        self.assertIn("WiFi.scanComplete()", WIFI)
        self.assertIn("WIFI_SCAN_RESULT_LIMIT", CFG)
        self.assertIn("homeWifiNetworkChangeBlocked()", WIFI)


if __name__ == "__main__":
    unittest.main()
