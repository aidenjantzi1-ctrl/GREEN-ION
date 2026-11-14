#pragma once

// GREEN ION V16.3 home Wi-Fi manager.
// All station operations are asynchronous and subordinate to the 50 Hz recorder.
// No password is ever returned by an API or printed to Serial.

static String homeWifiSsid;
static String homeWifiPassword;
static String homeWifiPendingSsid;
static String homeWifiPendingPassword;
static String homeWifiApPassword;
static String homeWifiLastResult = "not_configured";
static bool homeWifiConfigured = false;
static bool homeWifiPaused = false;
static bool homeWifiAttemptInProgress = false;
static bool homeWifiPendingSave = false;
static uint32_t homeWifiAttemptStartedMs = 0;
static uint32_t homeWifiRetryAtMs = 0;
static bool homeWifiScanActive = false;
static bool homeWifiScanRestoreApOnly = false;

static bool wifiManagementButtonWasDown = false;
static uint32_t wifiPhysicalAuthorizationUntilMs = 0;
static String wifiManagementToken;
static String wifiManagementCsrf;
static uint32_t wifiManagementSessionUntilMs = 0;

static bool wifiDeadlineActive(uint32_t nowMs, uint32_t deadlineMs) {
  return deadlineMs != 0 && static_cast<int32_t>(deadlineMs - nowMs) > 0;
}

static bool wifiManagementSessionActive(uint32_t nowMs = millis()) {
  return wifiManagementToken.length() >= 32 &&
         wifiDeadlineActive(nowMs, wifiManagementSessionUntilMs);
}

static bool homeWifiOperationActive() {
  return homeWifiAttemptInProgress || homeWifiScanActive;
}

static bool homeWifiNetworkChangeBlocked() {
  return green_ion::wifi_policy::networkChangeBlocked(
      state.recording || recorderCore.recording() || logFile || sensorLogFile,
      recorderMode == RECORDER_WATCHING,
      recorderMode == RECORDER_HOME_TEST,
      recorderMode == RECORDER_MAINTENANCE,
      otaUploadInProgress);
}

static bool homeWifiAuthorizationBlocked() {
  return state.recording || recorderCore.recording() || logFile || sensorLogFile ||
         recorderMode == RECORDER_RECORDING ||
         recorderMode == RECORDER_HOME_TEST ||
         recorderMode == RECORDER_MAINTENANCE ||
         otaUploadInProgress;
}

static String wifiJsonEscape(const String &value) {
  String out;
  out.reserve(value.length() + 8);
  static const char hex[] = "0123456789abcdef";
  for (size_t i = 0; i < value.length(); ++i) {
    const uint8_t ch = static_cast<uint8_t>(value[i]);
    if (ch == '"' || ch == '\\') {
      out += '\\';
      out += static_cast<char>(ch);
    } else if (ch == '\b') out += "\\b";
    else if (ch == '\f') out += "\\f";
    else if (ch == '\n') out += "\\n";
    else if (ch == '\r') out += "\\r";
    else if (ch == '\t') out += "\\t";
    else if (ch < 0x20) {
      out += "\\u00";
      out += hex[(ch >> 4) & 0x0f];
      out += hex[ch & 0x0f];
    } else {
      out += static_cast<char>(ch);
    }
  }
  return out;
}

static String randomManagementToken() {
  char token[65];
  for (uint8_t i = 0; i < 8; ++i) {
    const uint32_t value = esp_random();
    snprintf(token + i * 8, 9, "%08lx", static_cast<unsigned long>(value));
  }
  token[64] = '\0';
  return String(token);
}

static bool validCsrfValue(const String &value) {
  if (value.length() < 24 || value.length() > 128) return false;
  for (size_t i = 0; i < value.length(); ++i) {
    const char ch = value[i];
    if (!((ch >= '0' && ch <= '9') ||
          (ch >= 'a' && ch <= 'z') ||
          (ch >= 'A' && ch <= 'Z') ||
          ch == '-' || ch == '_')) return false;
  }
  return true;
}

static bool wifiManagementRequestAuthorized() {
  if (!wifiManagementSessionActive()) return false;
  const String token = dashboardServer.header("X-Green-Ion-Management");
  const String csrf = dashboardServer.header("X-Green-Ion-CSRF");
  return token == wifiManagementToken && csrf == wifiManagementCsrf &&
         validCsrfValue(csrf);
}

