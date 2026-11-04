# GREEN ION V12 development log

## 2026-10-31 — Session 1: isolated foundation and telemetry vertical slice

### Implemented

- Preserved the dirty, unfinished V11 home-test worktree without editing or copying over it.
- Added centralized Feather V2 pin/rate/radio configuration and compile-time chip-select checks.
- Added a shared packed 50-byte telemetry packet with protocol magic/version/type/length, sequence, current-vs-last-known GPS flags, health flags, and CRC-32.
- Added an initial nonblocking ESP32 V12 sketch for BMP388, LSM6DSO32, PA1010D, SD CSV logging, battery monitoring, and 1 Hz RFM95W telemetry. Sensor/radio initialization failures do not halt the loop.
- Added Uno R3 receiver firmware that validates packets, retains last position, counts errors, and emits JSON without ArduinoJson.
- Added a local Python ground station with simulated flight/GPS dropout, live/stale/missing display, offline relative trajectory map, graphs, and automatic CSV logs.
- Added V12 protocol/structure tests and a separate CI workflow for ESP32 and Uno compilation.
- Added build, wiring, test, roadmap, and hardware bring-up documents.
- Enabled the requested recurring local Codex automation at 12/3/6/9 AM and PM Pacific time.

### Evidence

- Official Adafruit documentation checked for Feather V2 buses/STEMMA power/battery monitor, LSM6DSO32 address and rates, PA1010D I2C usage, and RFM95W Uno wiring.
- Host tests: 56 tests passed with the bundled Python 3 runtime, including all existing V11 regressions plus 10 V12 structure/telemetry tests.
- Sample ground station generated CSV telemetry and passed a loopback HTTP smoke test: the JSON endpoint returned simulated live GPS data and the dashboard returned 6,154 bytes.
- ESP32 compile: PASS with Arduino CLI 1.5.1, ESP32 core 3.3.12, BMP3XX 2.1.6, LSM6DS 4.7.4, GPS 1.9.0, RadioHead 1.143.1, BusIO 1.17.4, Unified Sensor 1.1.15, and GreenIonProtocol 12.0.0. Result: 437,754 bytes flash (13%), 30,028 bytes global RAM (9%).
- Arduino Uno compile: PASS with AVR core 1.8.8. Result: 12,010 bytes flash (37%), 664 bytes global RAM (32%).
- GitHub Actions run `41276956130` passed host tests, ESP32 compilation, Uno compilation, and artifact upload for source commit `51fa8e0f03953f6ee9308037a7a8d33cedc87674`.
- Matching artifact `green-ion-v12-development-binaries` has artifact ID `13072085328`, size 610,299 bytes, and digest `sha256:149d859e975eee54ce54c2130ceb58aa550342979c23452b4abde22816b53229`.
- Physical hardware: **NOT TESTED**.

### Next task

Add host-executable C++ golden-vector tests and begin porting the V11 recorder/RKT4 core behind module interfaces. Continue keeping the pre-existing dirty V11 work separate from V12 commits.

## 2026-11-01 — Session 2: portable RKT4 recorder core

### Implemented

- Added `GreenIonRecorder`, a host-compatible module with explicit little-endian RKT4 v2 header/record encoding, header validation, complete-record/torn-tail accounting, and rollover-safe timestamp deltas.
- Replaced the V12 firmware's temporary CSV writer with the immutable V11-compatible raw format: 32-byte header followed by 16-byte BMP388 records.
- Added collision-safe `/V12D000.BIN` through `/V12D999.BIN` allocation so a restart does not overwrite an existing development flight.
- Preserved invalid sensor samples with the RKT4 validity field instead of stopping the recorder.
- Added fixed C++ golden vectors and independent Python checks for byte layout, invalid headers/rates, interrupted records, and `micros()` rollover.

### Evidence

- Host regression tests: 60 passed, including all V11 checks and four new V12 RKT4 tests.
- RKT4 browser analysis suite: passed.
- ESP32 compile: PASS; 437,958 bytes flash (13%) and 30,044 bytes global RAM (9%).
- Uno compile: PASS; 12,010 bytes flash (37%) and 664 bytes global RAM (32%).
- GitHub Actions run `41321455276`: PASS for 60 Python tests, native C++ RKT4 golden vectors, ESP32 compile, Uno compile, and artifact upload on source commit `046f1863014762bb80c5bc05e2753c89c2d84c17`.
- Matching artifact `green-ion-v12-development-binaries`: artifact ID `13089538068`, 610,666 bytes, digest `sha256:64691279d21d6173b928ae7bcf7b7ee094d944393afe3788da900a985b826bbf`.
- Physical SD writes, interrupted power recovery, and sensor behavior: **NOT TESTED**.

### Next task

Add deterministic recorder/storage/time interfaces and a bounded prebuffer so V11 autonomous launch recording and failure injection can be ported without coupling tests to Arduino hardware.

## 2026-11-01 — Session 3: shared radio contract, telemetry v2, and status dashboard

### Implemented

- Replaced independent radio literals with one transmitter/receiver contract: provisional 920.5 MHz, 125 kHz bandwidth, spreading factor 7, coding rate 4/5, and 13 dBm transmit power.
- Expanded the CRC-protected protocol to a fixed 70-byte v2 packet carrying pressure, barometric altitude, GPS altitude, acceleration magnitude and axes, rotation axes, battery, and GPS state.
- Updated the Uno JSON bridge, Python codec, simulator, CSV rows, and ground dashboard to preserve and display the new fields.
- Added the required `Green_Ion_V12/Dashboard.h` read-only recorder status page and `/api/status`; it exposes recording, log filename, sensors, battery, GPS freshness, and health without flight-control actions.
- Renamed the test document to the required `docs/V12_TESTING.md` path and documented that 920.5 MHz is provisional pending ROC coordination and applicable FCC review.
- Changed the existing development automation from every three hours to hourly, with overlap protection and a low-credit wrap-up/commit/push rule; no duplicate automation was created.

