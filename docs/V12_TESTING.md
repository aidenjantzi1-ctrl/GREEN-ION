# GREEN ION V12 testing

Evidence is reported in five separate levels: source inspection, automated host tests, target compilation, bench hardware tests, and flight tests. Passing one level does not imply the next.

## Available before hardware

- [x] Fixed 70-byte telemetry v2 layout with magic, version, type, length, sequence, pressure, barometric and GPS altitude, acceleration axes and magnitude, rotation axes, battery, GPS, and CRC-32.
- [x] Round-trip codec test and rejection of corrupted, truncated, and unsupported-version packets.
- [x] Shared transmitter/receiver radio constants for provisional 920.5 MHz, 125 kHz bandwidth, spreading factor 7, coding rate 4/5, and 13 dBm transmit power.
- [x] Live/stale/missing GPS classification test; stale coordinates remain last-known, never current.
- [x] Source-contract test for bounded IMU/GPS Serial bench commands, explicit IMU units/range/rate, runtime read validation, and current-fix gating.
- [x] Source-contract test for a five-packet LoRa bench marker, CRC-preserving shared packet layout, explicit receiver identification, and matching modem output.
- [x] Native fixed-capacity telemetry-queue tests for FIFO order, oldest replacement, drop accounting, radio/storage busy deferral, sample-deadline margin, and `micros()` rollover.
- [x] Native radio-recovery tests for transient failure reset, threshold degradation, retry backoff, recorder-critical deferral, initialization failure, recovery, invalid configuration, and timestamp rollover.
- [x] Native companion sensor-log golden vectors and rejection tests for header/record CRC, truncation, invalid rate/flight identity, IMU validity, GPS current/last-known semantics, bounds, and torn tails.
- [x] Native companion-log queue tests for bounded prebuffer/pending overflow, incremental service, start/write/flush failure isolation, and deterministic stop.
- [x] Source-contract tests prove primary RKT4 observe, stop, flush, and checkpoint work precedes companion-log work and radio dispatch.
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
- [ ] Add deterministic SD write-error and sensor-init failure harnesses.
- [ ] Port the remaining V11 autonomous flight watcher, recovery checkpoints, dashboard, control lease, and OTA paths with regression tests.

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
- [ ] Perform vibration, retention, antenna placement, thermal, and full-duration bench tests.
- [ ] Conduct flight testing only after a documented readiness review.

No item in the hardware-only section may be checked from simulation or compilation evidence. A frequency inside 902–928 MHz is not, by itself, proof of FCC compliance or an interference-free channel.
