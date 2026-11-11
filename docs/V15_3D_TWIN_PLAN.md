# GREEN ION future plan — 3D reconstruction + digital twin (not implemented)

## Placement and constraints

These are **future-only** modules running in a Web Worker on a capable Windows computer or in user-authorized cloud compute. The Feather ESP32 V2 does **not** run 3D analysis, aerodynamic simulation or visualization. The current RKT4 and GIS1 originals remain immutable and must be preserved.

## 3D flight reconstruction

1. Import original RKT4 BIN + GIS1 SEN, verify CRC32 and time continuity; preserve the original timestamps.
2. Align pressure altitude, GPS position/height/speed and IMU axes using GPS UTC and device monotonic time; use explicit time-offset uncertainty, not naive timestamp equality.
3. Establish reference frame (launch location, ENU coordinates and gravity) from measured calibration and manually specified rocket mounting orientation.
4. Estimate attitude using calibrated gyro/accelerometer fusion, accounting for high-acceleration segments that make gravity estimation invalid; do **not** silently integrate acceleration into GPS-quality position.
5. Render altitude against ground-relative XY on a computer with a 3D WebGL viewer, phase markers, interactive replay, uncertainty bands and clear GPS-data gaps.
6. Allow an explicit 'GPS observed path only' mode when fusion is not defensible. The last point must never be labeled a guaranteed touchdown.

## Rocket digital twin

1. Define per-rocket model with measured empty/dry mass, mass with motor, center of gravity, dimensions, drag estimate, fin and stability characteristics, motor thrust curve, launch rail guidance and atmospheric assumptions.
2. Import user-supplied manufacturer motor data and weather measurements, clearly identifying their sources and revisions.
3. Run a bounded numerical trajectory model on the computer/server, never on the recorder. Keep simulation data separate from observed flight data.
4. Compare altitude/velocity/apogee and timing to original recordings, quantify prediction errors and parameter sensitivity; export a reproducible JSON scenario.
5. No automated motor ignition, flight commands, remote arm or deployment initiation. The simulator is strictly analysis and planning, not an autopilot.

## Data architecture

- Inputs: original BIN/SEN/TXT, integrity SHA-256, derived analysisVersion, optional rocket-profile/meteorological metadata.
- Outputs: versioned simulated tracks, measured track, model parameters, confidence annotations, comparison graph and human-readable limitations.
- Persistence: IndexedDB and optional private Supabase backups under the existing user's account. Large derived artifacts should not overwrite sensor originals.
- Privacy: require user confirmation before uploading coordinates/motor/rocket-model details to remote analysis.
- Performance: cap sample counts, decimate display data, render in worker/WebGL, cancel long tasks and provide accessible 2D fallback.

## Acceptance tests for a future release

Unit tests with synthetic trajectories, GPS dropouts, stationary bias, time-shift, CRC corruption, motor thrust edge cases, large files, no GPS, mobile fallback and repeatable cross-device results. Validate known reference trajectories before comparing real flights. Do not present generated 3D tracks as directly measured where sensor coverage is inadequate.
