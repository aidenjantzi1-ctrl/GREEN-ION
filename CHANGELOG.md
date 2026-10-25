# Green Ion changelog

Every known or referenced version from **V1 through V9** is listed here. See [VERSIONS.md](VERSIONS.md) for the complete archive index and exact preserved sketch links.

**Evidence:** Actual firmware snapshots exist for V8–V9. V1, V1.2, V3 and V3.1 have documented development or serial-output evidence, but original source has not been preserved. V2 and V4–V7 are undocumented placeholders, not confirmed releases. No missing source is fabricated.

## V9 — Automatic Flight Workflow (October 24, 2026)

- Reorganized the dashboard into Status, Flight, Testing, Utilities, Storage, and System; moved all live graphs to Status and removed the visible Launch Assistant and flight-comparison interface.
- Added automatic control when one Wi-Fi station is connected, with the existing lease retained for multiple viewers.
- Added one-button preparation covering settings, time synchronization, sensor/SD/battery checks, calibration, and the complete 10-second ground test.
- Added a three-second launch prebuffer, adaptive launch confirmation, automatic Wi-Fi disconnect after a committed checkpoint, and automatic Wi-Fi restart after landing save.
- Added pressure-estimated powered ascent, coast, apogee, descent, probable parachute, and landing phases with confidence metadata.
- Added permanent mission numbers plus per-card sequence numbers persisted in `/GREENION.CFG`, independent of visible flight files.
- Simplified Storage and cleared all cached summaries/graphs after deletion before refreshing the card.
- Preserved the RKT4 header and 16-byte sample format.
- Published the verified `.ino.bin` beside the V9 source with a direct phone-download link and installation guide.

## V8.8 — Field Confidence Edition (October 24, 2026)

- Created the standalone `Green_Ion_V8_8` firmware snapshot and phone-update artifact.
- Added a 10-second end-to-end ground test covering BMP388 validity, sensor freshness, altitude noise, battery stability, sampling completeness, SD writes, and flush timing, with explicit PASS or REVIEW feedback.
- Added frozen-pressure detection, live successful-flush heartbeat, and maximum SD write/flush timing.
- Added configurable 2, 5, 10, or 20 minute automatic recording limits.
- Added phone-supplied date/time, firmware version, and Git commit build ID to flight metadata.
- Added minimum battery voltage, maximum voltage drop, postflight confidence score, and FNV-1a checksum metadata while retaining RKT4 binary compatibility.
- Added flight protection against dashboard deletion and post-recovery notes stored in the TXT companion file.
- Added a downloadable diagnostic report containing firmware/build, reset, sensor, SD, battery, timing, integrity, and latest error information.

**Dates:** October 24, 2026 repository commits are archive/commit dates, not necessarily original prototype creation dates.

This repository starts with complete source snapshots at V8. The V1–V7 history below is reconstructed from available conversation text and serial-output captures. Entries marked **documented by output** describe behavior visible in those captures but do not have preserved source in this repository. Entries marked **undocumented** are retained as version placeholders and do not claim features that could not be verified.

## V8.8 recording reliability (development, October 24, 2026)

- Added a five-second flight checkpoint journal alongside the existing 500 ms binary flushes.
- Added startup recovery of interrupted recordings, RKT4 header validation, complete-sample counting and trailing-partial-byte reporting.
- Preserved original BIN files during recovery; marked recovered metadata as interrupted, not complete.
- Prevented false interruption reports if a reset occurs between successful metadata finalization and journal cleanup.
- Added recording integrity, recovered flight count, last recovered sample count and numeric ESP32 reset reason to the dashboard.
- Strengthened CSV header validation and added synthetic RKT4 tests to the GitHub Actions workflow.
- Hardware power-loss and SD-card interruption testing remains outstanding; successful compilation is not a guarantee of crash consistency.

## V8.7 integrated launch workflow and report update (October 24, 2026)

- Extracted the embedded dashboard into `Past_Versions/Green_Ion_V8_7/Dashboard.h` to avoid Arduino's HTML/JS sketch preprocessing failure; restored successful firmware builds.
- Added CI validation of dashboard JavaScript syntax, duplicate IDs, and references to missing UI elements; the GitHub Actions artifact includes both the .ino and header source files.
- Added a contextual Launch Assistant, progress stages, physical preflight checklist, recovery review shortcut, and connection/troubleshooting help. Confirmed the system remains a **logger only** with no ignition or deployment controls.
- Added Sunlight high-contrast display mode, more responsive tablet/desktop cards, and improved button, status and file list layout.
- Kept unsaved motor and recording-type inputs from being reset by status polling. The Arm Logger action saves any changed mission and sampling-rate settings before arming.
- Reduced live polling from 500 ms to 1.2 seconds and only draws live graphs on visible tabs to reduce ESP32 and mobile-browser work.
- Improved Storage navigation so opening recent flight stats does not scroll away from SD diagnostics; corrected motor comparison selection; guarded asynchronous graph loads and long-flight calculations.
- Expanded downloadable/printable flight reports with altitude, velocity, estimated acceleration, vertical apparent G, battery voltage, pressure and temperature graphs, plus metadata, phases, and quality metrics.
- Added graphs to motor comparison printable PDF and included estimated acceleration and apparent vertical G columns in CSV exports. The raw RKT4 binary format is unchanged, and those estimates are not IMU readings.
- Hardware testing is still needed after firmware compilation; archived versions and their evidence remain preserved.

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

- [V8 source](Past_Versions/Green_Ion_V8/Green_Ion_V8.ino)
- [V8.1 source](Past_Versions/Green_Ion_V8_1/Green_Ion_V8_1.ino)
- [V8.2 source](Past_Versions/Green_Ion_V8_2/Green_Ion_V8_2.ino)
- [V8.3 source](Past_Versions/Green_Ion_V8_3/Green_Ion_V8_3.ino)
- [V8.4 source](Past_Versions/Green_Ion_V8_4/Green_Ion_V8_4.ino)
- [V8.5 source](Past_Versions/Green_Ion_V8_5/Green_Ion_V8_5.ino)
- [V8.6 source](Past_Versions/Green_Ion_V8_6/Green_Ion_V8_6.ino)
- [V8.7 source](Past_Versions/Green_Ion_V8_7/Green_Ion_V8_7.ino)

Earlier historical records (not compilable firmware): [V1](Past_Versions/V1/README.md) · [V1.2](Past_Versions/V1_2/README.md) · [V2](Past_Versions/V2/README.md) · [V3](Past_Versions/V3/README.md) · [V3.1](Past_Versions/V3_1/README.md) · [V4](Past_Versions/V4/README.md) · [V5](Past_Versions/V5/README.md) · [V6](Past_Versions/V6/README.md) · [V7](Past_Versions/V7/README.md).