static bool requireWifiManagement() {
  if (!wifiManagementRequestAuthorized()) {
    dashboardServer.send(403, "application/json",
      "{\"success\":false,\"message\":\"Wi-Fi management authorization required; press SW38 on the Feather, then authorize this browser\"}");
    return false;
  }
  if (homeWifiNetworkChangeBlocked()) {
    dashboardServer.send(409, "application/json",
      "{\"success\":false,\"message\":\"Wi-Fi changes are locked during launch watching, recording, home test or firmware maintenance\"}");
    return false;
  }
  return true;
}

static bool loadHomeWifiPreferences() {
  Preferences prefs;
  if (!prefs.begin("giowifi", true)) {
    homeWifiSsid = "";
    homeWifiPassword = "";
    homeWifiApPassword = config::DASHBOARD_PASSWORD;
    homeWifiConfigured = false;
    return false;
  }
  const String ssid = prefs.getString("ssid", "");
  const String password = prefs.getString("pass", "");
  const String apPassword = prefs.getString("ap_pass", "");
  prefs.end();

  if (green_ion::wifi_policy::validSsidLength(ssid.length()) &&
      green_ion::wifi_policy::validWifiPassphraseLength(password.length())) {
    homeWifiSsid = ssid;
    homeWifiPassword = password;
    homeWifiConfigured = true;
  } else {
    homeWifiSsid = "";
    homeWifiPassword = "";
    homeWifiConfigured = false;
  }
  homeWifiApPassword =
      green_ion::wifi_policy::validApPassphraseLength(apPassword.length())
          ? apPassword : String(config::DASHBOARD_PASSWORD);
  homeWifiLastResult = homeWifiConfigured ? "saved_network_loaded" : "not_configured";
  return true;
}

static bool persistHomeWifiCredentials(const String &ssid, const String &password) {
  Preferences prefs;
  if (!prefs.begin("giowifi", false)) return false;
  const bool ssidOk = prefs.putString("ssid", ssid) == ssid.length();
  const bool passwordOk = prefs.putString("pass", password) == password.length();
  prefs.end();
  return ssidOk && passwordOk;
}

static bool persistApPassword(const String &password) {
  Preferences prefs;
  if (!prefs.begin("giowifi", false)) return false;
  const bool ok = prefs.putString("ap_pass", password) == password.length();
  prefs.end();
  return ok;
}

static void useApOnlyMode() {
  WiFi.mode(WIFI_AP);
}

static void startHomeWifiAttempt(const String &ssid,
                                 const String &password,
                                 bool saveOnSuccess,
                                 uint32_t nowMs) {
  if (homeWifiNetworkChangeBlocked()) return;
  WiFi.mode(WIFI_AP_STA);
  WiFi.disconnect(false, false);
  WiFi.begin(ssid.c_str(), password.length() ? password.c_str() : nullptr);
  homeWifiPendingSsid = ssid;
  homeWifiPendingPassword = password;
  homeWifiPendingSave = saveOnSuccess;
  homeWifiAttemptInProgress = true;
  homeWifiAttemptStartedMs = nowMs;
  homeWifiLastResult = saveOnSuccess ? "connecting_new_network" : "reconnecting";
}

static void initializeHomeWifi() {
  pinMode(config::PIN_MANAGEMENT_BUTTON, INPUT);
  loadHomeWifiPreferences();
  WiFi.setAutoReconnect(false);

  const bool useSta = green_ion::wifi_policy::shouldUseApSta(homeWifiConfigured, false);
  WiFi.mode(useSta ? WIFI_AP_STA : WIFI_AP);
  const IPAddress apIp(192, 168, 4, 1);
  const IPAddress apMask(255, 255, 255, 0);
  WiFi.softAPConfig(apIp, apIp, apMask);
  WiFi.softAP(config::DASHBOARD_SSID, homeWifiApPassword.c_str());

  if (useSta) {
    startHomeWifiAttempt(homeWifiSsid, homeWifiPassword, false, millis());
  }
}

