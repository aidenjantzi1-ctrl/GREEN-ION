# GREEN ION V14 Improvement Plan

**Design constraint:** retain the complete blue dashboard, all legacy tabs and autonomous recorder. New features must be removable without changing RKT4/GIS1 file formats.

## Implemented in Beta 4

- Device-adaptive blue card CSS for narrow phones, large phones, tablets, laptops and desktops, compact mode, safe areas and reduced motion.
- Comprehensive optional Sensor Lab, real-time IMU/barometer/GPS/radio/SD health and bounded session peaks/JSON snapshot; display original values or clearly mark missing.
- Offline full GIS1 SEN axis/GPS analysis with CRC32, replay, CSV and last-known-location GeoJSON from **current** GPS fixes.
- Private Supabase cloud sync with opt-out auto mode, progress, paginated listings, retry backoff and verified restore.
- Easy toggles under System: Sensor Lab, SEN history, automatic cloud sync, compact cards and imperial units.
- Short README, complete soldering connections, per-device guides, sensor catalog and development docs.

## Next-generation optional ideas (not implemented)

1. **Sensor-quality confidence map:** correlate sample gaps, calibration drift, GPS age and radio losses; always label estimates.
2. **Recovery pack:** one offline ZIP containing the original BIN/SEN/TXT, a current GPS track and a printable last-good-fix timestamp.
3. **Flight-to-flight anomaly trends:** compare pressure and accelerometer peaks, missed samples, battery load, and temperature over several flights.
4. **Diagnostic replay:** replay recorded health anomalies alongside measurement timelines.
5. **Extra instruments:** magnetometer, second barometer, onboard GPS antenna improvements or BLE bridge, only after verifying power, weight, costs, wiring and applicable RF rules.

## How to turn optional features off

System → **Display and optional features** provides toggles. Modules are separate: web/v14_sensor_lab.mjs, web/v14_sensor_history.mjs, web/v14_sensor_catalog.mjs and web/v14_preferences.mjs. Cloud auto-sync can be turned off while keeping manual Cloud controls. Deleting a code module later requires removing its import and service-worker cache entry in a reviewable PR.

## Release gates

Python and Node regression tests, CRC golden vectors, JavaScript syntax checks, Feather ESP32 V2 and Uno firmware compilation, preview Vercel asset verification and production checks must pass. Keep V13 rollback and archived source. **Actual sensors, RF, LiPo, Safari local network, cloud accounts and OTA must be tested on hardware** before claiming launch readiness.
