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
| `speed_mps` | number | m/s | calculated |
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
