# GREEN ION V18 frontend contract — offline app

Status: frontend-owned contract proposal based on verified V17.0.1 `main` behavior. Primary Coder and Integration Manager approval is required before merge if V18 firmware/receiver changes any endpoint, field, or semantic below.

## Architectural boundary

- V18 browser app is served locally by the existing Windows ground-station HTTP server on loopback.
- Flight control/status uses same-origin local `/api/*` routes. The Windows bridge relays those routes to the Feather. The browser does not require a cloud origin.
- Recovery uses browser Web Serial directly with the Arduino Uno at 115200 baud.
- Data import/analysis is entirely browser-local. Original BIN bytes are never modified.
- No cloud account, remote telemetry upload, subscription, or internet access is required by the V18 frontend.
- This branch does not change Feather firmware, receiver firmware, Python bridge code, radio payloads, RKT4, or GIS1.

## Flight API contract currently verified on main

### GET /api/status

Fields consumed by V18:

| Field | Meaning used by frontend |
| --- | --- |
| `recording` | true only while the primary recorder is active |
| `armed` | firmware launch watcher is armed |
| `recorder_mode` | existing firmware recorder state; V18 does not assign new numeric meanings |
| `sd_ok` | critical storage availability |
| `imu_ok` | critical IMU availability |
| `barometer_ready` | critical pressure-sensor availability |
| `battery_v`, `battery_mv` | displayed battery evidence; V17 critical threshold remains firmware-owned |
| `gps_status` | `live`, `stale`, or `missing` warning state |
| `radio_ok` | optional LoRa readiness warning |
| `log_file` | firmware-selected current/last BIN filename |
| `flight_prep_active` | preparation in progress |
| `flight_prep_phase` | firmware-provided progress label |
| `flight_prep_progress` | firmware-provided progress percentage |
| `flight_prep_ready` | firmware readiness result |
| `flight_prep_failure` | actionable firmware failure text |

Frontend state mapping is display-only:

- OFFLINE: no current local-bridge status.
- READY: idle and no critical frontend-observable problem.
- TESTING: `flight_prep_active=true`.
- ARMED: `armed=true` or `flight_prep_ready=true`.
- FLYING: `recording=true`.
- SAVING: recorder has stopped but the current BIN is not yet firmware-confirmed complete.
- SAVED: only after the firmware file list reports the same BIN with `recording_complete=true`, positive byte length, and its metadata file can be read.
- ERROR: explicit preparation failure, missing microSD, or existing V17 recorder fault state.

The frontend never creates filenames and never declares readiness or save completion by timer.

### POST /api/preflight/start

V18 sends the existing no-argument V17 request. The firmware owns the complete critical readiness sequence and automatic arming.

No V18-specific `mode`, `quick`, confirmation, or synthetic-success parameter is assumed.

### GET /api/files

V18 consumes the existing array entries:

- `name`
- `size`
- `recording_complete`
- `recovery_state`

### GET /api/metadata?name=<BIN>

V18 reads the existing completion TXT and uses `duration_ms` only when present. The frontend does not invent duration when metadata is unavailable.

## Uno Web Serial contract currently verified on main

Accepted newline-delimited JSON types:

### receiver_status

Required semantics:

- `type="receiver_status"`
- optional `protocol`; when present it must equal 2
- `ok` boolean
- optional `receiver_firmware`
- optional `error`

### telemetry and radio_bench

V18 requires:

- `type` = `telemetry` or `radio_bench`
- `protocol=2`
- finite `altitude_m`
- finite `battery_v`

V18 may display, but never requires or fabricates:

- `sequence`
- `uptime_ms`
- `pressure_pa`
- `gps_status`
- `latitude`, `longitude`
- `gps_fix_age_ms`
- `last_position_age_ms`
- `rssi_dbm`
- `snr_db`
- `flight_state`
- `satellites`

Unknown packet types, wrong protocol versions, malformed JSON, impossible coordinates, non-finite altitude, or invalid battery values are discarded.

A live GPS label requires both `gps_status="live"` and valid non-zero coordinates. A prior valid position is shown only as last-known/stale evidence.

## RKT4 analysis contract

V18 reuses `web/analysis/rkt4.mjs` and the immutable RKT4 v2 format:

- 32-byte header
- 16-byte records
- timestamp, pressure, temperature, battery, validity only
- no direct acceleration field in RKT4
- altitude, velocity, acceleration, vertical-G and phases are pressure-derived estimates
- timestamp gaps, truncated tails and rejected motion spikes remain warnings
- original BIN bytes are stored unchanged with SHA-256 deduplication

## Coordination/merge gate

Primary Coder and Integration Manager should explicitly confirm before integration:

1. V18 keeps the above Flight endpoint names and meanings, or provides a reviewed replacement adapter contract.
2. The Uno continues newline-delimited protocol-v2 JSON, or the new receiver schema is supplied before frontend merge.
3. Optional GPS/LoRa absence remains advisory while critical microSD/sensor/power failures remain blocking.
4. Firmware remains authoritative for preparation, automatic arming, active recording, finalization and save verification.
5. No V18 firmware event/state field is added to the UI until its exact meaning and lifecycle are agreed in code/tests.

If another V18 branch lands first, rebase/review this branch rather than copying over concurrent work.
