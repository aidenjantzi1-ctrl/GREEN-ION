# GREEN ION V17 — install and update

## Windows app

1. Download `GREEN_ION_V17_WINDOWS.zip` and its SHA-256 file from the V17 release.
2. Verify the checksum, extract the ZIP, and run `START_GREEN_ION.cmd`.
3. Flight data remains under `%USERPROFILE%\GREEN_ION_Data`.

## Feather ESP32 V2

Use the release asset `Green_Ion_V17.ino.bin`.

Before updating: remove rocket motors, use stable bench power, attach the correct LoRa antenna, finish any recording/home test, and keep the payload stationary.

1. Join `GREEN-ION-V17`.
2. Open the local dashboard at `http://192.168.4.1`.
3. Open Settings & Advanced → firmware/update tools.
4. Select the official V17 application image or use the verified release updater.
5. Confirm the guarded maintenance prompt and do not interrupt power.
6. After reboot verify firmware **GREEN ION Feather V17 / 17.0.0**.
7. Run Full Home Test before use.

USB flashing remains the fallback. Do not use a merged/full-flash image where the application `.ino.bin` is expected.

## Arduino Uno receiver

Flash `Ground_Receiver_V17.ino.hex` or compile `Ground_Receiver_V17/` for Arduino Uno. Verify receiver firmware **17.0.0** in System Health and collect real receive-side evidence during Full Home Test.

## Rollback

The previous V16.3 source and release remain preserved. Windows rollback, Feather rollback/reinstall and receiver rollback are separate actions.
