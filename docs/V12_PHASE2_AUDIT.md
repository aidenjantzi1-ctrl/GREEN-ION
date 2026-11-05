# Phase 2 sensor and telemetry software audit

This is software evidence for the eight-phase V12 plan. No physical rocket hardware was connected during this run, and compilation is not a hardware test.

| Requirement | Code and verification | Limit |
| --- | --- | --- |
| BMP388 pressure, temperature, baseline, relative/peak altitude | Feather status and calibration/landing watcher; native calibration tests passed | `altitude_m` uses fixed 1013.25 hPa and is not verified absolute elevation; `relative_altitude_m` depends on launch calibration |
| GPS position, altitude, quality, ground speed, course and age | Feather RMC/GGA status; GPS altitude validity is separate from position fix; Ground Station loss/recovery tests passed | Library altitude is MSL; LoRa v2 lacks separate GGA altitude quality, course, and NMEA details |
| Independent altitude sources | Ground Station keeps `altitude_m`, `relative_altitude_m`, and `gps_altitude_m` separate; disagreement test passed | No unvalidated fusion or average is supplied |
| IMU axes, magnitude, gyro, temperature and health | Feather status, companion GIS1 log, LoRa axes and health flags; invalid fixed-packet zeros hidden | Motion is in the sensor frame; no claimed vertical acceleration or orientation solution |
| Battery voltage, validity, estimate and warnings | Feather ADC voltage; Ground Station range check and coarse 1S LiPo estimate; boundary and stale-preflight tests passed | Voltage divider and curve need multimeter and load validation; percentage is an estimate |
| SD and recorder state, capacity, errors and recovery | Feather status and idle-cached capacity; live written counts separated from committed checkpoints; native failure tests passed | Exact free space during recording is unavailable; cache is labeled stale; physical power-loss survival untested |
| Wi-Fi and LoRa communication status | Independent source state, gap/duplicate/out-of-order counters, RSSI and protocol/receiver errors; merge tests passed | Receiver is not physically connected; LoRa v2 carries fewer fields |
| RKT4 v2 and GIS1 preservation | Existing format tests and browser analysis passed; this phase did not change either file format | Hardware writes and recovery still require bench tests |

## Verification performed in this run

- Python regression suite: 111 tests passed.
- JavaScript RKT4 browser analysis suite: passed.
- Native calibration and coordinated recorder failure suites: passed.
- Feather ESP32 V2 and Arduino Uno target compilations: passed locally with installed ESP32 core 2.0.17 and AVR core 1.8.7. The ESP32 compile emitted Adafruit GPS/Wire overload warnings, without a build failure.
- GPS and storage browser smoke checks passed at phone width in Chromium.

Phase 2 software acceptance is complete for fields the current protocol can carry. Hardware validation remains **NOT TESTED**. The next phase expands Mission Control so the accepted fields are visible and usable live.
