# GREEN ION V12 Ground Station telemetry model

`/api/status` returns existing top-level values for compatibility and a `field_meta` object keyed by every accepted measurement field. Each entry has `type`, `unit`, `origin`, `source`, `received_utc`, `valid`, and `freshness`.

- `source` is `wifi`, `radio`, or `simulation` for a valid value; otherwise `null`. Wi-Fi is chosen while live, then live LoRa, then explicit simulation. If every source is stale, the retained value remains visible with stale metadata.
- `received_utc` is when the laptop accepted the packet, not the sensor sample time or GPS fix time. `uptime_ms` and `gps_fix_age_ms` provide device-relative timing when reported.
- `/api/status` adds `gps_fix_current_age_ms`, calculated as the reported fix age plus time since the selected packet was received. It is a display value; `gps_fix_age_ms` in the raw JSONL remains the reported value. GPS becomes stale when the calculated age exceeds three seconds, even while a radio link remains live.
- `valid` means a value is present and has the expected basic type. A numeric zero is valid. Missing or nonfinite numbers are invalid. Latitude and longitude require a valid coordinate pair. A valid historical value can still be stale.
- `freshness` is `live`, `stale`, or `missing`. GPS position, altitude, and satellite fields are stale unless the GPS fix is current and the selected link is live. The separate `last_position` and its age/source/time retain the last valid fix after disconnect. `last_position_phase`, `last_position_fix_quality`, and `last_position_satellites` describe that saved fix, not a later packet or a confirmed landing point.
- `origin` distinguishes sensor/hardware-reported values (`measured`) from firmware or Ground Station computations (`calculated`). The classification does not imply calibration accuracy.
- The underlying RKT4 v2 and GIS1 files are unaffected. Append-only session JSONL stores normalized accepted fields, while this metadata describes the current merged view.

| Field | Type | Unit | Origin |
| --- | --- | --- | --- |
| `uptime_ms` | integer | ms | measured |
| `firmware_version` | string | — | measured |
| `telemetry_protocol_version` | integer | version | measured |
| `recording_format_version` | integer | version | measured |
| `flight_state` | integer | — | calculated |
| `recording` | boolean | — | measured |
| `bench_recording` | boolean | — | measured |
| `bench_max_ms` | integer | ms | measured |
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
| `gps_altitude_valid` | boolean | — | calculated |
| `gps_fix_quality` | integer | code | measured |
| `gps_course_deg` | number | degrees | measured |
| `gps_communication` | string | — | calculated |
| `gps_nmea_sentences` | integer | count | measured |
| `gps_parsed_sentences` | integer | count | measured |
| `health` | integer | bitmask | calculated |
| `sd_ok` | boolean | — | measured |
| `imu_ok` | boolean | — | measured |
| `gps_ok` | boolean | — | measured |
| `radio_ok` | boolean | — | measured |
| `rssi_dbm` | number | dBm | measured |
| `flags` | integer | bitmask | measured |
| `log_file` | string | — | measured |
| `barometer_ready` | boolean | — | measured |
| `recorder_status` | integer | code | measured |
| `recorder_mode` | integer | code | measured |
| `calibration_baseline_pa` | number | Pa | calculated |
| `calibration_noise_m` | number | m | calculated |
| `prebuffer_records` | integer | count | measured |
| `checkpoint_generation` | integer | count | measured |
| `checkpoint_failures` | integer | count | measured |
| `checkpoint_committed_bytes` | integer | bytes | measured |
| `checkpoint_committed_records` | integer | count | measured |
| `recorder_written_records` | integer | count | measured |
| `recorder_written_bytes` | integer | bytes | calculated |
| `sample_rate_hz` | integer | Hz | calculated |
| `sd_capacity_known` | boolean | — | calculated |
| `sd_total_bytes` | integer | bytes | measured |
| `sd_used_bytes` | integer | bytes | measured |
| `sd_free_bytes` | integer | bytes | calculated |
| `sd_capacity_age_ms` | integer | ms | calculated |
| `barometer_sample_uptime_ms` | integer | ms | measured |
| `imu_sample_uptime_ms` | integer | ms | measured |
| `battery_sample_uptime_ms` | integer | ms | measured |
| `gps_fix_uptime_ms` | integer | ms | measured |
| `gps_altitude_uptime_ms` | integer | ms | measured |
| `gps_utc_hour` | integer | hour | measured |
| `gps_utc_minute` | integer | minute | measured |
| `gps_utc_second` | integer | second | measured |
| `gps_utc_valid` | boolean | — | calculated |
| `barometer_init_failures` | integer | count | measured |
| `barometer_recoveries` | integer | count | measured |
| `barometer_runtime_losses` | integer | count | measured |
| `metadata_file` | string | — | measured |
| `metadata_failures` | integer | count | measured |
| `sensor_log_status` | integer | code | measured |
| `recovered_records` | integer | count | measured |
| `recovered_sensor_records` | integer | count | measured |
| `recovered_trailing_bytes` | integer | bytes | measured |
| `sensor_recovery_issues` | integer | count | measured |
| `landing_peak_m` | number | m | calculated |
| `cabin_pressure_delta_m` | number | m | calculated |
| `airplane_state` | string | — | calculated |
| `airplane_recording` | boolean | — | measured |
| `airplane_radio_silence` | boolean | — | measured |
| `airplane_elapsed_ms` | integer | ms | calculated |
| `airplane_max_ms` | integer | ms | measured |
| `airplane_records` | integer | count | measured |
| `protocol` | integer | version | measured |
| `valid_packets` | integer | count | measured |
| `invalid_packets` | integer | count | measured |
| `sensor_log_file` | string | — | measured |
| `sensor_log_failures` | integer | count | measured |
| `sensor_log_pending` | integer | count | measured |
| `sensor_log_dropped` | integer | count | measured |
| `finalization_failures` | integer | count | measured |
| `missed_sample_periods` | integer | count | measured |
| `recovery_pending` | integer | flag | measured |
| `recovery_status` | string | — | measured |
| `recovered_logs` | integer | count | measured |
| `imu_test_result` | string | — | calculated |
| `imu_test_message` | string | — | calculated |
| `imu_range_32g` | boolean | — | measured |
| `imu_test_samples` | integer | count | measured |
| `imu_test_duration_ms` | integer | ms | calculated |
| `imu_test_peak_deviation_g` | number | g | calculated |
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

