# S09 — Local flight history and motor trends
**Status:** Approved; not implemented. **Dependencies:** S02/S03/S05.

## Implementation
1. Implement local, user-controlled flight library in browser (IndexedDB where available) containing original BIN, optional TXT metadata, parsed indexes, checksum and analysis algorithm version.
2. Key records by content checksum plus explicit flight ID; handle duplicates and renamed files without accidental merging; enable delete/export/backup.
3. Make search/filter by motor designation, flight date (if known), sample rate, quality flag, maximum altitude and flight duration. Unknown date must remain unknown.
4. Add cross-flight comparisons for altitude, ascent/descent rates, duration and power observations; display differing environment, rocket configuration and data quality.
5. Recompute results when analyzer version changes; always retain original raw files to make this possible.
6. Avoid server/cloud requirements. Do not assume IndexedDB persists forever; provide explicit backup and import.

## Acceptance
Import 10+ synthetic flights, correctly deduplicate, filter, compare, export/reimport and remove records offline; no overwriting original raw data; graceful storage quota failure.

## Shared constraints
- GREEN ION logs data only: **never** control ignition, staging, parachutes or recovery deployment.
- Recording and power-loss recovery must remain independent of Wi-Fi or a connected browser.
- Preserve RKT4 header/16-byte record layout, timestamp meaning, calibration metadata, OTA rollback and original BIN files.
- Label pressure-derived acceleration, G and phase events as estimates, not direct measurements.
- Compilation/simulation does not establish hardware or flight readiness.
