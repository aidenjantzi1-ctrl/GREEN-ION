# Codex 2 — V16.3 Browser Visualization and GPS Recovery Candidate

**Status: Unmerged development branch, not an official V16.3 release.** This integrates the user's requested V12 visualization ideas into the current V16.3 browser/Windows app, without rolling back or altering autonomous Feather firmware. No flashing, deployment, or automatic merging is performed.

## Features implemented in existing modules

- **Sensor Lab:** selected live altitude (estimated), IMU acceleration magnitude (measured), angular-rate magnitude (measured), pressure (measured), and real barometric vertical-velocity estimate (when Feather reports it); 10/30/60-second display windows, bounded to 300 recent samples; never use GPS ground speed as vertical velocity. Missing/stale readings show gaps.
- **Dashboard below existing rocket animation:** interactive generic WebGL rocket using the original postflight 3D viewer's mesh and shaders, XYZ color axes, touch drag camera orbit, reset, and follow. Keep the existing 2D fallback. Relative gyro integration can drift; no magnetometer heading is claimed. Missing or stale gyroscope evidence pauses integration. High-G is labeled uncertain.
- **Recovery:** strict GPS fix status, valid latitude/longitude and fix age checked before caching. Separate last-LoRa fix and packet evidence, Apple Maps and Google Maps links, copy, manual recovery-position input and distance/bearing. The original geolocation button remains permission-on-click only. Previously imported GIS1 SEN tracks can be decoded in a browser Web Worker and displayed offline, independently of the last received rocket location.
- **WebGL saved replay:** restart, Play/Pause, slider, phase jumps and recorded-sample step backward/forward; 0.25×, 0.5×, 1×, 2×, and 4×. Existing graph/replay events continue to synchronize. Only a matching GIS1 SEN can supply saved gyro orientation/GPS; a BIN alone cannot.
- **Comparison:** four selectable overlays (altitude, estimated velocity, estimated vertical acceleration, measured angular rate), launch-aligned traces, numerical differences, peak angular rate, GPS-track availability, completeness and decoder warnings. IndexedDB originals and checksums remain unchanged.

## Important limitations

1. This is an update to the standalone web app and Windows ground station UI under web/, **not** the HTML dashboard embedded into Feather firmware. Opening the Feather's local 192.168.4.1 dashboard still shows the unmodified firmware UI.
2. Live attitude integrates gyroscope readings at browser updates, not guaranteed 50 Hz sensor timestamps. It is qualitative and can drift; do not use it as navigation or absolute pitch/yaw. Postflight estimate uses the timestamps of GIS1 records.
3. Vertical velocity is shown only when the active Feather full-status payload supplies a vertical-velocity field. The unified telemetry schema does not currently provide this separately. No value is invented if absent.
4. GPS freshness must come from GPS fix status and age, never simply the presence of a LoRa radio packet. Missing fix age means the fix is not accepted as a new observation.
5. Embedded Google Maps may require an internet connection and a configured API key. Coordinate/cached saved SEN track recovery remains available offline; Apple/Google map apps themselves may require internet.
6. No actual hardware accuracy, GPS time-to-fix, LoRa range, battery margin, microSD bus coexistence, 50 Hz timing or iOS Safari performance is claimed tested.

## Installation and use (candidate, no flashing)

1. Open the draft Codex 2 PR against main. Leave it **unmerged** while the other Codex session works. Open **Checks**, verify the Python, Node, WebGL smoke, web package, Windows package and Feather/Uno compile checks.
2. For a separate Windows candidate after the CI checks pass, open its GitHub Actions run and download **GREEN_ION_V16_WINDOWS** artifact and corresponding checksum if provided. Verify the ZIP checksum, extract beside (not over) the stable V16.3 installation, and run **START_GREEN_ION.cmd** from that candidate directory. Keep the original V16.3 install and backup.
3. For a static browser candidate, use the Actions **green-ion-v16-3-web** artifact, or check out this branch and run **node web/build.mjs**, then serve web/dist over a desktop HTTP server or an HTTPS URL for iPhone/iPad. A file:// URL cannot provide the service worker/worker environment. **Codex 2 did not deploy this site.**
4. Open **Dashboard** → Sensor Lab → live sensor graphs. Choose a measurement and 10/30/60 s. Interact with the 3D rocket under the original animation using touch or mouse, Reset View or Follow Rocket.
5. Open **Flight** → Recovery. Use Copy GPS, Apple Maps or Google Maps for **last confirmed** rocket coordinates; manually enter your own recovery origin for distance and bearing, or choose the existing permission-on-click location button.
6. Import original .BIN plus companion .SEN in **Flights & Analysis**. In Recovery load a saved SEN for the offline GPS coordinate plot. In Flights & Analysis open the original WebGL 3D viewer for timeline/sample playback and compare two to four flights with overlaid graphs.
7. To roll back, close the candidate and run the previous verified V16.3 Windows package. No Feather or Uno firmware update is required, and no SD flight file should be deleted without backup evidence.

## Tests / remaining physical checks

Automated Codex 2 regression file: tests/v16_3_codex2_features.test.mjs. It exercises source freshness and bounded graph buffers, strict GPS validity, cached/saved-track evidence, orientation calculations, recorded-timestamp replay, comparison metrics, RKT4 v2 decoding and mobile/history integration. CI also retains existing test suites, source syntax checks, WebGL smoke testing, Windows packaging and Feather ESP32 V2 / Uno compilation.

Before flight: validate a real LSM6DSO32 mounting axis, orientation/rotation signs, high-G behavior, real GPS fix latency/age and position, LoRa reception/RSSI, 50 Hz sampling while serving UI, microSD/RFM9x shared SPI, battery voltage/safety, iPhone Safari touch/WebGL and recovery end-to-end. See V16_3_HOME_TEST.md and V16_3_HARDWARE_VALIDATION.md.

## Parallel work

Codex 2 does not touch Green_Ion_V16_3, Ground_Receiver_V16_3 or main. Potential overlaps with Codex 1: README.md, web/service-worker.js, web/v14_sensor_lab.mjs, web/v15_recovery.mjs, web/v16_2_dashboard.mjs, and .github/workflows/build-v14.yml. Merge conflicts require explicit review after both sessions are finished. GitHub-only branch-level isolation was used; no local worktree was created in this environment.
