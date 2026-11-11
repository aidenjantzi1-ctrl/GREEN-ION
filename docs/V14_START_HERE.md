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

GitHub Actions compiles both the ESP32 Feather V2 and Uno firmware and tests the web app. A newer official release triggers a dashboard popup asking **Download now** or **Later**. Downloads never flash firmware automatically.

Download the Feather **Green_Ion_V14.ino.bin** (not bootloader or merged images). On a stationary bench, secure the rocket against launch, finish all recordings, attach stable USB power and keep a charged LiPo, open the Feather's Wi-Fi fallback page and use **System → Firmware update** to enter maintenance and upload. The OTA partition must support it. Then reconnect to GREEN-ION-V14, verify version, rerun guided home tests, and preserve V13 release for recovery.

## Flight and compatibility

RKT4 v2 BIN, GIS1 SEN, TXT and V12D### naming remain unchanged. No experimental radio, cloud, OTA or website feature may interfere with autonomous onboard recording. Until actual board tests pass, the release is a software beta, not a launch-ready endorsement.
