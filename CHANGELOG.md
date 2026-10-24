# Green Ion changelog

Every known or referenced version from **V1 through V8.7** is listed here. See [VERSIONS.md](VERSIONS.md) for the complete archive index and exact preserved sketch links.

**Evidence:** Actual firmware snapshots exist for V8–V8.7. V1, V1.2, V3 and V3.1 have documented development or serial-output evidence, but original source has not been preserved. V2 and V4–V7 are undocumented placeholders, not confirmed releases. No missing source is fabricated.

**Dates:** October 24, 2026 repository commits are archive/commit dates, not necessarily original prototype creation dates.

This repository starts with complete source snapshots at V8. The V1–V7 history below is reconstructed from available conversation text and serial-output captures. Entries marked **documented by output** describe behavior visible in those captures but do not have preserved source in this repository. Entries marked **undocumented** are retained as version placeholders and do not claim features that could not be verified.

## V8.7 dashboard and SD reliability updates (October 24, 2026)

- Fixed the apparently unresponsive **Detect & Test SD Card** button: it now remains clickable in view-only mode, requests the controller lease automatically if available, and displays actionable progress and errors. If another phone holds control, it explains why the test cannot run.
- SD verification checks temporary-file write, readback, cleanup and at least 1 MB free. Added mounted, verified, free-space, and saved-recording diagnostics.
- Explicitly initialized ESP32 SPI for the documented breakout wiring: SCK GPIO5, MISO GPIO21, MOSI GPIO19, CS GPIO33. Confirm this wiring matches the physical board before using the updated firmware.
- Improved narrow-phone spacing, file action buttons, cards and status rows.
- Added an Overview recorder snapshot and a shortcut to saved flight stats. Storage now automatically opens the latest saved flight summary, with direct View stats buttons for others.
- Added saved-flight valid-sample percentage and imperial peak-altitude conversion (when Imperial is selected). Stats are read from saved TXT metadata; unavailable measurements are not invented.
- Corrected the status API version to 8.7.
- GitHub Actions compiles the sketch to the main `.ino.bin`; a successful build and real hardware verification are separate checks.

## V8.7 — Dashboard Reliability Edition

- Fixed sample-rate selection being overwritten by the 500 ms status refresh.
- Persisted 10/25/50 Hz sampling in ESP32 Preferences across restarts.
- Added automatic sequential recording labels, with Flight/Test selection.
- Added searchable motor suggestions while supporting custom designations.
- Fixed overlapping Take/Release Control buttons on narrow screens and improved padding.
- Preserved earlier V8.6 code under its original folder.

## V8.6 — Flight Control Edition

- Added a renewable controller lease so many phones can view while only one can change settings or recorder state.
- Added clear view-only/controller status and a Take Control action to the dashboard.
- Required the controller token for flight commands, SD deletion, LED changes, shutdown, and firmware installation.
- Kept read-only status, file listing, and downloads available to observers.

## V8.5 — Phone Update Edition

- Added safe upload of GitHub-built `.ino.bin` firmware from the phone dashboard.
- Blocked updates while armed, recording, calibrating, simulating, shutting down, or at critical battery voltage.
- Added automatic reboot only after the ESP32 Update library verifies the complete image.
- Updated GitHub Actions packaging to include only the correct main OTA image and the matching `.ino` source.

## V8.4 — Adaptive Flight Edition

- Added selectable 10, 25, and 50 Hz logging rates in the dashboard.
- Stored the selected rate in the RKT4 header and flight metadata.
- Added an adaptive altitude filter with a stationary deadband and faster response to large changes.
- Added velocity spike rejection and motion-sensitive speed filtering.

## V8.3 — Flight Data Edition

- Removed the four-flight concept in favor of sequential `FLT0001`–`FLT9999` recordings.
- Added flight labels and motor designations.
- Added companion postflight summaries with duration, peak altitude, minimum pressure, temperature range, sample counts, gaps, and completion reason.
- Added on-demand CSV conversion and phone downloads while preserving original RKT4 binary files.
- Added automated ESP32 compilation and downloadable GitHub Actions artifacts.

## V8.2

