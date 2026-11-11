# GREEN ION V12 six-feature delivery evidence

Software feature snapshot: version 12; expected Feather target **Adafruit Feather ESP32 V2** and Uno **Arduino Uno R3**.

## Implemented (software, not physical validation)

1. **Guided home test:** persistent nine-step NVS result codes, interrupted-session warning, reserved /GIHOME.BIN 30-second 50 Hz RKT4 test, temporary cleanup, current-session-only IMU/GPS results, safe calibration restoration.
2. **Automatic preflight advisory:** SD/BMP388/power/pressure calibration/watch state/recovery blockers, optional IMU/GPS/LoRa/sample errors and flight profile warnings, JSON export. An advisory is not range approval.
3. **Recording protection:** RKT4 v2 unchanged, collision-safe flight names check all .BIN/.SEN/.TXT/.JRA/.JRB/.RCV sidecars, existing checkpoint/recovery integrity with native tests.
4. **LoRa:** Uno receiver-side packet counters, sequence gaps, duplicates/out-of-order, sender restart tracking, RSSI, 5-packet bench test, GPS live/stale handling. Bench evidence is now logged to computer CSV.
5. **Windows computer:** resilient serial reconnection with user-selected COM port, offline live/stale display, fixed-column CSV with full raw JSON, telemetry and receiver-side bench packet export.
6. **Flight profiles:** Green Eggs, So Long or custom, optional motor label, persisted locally on Feather and stored in per-flight .TXT metadata, without mutating BIN/SEN format.

## Release gate

- [ ] Python regression suite, native C++ tests and both target compilers PASS for this exact commit in GitHub Actions.
- [ ] Verify actual Feather/Uno hardware, with measured logs for stable 50 Hz, SD/radio interaction and GPS outdoor fix.
- [ ] Demonstrate home-test reconnect, cleanup after power loss, and no stale sensor evidence.
- [ ] Demonstrate actual Uno receiver-side five-packet test with matching transmit and receive logs and signed-off frequency coordination.
- [ ] Verify preflight displays faults honestly with sensors/SD intentionally disconnected on a safe bench.
- [ ] Test battery-only operation, radio range, OTA/rollback, and flight-specific mechanical fit for each rocket.
- [ ] Before flight, complete range/safety inspection and deployment/recovery checks independently of GREEN ION.

Compilation and host testing do **not** establish that hardware has been soldered correctly or that a rocket is safe to launch.
