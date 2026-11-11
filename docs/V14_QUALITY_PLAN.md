# GREEN ION V14 — Quality and Validation Plan

Current implementation: **V14 beta 4**. Software-tested; **NOT flight-certified**. A hardware launch requires supervised physical testing.

## 1. Layout and accessibility

- Maintain the original dark blue V11/V13 theme, all original Status/Flight/Field/Testing/Utilities/Storage/System tabs, and extra Connections/History/Cloud/Updates tabs.
- Remove blank card-grid rows and inconsistent double margins. Use independent two-column card stacking on desktop, single-column cards on phones, a full-width flight history graph, consistent 14–16 px spacing and horizontally scrollable tabs.
- Keep tabs usable with keyboard arrow/Home/End navigation and aria-selected state. Avoid duplicate HTML IDs.
- Keep local-only actions (graphs, exports, history, appearance) usable without Feather Wi-Fi. Block only real hardware commands.

## 2. Recorder and link reliability

- Preserve autonomous recorder, SD integrity, RKT4 v2/GIS1, original V12D### filenames and latest tested V13 fallback. No cloud, LoRa or browser feature may control motor, staging, ignition or deployment.
- Feather local AP at 192.168.4.1 remains independent even when hosted Vercel website or Internet is absent.
- Windows Chrome/Edge uses USB Web Serial to Uno receiver for telemetry only; iPhone/iPad cannot use this Uno USB path from Safari.
- Hosted HTTPS-to-Feather HTTP private-network access is browser-dependent and must be physically verified. Do not claim Safari support without an iPhone test.

## 3. Data integrity and security

- Original BIN/SEN/TXT stored locally before cloud sync and never modified for analysis.
- SHA-256 on imports/restores, duplicate detection, downloadable offline archive.
- Supabase Auth, user-scoped Row Level Security on metadata, private storage bucket and user-specific storage paths.
- Cloud sync retries when online; flight recorder must never depend on Supabase availability.
- Release prompt reads only repository-controlled GitHub releases and asks before download. OTA is a separate manually supervised bench action with local maintenance checks.

## 4. Automated acceptance criteria

- Python regressions pass, including legacy V11–V13 compatibility and duplicate-ID checks.
- Node browser utility, offline RKT4 analysis and UI-action tests pass.
- JavaScript passes syntax checks; Vercel build contains blue HTML, required tabs, local analysis, styles, and offline assets.
- Arduino CLI successfully compiles Feather ESP32 V2 and Uno receiver.
- Vercel production serves the index, stylesheet, scripts, manifest and offline service worker (HTTP 200).
- GitHub Actions packages compiled firmware, complete ZIP and SHA256 manifest. Preserve prior tags and source.

## 5. Hardware acceptance — still to perform

1. On a stationary bench, flash an independently verified V14 Feather binary using Arduino IDE USB, then verify serial boot, SD mount, IMU, BMP388, GPS, LoRa and power on protected LiPo.
2. On Windows, join Feather AP and verify all live controls, guided home tests, SD write/read/delete and recording retention across network loss.
3. Confirm direct Uno USB Web Serial telemetry, RSSI, GPS validity/age, receive-only safety and link fallback; check interference and field range at the launch site.
4. On iPhone/iPad, test installed PWA offline analysis and attempt hosted HTTPS local access. If browser prevents it, use the direct Feather fallback dashboard at 192.168.4.1.
5. Test signed-in Supabase backup and verified restore of an actual BIN/SEN/TXT on a second device; ensure offline work survives browser/app restarts and export remains available.
6. Test OTA **only on the bench** on stable power, verify rollback and run a complete guided home test afterward.
7. Confirm mechanical mounting, retention, RF reception, recovery methods, range and battery under actual launch conditions with a qualified supervisor.

Do not label flight-ready, promise uninterrupted iOS local Wi-Fi access, or state physical OTA tested until these checks have been documented.

## Shipping

- Beta 4: multi-device CSS, optional live Sensor Lab and GIS1 CRC-backed historical IMU/GPS graphs, cloud progress and backoff, organized wiring/docs.
- Release automatically through Actions after tests and successful board compilation. Main is the deploy branch; older releases remain available.