### Evidence

- Host regression suite: 62 Python tests passed, including telemetry v2 round trips, CRC/version/size rejection, exact 70-byte layout, shared radio constants, GPS freshness, V11 regressions, and RKT4 checks.
- RKT4 browser analysis suite: passed.
- ESP32 compile: PASS after fixing an Arduino `String` overload found by the first real compile; final result 1,093,250 bytes flash (32%) and 55,224 bytes global RAM (16%).
- Arduino Uno compile: PASS; 14,010 bytes flash (43%) and 704 bytes global RAM (34%).
- Source formatting check: `git diff --check` passed.
- CI and downloadable binary evidence for this source commit: pending push.
- Physical radio, sensor, GPS, SD, power-loss, and dashboard hardware behavior: **NOT TESTED**.

### Next task

Add deterministic recorder/storage/time interfaces and a bounded prebuffer so the autonomous flight watcher and failure-injection tests can be ported without coupling them to Arduino hardware. The read-only V12 status page is not yet the complete V11 unified Flight, recovery, and OTA dashboard port.

## 2026-11-01 — Session 4: one-click ground station launch

### Implemented

- Added `Ground_Station/OPEN_GROUND_STATION.cmd` for one-click Windows startup.
- Made simulation the default when no receiver port is supplied and automatically opens the local website in the default browser.
- Added clear terminal guidance, an already-running recovery path, and an optional `--no-browser` flag for automated checks.
- Documented what the ground station is and separated normal website use from the optional physical Uno/serial setup.

### Evidence

- Local HTTP smoke test: dashboard returned HTTP 200 and 6,559 bytes; `/api/telemetry` returned live simulated pressure and rotation data.
- Simulation is clearly labeled and does not count as radio, receiver, GPS, or flight hardware evidence.
- Physical receiver and field behavior: **NOT TESTED**.

### Next task

Continue the recorder-first roadmap. When receiver hardware is available, validate the optional `--port COMx` path separately from this software-only website launch check.

## 2026-11-01 — Session 5: deterministic recorder boundary and prebuffer

### Implemented

- Added a platform-neutral `ByteSink` interface so recorder logic can be tested independently of Arduino SD hardware.
- Added a fixed-capacity, allocation-free record prebuffer that overwrites only the oldest sample and preserves chronological order across `micros()` rollover.
- Added a deterministic recorder core that drains the prebuffer before live samples and retains the failed record for retry after start, record-write, or flush failures.
- Added host C++ tests with a programmable mock sink for rollover ordering, capacity, retry, write failure, and flush failure; CI now builds and runs both native recorder suites.

### Evidence

- Host regression suite: 63 Python tests passed.
- ESP32 compile: PASS with the new recorder core header parsed by the production toolchain; 1,093,250 bytes flash (32%) and 55,224 bytes global RAM (16%), unchanged because the core is not yet instantiated in firmware.
- A native C++ compiler is not installed in the local Windows environment, so the new executable mock-sink test is **PENDING CI** rather than claimed as a local pass.
- Physical SD writes, injected hardware failures, and power-loss recovery: **NOT TESTED**.

### Next task

After CI validates the portable core, add the Arduino SD adapter and integrate the bounded prebuffer into the firmware without weakening the current recorder-first scheduling guarantees.

### CI follow-up

- GitHub Actions run `41329698963`: PASS for 63 Python tests, both native C++ recorder suites, ESP32 compilation, Uno compilation, and development artifact upload on commit `a7b9e69`.

## 2026-11-01 — Session 6: firmware recorder-core integration

### Implemented

- Added an Arduino `File` byte-sink adapter and routed all V12 RKT4 header, record, and flush writes through the tested recorder core.
- Instantiated a fixed three-second/150-record prebuffer at 50 Hz; no heap allocation is used.
- Preserved the failed sample in RAM after a record-write failure and kept the prebuffer intact when header, prebuffer-drain, or initial flush fails.
- Exposed recorder status and buffered-record count through the read-only onboard status API.
- Updated source-contract tests so the portable recorder module, not duplicated sketch code, remains responsible for RKT4 encoding.

### Evidence

- Host regression suite: 64 Python tests passed.
- ESP32 compile: PASS; 1,094,270 bytes flash (32%) and 57,648 bytes global RAM (17%). The 2,424-byte RAM increase is the instantiated fixed recorder/prebuffer state.
- Native C++ failure tests for the updated initial-flush behavior are **PENDING CI** because no native compiler is installed locally.
- Physical SD writes, SD removal, brownout, and power-loss recovery: **NOT TESTED**.

### Next task

Add deterministic sensor/time inputs and the autonomous flight watcher so the prebuffer is retained during idle and drained only when a tested recording trigger starts a flight log.

### CI follow-up

- GitHub Actions run `41335017774`: PASS for 64 Python tests, both native C++ recorder suites including initial-flush retention, ESP32 compilation, Uno compilation, and artifact upload on commit `120b512`.

## 2026-11-01 — Session 7: portable launch watcher

### Implemented

- Added a platform-neutral launch watcher derived from the V11 recorder thresholds: altitude gain must exceed the larger of 1.5 m or five times calibrated noise, vertical speed must reach 4 m/s, and pressure must be below the baseline.
- Requires consecutive confirmation samples scaled to the configured sample rate, rejects invalid/non-finite data, resets after timing gaps, and handles `millis()` rollover with unsigned time deltas.
- Keeps the watcher strictly limited to deciding when recording should begin; it has no ignition, staging, arming, or deployment outputs.
- Added native host scenarios for stationary noise, a single false spike, nominal launch, invalid-sample reset, sample gaps, cancellation, and timestamp rollover.

### Evidence

- Host regression suite: 64 Python tests passed.
- ESP32 compile: PASS with the new watcher parsed by the production toolchain; 1,094,270 bytes flash (32%) and 57,648 bytes global RAM (17%), unchanged because firmware integration is intentionally deferred until CI validates the deterministic scenarios.
- Native C++ launch-watcher execution is **PENDING CI** because no native compiler is installed locally.
- Physical launch detection and sensor accuracy: **NOT TESTED**.

