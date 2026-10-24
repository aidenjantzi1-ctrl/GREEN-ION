# Green Ion changelog

This repository starts with complete source snapshots at V8. Earlier version notes are retained only at the level supported by the available conversation history; no V1–V7 source files were available to verify.

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

- Undocumented early version. No verified source or complete change record was available when this changelog was created.

## V6

- Undocumented early version. No verified source or complete change record was available when this changelog was created.

## V5

- Undocumented early version. No verified source or complete change record was available when this changelog was created.

## V4

- Undocumented early version. No verified source or complete change record was available when this changelog was created.

## V3

- Undocumented early version. No verified source or complete change record was available when this changelog was created.

## V2

- Undocumented early version. No verified source or complete change record was available when this changelog was created.

## V1

- Undocumented initial version. No verified source or complete change record was available when this changelog was created.
