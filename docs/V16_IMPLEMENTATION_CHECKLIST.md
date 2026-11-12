# GREEN ION V16 implementation checklist

V16 is the next application/ground-station generation after V15 Beta 1. Feather and Uno firmware remain the V14 generation unless a later V16 phase explicitly changes them.

## Starting point

- Baseline main commit at Phase 1 start: `2dd20f73d7b0ea4264b452da9e07288d87fdf1ca`.
- Existing PR #30 already serialized Feather HTTP work and widened status freshness. V16 Phase 1 builds on that fix instead of replacing it.
- Autonomous SD recording, RKT4 v2, GIS1, receive-only LoRa, Supabase, OTA protections and original flight files must remain compatible.

## Gap assessment before V16 work

- Windows localhost ground station already survives loss of Feather Wi-Fi, but transient Feather status failures could still make the UI appear offline.
- Multiple browser status callers could overlap even though the ESP32 upstream bridge was serialized.
- A long file transfer could hold the single-client Feather connection while the background poll waited behind it, making status age look like a link failure.
- The UI did not expose the requested Connected / Delayed / Reconnecting / Disconnected state model or detailed last-success age.
- V15 still has more than the final five planned top-level tabs and does not yet have the V16 mission controller, unified telemetry fusion, packaging redesign or 3D/digital-twin work.

## Phase status

- [x] **Phase 1 — Stabilize Windows**
  - [x] Audit bridge, dashboard status polling and existing PR #30 behavior.
  - [x] Prevent concurrent browser `/api/status` requests.
  - [x] Defer background Feather polling during an active upstream transfer instead of counting local contention as a Wi-Fi failure.
  - [x] Add Connected / Delayed / Reconnecting / Disconnected states.
  - [x] Preserve last-known Feather status as explicitly stale and never command-authorized.
  - [x] Keep localhost responsive while Feather Wi-Fi is unavailable.
  - [x] Automatically resume normal status after Feather reconnect.
  - [x] Keep command authorization stricter than visual link state.
  - [x] Add last-success age, failure count and active-operation diagnostics.
  - [x] Add simulated one-hour and interruption regression coverage.
  - [x] Invalidate the PWA cache for the V16 Phase 1 transport changes.
- [x] **Phase 2 — Unified telemetry**
  - [x] Fuse Wi-Fi and LoRa into one source-aware telemetry schema.
  - [x] Track Wi-Fi and LoRa freshness independently.
  - [x] Preserve source, age and freshness for each selected live reading.
  - [x] Add receiver/sender session identity and stable packet IDs.
  - [x] Prevent duplicates/out-of-order frames from replacing the live sample.
  - [x] Retain bounded receiver-side radio history and packet classifications.
  - [x] Track packet gaps, duplicates, out-of-order frames, sender resets and receiver reconnects.
  - [x] Expose RSSI/SNR and radio protocol compatibility.
  - [x] Safely auto-discover an unambiguous Arduino Uno receiver.
  - [x] Drive Windows link diagnostics from one fused endpoint.
  - [x] Keep LoRa compact/receive-only and full-resolution sensor data on SD/history.
  - [x] Add Wi-Fi-only, LoRa-only, dual-link, stale/loss/recovery and integrity regressions.
- [x] **Phase 3 — Mission workflow**
  - [x] Add persistent IDLE→PREFLIGHT→READY→PRELAUNCH→ASCENT→DESCENT→LANDED→RECOVERY→DOWNLOADING→ANALYZING→COMPLETE state machine.
  - [x] Add ERROR and INTERRUPTED paths without claiming browser authority over the rocket.
  - [x] Integrate the existing measured full preflight and preserve its evidence.
  - [x] Require separate physical/manual confirmation after measured checks.
  - [x] Prevent missing receiver/non-physical evidence from being manually overridden.
  - [x] Drive airborne states only from fresh unified Feather/LoRa flight-state evidence.
  - [x] Preserve GPS-missing warnings for recovery without stopping autonomous recording.
  - [x] Bind download/save/report events to the current expected flight file.
  - [x] Ignore unrelated archived flights during an active mission.
  - [x] Persist workflow state and mark active browser restarts INTERRUPTED.
  - [x] Export bounded mission transition/evidence JSON.
  - [x] Add mission workflow regression coverage.
