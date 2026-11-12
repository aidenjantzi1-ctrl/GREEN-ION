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
  - [x] Group immutable BIN/SEN/TXT siblings into source-aware flight packages.
  - [x] Surface missing companions and checksum collisions without overwriting originals.
  - [x] Add local SHA-256 reread verification and exportable package manifest.
  - [x] Keep local browser/disk originals authoritative.
  - [x] Expose explicit cloud states: signed out, unavailable, offline, idle, pending, retrying, synced and error.
  - [x] Preserve optional Supabase retry/dedup/verified restore behavior.
  - [x] Ensure cloud availability never gates recording, recovery or local analysis.
  - [x] Add package/cloud regression coverage.
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

## Validation boundary

Phase 1 can be validated by automated software tests and CI. It does **not** prove real Feather Wi-Fi stability, real LoRa range, power integrity, SD timing, GPS reception or flight readiness. Those require later bench and field evidence.
