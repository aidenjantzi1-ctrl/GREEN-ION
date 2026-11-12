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
  - [x] Keep Feather SD → verified local originals → Windows disk mirror → optional cloud as the data priority order.
  - [x] Group BIN/SEN/TXT into one flight library entry without making optional companions mandatory.
  - [x] Detect same-flight/same-kind checksum collisions instead of overwriting them.
  - [x] Add local SHA-256 re-verification and manifest export.
  - [x] Persist a bounded Feather transfer journal with partial-transfer rejection and exponential retry.
  - [x] Make completed file transfers retry-safe across browser restarts.
  - [x] Expose explicit Supabase queue/sync/retry/restore/error state.
  - [x] Persist per-file cloud checksum state and attempts.
  - [x] Resume cloud backup at file granularity by skipping remote checksums already stored.
  - [x] Keep cloud restore byte-size/SHA-256 verified and non-overwriting.
  - [x] Audit the connected Supabase table, bucket, grants and owner-scoped RLS read-only.
  - [x] Add local-first data/cloud regression coverage and offline-cache integration.
- [ ] **Phase 6 — Windows packaging and updates**
- [ ] **Phase 7 — Advanced visualization, 3D trajectory and digital twin**
- [ ] **Phase 8 — Real hardware integration and measured evidence**

## Validation boundary

Phase 1 can be validated by automated software tests and CI. It does **not** prove real Feather Wi-Fi stability, real LoRa range, power integrity, SD timing, GPS reception or flight readiness. Those require later bench and field evidence.