- [x] **Phase 4 — Five-tab interface**
  - [x] Expose exactly Dashboard / Flight / Flights & Analysis / Testing / Settings & Advanced.
  - [x] Add a concise Dashboard mission summary and Prepare for Flight shortcut.
  - [x] Keep detailed live telemetry/graphs expandable instead of always visible.
  - [x] Keep the Mission Controller first in Flight and combine recovery into the flight workflow.
  - [x] Combine Storage + History into Flights & Analysis without breaking offline analysis.
  - [x] Keep one-button preflight and hardware/storage checks in Testing.
  - [x] Group profiles, sensors, radio, cloud, updates, help and diagnostics under expandable Settings & Advanced sections.
  - [x] Preserve hidden legacy tab aliases for internal/backward-compatible navigation.
  - [x] Route old destinations to their new visible parent tab.
  - [x] Preserve Alt+1…Alt+5 keyboard navigation for the exact five tabs.
  - [x] Cache both V16 layout/navigation modules for offline updates.
  - [x] Add strict regression coverage that fails if visible navigation stops being exactly five tabs.
- [x] **Phase 5 — Data and cloud**
  - [x] Keep Feather SD → verified browser originals → Windows disk mirror → optional cloud as the priority order.
  - [x] Group BIN/SEN/TXT into one flight entry without requiring optional companions.
  - [x] Detect same-flight/same-kind checksum collisions without overwriting originals.
  - [x] Add local SHA-256 reread verification and exportable package manifest.
  - [x] Persist a bounded Feather transfer journal with partial-transfer rejection and exponential retry.
  - [x] Resume completed Feather transfers safely across browser restarts.
  - [x] Expose signed-out / unavailable / offline / pending / syncing / retrying / synced / restoring / error cloud states.
  - [x] Persist per-file cloud checksum status and attempt count.
  - [x] Resume cloud backup at file granularity by skipping checksums already stored.
  - [x] Keep cloud restore byte-size/SHA-256 verified and non-overwriting.
  - [x] Read-only audit the connected Supabase table, bucket, grants and owner-scoped RLS.
  - [x] Ensure cloud availability never gates recording, recovery or local analysis.
  - [x] Add package/transfer/cloud regression coverage.
- [x] **Phase 6 — Windows packaging and updates**
  - [x] Add one-click START_GREEN_ION.cmd with no normal COM-port prompt.
  - [x] Auto-detect Python 3.10+ and an unambiguous Arduino Uno.
  - [x] Pin pyserial and fall back to Wi-Fi-only mode if USB support is unavailable.
  - [x] Keep GREEN_ION_Data outside the replaceable application folder.
  - [x] Add checksum-verified, side-by-side official-release updater.
  - [x] Never kill the running app or overwrite user data during update.
  - [x] Add /api/app/health and System runtime diagnostics.
  - [x] Build and verify a complete V16 Windows ZIP artifact in CI.
  - [x] Ensure Windows-only source changes trigger the main CI workflow.
  - [x] Add packaging/updater regression coverage.
- [x] **Phase 7 — Advanced visualization, 3D trajectory and digital twin**
  - [x] Keep all 3D/replay processing off the Feather and postflight-only.
  - [x] Reconstruct local East/North trajectory from verified current GIS1 GPS fixes.
  - [x] Prefer synchronized pressure-derived relative altitude for the vertical track.
  - [x] Fall back to an honest pressure-only vertical replay when GIS1/GPS is unavailable.
  - [x] Integrate measured gyro rates into normalized relative-orientation quaternions.
  - [x] Explicitly label orientation drift/initial-reference limits; never claim absolute attitude.
  - [x] Add orbit/zoom/replay controls with a generic, non-scale rocket marker.
  - [x] Preserve full-resolution original BIN/SEN data separately from display decimation.
  - [x] Add synthetic reconstruction/quaternion regression coverage.