- Added a dashboard deep-sleep shutdown control with physical RESET wake instructions.
- Preserved explicit recorder-only safety scope.

## V8.1

- Added non-blocking BMP388 calibration and progress feedback.
- Completed dashboard command/API matching and request error handling.
- Included live altitude, battery, unit conversion, simulation, graph, LED, and storage controls.

## V8

- First complete source version preserved in this repository.
- Added the embedded mobile dashboard, BMP388 calibration, live altitude, battery estimate, microSD diagnostics, flight controls, RKT4 logging, file downloads, graphs, simulation, and LED controls.

## V7

- **Undocumented.** No V7 source, serial output, or reliable feature list was available.
- The existence of a distinct V7 release could not be independently verified from the preserved materials.

## V6

- **Undocumented.** No V6 source, serial output, or reliable feature list was available.
- The existence of a distinct V6 release could not be independently verified from the preserved materials.

## V5

- **Undocumented.** No V5 source, serial output, or reliable feature list was available.
- The existence of a distinct V5 release could not be independently verified from the preserved materials.

## V4

- **Undocumented.** No V4 source, serial output, or reliable feature list was available.
- The existence of a distinct V4 release could not be independently verified from the preserved materials.

## V3.1 — microSD recorder prototype

- **Documented by output; source not preserved.**
- Added microSD initialization and allowed the recorder to continue when no card was detected.
- Retained startup calibration, the armed state, relative altitude, vertical velocity, peak altitude, and battery-voltage reporting.
- Serial captures show substantially steadier velocity values than the V3 test output, but the exact filtering algorithm is not available for verification.

## V3 — flight-state recorder prototype

- **Documented by output; source not preserved.**
- Expanded the project from a basic altimeter into a recorder-style state machine.
- Added `ARMED`, `FLIGHT`, and `LANDED` states with automatic launch detection.
- Reported relative altitude, vertical velocity, maximum altitude, battery voltage, and recorder state over Serial.
- Bench-test captures show a detected launch and transition to the landed state.

## V2

- **Undocumented.** No V2 source, serial output, or reliable feature list was available.
- The preserved history jumps from the V1.2 altimeter to the V3 recorder test, so no V2 behavior is claimed here.

## V1.2 — calibrated relative-altitude prototype

- **Documented by output; source not preserved.**
- Detected the BMP388 and added a sensor warm-up period.
- Established a starting pressure while the sensor was held still.
- Converted pressure changes into altitude relative to the calibrated starting point.
- Printed relative altitude, temperature, and pressure over Serial; stationary samples clustered near zero instead of the large offset seen in the earlier prototype.

## V1 — initial BMP388 altimeter prototype

- **Partially documented by output; source not preserved.**
- Established the first known BMP388 pressure, temperature, and altitude readout.
- Early serial captures show a large incorrect altitude offset of roughly `-1839 m`, indicating that the initial reference-pressure handling was not yet suitable for relative flight altitude.
- Exact V1 source and its original release notes were not preserved.

## Original-source and archive links

- [V8 source](Green_Ion_V8/Green_Ion_V8.ino)
- [V8.1 source](Green_Ion_V8_1/Green_Ion_V8_1.ino)
- [V8.2 source](Green_Ion_V8_2/Green_Ion_V8_2.ino)
- [V8.3 source](Green_Ion_V8_3/Green_Ion_V8_3.ino)
- [V8.4 source](Green_Ion_V8_4/Green_Ion_V8_4.ino)
- [V8.5 source](Green_Ion_V8_5/Green_Ion_V8_5.ino)
- [V8.6 source](Green_Ion_V8_6/Green_Ion_V8_6.ino)
- [V8.7 source](Green_Ion_V8_7/Green_Ion_V8_7.ino)

Earlier historical records (not compilable firmware): [V1](history/V1/README.md) · [V1.2](history/V1_2/README.md) · [V2](history/V2/README.md) · [V3](history/V3/README.md) · [V3.1](history/V3_1/README.md) · [V4](history/V4/README.md) · [V5](history/V5/README.md) · [V6](history/V6/README.md) · [V7](history/V7/README.md).