### Next task

After CI validates the watcher, integrate it behind explicit recorder preparation/calibration state so idle samples remain buffered and a confirmed launch drains the prebuffer into a new collision-safe RKT4 log.

### CI follow-up

- GitHub Actions run `41339399641`: PASS for 64 Python tests, all three native C++ recorder/watcher suites, ESP32 compilation, Uno compilation, and artifact upload on commit `882a6f4`.

## 2026-11-01 — Session 8: fixed-memory stationary calibration

### Implemented

- Added a platform-neutral, fixed-memory stationary pressure calibration analysis derived from the V11 recorder.
- Validates sample count, collection duration, sensor range, trimmed baseline, median, pressure/altitude noise, and chronological pressure drift without heap allocation.
- Added native scenarios for a stable dataset with isolated outliers, insufficient data, invalid pressure, excessive noise, excessive drift, and altitude conversion boundaries.

### Evidence

- Host regression suite: 64 Python tests passed and `git diff --check` passed.
- Native C++ calibration execution is **PENDING CI** because no native compiler is installed locally.
- The planned ESP32 compile check could not run because the Codex approval service reported the account usage limit; it is explicitly **NOT RUN**, not a compile pass or failure. CI remains configured to compile both boards after push.
- Physical stationary calibration, pressure noise, and drift behavior: **NOT TESTED**.

### Next task

After CI validates calibration and both board builds, integrate calibration and the launch watcher into an explicit recorder-only preparation state. Do not begin additional work while usage is constrained.

### CI follow-up

- GitHub Actions run `41349566720`: PASS for 64 Python tests, all four native C++ recorder/watcher/calibration suites, ESP32 compilation, Uno compilation, and development artifact upload on commit `8213c23`.

## 2026-11-01 — Session 9: autonomous launch-recording integration

### Implemented

- Added explicit calibrating, watching, recording, and fault modes to the ESP32 recorder.
- The recorder now collects a fixed 96-sample stationary pressure calibration, rejects invalid/noisy/drifting calibration data, and retries calibration after a bounded delay.
- Idle sensor records remain in the fixed three-second prebuffer. A confirmed recorder-only launch trigger opens a collision-safe RKT4 log and drains the prebuffer before live records.
- Calibration retries cancel the prior watcher and clear stale pre-calibration records. The read-only status API exposes recorder mode, baseline pressure, calibration noise, and relative altitude.
- Added portable recorder-core coverage for clearing a prebuffer before recording and rejecting a reset while a recording is active.

### Evidence

- Host source-contract regression suite: 64 Python tests passed.
- `git diff --check`: PASS.
- ESP32 compilation was attempted twice. Both attempts stopped before source compilation because Windows could not start `xtensa-esp32-elf-g++` and reported `{Illegal System DLL Relocation}`. This is recorded as an environment/toolchain failure, not a source compile result; the real ESP32 build is **PENDING CI**.
- Native C++ execution for the new reset-buffer scenario is **PENDING CI** because no native compiler is installed locally.
- Arduino Uno firmware was unchanged in this session; its regression build is **PENDING CI**.
- Physical stationary calibration, launch detection, SD writes, SD removal, brownout, and power-loss recovery: **NOT TESTED**.

### Next task

After CI validates this integration, continue the recorder-core port with deterministic checkpoints and recovery. Keep local data integrity ahead of telemetry and dashboard expansion.

### CI follow-up

- GitHub Actions run `41355060130`: PASS for 64 Python tests, all native C++ recorder/watcher/calibration suites, ESP32 compilation, Uno compilation, and development artifact upload on commit `50ca95b`.

## 2026-11-01 — Session 10: deterministic checkpoint contract

### Implemented

- Added a portable fixed 32-byte `GIC1` checkpoint format with version, generation, committed byte count, complete-record count, elapsed time, and CRC-32.
- Checkpoints reject misaligned committed lengths, inconsistent record counts, CRC corruption, unsupported headers, and claims beyond the actual RKT4 file size.
- Added deterministic alternating-slot selection that falls back from a corrupt or impossible newer slot and compares generation counters across unsigned rollover.
- Added RKT4 recovery inspection that reports validated headers, complete records, and a torn trailing-byte count without modifying the original BIN.
- Added native scenarios for the independent golden CRC, round trip, corrupt-slot fallback, file bounds, invalid fields, generation rollover, clean logs, torn logs, and invalid RKT4 headers.

### Evidence

- Host source-contract regression suite: 64 Python tests passed.
- `git diff --check`: PASS.
- Native C++ checkpoint execution is **PENDING CI** because no native compiler is installed locally.
- The ESP32 regression compile was attempted, but Windows again stopped before source compilation because `xtensa-esp32-elf-g++` could not start with `{Illegal System DLL Relocation}`. This is an environment/toolchain failure, not a source compile result; ESP32 and Uno builds are **PENDING CI**.
- No SD journal is written or consumed by firmware yet. Physical power-loss, SD-removal, and torn-write recovery remain **NOT TESTED**.

### Next task

Integrate alternating checkpoint writes only after a successful RKT4 flush, then add a boot-time scanner that reports interrupted logs while preserving the original BIN and any torn tail.

### CI follow-up

- GitHub Actions run `41360490347`: PASS for 64 Python tests, all five native C++ suites including checkpoint/recovery golden vectors, ESP32 compilation, Uno compilation, and development artifact upload on commit `de5dd1d`.

## 2026-11-01 — Session 11: firmware checkpoint writes

### Implemented

- Included the portable checkpoint contract in the ESP32 recorder and derived collision-matched `.JRA` and `.JRB` names from each new RKT4 BIN.
- Writes an initial checkpoint after `RecorderCore::start` successfully flushes the header and prebuffer, then alternates slots every five seconds only after the primary RKT4 flush succeeds.
- Records only the flushed file size, aligned complete-record count, generation, and recording-relative elapsed time.
- Refuses to append over a journal when removal fails, preserving the other slot as the last known valid checkpoint.
- Checkpoint failures do not stop the primary BIN recorder and are throttled to avoid retrying on every 500 ms log flush. Generation and failure counts are exposed through the read-only status API.

