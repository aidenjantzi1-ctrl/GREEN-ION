# GREEN ION V16.2 implementation

Baseline: `d7a0943835cd2908b0b335869f211d60760b55c4` (V16.1 RC2)

V16.2 preserves the autonomous SD recorder, RKT4 v2 BIN, GIS1 SEN, receive-only ground LoRa, existing cloud accounts/backups, Windows `GREEN_ION_Data`, OTA/rollback, the `#081420` visual identity, and exactly five visible tabs.

## Completion status

- [x] **Phase 0 — repository audit and baseline**
  - Audited V16.1 RC2, firmware/receiver, Windows bridge, five-tab UI, recovery, visualization, packaging, recent PRs and validation boundaries.
- [x] **Phase 1 — reliability and performance**
  - Direct-browser Wi-Fi display freshness is separated from strict command freshness.
  - Brief status failures keep the last observation visible but pause hardware controls immediately.
  - Existing Windows Connected / Delayed / Reconnecting / Disconnected bridge behavior is preserved.
  - Uno auto-rediscovery/COM-port changes and bounded telemetry history are preserved.
  - Automated one-hour and two-hour connection-soak simulations are included.
- [x] **Phase 2 — Feather/Uno V16.2 firmware generation**
  - New `Green_Ion_V16_2` and `Ground_Receiver_V16_2` build targets.
  - Firmware reports V16.2 identity, boot/session/reset evidence, sensor/recorder/radio/SD diagnostics and bounded failure counters.
  - RKT4 v2, GIS1 and the protocol-v2 70-byte LoRa packet remain byte-compatible.
  - GPS/radio failures do not gate the primary recorder.
  - OTA/rollback protections remain bench-only and deliberate.
- [x] **Phase 3 — simplified five-tab application**
  - Exactly Dashboard / Flight / Flights & Analysis / Testing / Settings & Advanced.
  - `#081420` dark-blue identity retained.
  - Balanced is the default display preset; Simple and Advanced affect presentation only.
  - Advanced settings remain collapsed by default.
- [x] **Phase 4 — live Dashboard rocket**
  - Lightweight Canvas2D side-view rocket with phase, relative gyro rotation, estimated flame and illustrative parachute.
  - Stale telemetry freezes instead of inventing an ongoing trajectory.
  - Saved-flight replay is explicitly labeled NOT LIVE.
- [x] **Phase 5 — genuine interactive 3D viewer**
  - WebGL depth/perspective renderer with real 3D rocket geometry.
  - Worker-derived GPS+pressure or pressure-only reconstruction.
  - Orbit/pan/zoom, mouse/touch, follow/ground/top/side/free cameras.
  - Timestamp playback at 0.25×/0.5×/1×/2×/5× and phase jumps.
  - Graph/replay timestamp synchronization and WebGL fallback behavior.
  - Real browser WebGL render smoke test in CI when Chromium is available.
- [x] **Phase 6 — embedded Google Maps recovery**
  - Optional Maps JavaScript API integration with no committed private key.
  - Satellite/terrain/road map options, confirmed GPS trail, last-known position and permission-based user location.
  - Offline plot, coordinates, distance/bearing and external Google Maps link remain available without a key or internet.
- [x] **Phase 7 — safer SD management**
  - Backup evidence shown before deletion.
  - Exact filename confirmation, completion checks, critical-operation blocking, companions-first/BIN-last deletion and post-delete directory verification.
  - Browser, Windows and cloud copies are never deleted by Feather SD cleanup.
  - Individual/package download paths compute SHA-256 and compare against known originals when possible.
- [x] **Phase 8 — flight library and comparisons**
  - Existing search/favorites/tags/notes retained.
  - Multi-flight comparison uses Web Workers and launch-relative time.
  - Metrics include altitude, duration, estimated velocity/acceleration, G provenance, ascent/descent, GPS distance, completeness and recovery position.
  - Exportable comparison JSON.
- [x] **Phase 9 — guided testing and diagnostics**
  - One visible primary **Run Full Home Test** workflow.
  - PASS / FAIL / PENDING / INCONCLUSIVE / NOT TESTED states remain evidence-based.
  - System diagnostics stay human-readable with downloadable JSON.
