# S02 — Browser analysis engine
**Status:** Approved; not implemented. **Dependencies:** S04 format contract; S06 sample fixtures.

## Implementation
1. Create independent browser-side module(s) (e.g. `web/analysis/rkt4.js`, `web/analysis/flight.js`) only when implementation begins; do not embed duplicate algorithms in both Feather and dashboard.
2. Parse RKT4 with `ArrayBuffer`/`DataView`: header version, explicit byte order, 16-byte sample length, units, timestamps, corrupt/trailing bytes. Derive schema from actual firmware, never guess offsets.
3. Normalize time, pressure, temperature, battery and metadata. Flag malformed readings, gaps, clock discontinuities, and truncated logs; never overwrite original BIN.
4. Implement pure functions for barometric altitude, configurable smoothing, vertical speed, second derivative acceleration, estimated vertical G, maximum altitude, phase estimates and recording quality. Keep units and filter assumptions explicit.
5. Phase estimates: launch, climb, probable coast, apogee, descent, landing. Do not pretend BMP388 directly senses burnout, deployment, motor thrust or true 3D acceleration.
6. Run heavy computation in browser Web Worker; allow cancel, progress, and small-file fallback. Return a versioned analysis result plus warnings.
7. Tests: static pressure, known synthetic ascent/descent, irregular timestamps, spikes, truncated/corrupt recordings; compare expected numerical tolerance and preserve compatibility with V10 files.

## Acceptance
Analysis works from a BIN imported on airplane-mode desktop/mobile; reproducible output, no network or Feather CPU required; derived values consistently marked estimated; UI responsive on representative phone-sized file.

## Shared constraints
- GREEN ION logs data only: **never** control ignition, staging, parachutes or recovery deployment.
- Recording and power-loss recovery must remain independent of Wi-Fi or a connected browser.
- Preserve RKT4 header/16-byte record layout, timestamp meaning, calibration metadata, OTA rollback and original BIN files.
- Label pressure-derived acceleration, G and phase events as estimates, not direct measurements.
- Compilation/simulation does not establish hardware or flight readiness.