### Evidence

- Host source-contract regression suite: 64 Python tests passed.
- `git diff --check`: PASS.
- The ESP32 compile was attempted, but Windows stopped before source compilation because `xtensa-esp32-elf-g++` could not start with `{Illegal System DLL Relocation}`. This is an environment/toolchain failure, not a source compile result; ESP32 and Uno regression builds are **PENDING CI**.
- Native checkpoint tests were unchanged and previously passed in CI; the firmware integration itself is **PENDING CI** compilation.
- No boot-time journal scanner exists yet. Physical checkpoint durability, power-loss recovery, SD removal, and torn writes remain **NOT TESTED**.

### Next task

Add a boot-time scanner that validates both journal slots against the actual RKT4 file, reports interrupted recordings and torn tails, and preserves the original BIN bytes unchanged.

### CI follow-up

- GitHub Actions run `41365003435`: PASS for 64 Python tests, all five native C++ suites, ESP32 compilation, Uno compilation, and development artifact upload on commit `8466de2`.

## 2026-11-01 — Session 12: boot-time interrupted-log reporting

### Implemented

- Added a bounded boot scan for `V12D000` through `V12D999` journal pairs. A fixed 250-byte bitmap records directory discoveries before any SD mutation, avoiding removal while a FAT directory iterator is open.
- Validates the RKT4 header and selects the newest CRC-valid checkpoint whose committed byte count does not exceed the actual BIN size.
- Writes a separate `.RCV` report containing checkpoint generation, committed and actual bytes, complete records, and trailing partial bytes. The original BIN is opened read-only and is never truncated or rewritten.
- Removes journals only after the recovery report is fully written and flushed. Missing BIN files, invalid headers, invalid checkpoints, report failures, and journal-removal failures remain pending for later inspection.
- Exposes recovered log/record/torn-byte counts and pending status through the read-only API and dashboard.

### Evidence

- Host source-contract regression suite: 64 Python tests passed.
- `git diff --check`: PASS.
- The ESP32 compile was attempted, but Windows stopped before source compilation because `xtensa-esp32-elf-g++` could not start with `{Illegal System DLL Relocation}`. This is an environment/toolchain failure, not a source compile result; ESP32 and Uno regression builds are **PENDING CI**.
- Native checkpoint selection and torn-tail scenarios previously passed in CI; Arduino SD scanning/report integration is **PENDING CI** compilation.
- Physical power interruption, recovery-report durability, SD removal, and torn writes remain **NOT TESTED**.

### Next task

Add deterministic storage failure injection around recovery-report creation and journal cleanup, then continue recorder metadata and rate-control work without weakening BIN preservation.

### CI follow-up

- GitHub Actions run `41370174679`: PASS for 64 Python tests, all five native C++ suites, ESP32 compilation, Uno compilation, and development artifact upload on commit `38d90e4`.

## 2026-11-01 — Session 13: recovery transaction failure injection

### Implemented

- Added a portable `RecoveryCommitter` boundary used by the ESP32 SD recovery path and native host tests.
- Recovery now follows a tested transaction order: write and flush the report first, remove slot A second when present, then remove slot B. Cleanup stops at the first failure so remaining journal evidence stays boot-visible.
- Added programmable mock scenarios for no journals, report-write failure, slot-A removal failure, slot-B removal failure, and complete success.
- Verified that report failure performs no cleanup and slot-A failure does not attempt slot-B removal.

### Evidence

- Host source-contract regression suite: 64 Python tests passed.
- `git diff --check`: PASS.
- Native recovery transaction execution is **PENDING CI** because no native compiler is installed locally.
- The ESP32 compile was attempted, but Windows stopped before source compilation because `xtensa-esp32-elf-g++` could not start with `{Illegal System DLL Relocation}`. This is an environment/toolchain failure, not a source compile result; ESP32 and Uno regression builds are **PENDING CI**.
- Physical report writes, journal removal failures, power interruption, and SD removal remain **NOT TESTED**.

### Next task

Continue the recorder-core port with deterministic per-flight metadata and rate-control state, then expand failure injection to sensor initialization and mid-run peripheral loss.

### CI follow-up

- GitHub Actions run `41375775300`: PASS for 64 Python tests, all five native C++ suites including recovery transaction failures, ESP32 compilation, Uno compilation, and development artifact upload on commit `00e4827`.

## 2026-11-01 — Session 14: deterministic sample-rate clock

### Implemented

- Added a portable fixed-rate `SampleClock` that advances from the prior deadline instead of the current loop time, preventing cumulative schedule drift.
- Handles unsigned `micros()` rollover, rejects zero and half-range intervals, advances over multiple overdue periods, and saturates the cumulative missed-period counter.
- Starts the clock only after setup completes so initialization time is not counted as missed acquisition, and services sensor recording before GPS, telemetry, and dashboard work.
- Exposes cumulative missed sample periods through the read-only status API and dashboard.
- Added native scenarios for early calls, exact deadlines, slight lateness without drift, multiple missed periods, invalid configuration, and timestamp rollover.

### Evidence

- Host source-contract regression suite: 64 Python tests passed.
- `git diff --check`: PASS.
- Native sample-clock execution is **PENDING CI** because no native compiler is installed locally.
- The ESP32 compile was attempted, but Windows stopped before source compilation because `xtensa-esp32-elf-g++` could not start with `{Illegal System DLL Relocation}`. This is an environment/toolchain failure, not a source compile result; ESP32 and Uno regression builds are **PENDING CI**.
- Physical 50 Hz timing, sustained SD latency, missed-period accuracy, and `micros()` rollover remain **NOT TESTED**.

### Next task

