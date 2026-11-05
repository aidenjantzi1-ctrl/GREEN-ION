# Phase 4 software acceptance audit

Phase 4's cloud-verifiable workflow is implemented on `codex/green-ion-v12`. This audit does **not** certify flight hardware or launch readiness. Physical results remain **NOT TESTED** until someone follows the motor-free procedure with the connected devices.

| Requirement | Software evidence | Physical boundary |
| --- | --- | --- |
| One-button Preflight and final report | The button requests safe idle Feather SD/IMU/GPS tests, refreshes idle SD capacity, tracks pending results, saves JSON reports, shows PASS/WARNING/FAIL/NOT TESTED with corrective steps. | Sensor results require actual connected hardware. |
| Connections and versions | Feather Wi-Fi/API, Ground Station, Uno USB/boot protocol, ground/Feather radio initialization and actual packets are separate checks. Firmware, telemetry, RKT4 and sample-rate versions are shown. | Uno LoRa hardware was disconnected in cloud testing; packet reception is NOT TESTED. |
| Battery, pressure and IMU | Battery voltage, low threshold and distinct-sample stability are separate; pressure baseline/noise and watcher readiness are shown. The IMU has five- and 30-second stationary tests, gyro bias, sample counts and peak acceleration-deviation checks. | Voltage calibration, battery endurance and sensor accuracy need physical measurement. |
| GPS | Communication, parsed NMEA, fix, satellites, quality, altitude validity, recent date/time, laptop UTC time-of-day comparison and distinct-fix position scatter are separate observations. | A software-stable position is not necessarily accurate. GPS date versus laptop date is not compared. Outdoor acquisition and accuracy are NOT TESTED. |
| SD and recording | Idle-only temporary write/read/cleanup, free space, separate TEST RKT4 bench recording, guarded stop, direct file download, byte-count/RKT4 validation, SHA-256 and gap/warning reports are implemented. Raw SD TEST files and imported BIN copies are retained. | Actual Wi-Fi disconnect continuity and power-loss survival are NOT TESTED. The test recording is intentionally retained until the operator verifies and manages it; only the SD self-test temporary file is automatically removed. |
| Recovery checkpoints | The Feather scans V12D, AIRP and TEST journals on boot; native tests cover checkpoint selection and cleanup ordering. Preflight reports pending recovery items. | Counters and compile tests cannot prove recovery after a physical interruption. Preserve BIN, journal and RCV evidence for a hardware test. |
| Diagnostics | Individual hardware/read-only checks, counters, recent JSONL history and export are present. A self-contained synthetic playback is labeled SIMULATION — NOT HARDWARE and does not enter live telemetry or recorder controls. | Simulation is never reported as a hardware pass. |

Acceptance evidence at this checkpoint: 130 Python tests, native metadata and checkpoint tests, RKT4 browser analysis, ESP32 V2 compilation, JavaScript syntax, and a 390-pixel Chromium smoke covering bench start, simulated link loss, reconnect, stop, file verification, 22 Diagnostics controls and synthetic playback. The smoke used fixtures; no rocket hardware was attached.

The next software phase is **Phase 5: Recovery**. Phase 4 physical verification remains open and must be updated from actual hardware evidence in `V12_IMPLEMENTATION_PROGRESS.md`.
