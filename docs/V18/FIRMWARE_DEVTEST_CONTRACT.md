# V18 firmware development-test contract

Status: development preview contract for `worker/v18-firmware`. This is software evidence only, not hardware or flight qualification.

The V18 guided UI does **not** need a new firmware endpoint. The existing same-origin V17.0.1 API already exposes the required safe checks. V18 adds only backward-compatible response fields/error codes documented here.

## Non-destructive rules

- `POST /api/sdtest` may create/read/delete only the reserved file `V12SD.TMP`.
- Guided home recording uses only the reserved `GIHOME.BIN` file and removes it when verification finishes successfully.
- Never deletes or rewrites V12D### flight BIN/TXT/SEN/JRA/JRB/RCV evidence.
- SD test is blocked while launch watching, PREPARE & ARM, primary recording, OTA maintenance, or the home recorder is active.
- GPS, LoRa, and Wi-Fi remain optional to SD-first recording.
- No endpoint in this contract arms ignition, stages a rocket, deploys recovery, or proves physical RF/range/battery endurance.

## Read-only status

### GET /api/status

Use this as the main guided-test status poll. Relevant fields include:

- `recording`, `recorder_mode`, `armed`, `log_file`
- `sd_ok`, `sd_integrity_verified`, `sd_total_bytes`, `sd_used_bytes`, `sd_free_bytes`
- `barometer_ready`, `imu_ok`, `imu_range_32g`
- `calibration_baseline_pa`, `calibration_noise_m`, `calibration_age_s`
- `battery_mv`, `battery_v`, `battery_read_age_ms`, `battery_measurement_available`
- `preflight_state`, `preflight_flags`
- `flight_prep_active`, `flight_prep_phase`, `flight_prep_progress`, `flight_prep_ready`, `flight_prep_failure`, `flight_prep_critical_flags`, `flight_prep_warning_flags`
- `finalization_failures`, `recovery_pending`, `recovery_status`
- `wifi_mode`, `wifi_home_connected`, `wifi_flight_suppressed`, `wifi_flight_suppression_pending`, `wifi_post_save_restore_pending`
- `hardware_tests_allowed`, `imu_test_result`, `gps_test_result`
- `home_test_active`, `home_record_result`, `home_record_samples`, `home_record_valid_samples`, `home_record_missed`, `home_record_cleanup_ok`, `home_shared_spi_result`
- `firmware_build`, `sample_rate_hz`

`battery_measurement_available=true` means the firmware has received a battery ADC measurement. It is not a battery-health or endurance PASS. PREPARE & ARM separately blocks below the firmware readiness threshold.

### GET /api/wifi/status

Read-only Wi-Fi state. V18-relevant fields:

- `mode`: `OFF`, `AP`, or `AP+STA`
- `flight_suppressed`: Wi-Fi has actually been powered off for flight
- `flight_suppression_pending`: recorder started and the guarded Wi-Fi-off transition has been requested but not yet executed
- `post_save_restore_pending`: structural save verification succeeded and Wi-Fi restore is waiting to run
- existing AP/STA connection fields and `last_result`

These fields diagnose the Wi-Fi power state only; they do not prove RF range or Internet access.

## Safe active checks

### POST /api/sdtest

Runs a tiny write/read/byte-compare/delete test using only `V12SD.TMP`.

Success body contains:
- `success=true`
- `error_code=null`
- `temp_file="V12SD.TMP"`
- `cleanup_verified=true`

Failure codes:
- `SD_TEST_BUSY` — launch watching, PREPARE & ARM, recording, home recording, or maintenance is active
- `SD_UNAVAILABLE` — SD is not mounted
- `SPI_BUSY` — shared SPI cannot be acquired immediately
- `SD_TEMP_CLEANUP_FAILED` — reserved temp file could not be removed; preserve the card for inspection
- `SD_VERIFY_FAILED` — write/read comparison failed, while reserved-file cleanup succeeded

The endpoint is designed for repeated safe use. A stale `V12SD.TMP` is removed before a new probe; failure to remove it aborts before writing.

### POST /api/hardware-test

Argument: `sensor=imu`, `sensor=gps`, or `sensor=all`.

Accepted request: HTTP 202 with `accepted=true` and `error_code=null`.
Rejected request codes:
- `MISSING_SENSOR`
- `UNKNOWN_SENSOR`
- `TEST_DEFERRED_RECORDER_ACTIVE`

Results are read from `GET /api/status`. A GPS communication/fix result is not a LoRa range test.

### POST /api/calibrate

Restarts stationary pressure calibration when recorder/home/maintenance state allows it. HTTP 202 only means calibration restarted; poll `/api/status` for baseline/noise/readiness. It never proves venting quality in the installed rocket.

### POST /api/home/start
### POST /api/home/record
### POST /api/home/finish

Existing guided bench session. `/api/home/record` records about 30 seconds at 50 Hz to reserved `GIHOME.BIN`, checks RKT4 header/rate/record timing/sample validity and cleanup, and may exercise bounded LoRa traffic only if antenna transmission was explicitly authorized. Missing LoRa never blocks the SD test and never fabricates an RF PASS.

The UI must always call `/api/home/finish` or instruct the user to preserve/inspect the SD if cleanup fails.

## Existing flight evidence

### GET /api/files

Lists only real V12D### BIN packages. `recording_complete=true` means TXT completion evidence exists and no JRA/JRB/RCV recovery artifact remains. It is structural save-state evidence, not a cryptographic checksum.

### GET /api/metadata?name=V12D###.BIN

Returns the matching TXT sidecar. The V18 Primary firmware finalizer structurally reopens the BIN/header and completion metadata before deleting journals and allowing post-flight Wi-Fi restore.

Do not use `POST /api/delete` in automated development checks.

## PREPARE & ARM

`POST /api/preflight/start` is firmware-authoritative and can eventually arm the pressure launch watcher. A generic CHECK EVERYTHING flow must not invoke it automatically. The UI may display readiness and may offer it only as a clearly separated user-requested motor-free bench action with abort instructions.

## Hardware-dependent limits

Software/CI cannot establish:
- actual SD electrical reliability under flight vibration
- battery endurance on the 350 mAh LiPo
- pressure-port/venting accuracy
- LoRa antenna installation or range
- GPS sky-view performance
- physical wake/hibernate behavior
- So Long fit, CG/stability, or recovery clearance

Those remain NOT TESTED until physically measured.
