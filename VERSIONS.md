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
