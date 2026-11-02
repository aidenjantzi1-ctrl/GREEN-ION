# GREEN ION V12 testing

Evidence is reported in five separate levels: source inspection, automated host tests, target compilation, bench hardware tests, and flight tests. Passing one level does not imply the next.

## Available before hardware

- [x] Fixed 70-byte telemetry v2 layout with magic, version, type, length, sequence, pressure, barometric and GPS altitude, acceleration axes and magnitude, rotation axes, battery, GPS, and CRC-32.
- [x] Round-trip codec test and rejection of corrupted, truncated, and unsupported-version packets.
- [x] Shared transmitter/receiver radio constants for provisional 920.5 MHz, 125 kHz bandwidth, spreading factor 7, coding rate 4/5, and 13 dBm transmit power.
- [x] Live/stale/missing GPS classification test; stale coordinates remain last-known, never current.
- [x] Simulated ascent/descent, GPS dropout, radio metrics, graphing, relative map, and CSV logging.
- [x] Static safety check for forbidden energetic-device control surfaces.
- [x] Compile ESP32 V12 firmware with all production libraries locally and in CI.
- [x] Compile Uno receiver locally and in CI.
- [x] Add a host-executable C++ golden-vector test for the exact RKT4 header/record bytes, corruption, torn records, and timestamp rollover.
- [x] Add native tests for CRC-protected alternating checkpoints, corrupt-slot fallback, generation rollover, file-size bounds, and torn-tail inspection.
- [x] Integrate checkpoint writes after successful firmware flushes and expose checkpoint generation/failure status.
- [ ] Add deterministic SD write-error and sensor-init failure harnesses.
- [ ] Port the remaining V11 autonomous flight watcher, recovery checkpoints, dashboard, control lease, and OTA paths with regression tests.

## Hardware-only tests — NOT TESTED

- [ ] Confirm I2C discovery and addresses with all three devices connected.
- [ ] Confirm 50/100 Hz IMU acquisition without dropped SD samples.
- [ ] Confirm GPS cold/warm start, outdoor fix, fix age, and last-known behavior.
- [ ] Confirm SD and RFM95W coexist on shared SPI under sustained writes and 1 Hz transmissions.
- [ ] Coordinate the channel with ROC, then measure 920.5 MHz packet range, loss, sequence gaps, CRC errors, RSSI, and recovery after signal loss.
- [ ] Measure battery-divider accuracy against a multimeter and current draw in every state.
- [ ] Verify buzzer transistor drive, current, acoustics, and absence of logging timing disruption.
- [ ] Perform controlled power-loss, SD-removal, sensor-disconnect, and brownout recovery tests.
- [ ] Perform vibration, retention, antenna placement, thermal, and full-duration bench tests.
- [ ] Conduct flight testing only after a documented readiness review.

No item in the hardware-only section may be checked from simulation or compilation evidence. A frequency inside 902–928 MHz is not, by itself, proof of FCC compliance or an interference-free channel.
