# S07 — CPU, battery and SD performance
**Status:** Approved; not implemented. **Dependencies:** S06 baseline measurement.

## Baseline measurements
Capture compiled flash/RAM usage, free heap, worst/max/p95 inter-sample interval, dropped/invalid samples, SD write duration, request/download latency, Wi-Fi idle vs recording power, and voltage droop with representative card/battery.

## Changes to evaluate one at a time
1. Prioritize sensor read + append into a bounded sample buffer over request rendering. Ensure no blocking Wi-Fi handling delays sample timestamps; benchmark rather than assuming RTOS will help.
2. Minimize dynamic String/heap churn and work during active recording; defer expensive scans/conversions until post-flight/browser.
3. Consider Wi-Fi power/scheduling policy while recording **only** if preserving arm/stop safety and recording independence; document tradeoffs for live status/control.
4. Tune buffer/flush/checkpoint cadence based on actual latency, power-loss window and card behavior. Never trade durability for a synthetic speed gain.
5. Validate low-voltage behavior and existing adaptive rate logic; if rate changes are retained, accurately capture effective rate and gaps in metadata.

## Acceptance
Before/after benchmark table; no worse sample integrity, recovery or UX; power claims supported by measured current and test conditions rather than speculation.

## Shared constraints
- GREEN ION logs data only: **never** control ignition, staging, parachutes or recovery deployment.
- Recording and power-loss recovery must remain independent of Wi-Fi or a connected browser.
- Preserve RKT4 header/16-byte record layout, timestamp meaning, calibration metadata, OTA rollback and original BIN files.
- Label pressure-derived acceleration, G and phase events as estimates, not direct measurements.
- Compilation/simulation does not establish hardware or flight readiness.
