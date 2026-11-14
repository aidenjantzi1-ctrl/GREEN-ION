# V17 — integrated Wi-Fi, visualization, replay and recovery

V17 is the current GREEN ION software generation. It integrates the V16.3 recorder/shared-SPI baseline with protected AP+STA home Wi-Fi, live sensor graphs, WebGL attitude/replay, multi-flight comparison, GPS freshness/offline recovery tools and the comparison-filename security fix.

It preserves 50 Hz recording, the three-second prelaunch buffer, RKT4 v2, GIS1, protocol-v2 70-byte LoRa, receive-only ground radio and recorder independence from Wi-Fi.

**Software validation and hardware validation are separate.** See [V17 implementation](docs/V17_IMPLEMENTATION.md), [update guide](docs/V17_UPDATE_GUIDE.md), [release notes](docs/V17_RELEASE_NOTES.md) and [hardware validation](docs/V17_HARDWARE_VALIDATION.md).

# V16.3 — shared-SPI reliability

V16.3 is the current GREEN ION application/firmware/receiver generation. It hardens the Feather ESP32 V2 shared SPI bus used by the microSD and RFM9x on the perfboard without changing RKT4 v2, GIS1, the 70-byte LoRa packet or receive-only ground-radio behavior.

The recorder remains authoritative: sensor sampling, BIN writes, checkpoints and companion logging take priority; DIO0 never performs SPI inside interrupt context; radio work is bounded/deferred and cannot be treated as proof of SD success. Full Home Test adds real temporary 50 Hz SD + LoRa scheduling evidence and only credits Uno reception when actual receiver packets are observed.

V16.2 WebGL, live visualization, Maps, library/comparison, cloud, update/rollback and five-tab blue UI remain intact. Real perfboard/RF/power/GPS/flight validation is still required.

[V16.3 implementation](docs/V16_3_IMPLEMENTATION.md) · [update guide](docs/V16_3_UPDATE_GUIDE.md) · [shared-SPI guide](docs/V16_3_SHARED_SPI_PERFBOARD.md) · [hardware validation](docs/V16_3_HARDWARE_VALIDATION.md)

---

# V16.2 — application + Feather + Uno generation

V16.2 is the first V16 application release in this repository that also carries clearly versioned **Feather V16.2** and **Uno V16.2** source/build outputs.

Compatibility is intentionally conservative: RKT4 v2, GIS1, existing flights, the receive-only protocol-v2 70-byte LoRa packet, Windows `GREEN_ION_Data`, cloud accounts/backups and the five-tab blue interface are preserved.

Major additions are transient-link resilience, live/replay Dashboard visualization, genuine WebGL 3D reconstruction, optional embedded Google Maps recovery, backup-aware SD cleanup, richer library/comparison, simplified home testing, expanded diagnostics and verified V16.2 release/source packaging.

Software validation does not replace the physical V16.2 hardware-validation checklist.

[V16.2 implementation](docs/V16_2_IMPLEMENTATION.md) · [release notes](docs/V16_RELEASE_NOTES.md) · [hardware validation](docs/V16_2_HARDWARE_VALIDATION.md)

---

# V16.1 RC2 — safe SD management + layout hardening

RC2 adds a narrow Feather-Wi-Fi SD deletion workflow for completed `V12D###.BIN` flight packages. It removes matching SEN/TXT/recovery companions, deletes the primary BIN last, verifies the package is absent, blocks deletion during recording/home-test/maintenance/OTA work, and restarts stationary calibration if launch watching had to be paused. Browser, Windows and cloud backups are not deleted.

The Flights & Analysis layout keeps the same dark-blue visual language and exact five tabs while making Feather SD management easier to scan. Broken legacy Protect/Add note/manifest/integrity controls were removed or replaced with current V14-backed behavior. Normal SD browsing/downloads are also blocked during critical recorder operations.

Real microSD/hardware behavior still requires bench validation.

---

# V16.1 RC1 — reliability + simplicity

V16.1 keeps the original blue five-tab application while making first use and recovery simpler: automatic Uno USB rediscovery, concise diagnostics, guided real preflight, Google Maps recovery with current-location comparison, event confidence and GPS/pressure cross-checks, improved 3D replay, a searchable annotated flight library, reversible Windows app updates and a verified `v16.*` release pipeline.

The compact 70-byte LoRa protocol remains compatible and receive-only on the ground side; it already carries critical flight state, health, battery and GPS recovery information. Feather/Uno firmware remains the V14 generation.

Real hardware/range/power/OTA/mechanical/flight validation is still required.

