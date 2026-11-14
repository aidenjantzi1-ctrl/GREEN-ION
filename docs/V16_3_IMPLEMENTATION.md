# GREEN ION V16.3 implementation and V16.2 completion audit

Baseline: V16.2 stable release `v16.2.0`, main commit `041af441901b8b0f2e2b17bc23db144465a3267e`.

Status vocabulary: **COMPLETE**, **PARTIAL**, **MISSING**, **BLOCKED**, **REQUIRES PHYSICAL TEST**.

## V16.2 completion matrix

| Requirement | Status | Evidence / V16.3 action |
|---|---|---|
| Feather V16.2 firmware generation | COMPLETE | `Past_Versions/V16/Green_Ion_V16_2`; compiled and released BIN. |
| Compatible Uno V16.2 receiver | COMPLETE | `Past_Versions/V16/Ground_Receiver_V16_2`; protocol v2 / 70 bytes retained; released HEX. |
| Windows reconnect / COM-port recovery | COMPLETE | Automatic Feather retry and Uno rediscovery retained. |
| Exactly five tabs / #081420 | COMPLETE | Regression-locked. |
| Balanced everyday controls | COMPLETE | Balanced is default; Simple/Advanced affect presentation only. |
| Live Dashboard rocket | COMPLETE | Sensor-based/estimated/illustrative Canvas2D visualization with stale freeze and replay. |
| Genuine WebGL 3D | COMPLETE | Perspective/depth WebGL renderer; real browser shader/readback smoke passes. |
| 3D rocket geometry | COMPLETE | Nose/body/fins geometry rendered in WebGL. |
| Timestamp replay / speed / jumps | COMPLETE | Actual timestamps, 0.25×–5×, launch/apogee/descent/landing jumps. |
| Camera / touch controls | COMPLETE | Orbit/pan/zoom/follow/ground/top/side/free. |
| Graph synchronization | COMPLETE | Replay events synchronize graph/3D time. |
| Embedded Google Maps | COMPLETE | Optional configured browser key, confirmed GPS only. |
| Offline map fallback | COMPLETE | Existing offline track/coordinates/distance/bearing/external Maps link preserved. |
| Safe SD manager / backup evidence | COMPLETE | Completion check, backup evidence, exact filename, companions first/BIN last, recheck. |
| Flight library / comparisons | COMPLETE | Search/annotations retained; multi-flight worker comparison added. |
| Guided testing / diagnostics | COMPLETE | One primary Full Home Test and System Health. |
| Windows startup/update/rollback | COMPLETE | Verified side-by-side package and persistent user data. |
| Phone/tablet improvements | COMPLETE | Responsive/touch surfaces; no claim of direct iPhone LoRa. |
| Supabase / data integrity | COMPLETE | Local-first originals, checksums, retry-safe optional cloud. |
| Data-quality scorecard | COMPLETE | Advisory data-quality summary; not flight authorization. |
| Rocket profiles / battery reporting | COMPLETE | Profile metadata + measured battery reporting without unsupported endurance precision. |
| First-run / accessibility | COMPLETE | Optional first-run and focus/touch improvements. |
| Protected OTA / rollback | COMPLETE | Deliberate bench-only firmware update remains separate from app update. |
| Automated software tests | COMPLETE | Python, Node, WebGL browser smoke, firmware compile, packaging and release smoke. |
| V16.2 documentation | COMPLETE | Setup, firmware, receiver, LoRa, SD, Maps, 3D, visualization, validation guides. |
| V16.2 release assets | COMPLETE | GitHub release `v16.2.0` targets the verified merged commit. |
| Real shared SD + LoRa SPI proof | REQUIRES PHYSICAL TEST | V16.3 hardens architecture and adds a measured bench procedure; CI cannot certify perfboard behavior. |
| Real RF/GPS/power/flight validation | REQUIRES PHYSICAL TEST | Remains in hardware evidence ledger. |

## V16.3 work

