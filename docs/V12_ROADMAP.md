# GREEN ION V12 roadmap

## Phase 1 — foundation

- [x] Preserve V11 under `Past_Versions/Green_Ion_V11` and keep V12 in its root-level project directories.
- [x] Centralize preliminary pins and runtime constants.
- [x] Create shared telemetry protocol and host tests.
- [x] Create initial ESP32 integration sketch, Uno receiver, simulated local ground station, and required documents.
- [x] Create the recurring hourly Codex development automation with overlap and low-credit wrap-up guards.
- [x] Obtain clean local target compilations for ESP32 and Uno and capture exact tool/library versions.
- [x] Push the isolated V12 work and verify the new CI workflow and downloadable artifacts.

## Phase 2 — complete pre-hardware software

- [x] Add independent serial bench diagnostics for LSM6DSO32 and PA1010D, including explicit units, live/stale GPS semantics, and first-day procedures.
- [x] Add phone/laptop hardware-test controls with stationary IMU bias classification, GPS communication-versus-fix classification, STEMMA QT rail power, reconnection handling, and recording lockout.
- [x] Add a first-packet LoRa bench mode and verify the shared Feather/Uno modem and packet contract before sensor integration.
- [x] Add a bounded telemetry queue so radio work is deferred behind recorder/flush operations and guarded from the next sample deadline.
- [x] Add bounded radio degradation/recovery policy that never performs blocking reinitialization during recorder-critical states.
- [x] Define a separate CRC-protected full-resolution IMU/GPS binary format synchronized to the immutable RKT4 v2 flight.
- [x] Integrate the IMU/GPS companion log with a bounded recorder-priority SD queue and failure isolation from RKT4.
- [x] Add read-only boot recovery inspection for companion header binding, complete-record CRCs, corrupt records, and torn tails.
- [x] Extend alternating checkpoints with backward-compatible, successfully flushed companion byte/count evidence.
- [x] Add deterministic offline GIS1 validation and CSV export without changing RKT4 v2.

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
- [x] Complete the V11 recorder-core port: autonomous flight watcher, prebuffer, checkpoints, recovery, calibration, metadata, and rate control.
- [ ] Add deterministic module interfaces and mocks for barometer, IMU, GPS, storage, radio, battery, and time.
  - [x] Add a portable barometer health/retry policy with initialization failure, transient-read reset, sustained runtime-loss, rollover, backoff, and recorder-critical deferral tests.
  - [ ] Extend deterministic health/recovery boundaries to IMU, GPS, storage, battery, and time.
- [x] Add a coordinated recorder failure harness for missing optional sensors, mid-run GPS loss, secondary failure isolation, and fatal primary SD write/flush loss.
- [ ] Add failure-injection integration tests for init loss, mid-run GPS/radio loss, SD write failure, and recovery.
- [x] Unify the offline Ground Station around Mission Control, Recovery, Flights, Diagnostics and Settings; merge simultaneous Wi-Fi/LoRa sources, reject duplicate/out-of-order packets, count gaps, retain last valid GPS, persist raw sessions incrementally, and provide explicit downloads.
- [ ] Add validated Feather SD download/import with progress, checksum-based session matching, local RKT4/GIS1 replay, comparisons, printable reports, and offline tile packages.
- [ ] Port the single unified Flight dashboard, home-test measurements, recovery, and OTA/rollback behavior.
  - [x] Restore the actual V11 blue tabbed visual foundation, mobile layouts, live graphs, RKT4 browser analysis, CSV export, printable reports, SD file listing/downloads, and offline dashboard shell in V12.
  - [x] Integrate live BMP388, LSM6DSO32, PA1010D, radio, battery, SD, recorder/recovery health, and nonblocking hardware-test results into that interface.
  - [x] Add recorder-safe V12 calibration, stop/finalize, temporary SD verification, file listing, BIN download, and metadata download endpoints; all SD browsing remains locked out while recording.
  - [ ] Port V11 browser OTA/rollback, configuration backup/restore, editable mission/sample-rate settings, RGB controls, and deep-sleep controls. Until real backends exist, the restored interface labels and disables these controls.

## Phase 3 — simulated integration and release candidate

- [x] Add a temporary separate airplane recorder with persistent transmitter lockout, USB controls, bounded duration, AIRP naming, recovery support, and non-rocket analysis semantics.

- Run full V11 regression suite plus V12 host tests and both target compilers.
- Replay nominal, no-GPS, no-radio, degraded-sensor, and SD-fault missions.
- Document resource usage, loop timing, maximum SD flush latency, packet airtime, and queue depth.
- Produce a matching compiled binary only from the exact committed source and successful CI run.

## Phase 4 — hardware arrival

Follow `V12_HARDWARE_BRINGUP.md`, record measured evidence, and keep every unperformed hardware test marked `NOT TESTED`.

