# GREEN ION V17 — Quick Fly Dashboard

This change adds a launch-day Quick Fly interface on top of the V17 flight-readiness work. It is intentionally stacked on `v17/flight-readiness-upgrade`; it must not be merged independently until that prerequisite is integrated or rebased.

## Launch-day entry points

- **Direct Feather / best iPhone and iPad field path:** connect directly to the GREEN ION access point and open `http://192.168.4.1/quick-fly`.
- **Installable web app:** `web/quick-fly.html` is the V17 PWA start page and is cached for offline use.
- **Advanced dashboard:** the existing root dashboard remains available for settings, diagnostics, replay, comparison, analysis, recovery, updates and maintenance.

No internet connection or home Wi-Fi is required for the direct Feather path. Browser rules can make an HTTPS-hosted page unable to reach a local HTTP device, especially on Safari, so the direct Feather page is the preferred field fallback.

## Firmware-owned truth

The Quick Fly UI does not simulate successful tests or arming. It starts a firmware operation and then renders only observed firmware status.

### Start preparation

`POST /api/preflight/start`

Form field:

- `mode=full` — full **PREPARE & ARM**
- `mode=quick` — repeat-flight **QUICK FLY / FLY AGAIN**

An omitted mode remains compatible with the full preparation sequence. Unknown modes are rejected.

### Status fields consumed

`GET /api/status`

Quick Fly uses:

- `version`
- `recording`
- `recorder_mode`
- `armed`
- `flight_state`
- `flight_prep_active`
- `flight_prep_mode`
- `flight_prep_phase`
- `flight_prep_progress`
- `flight_prep_ready`
- `flight_prep_failure`
- `flight_prep_critical_flags`
- `flight_prep_warning_flags`
- `sd_ok`
- `sd_integrity_verified`
- `sd_total_bytes`
- `sd_used_bytes`
- `sd_free_bytes`
- `barometer_ready`
- `imu_ok`
- `battery_v`
- `gps_status`
- `satellites`
- `radio_ok`
- `log_file`
- `launch_simulation_verified`
- `launch_simulation_cached`

The user-facing states map to firmware evidence only:

- **READY** — connected and idle, without a blocking fault.
- **TESTING** — `flight_prep_active`.
- **ARMED** — firmware reports the launch watcher armed.
- **FLYING** — recorder is actively recording.
- **SAVING** — recorder has completed but the file package has not yet been independently re-read and verified by the UI.
- **SAVED** — the current `log_file` appears in `/api/files` with `recording_complete=true` and its completion metadata can be read.
- **ERROR** — connection loss, firmware preparation failure, recorder fault or missing mandatory SD.

## Full PREPARE & ARM

The firmware performs the complete V17 sequence:

1. Recheck mandatory microSD, BMP388, LSM6DSO32 and battery.
2. Reject unresolved recovery or temporary-test cleanup problems.
3. Write a temporary RKT4 file, read it back, verify timestamps/records/header, then delete it.
4. Run IMU and GPS diagnostics.
5. Run the passive optional LoRa diagnostic.
6. Perform a fresh stationary altitude calibration.
7. Run the deterministic launch/landing detector self-test.
8. Recheck critical readiness immediately before arming.
9. Arm the real launch watcher.

GPS and LoRa are recovery/telemetry aids. Their absence produces warnings; they do not replace or block the required local microSD record.

## QUICK FLY / FLY AGAIN

Quick Fly is faster only where reuse is safe. Every repeat flight still:

- checks the current microSD presence,
- performs the temporary RKT4 write/read/integrity/delete test,
- checks battery voltage,
- checks barometer and IMU presence,
- runs a fresh stationary IMU diagnostic,
- performs a fresh stationary altitude calibration,
- performs a final critical recheck immediately before arming,
- arms the real launch watcher.

To shorten a repeat launch, Quick Fly does not wait on optional GPS and does not repeat the passive LoRa diagnostic. The deterministic launch/landing detector self-test may be reused only after it has passed once during the **same boot**. A Quick Fly after power-up still runs that self-test.

## Flight numbering and overwrite prevention

`GET /api/files` now returns `next_sequence` computed with the firmware's existing `flightSequenceOccupied()` policy.

A sequence is occupied if **any** matching `.BIN`, `.SEN`, `.TXT`, `.JRA`, `.JRB` or `.RCV` file exists. The displayed flight counter therefore follows the same reservation rule used by the recorder, and the recorder remains the authority that creates the actual filename.

The UI never creates a flight file or chooses a filename.

## Verified save and last-flight summary

After the recorder transitions from recording to complete, Quick Fly first shows **SAVING**. It then checks `GET /api/files` for the exact `log_file`.

It changes to **SAVED** only when:

1. that BIN exists,
2. the Feather reports `recording_complete=true`, and
3. `GET /api/metadata?name=<BIN>` succeeds.

The last-flight card uses the real BIN size plus `duration_ms` from completion metadata.

## Reconnection and duplicate-command protection

The page polls live status approximately once per second and automatically retries after connection loss with bounded backoff. Controls remain disabled while firmware status is unverified.

PREPARE, QUICK FLY and FLY AGAIN share a client command lock and a short duplicate-click guard. Firmware busy-state checks remain the final authority, so a second browser cannot manufacture a successful state.

## Flight history

The compact page shows recent Feather SD flights and supports direct BIN download. Replay, compare and analyze open the existing full **Flights & Analysis** page rather than duplicating the analysis system.

## Integration notes

### Primary coder

This branch depends on the V17 readiness API implemented on `v17/flight-readiness-upgrade`. Preserve these behaviors when integrating:

- `POST /api/preflight/start`
- the `flight_prep_*` status fields
- microSD integrity as a hard requirement
- battery threshold of 3.60 V as a hard blocker
- BMP388 and LSM6DSO32 as critical
- GPS and LoRa as warnings
- automatic launch watcher arming after successful critical checks

Quick Fly extends the start request with the optional `mode` field and adds `flight_prep_mode`, `launch_simulation_cached` and `/api/files.next_sequence`.

### Integration Manager

Recommended order:

1. Integrate or rebase the primary flight-readiness branch.
2. Rebase this Quick Fly branch on that result.
3. Resolve any overlap in `FlightPreparation.h` and `Green_Ion_V16_3.ino` by preserving the firmware-owned readiness state machine plus the Quick Fly mode selection.
4. Keep `web/index.html` as the advanced interface and `quick-fly.html` as the simple field interface.
5. Run the full app, firmware and packaging CI on the exact integrated head.
6. Do not infer physical flight readiness from CI alone.

## Physical validation still required

Before using the new flow for an actual launch, validate on the real Feather and disposable/test microSD:

- full PREPARE & ARM from cold boot,
- second-launch QUICK FLY in the same boot,
- SD test-file cleanup,
- flight numbering with gaps and sidecar-only occupied sequences,
- automatic launch detection and landing finalization,
- completion metadata and displayed size/duration,
- FLY AGAIN after a verified save,
- power loss/recovery behavior,
- low-battery blocking,
- nearly-full SD warning,
- missing GPS and missing LoRa while local SD still arms,
- iPhone Safari and iPad Safari through direct Feather Wi-Fi,
- reconnect behavior after normal Wi-Fi loss,
- 50 Hz recording with no dashboard-induced recorder timing regression.

No deployment, firmware flash, or merge is performed by this branch.
