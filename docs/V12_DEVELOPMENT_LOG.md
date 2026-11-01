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

