# S06 — Reliability and test gates
**Status:** Approved; not implemented. **Priority: start first and run at every stage.**

## Automated tests
1. Preserve current `tests/` tests and `.github/workflows/build-firmware.yml`; note that source-string checks are not behavioral proof.
2. Add executable pure-function tests for RKT4 parsing, timestamp/units, calibration/altitude math, smoothing, flight phases and export data calculations.
3. Build fixtures for normal complete log, interrupted log with valid header, partial trailing sample, bad header, empty file, clock gap and bad sensor readings.
4. Add simulated state-machine tests for arm/start/stop, timeout, recovery bookkeeping, new file naming, connection loss and SD write failure wherever logic can be separated from hardware.
5. CI should run parser/calculation tests, existing Python suite, JS syntax/ID checks and actual Feather V2 Arduino compile with pinned versions when practical.

## Hardware checklist (manual; never auto-claim pass)
- Confirm BMP388/390 initialization and calibration stationary; check battery voltage plausibility.
- Run 10, 25, 50 Hz ground captures and quantify timestamps, missed intervals, long gaps, card flush latency.
- Disconnect Wi-Fi and run phone downloads while stationary logging; confirm timing and completed file integrity.
- Remove/fault SD and fill card **only using disposable test media**; observe behavior and recovery.
- Interrupt power during throwaway recording at several timings; confirm journal recovery, preserve old files.
- Reboot, OTA failure/rollback, low-battery and watchdog scenarios under controlled bench conditions.

## Acceptance
Report exact tested firmware/board/library versions, fixture hashes, pass/fail and **NOT TESTED** physical scenarios. Do not declare flight-ready from CI alone.

## Shared constraints
- GREEN ION logs data only: **never** control ignition, staging, parachutes or recovery deployment.
- Recording and power-loss recovery must remain independent of Wi-Fi or a connected browser.
- Preserve RKT4 header/16-byte record layout, timestamp meaning, calibration metadata, OTA rollback and original BIN files.
- Label pressure-derived acceleration, G and phase events as estimates, not direct measurements.
- Compilation/simulation does not establish hardware or flight readiness.
