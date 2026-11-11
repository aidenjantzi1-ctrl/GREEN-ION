# GREEN ION V15 Local-First Windows Ground Station

**Run `OPEN_GROUND_STATION.cmd` from this folder.** It starts the current dark-blue GREEN ION dashboard at `http://127.0.0.1:8765`, not the older V13 static page. Python 3 is required. For Uno LoRa, install `py -3 -m pip install pyserial` and enter the Uno COM port; close Arduino Serial Monitor. Run with motors removed for testing.

Windows may join the Feather `GREEN-ION-V14` access point while the command window remains open. The access point has no internet. The local server polls Feather `192.168.4.1` and Uno LoRa independently. Wi-Fi status is authoritative for SD recording state; when Wi-Fi is lost, received LoRa telemetry remains visible but **cannot command the rocket**.

All finished SD BIN files and available SEN/TXT companions are automatically imported from Feather after reconnecting. The current web app saves immutable originals in IndexedDB and mirrors them through `/api/local/save` to `%USERPROFILE%\GREEN_ION_Data\flights` using SHA-256. The Python server verifies the hash, bounds file sizes, refuses unsafe filenames and only binds to loopback; disk originals are never overwritten. The browser can restore them using `/api/local/files` and `/api/local/file`. Radio CSV stays in `%USERPROFILE%\GREEN_ION_Data\logs`.

The `/api/status` endpoint merges verified Feather Wi-Fi and receive-only LoRa with accurate age/source labels. Other Feather `/api/*` calls are proxied to the fixed Feather IP only while verified. Use `/api/telemetry` and `/api/ground-link` for separate link diagnostics. Use `/config.js` for **publishable** browser cloud settings only; no service-role secret is exposed.

When internet returns on the Windows computer, the open browser tab retries automatic Supabase backups **only if signed in and auto-sync enabled**. This server is not a background cloud uploader when the browser is closed. Keep the local tab open. Cloud never controls flight or OTA.

See [complete V15 workflow](../docs/V15_LOCAL_FIRST.md) and [hardware quality plan](../docs/V14_QUALITY_PLAN.md). Physical tests of RF range, LiPo, SD, iPhone networking and OTA remain required.
