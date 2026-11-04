# GREEN ION V12 roadmap

## Phase 1 — foundation

- [x] Preserve V11 in place and create isolated V12 directories.
- [x] Centralize preliminary pins and runtime constants.
- [x] Create shared telemetry protocol and host tests.
- [x] Create initial ESP32 integration sketch, Uno receiver, simulated local ground station, and required documents.
- [x] Create the recurring hourly Codex development automation with overlap and low-credit wrap-up guards.
- [x] Obtain clean local target compilations for ESP32 and Uno and capture exact tool/library versions.
- [x] Push the isolated V12 work and verify the new CI workflow and downloadable artifacts.

## Phase 2 — complete pre-hardware software

- [x] Add independent serial bench diagnostics for LSM6DSO32 and PA1010D, including explicit units, live/stale GPS semantics, and first-day procedures.
- [x] Add a first-packet LoRa bench mode and verify the shared Feather/Uno modem and packet contract before sensor integration.
- [x] Add a bounded telemetry queue so radio work is deferred behind recorder/flush operations and guarded from the next sample deadline.
- [x] Add bounded radio degradation/recovery policy that never performs blocking reinitialization during recorder-critical states.
- [x] Define a separate CRC-protected full-resolution IMU/GPS binary format synchronized to the immutable RKT4 v2 flight.
- [x] Integrate the IMU/GPS companion log with a bounded recorder-priority SD queue and failure isolation from RKT4.
- [ ] Extend companion-log checkpoints/recovery and offline export without changing RKT4 v2.

- [x] Extract V11's immutable RKT4 v2 format into a portable module and write collision-safe raw V12 SD logs.
- [x] Put both radios on one provisional 920.5 MHz modem contract and expand telemetry v2 with pressure, both altitude sources, acceleration axes/magnitude, and rotation axes.
- [x] Add a deterministic byte-sink boundary and fixed-capacity recorder prebuffer with host failure tests.
- [x] Add a portable V11-derived launch watcher with confirmation, noise, invalid-sample, gap, and rollover tests.
- [x] Add fixed-memory V11-compatible stationary pressure calibration with noise, drift, range, and outlier tests.
- [x] Integrate automatic calibration and launch watching so idle samples remain buffered and confirmed launch opens the RKT4 log.
- [x] Define a portable CRC-protected alternating checkpoint and torn-tail inspection contract with host failure tests.
- [x] Write alternating V12 checkpoints only after successful RKT4 flushes, with failure throttling that preserves BIN priority.
- [x] Report interrupted V12 logs at boot from validated journals while preserving the original BIN and torn tail.
- [x] Add deterministic recovery-report and journal-cleanup failure injection with evidence-preserving operation order.
- [x] Add a rollover-safe fixed-rate sample clock that avoids schedule drift and reports missed acquisition periods.
- [x] Write bounded per-flight metadata tied to the RKT4 filename, calibration, sample rate, battery, prebuffer, and checkpoint format.
- [x] Add a portable pressure-only landing watcher for recorder finalization with stability, gap, invalid-sample, and rollover tests.
- [x] Integrate automatic landing finalization with BIN stop/flush, completion metadata, and recovery-journal cleanup ordered last.
- [ ] Complete the V11 recorder-core port: autonomous flight watcher, prebuffer, checkpoints, recovery, calibration, metadata, and rate control.
- [ ] Add deterministic module interfaces and mocks for barometer, IMU, GPS, storage, radio, battery, and time.
- [ ] Add failure-injection integration tests for init loss, mid-run GPS/radio loss, SD write failure, and recovery.
- [ ] Expand the ground station with sequence-gap statistics, session metadata, explicit download, and replay.
- [ ] Port the single unified Flight dashboard, home-test measurements, recovery, and OTA/rollback behavior.

## Phase 3 — simulated integration and release candidate

- Run full V11 regression suite plus V12 host tests and both target compilers.
- Replay nominal, no-GPS, no-radio, degraded-sensor, and SD-fault missions.
- Document resource usage, loop timing, maximum SD flush latency, packet airtime, and queue depth.
- Produce a matching compiled binary only from the exact committed source and successful CI run.

## Phase 4 — hardware arrival

Follow `V12_HARDWARE_BRINGUP.md`, record measured evidence, and keep every unperformed hardware test marked `NOT TESTED`.

