# GREEN ION Codex Handoff

This file is the starting point for the next software-update agent.

## Current release

- Current release: **V11 development** (V10 remains the last verified binary release)
- Root firmware: `Green_Ion_V10/Green_Ion_V10.ino`
- Embedded dashboard: `Green_Ion_V10/Dashboard.h`
- Phone-update binary: `Green_Ion_V10/Green_Ion_V10.ino.bin`
- User update instructions: `Green_Ion_V10/PHONE_UPDATE.md`
- Historical releases: `Past_Versions/`
- Repository: `https://github.com/aidenjantzi1-ctrl/GREEN-ION`

V10 is an experimental flight-data recorder only. It must not control ignition, recovery, or deployment hardware.

V11 development adds the browser analysis foundation in `web/analysis/rkt4.js`. Do not publish a V11 binary until the firmware is compiled and hardware-independent tests pass.

## What to read first

1. `README.md` for hardware, wiring, Wi-Fi, and phone flashing.
2. `Green_Ion_V10/PHONE_UPDATE.md` for the shortest update path.
3. `Green_Ion_V10/Green_Ion_V10.ino` for firmware/API/storage behavior.
4. `Green_Ion_V10/Dashboard.h` for the phone dashboard and JavaScript IDs.
5. `CHANGELOG.md` and `VERSIONS.md` before changing release numbering.

## Local validation

From the repository root:

```powershell
python tests/test_rkt4_recovery.py
python tests/test_v9_flight_workflow.py
```

The tests are software checks for RKT4 compatibility, recovery/checkpoints, numbering, dashboard/API consistency, and release layout. They do not replace an ESP32, BMP388, battery, or microSD hardware test.

If Arduino CLI and the required libraries are installed, compile the root sketch:

```powershell
arduino-cli compile --fqbn esp32:esp32:adafruit_feather_esp32s2 Green_Ion_V10
```

The exact FQBN may differ between Arduino-ESP32 package versions; check `arduino-cli board listall Feather` if compilation cannot find the board. Required libraries are listed in `README.md` and in the sketch comments.

## Release workflow

1. Make changes in a new `Green_Ion_VX` folder at the repository root.
2. Keep the RKT4 header and 16-byte sample record format backward-compatible unless explicitly documenting a format revision.
3. Update `README.md`, `CHANGELOG.md`, `VERSIONS.md`, and the new folder's `PHONE_UPDATE.md`.
4. Build a matching `.ino.bin` beside the `.ino`; do not commit temporary build directories.
5. Run both Python test files and the dashboard ID/API check included by the workflow.
6. Move the previous current release into `Past_Versions/` and update `Past_Versions/README.md`.
7. Commit to `main` with a clear release message and push.
8. Verify the raw `.ino` and `.bin` links and record the commit SHA in the handoff response.

The GitHub Actions workflow at `.github/workflows/build-firmware.yml` rebuilds the firmware and publishes the phone-update artifact. A queued or failed action does not mean the local source or binary is lost; report it accurately.

## V10 feature areas

V10 includes adaptive altitude filtering and pressure-spike recovery, sensor plausibility scoring, launch/landing estimates, alternating recovery checkpoints, an emergency RAM spill buffer, health snapshots, post-flight reports, manifests, SD profiling, maintenance/environment warnings, config backup/restore, integrity scanning, watchdog recovery, field mode, and phone-friendly firmware/update links.

## Safety and hardware limits

Do not claim flight readiness from compilation alone. Before a real flight, perform stationary preparation, hand-motion launch simulation, sensor-disconnect handling, SD-removal handling, and abrupt-power recovery tests. Pressure-derived launch, apogee, parachute, and landing phases are estimates and must be labeled as such.