static void serviceHomeWifi(uint32_t nowMs) {
  const bool buttonDown = digitalRead(config::PIN_MANAGEMENT_BUTTON) == LOW;
  if (buttonDown && !wifiManagementButtonWasDown && !homeWifiAuthorizationBlocked()) {
    wifiPhysicalAuthorizationUntilMs = nowMs + config::WIFI_PHYSICAL_AUTH_WINDOW_MS;
  }
  wifiManagementButtonWasDown = buttonDown;

  if (wifiManagementToken.length() && !wifiManagementSessionActive(nowMs)) {
    wifiManagementToken = "";
    wifiManagementCsrf = "";
    wifiManagementSessionUntilMs = 0;
    if (recorderMode == RECORDER_CALIBRATING && !homeWifiOperationActive())
      resetCalibration(nowMs);
  }

  // Never trigger, cancel, scan or reconfigure Wi-Fi while the recorder is
  // watching for launch, recording, running the home test, or in OTA maintenance.
  if (homeWifiNetworkChangeBlocked()) return;

  if (homeWifiAttemptInProgress) {
    if (WiFi.status() == WL_CONNECTED) {
      if (homeWifiPendingSave) {
        if (persistHomeWifiCredentials(homeWifiPendingSsid, homeWifiPendingPassword)) {
          homeWifiSsid = homeWifiPendingSsid;
          homeWifiPassword = homeWifiPendingPassword;
          homeWifiConfigured = true;
          homeWifiPaused = false;
          homeWifiLastResult = "connected_and_saved";
        } else {
          homeWifiLastResult = "connected_but_save_failed";
        }
      } else {
        homeWifiLastResult = "connected";
      }
      homeWifiPendingSsid = "";
      homeWifiPendingPassword = "";
      homeWifiPendingSave = false;
      homeWifiAttemptInProgress = false;
      homeWifiRetryAtMs = 0;
    } else if (nowMs - homeWifiAttemptStartedMs >= config::WIFI_CONNECT_TIMEOUT_MS) {
      WiFi.disconnect(false, false);
      const bool wasNewNetwork = homeWifiPendingSave;
      homeWifiPendingSsid = "";
      homeWifiPendingPassword = "";
      homeWifiPendingSave = false;
      homeWifiAttemptInProgress = false;
      homeWifiLastResult = wasNewNetwork ? "connection_failed_not_saved" : "router_unavailable";
      if (homeWifiConfigured && !homeWifiPaused) {
        homeWifiRetryAtMs = nowMs + config::WIFI_RETRY_INTERVAL_MS;
      } else {
        useApOnlyMode();
      }
    }
    return;
  }

  if (green_ion::wifi_policy::shouldRetryStation(
          homeWifiConfigured, homeWifiPaused, WiFi.status() == WL_CONNECTED,
          homeWifiAttemptInProgress, homeWifiNetworkChangeBlocked(),
          nowMs, homeWifiRetryAtMs)) {
    startHomeWifiAttempt(homeWifiSsid, homeWifiPassword, false, nowMs);
  }
}

static void authorizeWifiManagement() {
  const String csrf = dashboardServer.header("X-Green-Ion-CSRF");
  if (!validCsrfValue(csrf)) {
    dashboardServer.send(403, "application/json",
      "{\"success\":false,\"message\":\"Missing or invalid CSRF protection header\"}");
    return;
  }
  if (homeWifiAuthorizationBlocked()) {
    dashboardServer.send(409, "application/json",
      "{\"success\":false,\"message\":\"Wi-Fi setup cannot begin during recording, home test or firmware maintenance\"}");
    return;
  }
  if (!wifiDeadlineActive(millis(), wifiPhysicalAuthorizationUntilMs)) {
    dashboardServer.send(403, "application/json",
      "{\"success\":false,\"message\":\"Press the physical SW38 button on the Feather, then tap Authorize within 60 seconds\"}");
    return;
  }

  // Explicit local authorization pauses automatic launch watching before any
  // credential or mode change can occur. A fresh calibration runs when setup ends.
  if (recorderMode == RECORDER_WATCHING) {
    flightWatcher.cancel();
    resetCalibration(millis());
  }
  wifiManagementToken = randomManagementToken();
  wifiManagementCsrf = csrf;
  wifiManagementSessionUntilMs = millis() + config::WIFI_MANAGEMENT_SESSION_MS;
  wifiPhysicalAuthorizationUntilMs = 0;
  String body = "{\"success\":true,\"token\":\"";
  body += wifiManagementToken;
  body += "\",\"expires_seconds\":" +
      String(config::WIFI_MANAGEMENT_SESSION_MS / 1000UL) +
      ",\"message\":\"Wi-Fi management authorized on this browser; launch watching is paused until setup is finished\"}";
  dashboardServer.send(200, "application/json", body);
}

static void releaseWifiManagement() {
  if (!wifiManagementRequestAuthorized()) {
    dashboardServer.send(403, "application/json",
      "{\"success\":false,\"message\":\"Wi-Fi management authorization required\"}");
    return;
  }
  wifiManagementToken = "";
  wifiManagementCsrf = "";
  wifiManagementSessionUntilMs = 0;
  if (!homeWifiOperationActive() && recorderMode == RECORDER_CALIBRATING)
    resetCalibration(millis());
  dashboardServer.send(200, "application/json",
    "{\"success\":true,\"message\":\"Wi-Fi setup finished; stationary calibration restarted and automatic launch watching will resume afterward\"}");
}

