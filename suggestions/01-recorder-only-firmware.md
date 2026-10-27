# S01 — Recorder-only Feather firmware
**Status:** Approved; not implemented. **Dependency:** S02/S03/S04/S06 verified.

## Keep on Feather
BMP388 sensor discovery/read/validation; monotonic timestamp; 10/25/50 Hz scheduling; ground calibration; raw battery samples and critical-power awareness; SD mount/test; raw RKT4 sample writes; buffering, checkpoints and interrupted-flight recovery; file numbering/metadata; simple thresholds needed to start/stop/timeout autonomously; compact status/config/control/download API; guarded OTA rollback; basic LED status if cheap.

## Move to browser
Presentation filters, detailed altitude/velocity/acceleration/G calculations, full postflight phase classification, graphs, CSV/PDF exports, motor comparisons, flight replay and historical trends. Keep only the minimal onboard detection required to safely manage the logger itself.

## Exact implementation steps
1. In `Green_Ion_V10/Green_Ion_V10.ino` identify code paths for sampling/recording, SD recovery, API routes, derived metrics and postflight export. Make a one-page keep/move mapping **once**; no mass rewrite.
2. Protect log API, binary format and firmware update endpoints with S06 regression tests.
3. Substitute compact recorder status responses for expensive calculations **only once browser replacements work**. Retain compatibility aliases during migration.
4. Ensure HTTP handling, downloads, status polls and Wi-Fi loss cannot block or terminate recording. Capture worst sample gap/jitter under load.
5. Remove obsolete computations incrementally; do not remove health checks, calibrate, recording timeout, preflight checks, recovery or metadata needed to interpret BIN files.

## Acceptance
All original BIN fixtures decode; full flight recording works with phone absent; SD/power recovery and OTA guards remain functional; baseline vs new flash/RAM/timing measured; failed SD operations visibly reported; no regression in preflight and manual recorder controls.

## Shared constraints
- GREEN ION logs data only: **never** control ignition, staging, parachutes or recovery deployment.
- Recording and power-loss recovery must remain independent of Wi-Fi or a connected browser.
- Preserve RKT4 header/16-byte record layout, timestamp meaning, calibration metadata, OTA rollback and original BIN files.
- Label pressure-derived acceleration, G and phase events as estimates, not direct measurements.
- Compilation/simulation does not establish hardware or flight readiness.
