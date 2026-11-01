# GREEN ION V12 roadmap

## Phase 1 — foundation

- [x] Preserve V11 in place and create isolated V12 directories.
- [x] Centralize preliminary pins and runtime constants.
- [x] Create shared telemetry protocol and host tests.
- [x] Create initial ESP32 integration sketch, Uno receiver, simulated local ground station, and required documents.
- [x] Create the recurring three-hour Codex development automation.
- [x] Obtain clean local target compilations for ESP32 and Uno and capture exact tool/library versions.
- [ ] Push the isolated V12 work and verify the new CI workflow and downloadable artifacts.

## Phase 2 — complete pre-hardware software

1. Port V11's proven recorder core and RKT4 v2 format into modular V12 code without changing V11.
2. Add deterministic module interfaces and mocks for barometer, IMU, GPS, storage, radio, battery, and time.
3. Add failure-injection integration tests for init loss, mid-run GPS/radio loss, SD write failure, and recovery.
4. Add a bounded telemetry queue so radio work can never delay the recorder.
5. Expand the ground station with sequence-gap statistics, session metadata, explicit download, and replay.
6. Port the single unified Flight dashboard, home-test measurements, recovery, and OTA/rollback behavior.

## Phase 3 — simulated integration and release candidate

- Run full V11 regression suite plus V12 host tests and both target compilers.
- Replay nominal, no-GPS, no-radio, degraded-sensor, and SD-fault missions.
- Document resource usage, loop timing, maximum SD flush latency, packet airtime, and queue depth.
- Produce a matching compiled binary only from the exact committed source and successful CI run.

## Phase 4 — hardware arrival

Follow `V12_HARDWARE_BRINGUP.md`, record measured evidence, and keep every unperformed hardware test marked `NOT TESTED`.

