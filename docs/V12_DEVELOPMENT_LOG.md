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