Add deterministic per-flight metadata tied to the RKT4 filename, calibration, sample rate, battery, checkpoint format, and interrupted-recovery status.

### CI follow-up

- GitHub Actions run `41380948358`: PASS for 64 Python tests, all six native C++ suites including sample-clock timing, ESP32 compilation, Uno compilation, and development artifact upload on commit `7abc707`.

## 2026-11-01 — Session 15: deterministic flight-start metadata

### Implemented

- Added a portable fixed-format metadata encoder with a bounded caller-provided buffer and no heap allocation.
- Metadata validates the collision-safe `V12D000.BIN` through `V12D999.BIN` filename contract, allowed RKT4 sample rates, finite calibration values, battery, prebuffer size, and output capacity.
- Writes a matching `.TXT` sidecar after the primary RKT4 header/prebuffer flush and initial recovery checkpoint. Metadata failure is counted and exposed but never stops the primary BIN recorder.
- The sidecar records status, filename, RKT4 version, sample rate, baseline pressure, calibration noise, starting battery, prebuffer records, and `GIC1` checkpoint format.
- Added a native golden text vector plus short-buffer, invalid-filename, invalid-rate, and invalid-noise rejection scenarios. Metadata filename and failures are visible through the read-only API and dashboard.

### Evidence

- Host source-contract regression suite: 64 Python tests passed.
- `git diff --check`: PASS.
- Native metadata execution is **PENDING CI** because no native compiler is installed locally.
- The ESP32 compile was attempted, but Windows stopped before source compilation because `xtensa-esp32-elf-g++` could not start with `{Illegal System DLL Relocation}`. This is an environment/toolchain failure, not a source compile result; ESP32 and Uno regression builds are **PENDING CI**.
- Physical sidecar writes, calibration values, battery accuracy, and power-loss behavior remain **NOT TESTED**.

### Next task

Add recorder finalization/landing state so completed flights flush the BIN, write final metadata, and remove journals only after completion evidence is durable; keep this separate from any deployment control.

### CI follow-up

- GitHub Actions run `41386230250`: PASS for 64 Python tests, all seven native C++ suites including metadata golden/error scenarios, ESP32 compilation, Uno compilation, and development artifact upload on commit `10830fe`.

## 2026-11-01 — Session 16: portable landing watcher

### Implemented

- Added a portable pressure-only landing watcher derived from the V11 recorder criteria. It is strictly a recording-finalization aid and has no ignition, staging, arming, or recovery-deployment outputs.
- Requires at least 12 m peak gain, at least 8 m descent from peak, more than 15 seconds elapsed, bounded vertical speed, and continuous altitude stability within a 2 m anchor window.
- Adapts the landing speed limit and required stable duration to calibrated altitude noise, with bounded limits of 0.9–2.0 m/s and 14–26 seconds.
- Resets stability on invalid/non-finite samples, excessive altitude movement, or sample gaps, while preserving the observed peak needed to prove a real ascent/descent occurred.
- Added native scenarios for stationary false positives, nominal landing, invalid-sample reset, anchor reset, timing-gap reset, invalid configuration, and `millis()` rollover.

### Evidence

- Host source-contract regression suite: 64 Python tests passed.
- `git diff --check`: PASS.
- Native landing-watcher execution is **PENDING CI** because no native compiler is installed locally.
- The unchanged ESP32 regression compile was attempted, but Windows stopped before source compilation because `xtensa-esp32-elf-g++` could not start with `{Illegal System DLL Relocation}`. This is an environment/toolchain failure, not a source compile result; ESP32 and Uno regression builds are **PENDING CI**.
- Physical landing detection, altitude noise, descent behavior, and automatic recorder finalization remain **NOT TESTED**.

### Next task

Integrate the landing watcher into the recording state, then finalize only after a successful BIN flush and durable completion metadata; remove recovery journals last and never use landing state for deployment control.

### CI follow-up

- GitHub Actions run `41391529741`: PASS for 64 Python tests, all eight native C++ suites including landing false-positive/reset/rollover scenarios, ESP32 compilation, Uno compilation, and development artifact upload on commit `d6f9576`.

## 2026-11-01 — Session 17: automatic recorder finalization

### Implemented

- Integrated the recorder-only landing watcher into the ESP32 recording state using the continuously filtered relative altitude and vertical speed paths.
- Added a tested `RecorderCore::stop()` operation that flushes the sink, detaches it, and reports a distinct stopped state; flush failure leaves the core faulted.
- Added bounded completion metadata with automatic-landing reason, duration, final BIN byte/record counts, missed sample periods, ending battery, and peak altitude.
- Finalization order is primary-data first: write the triggering sample, stop/flush the BIN, close it, append and flush completion metadata, remove journal A, then remove journal B.
- Any stop, metadata, or cleanup failure leaves remaining journals as recovery evidence and exposes a finalization failure instead of claiming a completed flight.
- Successful finalization enters the landed telemetry state and stops further recorder writes. Landing remains an estimate used only to close recording, never to control deployment.

### Evidence

- Host source-contract regression suite: 64 Python tests passed.
- `git diff --check`: PASS.
- Updated native recorder-stop and completion-metadata execution is **PENDING CI** because no native compiler is installed locally.
- The ESP32 compile was attempted, but Windows stopped before source compilation because `xtensa-esp32-elf-g++` could not start with `{Illegal System DLL Relocation}`. This is an environment/toolchain failure, not a source compile result; ESP32 and Uno regression builds are **PENDING CI**.
- Physical landing detection, final flush durability, completion metadata, journal cleanup, SD removal, and power-loss behavior remain **NOT TESTED**.

### Next task

Add a deterministic end-to-end recorder mission harness covering calibration, launch, prebuffer drain, landing, successful finalization, metadata failure, and stop-flush failure before marking the recorder-core port complete.

## 2026-11-03 — Session 18: IMU and GPS first-day bench mode

### Implemented

