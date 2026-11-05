# GREEN ION V12 Ground Station telemetry model

`/api/status` returns existing top-level values for compatibility and a `field_meta` object keyed by every accepted measurement field. Each entry has `type`, `unit`, `origin`, `source`, `received_utc`, `valid`, and `freshness`.

- `source` is `wifi`, `radio`, or `simulation` for a valid value; otherwise `null`. Wi-Fi is chosen while live, then live LoRa, then explicit simulation. If every source is stale, the retained value remains visible with stale metadata.
- `received_utc` is when the laptop accepted the packet, not the sensor sample time or GPS fix time. `uptime_ms` and `gps_fix_age_ms` provide device-relative timing when reported.
- `valid` means a value is present and has the expected basic type. A numeric zero is valid. Missing or nonfinite numbers are invalid. Latitude and longitude require a valid coordinate pair. A valid historical value can still be stale.
- `freshness` is `live`, `stale`, or `missing`. GPS position, altitude, and satellite fields are stale unless the GPS fix is current and the selected link is live. The separate `last_position` and its age/source/time retain the last valid fix after disconnect.
- `origin` distinguishes sensor/hardware-reported values (`measured`) from firmware or Ground Station computations (`calculated`). The classification does not imply calibration accuracy.
- The underlying RKT4 v2 and GIS1 files are unaffected. Raw session JSONL retains the accepted source packet, while this metadata describes the current merged view.

| Field | Type | Unit | Origin |
| --- | --- | --- | --- |
| `uptime_ms` | integer | ms | measured |
| `flight_state` | integer | — | calculated |
| `recording` | boolean | — | measured |
| `altitude_m` | number | m | calculated |
| `pressure_pa` | number | Pa | measured |
| `vertical_speed_mps` | number | m/s | calculated |
| `speed_mps` | number | m/s | measured |
| `gps_speed_mps` | number | m/s | measured |
| `relative_altitude_m` | number | m | calculated |
| `temperature_c` | number | C | measured |
| `acceleration_g` | number | g | measured |
| `acceleration_x_g` | number | g | measured |
| `acceleration_y_g` | number | g | measured |
| `acceleration_z_g` | number | g | measured |
| `rotation_x_dps` | number | deg/s | measured |
| `rotation_y_dps` | number | deg/s | measured |
| `rotation_z_dps` | number | deg/s | measured |
| `imu_temperature_c` | number | C | measured |
| `battery_v` | number | V | measured |
| `satellites` | integer | count | measured |
| `gps_fix_age_ms` | integer | ms | calculated |
| `gps_status` | string | — | calculated |
| `latitude` | number | degrees | measured |
| `longitude` | number | degrees | measured |
| `gps_altitude_m` | number | m | measured |
| `health` | integer | bitmask | calculated |
| `sd_ok` | boolean | — | measured |
| `imu_ok` | boolean | — | measured |
| `gps_ok` | boolean | — | measured |
| `radio_ok` | boolean | — | measured |
| `rssi_dbm` | number | dBm | measured |
| `flags` | integer | bitmask | measured |
| `log_file` | string | — | measured |
| `barometer_ready` | boolean | — | measured |
| `recorder_status` | string | — | measured |
| `recorder_mode` | string | — | measured |
| `calibration_baseline_pa` | number | Pa | calculated |
| `calibration_noise_m` | number | m | calculated |
| `imu_test_result` | string | — | calculated |
| `imu_test_message` | string | — | calculated |
| `imu_range_32g` | boolean | — | measured |
| `imu_test_samples` | integer | count | measured |
| `imu_test_invalid` | integer | count | measured |
| `imu_test_mean_g` | number | g | calculated |
| `imu_test_bias_x_dps` | number | deg/s | calculated |
| `imu_test_bias_y_dps` | number | deg/s | calculated |
| `imu_test_bias_z_dps` | number | deg/s | calculated |
| `imu_test_temperature_c` | number | C | measured |
| `gps_test_result` | string | — | calculated |
| `gps_test_message` | string | — | calculated |
| `gps_test_communication` | boolean | — | measured |
| `gps_test_nmea` | integer | count | measured |
| `gps_test_parsed` | integer | count | measured |
| `hardware_tests_allowed` | boolean | — | calculated |

Fields not transmitted by a source are absent from that source packet and have `valid=false` in the merged view if no selected-source value exists. The Ground Station never fills an unavailable sensor measurement with zero. LoRa and Wi-Fi fields share the same units and names where present. Simulation must remain explicitly labeled and is not hardware evidence.

`altitude_m` is the BMP388 pressure estimate against a fixed 1013.25 hPa reference; it is not launch-relative or verified absolute elevation. `relative_altitude_m` is the calibrated pressure-derived height above the launch baseline when available. `gps_altitude_m` is a separate GPS-reported altitude. `vertical_speed_mps` is the barometric flight estimate; `gps_speed_mps` is horizontal GPS ground speed. Legacy LoRa `speed_mps` means GPS ground speed and is not used as vertical speed. Acceleration magnitude is in the sensor frame and is not vertical acceleration. No combined altitude is asserted.

## Ground Station battery estimate

`battery_estimate_pct` and `battery_warning` are calculated only when a numeric one-cell LiPo voltage is between 3.0 and 4.25 V. Outside that range or when missing, the estimate is `null` and the warning is `unavailable`. The approximate rested-voltage curve is 3.30 V = 0%, 3.60 V = 10%, 3.70 V = 25%, 3.80 V = 45%, 3.90 V = 65%, 4.00 V = 80%, 4.10 V = 95%, and 4.20 V = 100%, with linear interpolation between points. Under 3.30 V the display estimate remains 0%. `critical` is below 3.50 V and `low` is below 3.65 V. These are display warnings, not flight-control thresholds. Chemistry, temperature, age, load, and voltage sag can make the estimate substantially wrong. A measured capacity or fuel gauge is not installed. The estimate inherits the source packet's freshness and receive time.

## Fixed LoRa packet validity

The Uno bridge's fixed-width v2 JSON includes a `flags` bitmask. The Ground Station reads bits 2–6 as SD ready, IMU ready, barometer ready, radio ready, and recording. If the barometer or IMU ready bit is clear, their zero-filled fixed packet fields become `null` in the merged status view. A genuine zero with the ready bit set remains valid. The original RKT4 and GIS1 flight recordings are untouched. LoRa has no separate vertical-speed field, barometer temperature, or calibrated relative altitude; these remain unavailable during radio-only reception.
