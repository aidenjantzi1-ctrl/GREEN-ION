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