- Synchronized remote branches and confirmed no newer commits on `origin/codex/green-ion-v12`; brought over only the eight obsolete `CODEX_HANDOFF.md`/`suggestions/` removal commits from `main`, leaving its README and V11 changes untouched.
- Added bounded Serial commands (`bench imu`, `bench gps`, `bench all`, `bench status`, and `bench off`) with once-per-second output enabled by default.
- IMU output now names both native SI units and convenient bench units: acceleration in m/s² and g, and angular rate in rad/s and degrees/s. It reports the configured ±32 g range and 104 Hz data rate.
- Validates every LSM6DSO32 event read and all six numeric axes. Invalid runtime reads clear the published sample, increment a failure counter, and latch an IMU-read health bit without stopping the recorder.
- GPS state now tracks the current parsed fix separately from the last valid position. A recent last-known coordinate is no longer marked `LIVE` after a parsed no-fix sentence; bench output reports `NO_FIX`, `LIVE`, or `STALE` with fix state, satellites, age, position, altitude, and UTC.
- Added `docs/V12_FIRST_DAY_TEST.md` with the exact Feather-first, cable-only IMU/GPS, antenna safety, Uno, and first-radio steps.

### Evidence

- Python V11/V12 regression suite: 65 tests passed.
- Browser RKT4 analysis test: PASS.
- `git diff --check`: PASS before documentation updates.
- Arduino Uno R3 production target: local `arduino-cli` compile PASS (exit 0).
- Adafruit Feather ESP32 V2 compile was attempted with the installed production toolchain. The process produced no diagnostic output and did not return an exit code within the bounded wait, so this run does **not** claim a local Feather compile pass; CI compilation is required before completion.
- Physical LSM6DSO32, PA1010D, radio, shared-SPI, sensor-accuracy, and GPS-fix tests remain **NOT TESTED** pending the November 4 hardware.

### Next task

Add a deterministic first-packet LoRa bench mode shared by the Feather and Uno, then implement the bounded transmit queue/shared-SPI scheduling needed to keep telemetry below recorder priority.

### CI follow-up

- GitHub Actions run `41783269086`: PASS for 65 Python tests, all native C++ suites, Adafruit Feather ESP32 V2 compilation, Arduino Uno R3 compilation, and development artifact upload on commit `19e21aa`.

## 2026-11-03 — Session 19: first-packet LoRa bench test

### Implemented

- Added `radio test` and `radio status` commands to the Feather Serial interface. The test sends exactly five marked packets and reports each transmitted sequence number, remaining count, initialization failure, and send failure.
- Reused the existing fixed 70-byte telemetry packet and CRC. A shared reserved-field marker identifies bench packets without creating a second radio format or changing existing telemetry decoding.
- The Uno now identifies marked packets as `radio_bench` JSON while retaining protocol version, sequence, RSSI, telemetry fields, validation, and counters.
- The Uno startup status and Feather `radio status` output expose the same frequency, bandwidth, spreading factor, coding rate, and transmit power so a first-day mismatch is visible before transmission.
- Added an explicit antenna warning and updated the first-day guide with the exact five-packet success evidence.
- Inspected the installed RadioHead implementation: `RH_RF95::send()` waits for a previous transmission and performs channel-activity detection before starting asynchronous TX. The old claim that the call itself was nonblocking was removed; shared-SPI/recorder safety remains incomplete until the bounded queue milestone.

### Evidence

- Python V11/V12 regression suite: 66 tests passed.
- Browser RKT4 analysis test: PASS.
- Arduino Uno R3 production target: local `arduino-cli` compile PASS (exit 0).
- `git diff --check`: PASS before documentation updates.
- The local Adafruit Feather ESP32 V2 compile produced no diagnostics and did not return within the bounded five-minute poll window. This is not recorded as a pass or source failure; CI compilation is required.
- Physical RFM95W initialization, packet reception, RSSI, range, CRC behavior, antenna safety, and shared-SPI timing remain **NOT TESTED**.

### Next task

Move RadioHead work behind a fixed-capacity telemetry queue and a recorder-priority SPI scheduler. Add portable failure tests for full-queue replacement/drop behavior, radio-busy timeouts, send failure, and continued recorder progress.

### CI follow-up

- GitHub Actions run `41784707287`: PASS for 66 Python tests, all native C++ suites, Adafruit Feather ESP32 V2 compilation, Arduino Uno R3 compilation, and development artifact upload on commit `cb75e43`.

## 2026-11-03 — Session 20: recorder-priority telemetry queue

### Implemented

- Added a fixed-capacity three-packet telemetry queue with no dynamic allocation. When full, it replaces the oldest unsent snapshot and saturatingly counts drops so the newest recovery data remains available.
- Split packet creation from radio service. Sensor acquisition, recorder writes, periodic flushes, and checkpoints execute before telemetry dispatch in the loop.
- Added a portable dispatch policy that refuses radio work when the queue is empty, the radio is unavailable or transmitting, storage is active, the sample deadline has passed, or less than 5 ms remains before the next 50 Hz sample.
- Explicitly disabled RadioHead CAD waiting and checks `RHModeTx` before calling `send()`, making its previous-packet wait immediate in the single-threaded dispatch path.
- Transient `send()` failure no longer disables the radio or stops recording. The packet remains queued and retry attempts are limited to once per second; newer data eventually replaces stale packets if the queue stays full.
- Exposed queued, dropped, and send-failure counts through `radio status`, and added first-day success criteria for all three.
- Added a native queue/policy suite covering FIFO order, overflow replacement, drop accounting, every busy/unavailable gate, insufficient deadline margin, past deadlines, and `micros()` rollover. CI now builds and runs it.

### Evidence

- Python V11/V12 regression suite: 67 tests passed.
- Browser RKT4 analysis test: PASS.
- `git diff --check`: PASS before documentation updates.
- Native telemetry-queue execution and both board compilations are **PENDING CI**; no native C++ compiler is installed locally, and the local ESP32 compiler has repeatedly failed to return reliable bounded results.
- Physical SPI transaction exclusion, RFM95W interrupt timing, actual send duration, queue depth under load, SD latency, and uninterrupted 50 Hz recording remain **NOT TESTED**.