[V16.1 details](docs/V16_1_RELIABILITY_SIMPLICITY.md) · [Release notes](docs/V16_RELEASE_NOTES.md)

---

# V16 software release candidate — hardware validation pending

V16 software phases 1–7 are implemented. Phase 8 now includes the evidence framework and exact physical acceptance plan, but real hardware measurements are deliberately still pending and cannot be replaced by CI.

The application may report **READY_FOR_HUMAN_REVIEW_NOT_FLIGHT_AUTHORIZATION** only after required physical evidence is entered. It never grants launch permission.

[Phase 8 hardware evidence](docs/V16_PHASE8_HARDWARE_VALIDATION.md) · [V16 checklist](docs/V16_IMPLEMENTATION_CHECKLIST.md)

---

# V16 development — Phase 7 advanced visualization

Phase 7 adds offline postflight 3D trajectory replay and a relative gyro-integrated digital-twin view from immutable RKT4/GIS1 originals. It runs entirely on the browser/computer and explicitly avoids claims of absolute attitude or aerodynamic simulation.

[Phase 7 visualization](docs/V16_PHASE7_ADVANCED_VISUALIZATION.md) · [V16 checklist](docs/V16_IMPLEMENTATION_CHECKLIST.md)

---

# V16 development — Phase 6 Windows packaging and updates

Phase 6 adds a one-click Windows launcher, automatic Uno discovery, package/runtime diagnostics, deterministic CI ZIPs and a checksum-verified side-by-side updater. User flight data remains outside the replaceable application folder.

[Phase 6 Windows packaging](docs/V16_PHASE6_WINDOWS_PACKAGING.md) · [V16 checklist](docs/V16_IMPLEMENTATION_CHECKLIST.md)

---

# V16 development — Phase 5 data and cloud

Phase 5 makes immutable local BIN/SEN/TXT originals easier to verify and group, adds exportable package manifests, and exposes explicit optional-cloud sync state. Local browser/disk originals remain authoritative; Supabase is backup only and never gates recording, recovery or analysis.

[Phase 5 data/cloud](docs/V16_PHASE5_DATA_CLOUD.md) · [V16 checklist](docs/V16_IMPLEMENTATION_CHECKLIST.md)

---

# V16 development — Phase 4 five-tab interface

Phase 4 replaces the intermediate navigation shell with the approved five top-level destinations: **Dashboard, Flight, Flights & Analysis, Testing, and Settings & Advanced**. Mission-critical information stays visible while detailed telemetry, radio diagnostics, cloud, updates and troubleshooting are grouped into expandable advanced sections.

Recovery is integrated into Flight; Storage and History are combined into Flights & Analysis; Testing remains a dedicated bench workflow. Hidden legacy tab aliases remain only for compatibility with older internal navigation. Alt+1 through Alt+5 follows the five visible tabs.

[Phase 4 details](docs/V16_PHASE4_FIVE_TAB_INTERFACE.md) · [V16 checklist](docs/V16_IMPLEMENTATION_CHECKLIST.md)

---

# V16 development — Phase 3 mission workflow

Phase 3 adds a persistent mission controller around the existing preflight, unified telemetry, recovery, verified flight retrieval and local postflight analysis. The browser tracks IDLE → PREFLIGHT → READY → PRELAUNCH → ASCENT → DESCENT → LANDED → RECOVERY → DOWNLOADING → ANALYZING → COMPLETE, with ERROR and INTERRUPTED states. Real flight transitions come from Feather/LoRa flight-state evidence; browser timers do not declare launch or landing.

Measured preflight failures and missing receiver evidence cannot be manually overridden. Physical checks require explicit user confirmation. Postflight file/report events are matched to the expected mission recording when known, so older archived flights cannot complete the active mission.

[Phase 3 details](docs/V16_PHASE3_MISSION_WORKFLOW.md) · [V16 checklist](docs/V16_IMPLEMENTATION_CHECKLIST.md)

---

# V16 development — Phase 2 unified telemetry

Phase 2 adds one source-aware Windows telemetry contract across Feather Wi-Fi and receive-only Uno/LoRa. It preserves independent source freshness, per-reading provenance, packet/session identity, receiver integrity counters, RSSI/SNR, bounded radio history, duplicate/out-of-order rejection for the live sample, and safe automatic Uno discovery. The Windows diagnostics panel now polls one fused endpoint instead of separate telemetry/link calls.

The 70-byte LoRa packet, RKT4 v2 and GIS1 formats remain unchanged. The Uno USB JSON adds protocol/SNR evidence. Feather firmware remains V14 generation. Physical link validation is still pending.

