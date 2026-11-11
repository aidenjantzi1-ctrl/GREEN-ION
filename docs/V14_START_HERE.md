# GREEN ION V14 — Start Here

**Software beta. Physical hardware, iPhone local-network, USB receiver, OTA and flight validation remain required before launch.**

## Open the hosted website

Open the GREEN ION Vercel project's production URL. In Chrome/Edge on Windows you can install the website and connect the Arduino Uno receiver by USB. On iPhone/iPad, add the website to Home Screen; use Feather Wi-Fi for live data and no Uno/LoRa.

**Do not expect Safari on iPhone to read Uno USB**. Web Serial support is for compatible desktop browsers. Your existing Uno needs no new Wi-Fi board.

## Blue dashboard and connection

The original Status, Flight, Field, Testing, Utilities, Storage, and System tabs are preserved. V14 adds Connections, History, Cloud, and Updates without discarding existing tests, graphs, calibration and storage views.

The Feather hosts its own independent blue fallback dashboard. Connect to **GREEN-ION-V14** Wi-Fi after installing the V14 firmware and open **http://192.168.4.1**. Existing V13 hardware uses **GREEN-ION-V13** until updated. Both use 192.168.4.1.

A hosted HTTPS website cannot always fetch HTTP endpoints on a private Wi-Fi address because of mixed-content, CORS and local network permissions; Safari is particularly restrictive. If live Feather connection is not available through the hosted app, open its local fallback dashboard. This is a browser limitation, not an excuse to expose unrestricted remote control on the rocket. Do not configure cloud proxies to send flight commands.

## Windows Uno/LoRa

Connect Uno by USB, close Arduino Serial Monitor, open the hosted website in Chrome or Edge, select **Connections → Connect Arduino Uno** and grant serial-port permission. Received LoRa telemetry is displayed if Feather Wi-Fi is unavailable; LoRa does not prove onboard recording and never grants flight controls. You can export the received radio CSV.

The older V13 Python ground bridge remains in Ground_Station for compatible Windows local use and historical troubleshooting.

## Offline history and Supabase

The website caches application files for offline use. In History, original BIN/SEN/TXT files are stored in IndexedDB with SHA-256. Import recordings, download an archive to your computer or phone Files app, and restore after reinstalling. The app attempts to copy completed Feather flight files into local history only after recording is inactive.

In Cloud, create an account or sign in with email and password; confirm email if Supabase requests it. Supabase stores files in a private **green-ion-flights** bucket and owns metadata by authenticated user ID with row-level security. Files sync when internet is available; they do not replace the SD card or device backup. No cloud connection is necessary for launch, test or analysis. The website uses **only a publishable browser API key**, never the service-role key.

## GitHub firmware and website updates

GitHub Actions compiles and tests the complete ESP32 Feather V2 application, Uno firmware and hosted website. The Feather **Green_Ion_V14.ino.bin** is the complete compiled application: `Green_Ion_V14.ino`, `Dashboard.h`, `FirmwareMaintenance.h`, `SensorState.h`, `HomeVerification.h`, `Config.h`, and linked library code are compiled together. There is no separate upload for these headers; the firmware OTA installs them together. GitHub's complete release ZIP also contains original `.ino`, `.h`, and library source files. Uno firmware and the Vercel website are separate update targets.

A new official release displays **Download now** or **Later**. **Download now** attempts a direct Feather update: the hosted website downloads the approved Feather app image and its official `SHA256_MANIFEST.json` via a same-origin, restricted Vercel relay, checks size, board and SHA-256, verifies live Feather Wi-Fi and hardware readiness, and then shows a **separate bench-only confirmation** before entering OTA maintenance and uploading the full app image. The current Feather firmware must already support OTA partitions and maintenance; the board should not be recording. Never install firmware with a motor fitted or at a launch site.

**Network limitation:** On iPhone/iPad connected only to the Feather's Wi-Fi access point, GitHub internet access may be unavailable. HTTPS browser private-network restrictions can also block direct Feather commands. If either connection is unavailable, choose **Download file manually** while online, then connect to Feather Wi-Fi and open `http://192.168.4.1` → System → Install firmware, choose the main **Green_Ion_V14.ino.bin**, confirm maintenance and upload. Do not use ZIP, bootloader or merged images in the OTA upload. Reconnect after restart, verify the installed build, run guided home tests, and keep the last working release for recovery. Software CI does not replace physical OTA testing.

## Flight and compatibility

RKT4 v2 BIN, GIS1 SEN, TXT and V12D### naming remain unchanged. No experimental radio, cloud, OTA or website feature may interfere with autonomous onboard recording. Until actual board tests pass, the release is a software beta, not a launch-ready endorsement.

## Analyze and compare flights offline

In **History**, import or select an original RKT4 BIN and select **Analyze saved flight**. The Web Worker calculates pressure-derived apogee, velocity, acceleration, G, flight phase estimates and the interactive timeline. Select a second BIN and **Compare flights** for overlaid charts and peak differences. CSV, JSON report and Print / Save PDF exports run without internet. The original recordings remain unchanged.

## V14 beta 3 — cleaner dashboard

The original blue colors and Status, Flight, Field, Testing, Utilities, Storage and System tabs are preserved. On desktop, cards now stack independently in two columns instead of reserving an empty grid row beneath every short card; on phones, cards remain one column. The tabs scroll horizontally and support arrow-key navigation. Graphs, PDF/CSV exports and History remain available even when the Feather is disconnected. Firmware commands still require verified Feather Wi-Fi.
