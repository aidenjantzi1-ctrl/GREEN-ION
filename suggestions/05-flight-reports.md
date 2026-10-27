# S05 — Browser-based advanced reports
**Status:** Approved; not implemented. **Dependencies:** S02/S03.

## Deliverables
- Interactive charts: altitude-time, raw/filtered pressure, temperature, battery voltage, vertical velocity, barometric-estimated acceleration/G, sample interval and quality, with understandable units.
- Flight timeline/replay with estimated launch/apogee/landing and optional probable phase boundaries; tooltips show uncertainty and gaps.
- Flight summary: peak altitude, duration, maximum inferred climb/descent rates, battery sag, recording completeness and quality warnings.
- Compare flights by motor label and date, preserving different conditions and sample rates; never claim direct thrust or motor burn measurement without sensors.
- CSV, readable print-to-PDF report, optionally packaged archive with original BIN + metadata + analysis algorithm version.

## Implementation
Use S02 analysis result as sole numeric source. Build visual/export modules in browser, not firmware. Use print CSS for PDF; verify mobile Safari and desktop print-to-PDF. Label calculated versus measured data, note units/baseline and assumptions. Do not block normal recorder UI with rendering.

## Acceptance
All exports generated offline from fixture files; unit conversions and phase labels consistent across charts/CSV/PDF; comparisons reproducible; malformed logs produce explicit warning rather than fabricated numbers.

## Shared constraints
- GREEN ION logs data only: **never** control ignition, staging, parachutes or recovery deployment.
- Recording and power-loss recovery must remain independent of Wi-Fi or a connected browser.
- Preserve RKT4 header/16-byte record layout, timestamp meaning, calibration metadata, OTA rollback and original BIN files.
- Label pressure-derived acceleration, G and phase events as estimates, not direct measurements.
- Compilation/simulation does not establish hardware or flight readiness.
