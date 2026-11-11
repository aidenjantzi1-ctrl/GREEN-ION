# GREEN ION V13 Unified Ground Station

**Use the ONE launcher: OPEN_GROUND_STATION.cmd.** This starts the full Feather dashboard locally at **http://127.0.0.1:8765/** and combines two independent connections.

- **Wi-Fi:** Windows connects to GREEN-ION-V13 (or existing V12) and the Python app fetches and proxies the actual Feather's 192.168.4.1 API. Full hardware tests, calibration, SD downloads, profile controls, bench OTA and live sensors work through the existing Feather endpoints.
- **LoRa:** Feather radio → Arduino Uno radio → USB COM port → Python. LoRa packets continue displaying live altitude, acceleration, rotation, battery, GPS, RSSI and estimated loss even when Wi-Fi drops. No rocket commands, firmware uploads or file downloads operate through LoRa.
- **Both lost:** no invented live sensor data; last-known GPS and age are retained.
- **Reconnect:** After Windows rejoins the Feather AP, full dashboard access returns without restarting the local app.

In the launcher enter a COM port (COM5 for example), press Enter for Wi-Fi only, or type DEMO for explicitly simulated telemetry. Python 3 is required; **py -3 -m pip install pyserial** is also required for real Uno USB reception. Keep the CMD window open. Close Arduino Serial Monitor before running the ground app.

**Local endpoints:** / = full Feather dashboard; /radio.html = optional receiver-only diagnostic dashboard; /api/ground-link = independent link states; /api/telemetry = receiver packets; /api/export = receiver CSV; other /api routes = Feather-only Wi-Fi proxy. Bind address is loopback 127.0.0.1 by default. Only fixed 192.168.4.1 Feather requests are sent; no arbitrary network proxy.

**Files:** new logs go under **%USERPROFILE%\GREEN_ION_Data\logs**, not in the replaceable Ground_Station folder. Existing radio export remains CSV; full-resolution recorded data is on the rocket microSD and requires Wi-Fi or physical card retrieval.

**Safety:** Run testing with motors removed. LoRa link availability is not proof of flight recording. Verify antenna, radio frequency coordination, wiring, power and radio range independently. No code acts on energetic devices.

For exact setup and replacement see [V13_START_HERE](../docs/V13_START_HERE.md) and [V13_UPDATE_GUIDE](../docs/V13_UPDATE_GUIDE.md).