static void sendHomeWifiStatus() {
  const bool connected = WiFi.status() == WL_CONNECTED;
  String body;
  body.reserve(700);
  body += "{\"success\":true";
  body += ",\"mode\":\"";
  body += (homeWifiConfigured && !homeWifiPaused) || connected ||
          homeWifiAttemptInProgress || homeWifiScanActive ? "AP+STA" : "AP";
  body += "\"";
  body += ",\"ap_ssid\":\"" + wifiJsonEscape(String(config::DASHBOARD_SSID)) + "\"";
  body += ",\"ap_ip\":\"" + WiFi.softAPIP().toString() + "\"";
  body += ",\"ap_clients\":" + String(WiFi.softAPgetStationNum());
  body += ",\"home_configured\":";
  body += homeWifiConfigured ? "true" : "false";
  body += ",\"home_ssid\":\"" + wifiJsonEscape(homeWifiConfigured ? homeWifiSsid : String("")) + "\"";
  body += ",\"station_connected\":";
  body += connected ? "true" : "false";
  body += ",\"station_paused\":";
  body += homeWifiPaused ? "true" : "false";
  body += ",\"connecting\":";
  body += homeWifiAttemptInProgress ? "true" : "false";
  body += ",\"station_ip\":\"";
  body += connected ? WiFi.localIP().toString() : String("");
  body += "\"";
  body += ",\"rssi_dbm\":";
  body += connected ? String(WiFi.RSSI()) : String("null");
  body += ",\"last_result\":\"" + wifiJsonEscape(homeWifiLastResult) + "\"";
  body += ",\"management_button_window\":";
  body += wifiDeadlineActive(millis(), wifiPhysicalAuthorizationUntilMs) ? "true" : "false";
  body += ",\"management_authorized\":";
  body += wifiManagementRequestAuthorized() ? "true" : "false";
  body += ",\"scan_active\":";
  body += homeWifiScanActive ? "true" : "false";
  body += ",\"supports_24ghz_only\":true";
  body += "}";
  dashboardServer.send(200, "application/json", body);
}

static void startHomeWifiScan() {
  if (!requireWifiManagement()) return;
  if (homeWifiOperationActive()) {
    dashboardServer.send(409, "application/json",
      "{\"success\":false,\"message\":\"Another Wi-Fi operation is already running\"}");
    return;
  }
  homeWifiScanRestoreApOnly = !homeWifiConfigured && !WiFi.isConnected();
  WiFi.mode(WIFI_AP_STA);
  WiFi.scanDelete();
  const int16_t result = WiFi.scanNetworks(
      true, true, false, config::WIFI_SCAN_MAX_MS_PER_CHANNEL);
  if (result == WIFI_SCAN_FAILED) {
    homeWifiScanActive = false;
    if (homeWifiScanRestoreApOnly) useApOnlyMode();
    dashboardServer.send(500, "application/json",
      "{\"success\":false,\"message\":\"Wi-Fi scan could not start\"}");
    return;
  }
  homeWifiScanActive = true;
  dashboardServer.send(202, "application/json",
    "{\"success\":true,\"message\":\"2.4 GHz Wi-Fi scan started asynchronously\"}");
}

static void sendHomeWifiScanResults() {
  const int16_t count = WiFi.scanComplete();
  if (count == WIFI_SCAN_RUNNING) {
    dashboardServer.send(200, "application/json",
      "{\"success\":true,\"scanning\":true,\"networks\":[]}");
    return;
  }
  if (count == WIFI_SCAN_FAILED) {
    homeWifiScanActive = false;
    if (homeWifiScanRestoreApOnly && !homeWifiNetworkChangeBlocked()) useApOnlyMode();
    dashboardServer.send(500, "application/json",
      "{\"success\":false,\"message\":\"Wi-Fi scan failed\",\"networks\":[]}");
    return;
  }

  String body = "{\"success\":true,\"scanning\":false,\"networks\":[";
  const int16_t limited = min<int16_t>(count, config::WIFI_SCAN_RESULT_LIMIT);
  for (int16_t i = 0; i < limited; ++i) {
    if (i) body += ',';
    body += "{\"ssid\":\"" + wifiJsonEscape(WiFi.SSID(i)) + "\"";
    body += ",\"rssi_dbm\":" + String(WiFi.RSSI(i));
    body += ",\"channel\":" + String(WiFi.channel(i));
    body += ",\"open\":";
    body += WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "true" : "false";
    body += "}";
  }
  body += "]}";
  WiFi.scanDelete();
  homeWifiScanActive = false;
  if (homeWifiScanRestoreApOnly && !homeWifiNetworkChangeBlocked()) useApOnlyMode();
  homeWifiScanRestoreApOnly = false;
  dashboardServer.send(200, "application/json", body);
}