- [x] Centralize shared SPI ownership and defer RFM9x DIO0 SPI work out of interrupt context.
- [x] Give sensor sampling / BIN / checkpoints / SEN priority over radio.
- [x] Add shared-SPI diagnostics and bounded radio recovery evidence.
- [x] Extend Full Home Test with concurrent SD + LoRa evidence and actual Windows Uno receipt when available.
- [x] Add host scheduler/protocol regressions and a physical perfboard stress procedure.
- [x] Improve radio recovery without reset storms or recorder interaction.
- [x] Preserve the simplified five-tab #081420 application and remove duplicate primary-tab semantics from hidden legacy routing.
- [x] Revalidate WebGL, live rocket, Maps, library/report and mobile behavior without replacing working V16.2 code.
- [x] Improve power/reset/sensor recovery evidence.
- [x] Add `docs/V16_3_SHARED_SPI_PERFBOARD.md`, update/Home Test and V16.3 physical-validation guides.
- [x] Update Windows/app/Feather/Uno identities to stable V16.3 and preserve rollback.
- [x] Final stable feature-branch CI passed: Python, native shared-SPI C++, Node/browser, headless WebGL, JS syntax, offline web build, Windows packaging, Feather compile, Uno compile and release-bundle smoke.
- [x] PR → main → verified `v16.3.0` release. Merged-main run `43443530326` passed app tests, Feather/Uno builds, Windows packaging, release smoke, and publication; GitHub release `v16.3.0` targets `e54ff44727387143f747b261b16138ad7b7deafe`.

## Current software validation

Stable feature-branch run `43345164016` passed the complete configured software/compile/package matrix. The subsequent merged-main run `43443530326` also passed all configured V16.3 jobs and published release `v16.3.0` from commit `e54ff44727387143f747b261b16138ad7b7deafe`. Physical validation remains a separate required gate.

## Physical validation status

**REQUIRES PHYSICAL TEST:** assembled perfboard SD+LoRa concurrency, RF range/link loss, outdoor GPS, LiPo/USB/brownout behavior, OTA interruption/rollback on real hardware, mechanical/RF installation and supervised real flight. Software/CI results must not be entered as physical PASS.

## Nonnegotiable compatibility

- RKT4 v2 BIN unchanged.
- GIS1 SEN unchanged.
- LoRa protocol v2 remains 70 bytes unless a measured need proves otherwise.
- Ground LoRa receive-only.
- Autonomous recording remains authoritative.
- No ignition, staging, deployment, pyrotechnic or guidance controls.
- `#081420` and exactly five visible tabs remain regression-locked.
- Physical hardware results are never inferred from CI.

## Home Wi-Fi / dual-network extension

- [x] AP-only fallback preserves `GREEN-ION-V16-3` at `192.168.4.1`.
- [x] Optional AP+STA joins a saved 2.4 GHz router while retaining the direct AP.
- [x] Station association/retry is bounded and nonblocking; asynchronous scans are capped and deleted after use.
- [x] Network reconfiguration, scans, credential writes and reconnect attempts are blocked while launch watching, flight recording, Home Test, OTA or maintenance are active.
- [x] Home SSID/password and optional custom AP password use Preferences/NVS; passwords are write-only from the dashboard and are not included in status/report/export responses.
- [x] Credential management requires a recent physical SW38 press plus a temporary RAM-only management token and per-browser CSRF header.
- [x] Embedded and hosted dashboards show direct/home addresses, RSSI, connection mode, scan/manual SSID, connect/disconnect/forget and AP-password controls.
- [x] Software regression coverage added for AP-only, success/failure, unavailable router, reconnect policy, forget, dual-interface dashboard, authorization, recording lockout and reboot persistence.
- [ ] **NOT VERIFIED ON HARDWARE:** real AP+STA association, router disconnect/rejoin, iPhone access through both interfaces, SW38 flow, RSSI, reboot persistence and 50 Hz recording during network loss still require the assembled Feather/router bench test.

See [V16.3 Home Wi-Fi + AP+STA](V16_3_HOME_WIFI.md).