[V16 Phase 2 details](docs/V16_PHASE2_UNIFIED_TELEMETRY.md) · [V16 checklist](docs/V16_IMPLEMENTATION_CHECKLIST.md)

---

# V16 development — Phase 1 Windows reliability

**Current development line: V16.** Phase 1 hardens the Windows localhost ground station before the mission-controller or five-tab redesign. It coalesces overlapping status requests, prevents long file transfers from producing false Wi-Fi failures, adds Connected / Delayed / Reconnecting / Disconnected states, preserves last-known Feather data as explicitly stale, and keeps command authorization on a stricter freshness window. The PWA cache is bumped so the transport fix actually reaches installed clients.

Feather and Uno firmware remain the V14 generation in this phase. RKT4 v2, GIS1, autonomous SD recording, receive-only LoRa, Supabase and OTA safety behavior are unchanged. Real Windows/Feather hardware validation is still pending.

[V16 checklist](docs/V16_IMPLEMENTATION_CHECKLIST.md) · [Phase 1 reliability details](docs/V16_PHASE1_WINDOWS_RELIABILITY.md)

---

# V15 Beta 1 — Local-first dashboard and Windows ground station (published)

**Current application release: V15 Beta 1.** Merged into GitHub `main` and deployed to Vercel production. Includes one-button bench preflight, new Recovery and Help tabs, offline advanced RKT4/GIS1 analysis and health reports, computer-based SHA-256 flight archiving, optional cloud retry when internet returns, battery forecasts, and combined Wi-Fi/LoRa diagnostics. The original dark-blue layout and existing flight formats are preserved.

**Important firmware distinction:** the Feather ESP32 V2 and Uno firmware remain the V14 generation. V15 is an application/ground-station release, not a newly flashed V15 firmware image. Do not use the website version as proof that Feather hardware was updated or bench-tested.