### Next task

Add bounded radio reinitialization after startup or runtime failure and deterministic failure-injection tests that prove the recorder continues while radio service degrades and recovers. Then begin the synchronized full-resolution IMU/GPS companion log.

### CI follow-up

- Run `41795815106` passed all host/native tests but caught a const-incorrect RadioHead send-buffer cast before the Uno step. Commit `15fe85c` corrected only that compile error.
- GitHub Actions run `41796060698`: PASS for 67 Python tests, all native C++ suites including telemetry queue/policy failures and rollover, Adafruit Feather ESP32 V2 compilation, Arduino Uno R3 compilation, and development artifact upload.

## 2026-11-04 — Session 21: bounded radio degradation and recovery

### Implemented

- Added a portable radio-recovery state machine with a three-consecutive-send-failure threshold, 10-second retry backoff, rollover-safe deadlines, saturating initialization/recovery counters, and successful-send reset.
- A transient send failure keeps the initialized radio available. Three consecutive failures mark only the radio unavailable, latch a radio-send health bit, retain the bounded telemetry queue, and leave recorder state untouched.
- Blocking RadioHead hardware reinitialization is automatically allowed only after recording is complete or the recorder is already faulted. It is never started during calibration, launch watching, or active recording.
- Added a `radio retry` bench command. It is refused during recording; when explicitly requested during calibration/watching it reinitializes the radio and restarts pressure calibration so delayed samples and the old prebuffer cannot contaminate launch detection.
- `radio status` now reports consecutive send failures, initialization failures, and successful recoveries. The first-day guide explains the safe retry sequence.
- Added native failure-injection coverage for startup initialization loss, early retry rejection, recorder-critical deferral, transient failure recovery, threshold degradation, failed and successful reinitialization, invalid configuration, and `millis()` rollover.

### Evidence

- Python V11/V12 regression suite: 68 tests passed.
- Browser RKT4 analysis test: PASS.
- `git diff --check`: PASS before documentation updates.
- Native radio-recovery execution and both target compilations are **PENDING CI**.
- Physical radio removal/reconnection, recovery timing, RFM95W reset behavior, queue delivery after recovery, shared-SPI operation, and uninterrupted recorder timing remain **NOT TESTED**.

### Next task

Design and implement the synchronized CRC-protected full-resolution IMU/GPS companion log without changing RKT4 v2 or any V11 flight file. Begin with a portable fixed binary format and golden/failure tests before SD integration.

### CI follow-up

- GitHub Actions run `41804444723`: PASS for 68 Python tests, all native C++ suites including radio recovery failure/backoff/rollover scenarios, Adafruit Feather ESP32 V2 compilation, Arduino Uno R3 compilation, and development artifact upload on commit `a20fcc1`.

## 2026-11-04 — Session 22: synchronized full-resolution sensor-log contract

### Implemented

- Added a separate `GIS1` binary contract without changing RKT4 v2. Each companion header records its flight sequence, 50 Hz sample rate, common monotonic start timestamp, and the CRC-32 of the matching RKT4 header.
- Defined a fixed 32-byte header and fixed 68-byte record with explicit little-endian encoding and no struct-layout dependency.
- Each record uses the same `micros()` time domain as RKT4 and stores a sequence number, float32 X/Y/Z acceleration, float32 X/Y/Z angular rate, latitude/longitude at 1e-7 degrees, GPS altitude/speed, fix age, last-fix uptime, live/stale validity flags, satellites, and UTC fields.
- Added CRC-32 to every header and record. Validation rejects corrupt/truncated data, unsupported layout/version, invalid sample rate/flight sequence, non-finite valid IMU axes, current GPS without a last-known position, out-of-range coordinates/time/satellite values, and unlabeled stale GPS data.
- Added complete-record and torn-tail accounting for deterministic later recovery. Existing V11 files and the RKT4 v2 encoder were not modified.
- Added native exact golden vectors plus round-trip and failure scenarios; CI now builds and executes the suite.

### Evidence

- Python V11/V12 regression suite: 69 tests passed.
- Browser RKT4 analysis test: PASS.
- `git diff --check`: PASS before documentation updates.
- Local native execution was attempted through WSL, but Windows reported that the environment could not access the checkout. Native sensor-log execution and both target compilations are **PENDING CI**.
- No firmware SD integration was added in this format-only milestone. Physical IMU/GPS sampling, timestamps, companion-file durability, recovery, storage load, and offline analysis remain **NOT TESTED**.

### Next task

Integrate the `GIS1` companion file beside each `V12Dxxx.BIN`, write records from the same sample timestamp without weakening primary RKT4 priority, and add deterministic sink/flush failure behavior before extending checkpoints and offline analysis.

### CI follow-up

- GitHub Actions run `41811405967`: PASS for 69 Python tests, all native C++ suites including exact `GIS1` golden bytes and corruption/semantic/torn-tail failures, Adafruit Feather ESP32 V2 compilation, Arduino Uno R3 compilation, and development artifact upload on commit `76988d6`.

## 2026-11-04 — Session 23: recorder-priority GIS1 firmware integration

### Implemented

- Each confirmed V12 flight now opens a matching `V12Dxxx.SEN` companion only after the primary RKT4 header/prebuffer has been flushed, its first recovery checkpoint saved, and start metadata attempted.
- Added a fixed-memory sensor-log core with the existing 150-sample prebuffer and a 16-record post-launch queue. Startup writes only the 32-byte companion header; prelaunch sensor records are drained incrementally when at least 7 ms remains before the next 50 Hz sample instead of blocking launch-time acquisition with a bulk write.
- Every companion record uses the same `micros()` value as its RKT4 sample and stores validated full-resolution IMU axes plus live/stale last-known GPS fields. The header binds the `.SEN` to its flight number and matching RKT4-header CRC.
- Primary RKT4 observe, periodic flush/checkpoint, and final stop/flush execute before companion service. Companion open, start, write, flush, queue-overflow, or final-stop failure increments separate status and never marks the primary SD recorder failed.
- Added native failure injection for start/write/flush failures, incremental prebuffer service, bounded overflow/drop accounting, and ordered shutdown. Dashboard JSON exposes companion filename, state, backlog, drops, and failures.

