# GREEN ION V17.0.1 — SD-first flight readiness release notes

**Current software version:** `17.0.1` (Feather, Uno receiver, Windows, web dashboard).

**Verification scope:** GitHub Actions software tests, firmware compilation, Windows/OTA packaging and SHA-256 manifest checks. **No hardware flight-test results are claimed. This release is NOT flight-certified and NOT a launch authorization.**

## V17.0.1 flight-readiness upgrade

- Makes the 50 Hz RKT4 v2 microSD recorder the explicit authoritative flight-data path; GPS, LoRa, Wi-Fi and an active phone remain optional.
- Adds firmware-owned **PREPARE FOR FLIGHT**: RKT4 SD write/read/delete integrity, critical sensor/battery checks, optional GPS/LoRa diagnostics, fresh calibration, synthetic launch/landing self-test and automatic launch-watcher arming.
- Adds passive SX127x RegVersion/SPI diagnostics with exact Feather pins and probable wiring faults. Any transmit check remains explicit and requires antenna confirmation.
- Adds ascent/coast/apogee/descent status while preserving automatic landing finalization and interrupted-flight recovery.
- Fixes stale Uno receiver version output and adds receive-side chip/settings diagnostics.
- Hardware-dependent behavior remains **NOT VERIFIED** until tested on the real Feather, SD card, RFM95W, Uno, battery, rocket and launch range.

## Features integrated into V17

- Dual AP+STA Wi-Fi, with optional home-network connection, physical management authorization, password protection and bounded reconnection while the original direct AP remains available.
- Live graphs and comprehensive sensor status (barometric/GPS data, X/Y/Z IMU, gyro, battery, SD and LoRa health).
- WebGL orientation, timestamped sample-based 3D flight replay, multi-flight overlay comparison and GPS/LoRa recovery improvements.
- Security hardening for untrusted imported flight filenames and regression tests.
- Preserved autonomous **50 Hz** recorder, **three-second prelaunch buffer**, shared microSD/LoRa SPI integrity, **RKT4 v2**, **GIS1**, and **70-byte** protocol-v2 receive-only ground telemetry.
- V17 Windows software packaging with checksum verification, side-by-side update/rollback and unchanged user-data location.

## Compatibility notes

The active Arduino source folder names remain `Green_Ion_V16_3/` and `Ground_Receiver_V16_3/` because they are build entrypoints. The compiled application image is published as `Green_Ion_V17.ino.bin`; Uno HEX as `Ground_Receiver_V17.ino.hex`. Windows package's internal folder remains `GREEN_ION_V16_WINDOWS` for launcher/update compatibility; the release download is named `GREEN_ION_V17_WINDOWS.zip`. Original historical tags and all saved RKT4 v2 files remain supported.

The `Past_Versions/` archive now contains historical V12, V13, V14 and V16.2 firmware/receiver trees without deleting source snapshots. Regression tests and historical CI paths were updated.

## Physical launch hold

Real board wiring, LoRa antenna and range, frequency coordination, battery voltage/current margin, GPS reception, microSD timing/retention, uninterrupted recording, water/dust/acceleration environment, rocket stability and recovery system **must still be physically verified** before flight. Use [V17 install/update instructions](V17_INSTALL_UPDATE.md) and [physical validation checklist](V17_RELEASE_CANDIDATE.md).

This software **never controls ignition, staging, guidance or deployment**.