static void connectHomeWifi() {
  if (!requireWifiManagement()) return;
  if (homeWifiOperationActive()) {
    dashboardServer.send(409, "application/json",
      "{\"success\":false,\"message\":\"Another Wi-Fi operation is already running\"}");
    return;
  }
  const String ssid = dashboardServer.arg("ssid");
  String password = dashboardServer.arg("password");
  if (!green_ion::wifi_policy::validSsidLength(ssid.length())) {
    dashboardServer.send(400, "application/json",
      "{\"success\":false,\"message\":\"SSID must be 1-32 characters\"}");
    return;
  }
  if (password.isEmpty() && homeWifiConfigured && ssid == homeWifiSsid)
    password = homeWifiPassword;
  if (!green_ion::wifi_policy::validWifiPassphraseLength(password.length())) {
    dashboardServer.send(400, "application/json",
      "{\"success\":false,\"message\":\"Wi-Fi password must be empty for an open network or 8-63 characters\"}");
    return;
  }
  homeWifiPaused = false;
  startHomeWifiAttempt(ssid, password, true, millis());
  dashboardServer.send(202, "application/json",
    "{\"success\":true,\"message\":\"Connection attempt started; the direct GREEN ION access point remains available\"}");
}

static void disconnectHomeWifi() {
  if (!requireWifiManagement()) return;
  if (homeWifiOperationActive()) {
    dashboardServer.send(409, "application/json",
      "{\"success\":false,\"message\":\"Wait for the current Wi-Fi operation to finish\"}");
    return;
  }
  WiFi.disconnect(false, false);
  homeWifiPaused = true;
  homeWifiLastResult = "station_disconnected_by_user";
  useApOnlyMode();
  dashboardServer.send(200, "application/json",
    "{\"success\":true,\"message\":\"Home Wi-Fi disconnected. Saved settings are retained for the next reconnect or reboot; direct GREEN ION Wi-Fi remains active\"}");
}

static void forgetHomeWifi() {
  if (!requireWifiManagement()) return;
  if (homeWifiOperationActive()) {
    dashboardServer.send(409, "application/json",
      "{\"success\":false,\"message\":\"Wait for the current Wi-Fi operation to finish\"}");
    return;
  }
  Preferences prefs;
  bool ok = false;
  if (prefs.begin("giowifi", false)) {
    const bool a = prefs.remove("ssid") || !prefs.isKey("ssid");
    const bool b = prefs.remove("pass") || !prefs.isKey("pass");
    ok = a && b;
    prefs.end();
  }
  if (!ok) {
    dashboardServer.send(500, "application/json",
      "{\"success\":false,\"message\":\"Could not remove saved home Wi-Fi settings\"}");
    return;
  }
  WiFi.disconnect(false, true);
  homeWifiSsid = "";
  homeWifiPassword = "";
  homeWifiPendingSsid = "";
  homeWifiPendingPassword = "";
  homeWifiConfigured = false;
  homeWifiPaused = false;
  homeWifiAttemptInProgress = false;
  homeWifiPendingSave = false;
  homeWifiRetryAtMs = 0;
  homeWifiLastResult = "forgotten";
  useApOnlyMode();
  dashboardServer.send(200, "application/json",
    "{\"success\":true,\"message\":\"Saved home Wi-Fi credentials forgotten. Direct GREEN ION Wi-Fi remains active\"}");
}

static void saveDirectApPassword() {
  if (!requireWifiManagement()) return;
  const String password = dashboardServer.arg("password");
  if (!green_ion::wifi_policy::validApPassphraseLength(password.length())) {
    dashboardServer.send(400, "application/json",
      "{\"success\":false,\"message\":\"Direct Wi-Fi password must be 8-63 characters\"}");
    return;
  }
  if (!persistApPassword(password)) {
    dashboardServer.send(500, "application/json",
      "{\"success\":false,\"message\":\"Could not save the direct Wi-Fi password\"}");
    return;
  }
  homeWifiApPassword = password;
  dashboardServer.send(200, "application/json",
    "{\"success\":true,\"message\":\"New direct Wi-Fi password saved securely. It takes effect after the next Feather reboot so this connection is not dropped unexpectedly\"}");
}