- [x] **Phase 10 — Windows and phone/tablet experience**
  - One-click Windows start, Uno auto-discovery, verified side-by-side updates and rollback retained.
  - `%USERPROFILE%\GREEN_ION_Data` remains outside replaceable app packages.
  - Responsive/touch UI and direct Feather fallback retained; iPhone/iPad never claim direct LoRa reception.
- [x] **Phase 11 — cloud and data integrity**
  - Local originals remain authoritative.
  - IndexedDB + Windows archive + optional Supabase synchronization/checksum behavior retained.
  - Cloud is never required for recording, recovery or analysis.
- [x] **Phase 12 — high-value usability improvements**
  - Smart mission next-action summary, one-click postflight report entry, Dashboard saved-flight replay, missing-data explanations, data-quality scorecard, richer local rocket profiles, backup-before-cleanup guidance, accessibility improvements and optional first-run helper.
  - Battery UI only displays measured voltage/estimates supported by existing calibration inputs; no unsupported precise endurance claim.
- [x] **Phase 13 — visual honesty rules**
  - Measured, estimated and illustrative values/effects are kept distinct.
  - Relative gyro orientation is not described as absolute attitude.
  - Visual effects are isolated from recorder/control logic.
- [x] **Phase 14 — automated testing**
  - Python, Node/browser, firmware-contract, Windows reliability, WebGL, release, SD/cloud/protocol and long-duration simulated connection coverage.
  - Actual headless Chromium WebGL rendering is exercised when available.
- [x] **Phase 15 — documentation**
  - Windows setup, Feather update, Uno update, LoRa protocol, SD management, Google Maps, WebGL 3D, live visualization, hardware-validation and release-note guides are present.
- [ ] **Phase 16 — GitHub integration and stable V16.2 release**
  - [x] Feature branch contains complete V16.2 source.
  - [x] Main workflow compiles V16.2 Feather and Uno targets and builds web/Windows/release-smoke artifacts.
  - [x] Post-merge main workflow is configured to publish immutable `v16.2.0` only after all mandatory gates pass.
  - [ ] Open/merge V16.2 PR after final branch CI is green.
  - [ ] Confirm merged `main` CI.
  - [ ] Confirm `v16.2.0` tag/release and published asset checksums on GitHub.
- [x] **Phase 17 — final software cleanup**
  - Legacy flat Canvas projection is no longer initialized as the primary V16.2 viewer.
  - No private Google API key or cloud administrator credential is committed.
  - Five-tab navigation, blue identity, RKT4/GIS1 compatibility and receive-only LoRa have dedicated regressions.

## Compatibility decisions

1. LoRa remains protocol v2 / 70 bytes. V16.2 adds diagnostics around the packet instead of increasing airtime.
2. RKT4 v2 and GIS1 byte layouts are unchanged.
3. The evidence reconstruction worker is retained; only the display layer becomes genuine WebGL 3D.
4. Google Maps is optional. Without a configured restricted browser key or internet, Recovery retains the offline plot, coordinates, distance/bearing and external Maps shortcut.
5. Flame, smoke, parachute and ground contact are illustrative unless separate physical evidence exists. Relative gyro orientation is estimated and never absolute attitude.
6. Windows app updates and Feather OTA remain separate, independently confirmed operations.

## Physical validation still required

Automated tests and CI **do not** prove the assembled hardware. The following remain **NOT TESTED / REQUIRES PHYSICAL TEST** until measured:

- Feather + Windows reconnect soak on the actual laptop/network.
- Real microSD timing, write/flush/checkpoint behavior and power-loss recovery.
- Shared SD/LoRa SPI behavior on the assembled perfboard.
- BMP388/LSM6DSO32 measurements and mounting-axis verification.
- Outdoor GPS acquisition/age/accuracy evidence.
- LoRa range, packet loss, RSSI/SNR and antenna installation.
- LiPo voltage-divider behavior, current bursts, reset/brownout behavior.
- OTA/rollback/interruption behavior on stable bench power.
- Mechanical installation/RF placement and supervised real flight.

No software checklist item is launch authorization.
