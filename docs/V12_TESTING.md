# GREEN ION V12 testing

Evidence is reported in five separate levels: source inspection, automated host tests, target compilation, bench hardware tests, and flight tests. Passing one level does not imply the next.

## Available before hardware

- [x] Fixed 70-byte telemetry v2 layout with magic, version, type, length, sequence, pressure, barometric and GPS altitude, acceleration axes and magnitude, rotation axes, battery, GPS, and CRC-32.
- [x] Round-trip codec test and rejection of corrupted, truncated, and unsupported-version packets.
- [x] Shared transmitter/receiver radio constants for provisional 920.5 MHz, 125 kHz bandwidth, spreading factor 7, coding rate 4/5, and 13 dBm transmit power.
- [x] Live/stale/missing GPS classification test; stale coordinates remain last-known, never current.
- [x] Source-contract test for bounded IMU/GPS Serial bench commands, explicit IMU units/range/rate, runtime read validation, and current-fix gating.
- [x] Native nonblocking hardware-test classifiers for stationary IMU PASS, motion INCONCLUSIVE, missing IMU FAIL, GPS communication-without-fix INCONCLUSIVE, current-fix PASS, absent NMEA FAIL, deferral, and timestamp rollover.
- [x] Dashboard source tests for Test IMU, Test GPS, Test All Sensors, ±32 g readback, NMEA/parsed counts, stale fix reporting, sensor reconnection, and active-recording lockout.
- [x] Source-contract test for a five-packet LoRa bench marker, CRC-preserving shared packet layout, explicit receiver identification, and matching modem output.
- [x] Native fixed-capacity telemetry-queue tests for FIFO order, oldest replacement, drop accounting, radio/storage busy deferral, sample-deadline margin, and `micros()` rollover.
- [x] Native radio-recovery tests for transient failure reset, threshold degradation, retry backoff, recorder-critical deferral, initialization failure, recovery, invalid configuration, and timestamp rollover.
- [x] Native companion sensor-log golden vectors and rejection tests for header/record CRC, truncation, invalid rate/flight identity, IMU validity, GPS current/last-known semantics, bounds, and torn tails.
- [x] Native companion-log queue tests for bounded prebuffer/pending overflow, incremental service, start/write/flush failure isolation, and deterministic stop.
- [x] Source-contract tests prove primary RKT4 observe, stop, flush, and checkpoint work precedes companion-log work and radio dispatch.
- [x] Native companion-recovery tests for RKT4-header/flight binding, clean records, CRC corruption, short reads, invalid headers, and torn-tail accounting.
- [x] Source-contract test proves boot recovery reports `.SEN` evidence without deleting or truncating the companion file.
- [x] Native GIC2 golden-vector and rejection tests for companion byte/count alignment, flags, file bounds, CRC, and backward-compatible GIC1 decoding.
- [x] Source-contract test proves firmware advances companion checkpoint evidence only after a successful `.SEN` flush.
- [x] Offline GIS1 parser/export tests for header and record CRCs, RKT4 binding, filename identity, live/stale/missing GPS rows, corruption, torn tails, and deterministic CSV columns.
- [x] Coordinated native failure harness using both production recorder cores for absent IMU/GPS, live-to-stale-to-missing GPS loss, secondary write/flush failure isolation, and fatal primary SD write/flush failure.
- [x] Simulated ascent/descent, GPS dropout, radio metrics, graphing, relative map, and CSV logging.
- [x] Static safety check for forbidden energetic-device control surfaces.
- [x] Compile ESP32 V12 firmware with all production libraries locally and in CI.
- [x] Compile Uno receiver locally and in CI.
- [x] Add a host-executable C++ golden-vector test for the exact RKT4 header/record bytes, corruption, torn records, and timestamp rollover.
- [x] Add native tests for CRC-protected alternating checkpoints, corrupt-slot fallback, generation rollover, file-size bounds, and torn-tail inspection.
- [x] Integrate checkpoint writes after successful firmware flushes and expose checkpoint generation/failure status.
- [x] Add a bounded boot scanner that validates journals against actual RKT4 size and writes a separate recovery report without modifying BIN data.
- [x] Add native recovery transaction tests proving journals remain when report creation or earlier cleanup steps fail.
- [x] Add native fixed-rate clock tests for early, exact, late, multi-period, invalid-interval, and `micros()` rollover behavior.
- [x] Add a fixed-format metadata golden vector plus filename, value, and buffer-bound rejection tests.
- [x] Add native landing-watcher tests for stationary false positives, descent, adaptive stability, invalid samples, gaps, anchor movement, and timestamp rollover.
- [x] Integrate landing finalization with recorder stop success/failure and completion-metadata golden/error tests.
- [x] Add native barometer health-policy tests for initialization loss, transient read failure, sustained runtime loss, retry failure, recovery, rollover, invalid configuration, and recorder-critical deferral; add source checks for firmware ordering and stale-value clearing.
- [ ] Add firmware storage-adapter failure and boot recovery/checkpoint/report transaction harnesses.
- [x] Restore V11-style browser OTA and rollback controls in V12 with explicit maintenance, token-gated POSTs, battery/recording guards, alternate-partition selection and source regression tests.
- [x] Add optional human-readable Uno console summaries while retaining JSON telemetry; add V12 dashboard JavaScript syntax validation in host tests.
- [ ] Validate OTA/rollback, power interruption and failed uploads on actual Feather hardware; control leases/configuration restore and other V11 UI features are not implemented in V12.