- [ ] **Phase 8 — Real hardware integration and measured evidence**
  - [x] Implement a hardware-evidence ledger with NOT_TESTED / PASS / FAIL / INCONCLUSIVE.
  - [x] Default every physical requirement to NOT_TESTED.
  - [x] Allow local runtime/telemetry diagnostic snapshots without auto-passing physical tests.
  - [x] Require a measured note or retained evidence reference before manual PASS is accepted.
  - [x] Export validation evidence JSON with explicit advisory-only status.
  - [x] Ensure even all required PASS results only in READY_FOR_HUMAN_REVIEW_NOT_FLIGHT_AUTHORIZATION.
  - [x] Document exact Windows, SD, sensor, GPS, LoRa, SPI, power, OTA, mechanical and flight evidence requirements.
  - [ ] Run the Windows + Feather reconnect soak on the actual hardware.
  - [ ] Verify autonomous SD recording and original-file integrity through real network loss.
  - [ ] Complete assembled IMU/barometer and outdoor GPS measurements.
  - [ ] Measure receiver-side LoRa range/integrity and shared-SPI behavior.
  - [ ] Measure real LiPo/voltage/brownout behavior.
  - [ ] Perform bench OTA/rollback/interruption validation.
  - [ ] Inspect actual rocket mounting/antenna/fit and complete supervised real-flight evidence.

## V16.1 reliability + simplicity pass

Requested improvement set implemented without changing the five-tab blue navigation or onboard file formats:

- [x] **2 — Better automatic recovery**
  - [x] Continuously rediscover Uno USB after unplug/replug or COM-port change.
  - [x] Preserve V16 Feather reconnect state behavior.
  - [x] Keep partial flight transfers rejected/retryable instead of saving incomplete originals.
- [x] **3 — Simpler first-time Windows setup**
  - [x] Keep one-click launcher and no normal COM prompt.
  - [x] Explain first-run steps in the launcher.
  - [x] Allow Uno to be plugged in after app startup.
- [x] **4 — Stronger connection diagnostics**
  - [x] Add concise Feather / Uno / LoRa / GPS / SD / cloud / archive health view.
  - [x] Add downloadable local diagnostic evidence.
- [x] **6 — Improve automated home test**
  - [x] Add a simple guided wrapper around the existing real full preflight.
  - [x] Never simulate PASS or override missing measured/receiver evidence.
- [x] **7 — Improve flight analysis**
  - [x] Require sustained pressure evidence for phase detection.
  - [x] Add event confidence scores.
  - [x] Add IMU/GPS corroboration and pressure-vs-GPS altitude comparison.
  - [x] Label any inferred burnout transition as an estimate.
- [x] **10 — Improve 3D reconstruction**
  - [x] Align pressure/GIS1 timelines by recorded timestamps.
  - [x] Add light display smoothing without modifying raw GPS.
  - [x] Add flight-phase markers and animation-frame replay.
- [x] **11 — Better flight library**
  - [x] Add search, favorites, tags and notes.
  - [x] Keep incomplete/collision status obvious.
  - [x] Store annotations separately from immutable originals.
- [x] **12 — Safer/easier updates**
  - [x] Show release notes before Windows app update.
  - [x] Keep verified side-by-side packages.
  - [x] Add one-click app rollback to the prior complete package.
  - [x] Never modify GREEN_ION_Data during app update/rollback.
- [x] **13 — Performance cleanup**
  - [x] Keep full-resolution originals while bounding/decimating display work.
  - [x] Use background worker for advanced reports and animation-frame 3D replay.
- [x] **14 — UI polish**
  - [x] Preserve original blue background and exactly five visible tabs.
  - [x] Keep only critical Dashboard readings visible by default.
  - [x] Add direct Find Rocket shortcut.
- [x] **15 — Release automation**
  - [x] Add verified `v16.*` tag workflow.
  - [x] Gate release on tests, site build, Feather compile, Uno compile and Windows package verification.
  - [x] Publish Windows ZIP/checksum/manifest, web ZIP, Feather BIN, Uno HEX and release SHA manifest.
- [x] **LoRa recovery requirement**
  - [x] Preserve compact 70-byte CRC packet.
  - [x] Regression-lock GPS, altitude, pressure, battery, motion, satellites, flight state, flags and health.
  - [x] Preserve receiver RSSI/SNR and packet-integrity evidence.
  - [x] Keep ground LoRa receive-only.
- [x] **Finder requirement**
  - [x] Open last confirmed rocket GPS fix in Google Maps.
  - [x] With user permission, compare to current browser location.
  - [x] Show straight-line distance/bearing and Google Maps directions.
  - [x] Never replace raw recovery GPS with 3D-smoothed coordinates.

## Validation boundary

Phase 1 can be validated by automated software tests and CI. It does **not** prove real Feather Wi-Fi stability, real LoRa range, power integrity, SD timing, GPS reception or flight readiness. Those require later bench and field evidence.