[Open V15 website](https://green-ion-beta.vercel.app/) · [V15 local-first setup guide](docs/V15_LOCAL_FIRST.md) · [V15 3D and digital twin plan (not yet implemented)](docs/V15_3D_TWIN_PLAN.md) · [Merged V15 PR #28](https://github.com/aidenjantzi1-ctrl/GREEN-ION/pull/28)

---

# V14.0.0-beta.4

Original dark-blue tabs + adaptive multi-device dashboard, optional sensor/recording diagnostics, GIS1 offline IMU/GPS history, private Supabase cloud sync, concise README with soldering and sensor guides. Hardware validation pending. [Start](README.md) · [Soldering](docs/V14_WIRING_SOLDER.md) · [Cloud](docs/V14_CLOUD.md).

---

## V14 (software beta)

Full blue hosted PWA with original tabs, offline/phone support, Windows Uno/LoRa, optional private Supabase backups, GitHub update prompts, compiled firmware and Feather local fallback. See [V14 start](docs/V14_START_HERE.md). Hardware and iPhone browser connectivity validation pending.

---

## V13 (software beta)

Unified Windows ground station at 127.0.0.1:8765 with full Feather dashboard, automatic Wi-Fi/LoRa telemetry failover, downloadable one-ZIP GitHub package and independent telemetry logs. New Feather sketch folder Green_Ion_V13; existing V12D### BIN/SEN/TXT file naming is deliberately retained. Hardware and flight validation pending.

# Green Ion — Complete Version Archive

This index records every known or referenced Green Ion version, starting with V1. **Historical placeholder folders do not contain source and must not be used as firmware.** V10 is the latest standalone snapshot.

## Original-source availability

| Version | Source archived? | Status |
| --- | --- | --- |
| V1 | No | partially documented |
| V1.2 | No | documented by output |
| V2 | No | undocumented |
| V3 | No | documented by output |
| V3.1 | No | documented by output |
| V4 | No | undocumented |
| V5 | No | undocumented |
| V6 | No | undocumented |
| V7 | No | undocumented |
| V8 | Yes — original .ino snapshot | Archived |
| V8.1 | Yes — original .ino snapshot | Archived |
| V8.2 | Yes — original .ino snapshot | Archived |
| V8.3 | Yes — original .ino snapshot | Archived |
| V8.4 | Yes — original .ino snapshot | Archived |
| V8.5 | Yes — original .ino snapshot | Archived |
| V8.6 | Yes — original .ino snapshot | Archived |
| V8.7 | Yes — original .ino snapshot | Archived |
| V8.8 | Yes — original .ino snapshot | Archived |
| V9 | Yes — original .ino snapshot | Archived |
| V9.1 | Yes — original .ino snapshot | Archived |
| V9.2 | Yes — original .ino snapshot | Archived |
| V10 | Yes — original .ino snapshot | Current |

## Historical version notes

## V1 — Initial BMP388 altimeter prototype

**Evidence:** partially documented.

Partial serial-output evidence exists; relative-altitude reference initially produced a large offset (approximately -1839 m).

This folder is an archive record, not a compilable Arduino sketch. No original source was available to preserve.

## V1.2 — Calibrated relative-altitude prototype

**Evidence:** documented by output.

BMP388 warm-up and ground-pressure calibration, relative-altitude output, temperature/pressure serial readings.

This folder is an archive record, not a compilable Arduino sketch. No original source was available to preserve.

## V2 — Unverified version

**Evidence:** undocumented.

No archived firmware or reliable change evidence. Do not assume this version was formally released.

This folder is an archive record, not a compilable Arduino sketch. No original source was available to preserve.

## V3 — Flight-state recorder prototype

**Evidence:** documented by output.

Serial output documents ARMED, FLIGHT and LANDED states, automatic launch detection, velocity, max altitude and battery monitoring.

This folder is an archive record, not a compilable Arduino sketch. No original source was available to preserve.

## V3.1 — microSD recorder prototype

**Evidence:** documented by output.

Serial output documents SD initialization and operation when no card was detected, with improved observed velocity stability.

This folder is an archive record, not a compilable Arduino sketch. No original source was available to preserve.

## V4 — Unverified version

**Evidence:** undocumented.

No archived firmware or reliable change evidence. Do not assume this version was formally released.

This folder is an archive record, not a compilable Arduino sketch. No original source was available to preserve.

## V5 — Unverified version

**Evidence:** undocumented.

No archived firmware or reliable change evidence. Do not assume this version was formally released.

This folder is an archive record, not a compilable Arduino sketch. No original source was available to preserve.

## V6 — Unverified version

**Evidence:** undocumented.

No archived firmware or reliable change evidence. Do not assume this version was formally released.

This folder is an archive record, not a compilable Arduino sketch. No original source was available to preserve.

## V7 — Unverified version

**Evidence:** undocumented.

No archived firmware or reliable change evidence. Do not assume this version was formally released.

This folder is an archive record, not a compilable Arduino sketch. No original source was available to preserve.

## Compilable firmware snapshots

- [V8](./Past_Versions/Green_Ion_V8/Green_Ion_V8.ino)
- [V8.1](./Past_Versions/Green_Ion_V8_1/Green_Ion_V8_1.ino)
- [V8.2](./Past_Versions/Green_Ion_V8_2/Green_Ion_V8_2.ino)
- [V8.3](./Past_Versions/Green_Ion_V8_3/Green_Ion_V8_3.ino)
- [V8.4](./Past_Versions/Green_Ion_V8_4/Green_Ion_V8_4.ino)
- [V8.5](./Past_Versions/Green_Ion_V8_5/Green_Ion_V8_5.ino)
- [V8.6](./Past_Versions/Green_Ion_V8_6/Green_Ion_V8_6.ino)
- [V8.7](./Past_Versions/Green_Ion_V8_7/Green_Ion_V8_7.ino)
- [V8.8](./Past_Versions/Green_Ion_V8_8/Green_Ion_V8_8.ino)
- [V9](./Past_Versions/Green_Ion_V9/Green_Ion_V9.ino)
- [V9.1](./Past_Versions/Green_Ion_V9_1/Green_Ion_V9_1.ino)
- [V9.2](./Past_Versions/Green_Ion_V9_2/Green_Ion_V9_2.ino)
- [V10](./Green_Ion_V10/Green_Ion_V10.ino)

## V8.8 recording-reliability development

V9.2 and all earlier versions are preserved under `Past_Versions`. V10 is the standalone current firmware folder and its GitHub Actions artifact is named `green-ion-v10-phone-update`.

## Usage notes

- Use [V10](./Green_Ion_V10/Green_Ion_V10.ino) for the latest buildable source, subject to successful build and hardware verification.
- Previous snapshots are retained for comparison and rollback; they have not necessarily passed hardware testing.
- Git commit dates beginning October 24, 2026 are repository import dates, not necessarily the original dates of the early prototypes.
- The full feature-by-feature change history is in [CHANGELOG.md](./CHANGELOG.md).
# Current candidate

- **V12 Development Firmware — Hardware Validation In Progress**: `Green_Ion_V12/`
- **V12 Arduino Uno receiver**: `Ground_Receiver/`
- **V12 Windows ground station**: `Ground_Station/`
- **V11 Release Candidate — archived for rollback**: `Past_Versions/Green_Ion_V11/`
- **V10 preserved release**: `Past_Versions/Green_Ion_V10/`

