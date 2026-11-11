# V15 hosted dashboard — Feather Wi-Fi and cloud on a phone or computer

## What is supported

- **Chromium (Chrome/Edge) on a supported computer or Android phone:** after installing the **updated Feather firmware** from this change, the HTTPS website can request Local Network Access permission to talk to the Feather at `http://192.168.4.1`. Connect to the Feather Wi-Fi, open the [Vercel dashboard](https://green-ion-beta.vercel.app/) (or its previously installed offline copy), choose **Connections → Connect to Feather Wi-Fi**, and approve any **local network** prompt. Confirm actual live status, rather than assuming success because Wi-Fi is associated.
- **Safari/iPhone/iPad:** direct requests from an HTTPS page to an HTTP Feather are not reliably supported. This cannot be overridden by JavaScript or Vercel. Open [the Feather dashboard](http://192.168.4.1/) directly, save original BIN/SEN/TXT recordings to Files, and then import those into **V15 → History**. The V15 app can be installed/opened offline in advance. Offline analysis, recovery history and reports remain local.
- **Windows alternative:** run `Ground_Station/OPEN_GROUND_STATION.cmd` and open `http://127.0.0.1:8765` for an HTTP same-machine bridge. This does not require Local Network Access permissions or the trusted-origin firmware upgrade.

## Firmware is required for hosted-to-Feather API access

The existing Feather firmware lacks CORS responses for the Vercel origin. The current change adds a narrow origin allowlist (`https://green-ion-beta.vercel.app`) and handles browser preflight requests. **Deploying the website alone cannot change firmware already on your Feather.** Compile/install the updated sketch (or obtain the **firmware-build** artifact from a successful GitHub Actions run for this commit). For any OTA update: remove motors, stop recording, connect reliable USB power and a charged battery, save existing recordings, verify the binary/board and firmware image, and keep rollback available. Do not flash during flight. No physical OTA validation has been performed.

The firmware does **not** allow wildcard origins: untrusted websites' cross-origin requests are denied. The AP credentials and hardware permissions still matter. Do not expose the Feather API to the public internet.

## Cloud access while on Feather Wi-Fi

The Feather uses `WiFi.mode(WIFI_AP)` and does not provide internet connectivity. Vercel and Supabase need an internet route. Some phones may maintain mobile data while connected to no-internet Feather Wi-Fi; whether they do is device/carrier/browser dependent. If not, **save locally while connected to Feather, then switch back to internet**, reopen the V15 tab with your Supabase account signed in, and let auto sync upload your imported/saved originals. Do not assume a file was uploaded until the Cloud tab verifies it.

**Cloud configuration:** hosted V15 uses its separate public Supabase project URL/publishable key. No private service-role secrets should ever be deployed to browsers. Local recordings are preserved if internet is unavailable.

## Verification

1. Connect to the Feather AP; independently verify `http://192.168.4.1/api/status` returns actual Feather telemetry.
2. In a supported Chromium browser, load V15 and choose **Connections → Connect to Feather Wi-Fi**. Approve Local Network Access when prompted. Confirm status says verified.
3. Try read-only status and file listing before attempting any controls. Do not infer hardware readiness from connection alone.
4. Save a small test recording, verify it is stored locally, disconnect Feather Wi-Fi, restore internet, and confirm cloud backup only after it is listed and verified.
5. For Safari, test direct Feather download, V15 History import, offline analysis, and cloud upload after internet returns.

**Limitations:** browser support varies; Safari direct API access can remain blocked even with updated firmware. Cloud and Feather cannot both be live without two working network paths. Firmware compile/tests are not a substitute for physical device tests.

Sources: [Chrome Local Network Access](https://developer.chrome.com/blog/local-network-access), [MDN Local Network Access](https://developer.mozilla.org/en-US/docs/Web/Security/Defenses/Local_network_access), [MDN Mixed Content](https://developer.mozilla.org/en-US/docs/Web/Security/Defenses/Mixed_content).
