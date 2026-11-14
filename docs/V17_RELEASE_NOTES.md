# GREEN ION V17

GREEN ION V17 integrates the completed V16.3 reliability work with the new home Wi-Fi, live visualization, replay, comparison and GPS recovery improvements.

## Integrated software

- Feather ESP32 V2 firmware identity: **17.0.0** in `Green_Ion_V17/`.
- Arduino Uno receive-only LoRa receiver identity: **17.0.0** in `Ground_Receiver_V17/`.
- Simultaneous direct AP + optional 2.4 GHz home Wi-Fi with bounded nonblocking reconnect, write-only credential handling, SW38 physical authorization and CSRF/session protection.
- Recorder-first shared-SPI ownership for microSD + RFM9x with deferred radio IRQ SPI work.
- 50 Hz primary recording, 3-second prelaunch buffer, RKT4 v2, GIS1, checkpoints, calibration, launch/landing detection, GPS, LoRa, battery monitoring, OTA and rollback preserved.
- Bounded live sensor graphs with stale/missing-data gaps.
- WebGL rocket orientation/replay using recorded timestamps and IMU-derived relative attitude.
- Multi-flight comparison with altitude, velocity, acceleration, rotation, data quality and GPS evidence.
- GPS recovery improvements with freshness validation, last-known evidence, saved SEN tracks and offline recovery tools.
- Imported flight filenames are rendered as text in comparison tables to prevent markup injection.
- Offline PWA, IndexedDB flight history, backup/restore, Windows ground station and release packaging preserved.

## Validation boundary

V17 software is release-ready only after the V17 CI matrix passes on the exact merged commit. Automated tests and compilation do **not** prove actual perfboard wiring, RF range, outdoor GPS, battery margin, OTA interruption recovery, mechanical installation or a real launch.

See `docs/V17_HARDWARE_VALIDATION.md` before calling the hardware flight-ready.