## Follow-up software hardening

- [x] Home-session IMU/GPS PASS cannot be recorded from a previous bench session: the Feather requires a newly started and completed measurement.
- [x] Cancel launch watching before home-test SD cleanup or NVS persistence. On setup failure, restart stationary calibration.
- [x] Use a fixed Windows ground-station CSV header for mixed radio bench and telemetry events; preserve full original JSON and log actual Uno bench reception.
- [x] Add host regression tests for CSV schema, receiver proof storage and fresh home-test status gates.
- [ ] Prove the new guards, temp cleanup and receiver CSV on assembled Feather/Uno hardware. Compilation and simulated host tests do not count as physical verification.

## Guided temporary home test — code integrated, hardware untested

- [x] Stepwise Testing-tab instructions; actual BMP388/battery status, existing IMU/GPS and SD diagnostics, manual Uno reception and battery-only confirmation, explicit PASS/FAIL/INCONCLUSIVE/NOT VERIFIED and JSON report.
- [x] Isolated home mode cancels launch watching until explicit cleanup and stationary recalibration; firmware maintenance is blocked during home testing.
- [x] Real 50 Hz SD recording for 30 seconds to reserved /GIHOME.BIN, RKT4 header and record verification, sample validity/timing check, cleanup; never saves or deletes a normal V12D flight file.
- [x] Report loss of phone Wi-Fi separately from SD logging success; no false claim that RF receiver reception or real flight was simulated.
- [ ] Complete full workflow on real Feather/Uno hardware, verify unexpected power removal, reserved-file recovery, GPS outdoors, SD behavior, and Wi-Fi dropout.

## Hardware-only tests — NOT TESTED

- [ ] Confirm I2C discovery and addresses with all three devices connected.
- [ ] Confirm 50/100 Hz IMU acquisition without dropped SD samples.
- [ ] Confirm GPS cold/warm start, outdoor fix, fix age, and last-known behavior.
- [ ] Run `docs/V12_FIRST_DAY_TEST.md` on the LSM6DSO32 and PA1010D and record measured output.
- [ ] Confirm SD and RFM95W coexist on shared SPI under sustained writes and 1 Hz transmissions.
- [ ] Measure actual radio service time and confirm the 5 ms dispatch guard is sufficient under SD writes, Wi-Fi activity, and RFM95W interrupts.
- [ ] Coordinate the channel with ROC, then measure 920.5 MHz packet range, loss, sequence gaps, CRC errors, RSSI, and recovery after signal loss.
- [ ] Measure battery-divider accuracy against a multimeter and current draw in every state.
- [ ] Verify buzzer transistor drive, current, acoustics, and absence of logging timing disruption.
- [ ] Perform controlled power-loss, SD-removal, sensor-disconnect, and brownout recovery tests.
- [ ] Perform real Feather OTA update, update interruption and alternate-slot rollback tests with the charged LiPo and USB connected; verify files survive and re-calibration after restart.
- [ ] Perform vibration, retention, antenna placement, thermal, and full-duration bench tests.
- [ ] Conduct flight testing only after a documented readiness review.

No item in the hardware-only section may be checked from simulation or compilation evidence. A frequency inside 902–928 MHz is not, by itself, proof of FCC compliance or an interference-free channel.

## V12 reliability/preflight/profile upgrade — source + CI evidence

- [x] Native deterministic preflight classification, with blocking primary-recorder conditions and separate optional warnings, plus JSON snapshot export.
- [x] Rocket profiles persisted in Feather Preferences; validated labels stored in additional TXT lines without changing RKT4 v2 or .SEN bytes.
- [x] Collision-safe new flight-file allocation checks BIN, SEN, TXT, JRA, JRB and RCV siblings.
- [x] Home-test result codes persisted in Feather NVS, sensor PASS independently re-verified on device, interrupted session marker detected after Feather restart, and phone reconnection restoration.
- [x] Receiver heartbeat with packet-gap, duplicate, out-of-order, reboot and five-packet bench reception counters; Python serial auto-retry, staleness handling and CSV export.
- [x] Python ground-station tests, native C++ preflight tests, source-contract and JavaScript syntax checks added to CI.
- [ ] Full physical home-test and NVS power-interruption/resume test with SD card, LiPo and Wi-Fi dropout.
- [ ] Flight recording with deliberate SD/LoRa contention and GPS failures, plus direct pressure calibration accuracy measurements.
- [ ] Actual Uno receiver end-to-end bench confirmation, legal frequency coordination, radio range measurement and antenna inspection.
- [ ] Rocket-by-rocket mechanical fit and range safety review for Green Eggs and So Long.

Software status flags are only a **readiness advisory**; none constitute approval to fly.

