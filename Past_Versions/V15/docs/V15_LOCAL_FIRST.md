# GREEN ION V15 local-first operations — implementation and launch workflow

**Software preview. This does not certify the rocket for flight.** Physical SD, LiPo, Feather/Uno, LoRa antennas, GPS, Wi-Fi disconnection, OTA and recovery tests remain mandatory.

## Start the Windows launch-pad dashboard

1. Use Python 3 on Windows. To receive Uno LoRa by USB, install `py -3 -m pip install pyserial`.
2. Plug the Arduino Uno into the computer using USB and close Arduino Serial Monitor.
3. Connect Windows Wi-Fi to the Feather's `GREEN-ION-V14` network; its local address is `192.168.4.1`. **This network has no internet.**
4. Run `Ground_Station/OPEN_GROUND_STATION.cmd` and enter the real Uno COM port, e.g. `COM5` (or Enter for Wi-Fi only).
5. The launcher opens `http://127.0.0.1:8765`. It serves the **current blue web dashboard**, not the older V13 HTML, over a loopback-only Python server. A launch does not require internet.
6. Keep the launcher and browser open. Receiver packets are stored in CSV under `%USERPROFILE%\GREEN_ION_Data\logs`. When a completed flight is retrieved, verified originals are stored in browser IndexedDB **and** in `%USERPROFILE%\GREEN_ION_Data\flights`, each by SHA-256 with an original-name metadata record.

`Ground_Station/ground_station.py` polls the Feather's fixed local HTTP address and the Uno USB independently. Wi-Fi data wins while fresh, but LoRa telemetry remains available when Wi-Fi disappears. Wi-Fi must reconnect before complete SD BIN/SEN/TXT recordings can be retrieved. The cloud cannot send rocket commands. The Uno is receive-only.

If a browser-installed version is already open, reload the new localhost page once so the service worker caches the V15 modules.

## At the pad: one button, real evidence

**Motors removed.** Connect both radio antennas and safe power before testing; do not run maintenance with motors installed. Go to Testing and select **RUN FULL PREFLIGHT · ONE BUTTON**. The existing Feather firmware starts a protected home-verification session, runs battery/barometer/calibration, real IMU, real GPS, temporary SD write/read/delete, five LoRa bench packets, and a 30-second temporary recording with integrity checks. It attempts receiver confirmation only after the Uno has actually received five packets. Firmware records test classifications in NVS. Automatic mode marks physical/manual checks NOT VERIFIED, never fabricates them.

The Wi-Fi-disconnect test requires actually disconnecting/reconnecting a browser and may remain INCONCLUSIVE during the automatic sequence. Finish remaining physical checks manually. The session cleans its temporary file and recalibrates. Results are NOT launch authorization.

## Launch and recovery

- Feather V14 firmware already schedules LoRa transmit periodically before, during and after launch (except explicit maintenance), and continuously serves Wi-Fi within range. The independent full-rate RKT4/GIS1 recorder prioritizes sensor sampling and SD writes.
- When Wi-Fi goes out of range, the computer uses received Uno LoRa packets, with timestamps/age/source. LoRa silence means no fresh radio evidence, **not** proof that SD logging stopped.
- The new Recovery tab saves **observed live GPS fixes** locally across tab reloads and shows coordinates, source and fix age, LoRa freshness and a schematic offline track. The external OpenStreetMap navigation link needs internet; the last fix might not be the landing site.
- After recovering the rocket, rejoin Feather Wi-Fi. When no recording/maintenance is active, completed BIN files and available SEN/TXT companions are imported in short batches. BIN transfers must match the SD file's reported byte size. Originals are SHA-256 deduplicated and never removed from SD.
- The computer mirrors saved originals to the private loopback server's content-addressed disk folder. Use History → **Reconcile PC archive** if the browser cache was cleared. Archive reads are verified by SHA-256 before restoring to browser storage.

## Advanced analysis and automatic reports

Original RKT4 v2 BIN and GIS1 SEN files are decoded locally using a browser Web Worker. The postflight report includes timing continuity, RKT4 validity/truncation, pressure-derived estimated apogee/speed/acceleration/phases, battery before/after, GIS1 CRC32 and measured IMU total G, GPS fix count and warnings. Reports auto-generate after completed files arrive and can be exported as JSON.

A numeric data-completeness score is **not** sensor calibration accuracy, launch safety or proof that the rocket landed. LoRa landing states give only preliminary recovery indications. Full advanced analysis runs locally and can be repeated offline. Neither 3D visualization nor digital twin is implemented yet.

## Automatic cloud backup later

Local files are authoritative. With the Cloud account signed in and **Automatic cloud sync** enabled, the existing private Supabase uploader retries after internet returns, deduplicates by original SHA-256, stores by authenticated user ID and verifies restore. Keep the browser tab open for browser-driven sync. Without sign-in or internet, originals remain on disk, in browser and on Feather SD. The local server's publishable Supabase browser key is **not** a service role secret.

You can remain on Feather-only Wi-Fi for a launch, then switch the computer's Wi-Fi back to your internet connection afterward. The localhost browser tab remains open and uses locally saved files for analysis and eventual cloud upload.

## Battery and connections

Battery forecast uses observed voltage and user-supplied tested capacity, measured load current and required duration. It is a rough estimate with 50% reserve margin, not a cell fuel gauge or flight clearance. No tested capacity/current means **NOT VERIFIED**; below 3.60 V is **NOT READY**.

Connections adds independent Wi-Fi/LoRa live states, Uno packet count, sequence gaps, inferred loss, RSSI/SNR (if reported), packet age and receiver state. Link statistics are receiver-side estimates; no LoRa control commands are sent.

## Offline Help Assistant

The Help tab searches a bundled, fixed topic index and gives exact instructions and navigation to real dashboard controls. It works offline, uses no external model/service, and does not transmit GPS/account/flight data to search providers. It does not pretend to be an AI model or invent hardware-specific answers.

## Deferred future work

See [3D reconstruction and digital twin plan](V15_3D_TWIN_PLAN.md). Neither feature is currently running on Feather, browser or cloud.

## Release/validation boundary

The V15 website/local-server features are built on existing validated **software contracts** and V14 firmware. Original dark-blue layouts and RKT4 v2/GIS1 formats are unchanged. Automated CI checks do not validate a real rocket; physical range, power, SD, OTA, home tests and receiver performance remain open until measured with hardware.
