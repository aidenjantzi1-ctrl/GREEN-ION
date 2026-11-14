# GREEN ION Feather V16.2 — update and rollback

Target board: **Adafruit Feather ESP32 V2**.

The V16.2 release asset is `Green_Ion_V16_2.ino.bin`. It is the ESP32 application image compiled from `Past_Versions/V16/Green_Ion_V16_2/` plus the shared GreenIonProtocol and GreenIonRecorder libraries.

## Before OTA

Do this on a stationary bench with **all motors removed**.

1. Finish any recording or home test.
2. Use stable USB power and a charged protected 1S LiPo.
3. Confirm the board is the intended Feather ESP32 V2.
4. Download the official V16.2 BIN and verify the release SHA-256 manifest.
5. Open Settings & Advanced → firmware update.
6. Enter/confirm maintenance mode when prompted.
7. Select `Green_Ion_V16_2.ino.bin`.
8. Confirm the upload deliberately.
9. Do not disconnect power during upload/restart.
10. After reboot, System Health should report **GREEN ION Feather V16.2 / 16.2.0**.
11. Run the full home test before relying on the updated payload.

The app must never silently flash firmware, and firmware must never update automatically during a mission.

## Rollback

Use the existing Feather rollback action only on the bench and only when the previous OTA partition is still valid. Keep a known-good prior firmware image as a separate recovery option.

Rollback does not alter existing RKT4/GIS1 flight files. Physical OTA interruption/rollback behavior still requires real bench validation.