### Evidence

- Browser RKT4 analysis test: PASS.
- `git diff --check`: PASS before documentation updates.
- The Windows Python launcher, WSL checkout access, native C++ compiler, and Arduino CLI are unavailable in this run. The full Python suite, new native queue suite, and both production target compilations are **PENDING CI** and must pass before this milestone is complete.
- Physical SD latency, 50 Hz timing, GIS1/RKT4 timestamp alignment, queue depth/drop rate, power-loss/torn-tail behavior, real IMU/GPS values, and shared-SPI behavior remain **NOT TESTED**.

### Next task

Extend recovery checkpoints to describe the companion file, validate its CRC-framed complete records and torn tail without modifying it, then add deterministic offline export. Hardware testing remains governed by `docs/V12_FIRST_DAY_TEST.md`.

### CI follow-up

- GitHub Actions run `41818282118`: PASS for 70 Python tests, all native C++ suites including bounded companion prebuffer/queue and start/write/flush failures, Adafruit Feather ESP32 V2 compilation, Arduino Uno R3 compilation, and development artifact upload on commit `d8be537`.

## 2026-11-04 — Session 24: read-only GIS1 recovery inspection

### Implemented

- Interrupted-flight boot recovery now looks for the matching `V12Dxxx.SEN` after the primary RKT4 header and journal are validated. A missing companion is reported explicitly and does not invalidate older RKT4-only flights.
- Added a portable fixed-memory inspector that validates the GIS1 header, binds its flight sequence and stored CRC to the exact RKT4 header, and scans each complete 68-byte record through the existing semantic and CRC decoder.
- Recovery reports now include companion presence, actual bytes, header result, RKT4 binding, complete records, valid CRC prefix, torn-tail bytes, first invalid record/result, and a clear `valid`, `torn_tail`, `invalid`, `unreadable`, or `missing` status.
- The companion is opened read-only and is never truncated or deleted. Primary recovery-report durability and journal-cleanup ordering are unchanged. Dashboard JSON exposes recovered companion records and recovery issues.
- Added native tests for correct/mismatched flight binding, changed RKT4 headers, clean records, corrupt record CRC, short reads, bad GIS1 headers, and torn-tail accounting. CI now builds and runs this suite.

### Evidence

- Browser RKT4 analysis test: PASS.
- `git diff --check`: PASS before documentation updates.
- The Windows Python launcher, WSL checkout access, native C++ compiler, and Arduino CLI remain unavailable. Full Python/native execution and both target compilations are **PENDING CI**.
- Physical power-loss, SD-removal, FAT behavior, recovery duration on a full flight, and real companion-file corruption remain **NOT TESTED**.

### Next task

Extend alternating recovery checkpoints with optional companion committed-byte/count evidence while retaining backward compatibility with existing GIC1 journals, then add deterministic offline GIS1 export.

### CI follow-up

- GitHub Actions run `41826379689`: PASS for 71 Python tests, all native C++ suites including GIS1 flight binding, CRC corruption, short-read, bad-header, and torn-tail recovery cases, Adafruit Feather ESP32 V2 compilation, Arduino Uno R3 compilation, and development artifact upload on commit `56fed5e`.

## 2026-11-04 — Session 25: backward-compatible companion checkpoints

### Implemented

- Added the 48-byte CRC-protected `GIC2` alternating checkpoint format. It preserves the primary RKT4 generation, committed bytes, record count, and elapsed time while adding an explicit companion-present flag plus committed GIS1 bytes and complete-record count.
- Companion checkpoint evidence advances only after `flushIfDrained()` succeeds. Pending or merely written `.SEN` records are never represented as durable; open/write/flush failure retains the last successfully flushed prefix.
- Boot recovery accepts both legacy 32-byte `GIC1` and new `GIC2` journal slots, including mixed alternating slots, and still selects the newest valid primary checkpoint. GIC1 decodes with no companion claim.
- Recovery reports identify the selected checkpoint format and verify that the claimed companion prefix is within the actual read-only `.SEN` file. A bad secondary claim is reported without preventing primary RKT4 recovery.
- Start metadata now names `GIC2`. RKT4 v2, GIS1, all V11 files, recovery-report durability, and journal-cleanup ordering remain unchanged.
- Expanded native checkpoint coverage with an exact GIC2 golden vector, legacy GIC1 golden decode, companion alignment/count/flag/file-bound failures, CRC rejection, and existing generation/fallback cases.

### Evidence

- Browser RKT4 analysis test: PASS.
- `git diff --check`: PASS before documentation updates.
- The Windows Python launcher, WSL checkout access, native C++ compiler, and Arduino CLI remain unavailable. Full Python/native execution and both target compilations are **PENDING CI**.
- Physical checkpoint durability, FAT flush semantics, power-loss timing, journal alternation on real SD media, and sustained 50 Hz companion logging remain **NOT TESTED**.

### Next task

Add a deterministic offline GIS1 parser/export path that validates header binding and record CRCs, preserves live versus stale GPS semantics, and clearly reports corrupt or torn input.

### CI follow-up

- Run `41835223285` passed all committed Python source tests but stopped the native job because two negative-test aggregate initializers omitted the newly added fields under `-Werror=missing-field-initializers`. Commit `d93013f` explicitly zero-initialized those test-only fields; production firmware logic was unchanged.
- GitHub Actions run `41835459801`: PASS for 67 committed Python tests, all native C++ suites including exact GIC2 bytes, GIC1 compatibility, secondary alignment/count/flag/file-bound failures, Adafruit Feather ESP32 V2 compilation, Arduino Uno R3 compilation, and development artifact upload on commit `d93013f`.

