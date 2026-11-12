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
- [ ] **Phase 2 — Unified telemetry**
- [ ] **Phase 3 — Mission workflow**
- [ ] **Phase 4 — Five-tab interface**
- [ ] **Phase 5 — Data and cloud**
- [ ] **Phase 6 — Windows packaging and updates**
- [ ] **Phase 7 — Advanced visualization, 3D trajectory and digital twin**
- [ ] **Phase 8 — Real hardware integration and measured evidence**

## Validation boundary

Phase 1 can be validated by automated software tests and CI. It does **not** prove real Feather Wi-Fi stability, real LoRa range, power integrity, SD timing, GPS reception or flight readiness. Those require later bench and field evidence.