The PA1010D parser uses Adafruit GPS `altitude`, which that library documents as meters above mean sea level (MSL). Firmware marks `gps_altitude_valid` only when a parsed GGA sentence reports a positive fix quality and a finite altitude, and the altitude observation is still recent. RMC may supply a position and course before a valid GGA altitude arrives. `gps_fix_quality` is the GGA quality code (0 means invalid; 1 means GPS; 2 means DGPS). `gps_course_deg` is the RMC course clockwise from true north. The Feather also reports whether NMEA messages are arriving and the parsed/received sentence counts. Fixed LoRa v2 packets do not carry separate altitude quality, fix quality, course, or NMEA communication fields, so those fields are unknown in radio-only reception. Firmware compilation does not verify actual PA1010D behavior.

Feather Wi-Fi reports recorder filenames, mode/status numeric codes, prebuffer count, checkpoint generation/failures, sensor-log queue/failures/drops, missed sample periods, and recovery state. These fields are absent from fixed LoRa v2 telemetry; the Ground Station marks them unavailable during radio-only reception rather than retaining old Wi-Fi values as current. `recovery_pending` is a 0/1 numeric flag in the current Feather JSON. SD capacity is available only from the separate `/api/files` endpoint when recording is idle; it is not sampled in the live status loop.

`sd_total_bytes`, `sd_used_bytes`, and `sd_free_bytes` are cached values measured at SD initialization and refreshed only by an idle `/api/files` request. Their `field_meta.freshness` becomes `stale` during recording or when the capacity reading is older than 10 seconds. `sd_capacity_age_ms` reports the age; `sd_capacity_known=false` yields null capacity values. `checkpoint_committed_bytes` and `checkpoint_committed_records` advance only after a successfully written recovery checkpoint. They are committed checkpoint evidence, not the current file size or all samples collected. `sample_rate_hz` is the configured rate, not a guarantee that no sample deadlines were missed.

## Sensor sample timing

Each `field_meta` entry also includes `sample_uptime_ms` and `sample_time_basis`. Feather Wi-Fi status supplies millisecond `uptime_ms` and the last valid barometer, IMU, battery, GPS fix, and GGA altitude sample times in milliseconds since Feather boot. Older firmware supplies only whole-second `uptime_s`; the laptop converts that to milliseconds as a compatibility fallback. The Ground Station associates those times with the related sensor values. They are distinct from `received_utc`, which is laptop packet receipt time. Missing sensor samples have no sample time. LoRa v2 has only packet `uptime_ms`, so `sample_time_basis=packet_uptime` means an approximate upper bound for when the reading was packaged, not the exact sensor read time. Device uptime can wrap; cross-device absolute time alignment must not be inferred from these fields alone. `gps_utc_valid` is true only when the Feather has a recent parsed NMEA time and RMC date with plausible clock components. Preflight compares GPS and laptop UTC time of day within two minutes; it does not compare calendar dates or prove the laptop clock is correct.

`recorder_written_records` counts records accepted by the primary SD write path, including the prebuffer drained at recording start. `recorder_written_bytes` is the RKT4 header size plus record size times that count. These figures can include bytes that have not survived a flush or power loss. They are not a replacement for `checkpoint_committed_records` and `checkpoint_committed_bytes`. A failed or partial write is not added to the written record count. No extra SD read is performed to calculate these values.

`battery_measurement_valid` is a Ground Station boolean for whether a reported one-cell LiPo voltage is numeric and within the documented 3.0–4.25 V estimate range. A raw out-of-range voltage remains visible with an explicit warning, while the percentage estimate is unavailable. The Preflight battery check cannot pass on stale telemetry, implausible voltage, or critical voltage below 3.50 V; it reports a low-voltage warning below 3.65 V. These are software classifications and require comparison with a multimeter during hardware bring-up.

The Ground Station accepts all named Feather `/api/status` fields except legacy `uptime_s`, which is used only when direct `uptime_ms` is absent. It also accepts Uno `protocol`, `valid_packets`, and `invalid_packets` from telemetry messages. Uno receiver bench/configuration messages are a separate message type, not flight measurements. Uno `packet_age_ms` is ignored because the laptop computes packet age from local receipt time; `last_position_age_ms` is recomputed from the retained last valid fix.

Uno `receiver_status` is separate from flight telemetry and `field_meta`. It records firmware/protocol and ground-radio initialization while the USB connection is open. A receiver status message does not prove any LoRa packet was received.
