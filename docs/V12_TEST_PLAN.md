# GREEN ION V12 test plan

Evidence is reported in five separate levels: source inspection, automated host tests, target compilation, bench hardware tests, and flight tests. Passing one level does not imply the next.

## Available before hardware

- [x] Fixed 50-byte telemetry layout with magic, version, type, length, sequence, and CRC-32.
- [x] Round-trip codec test and rejection of corrupted, truncated, and unsupported-version packets.
- [x] Live/stale/missing GPS classification test; stale coordinates remain last-known, never current.
- [x] Simulated ascent/descent, GPS dropout, radio metrics, graphing, relative map, and CSV logging.
- [x] Static safety check for forbidden energetic-device control surfaces.
- [x] Compile ESP32 V12 firmware with all production libraries locally (CI remains pending).
- [x] Compile Uno receiver locally (CI remains pending).
- [ ] Add host-executable C++ golden-vector tests for the exact shared header.
- [ ] Add deterministic SD write-error and sensor-init failure harnesses.
- [ ] Port V11 RKT4 v2, autonomous flight watcher, recovery checkpoints, dashboard, control lease, and OTA paths with regression tests.

## Hardware-only tests — NOT TESTED

- [ ] Confirm I2C discovery and addresses with all three devices connected.
- [ ] Confirm 50/100 Hz IMU acquisition without dropped SD samples.
- [ ] Confirm GPS cold/warm start, outdoor fix, fix age, and last-known behavior.
- [ ] Confirm SD and RFM95W coexist on shared SPI under sustained writes and 1 Hz transmissions.
- [ ] Measure 915 MHz packet range, loss, sequence gaps, CRC errors, RSSI, and recovery after signal loss.
- [ ] Measure battery-divider accuracy against a multimeter and current draw in every state.
- [ ] Verify buzzer transistor drive, current, acoustics, and absence of logging timing disruption.
- [ ] Perform controlled power-loss, SD-removal, sensor-disconnect, and brownout recovery tests.
- [ ] Perform vibration, retention, antenna placement, thermal, and full-duration bench tests.
- [ ] Conduct flight testing only after a documented readiness review.

No item in the hardware-only section may be checked from simulation or compilation evidence.

