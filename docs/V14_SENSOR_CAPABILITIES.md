# Sensor and Data Capabilities — GREEN ION V14

The optional **Status → Sensor Lab** displays real Feather/Uno values and health counters. A dash (—) means the current source did not supply the measurement. Unit conversion changes the display only, never the original file.

## Live Feather measurements

- **BMP388:** atmospheric pressure in Pa, barometer temperature in °C, estimated altitude/relative altitude, estimated vertical speed, calibration baseline/noise/age and barometer recoveries/failures.
- **LSM6DSO32:** X/Y/Z acceleration and magnitude in g; X/Y/Z gyroscope angular rate in °/s; IMU temperature, sample validity, number of failed reads and 32 g range verification. Diagnostic gyro bias and test-sample counters are available separately.
- **PA1010D GPS:** current/stale/missing position, latitude, longitude, altitude, ground speed, satellites, UTC time, course over ground (only while moving), GGA fix quality, HDOP, geoid separation, fix age, NMEA received/parsed counts, parse failures, communications age and bench-test results.
- **Feather/SD:** LiPo voltage (not true measured state of charge), SD mount, recording mode, sample rate, sample gaps, dropped records, checkpoints, recovery status, write errors, and preflight state.
- **LoRa/Uno:** transmitter health and failure counters; receiver RSSI, sequence/packet counts, estimated missed and duplicate packets, reboot detection, radio fix age and received GPS/IMU data. **RSSI is measured by the receiver, not by the rocket.**

## Flight recordings

| Source | Recorded original measurements | What analysis adds |
|---|---|---|
| **RKT4 v2 BIN** | Timestamp, pressure, barometer temperature, battery mV, validity | Estimated apogee, vertical speed, acceleration, G, phases |
| **GIS1 v1 SEN** | Timestamp, sequence, X/Y/Z acceleration (m/s²), X/Y/Z angular rate (rad/s), GPS coords/altitude/speed/age/time/satellites, validity flags | Total acceleration in g, axis charts, angular-rate charts, GPS track |
| **TXT** | Original flight metadata and completion notes | Preserved alongside BIN and SEN |

V14's GIS1 parser verifies a **32-byte CRC32 header and 68-byte CRC32 records**, refuses corrupt headers and uses only a verified prefix if damaged records exist. If the matching BIN is available, the SEN explorer confirms the **RKT4 header CRC32 linkage**. Both decoders run offline in browser Web Workers and do not modify the original data.

**Inventive removable feature:** Export **GeoJSON recovery track**, containing only actually reported current GPS fixes sampled at intervals; no position is invented or extrapolated. GPS coordinates are private location information.

## Actual limitations

No magnetometer/true compass, optical distance sensor, thrust load cell, charge-current monitor, direct rocket orientation measurement, independent deployment tracker or ignition/staging control is fitted. Pressure/GPS altitude and barometric derivatives are estimates; a 32 g configured range is not proof of sensor accuracy under shock. A stale GPS fix must never be represented as live.

Sources: [Firmware SensorState.h](../Past_Versions/V14/Green_Ion_V14/SensorState.h), [GIS1 record format](../libraries/GreenIonRecorder/src/GreenIonSensorLog.h), [RKT4 v2](RKT4_V2_FORMAT.md), [Ground Receiver](../Past_Versions/V12/Ground_Receiver/Ground_Receiver.ino).

**GPS course is not a magnetic compass:** The RMC course over ground is presented only with a current GPS fix moving at least 0.75 m/s. HDOP expresses relative geometric dilution, **not** meters of accuracy. A GGA fix quality code does not certify a flight-safe GPS fix. They are live status additions; original GIS1 v1 files and LoRa packet layout are intentionally unchanged.
