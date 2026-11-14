# GREEN ION V17.0 — installation and update guide

GREEN ION V17.0 has three independently updated pieces: **Windows dashboard**, **Feather ESP32 V2 flight recorder**, and **Arduino Uno ground receiver**. Changing one does not flash the others.

## Download and verify

Use only the official [v17.0.0 release](https://github.com/aidenjantzi1-ctrl/GREEN-ION/releases/tag/v17.0.0). The software release contains:

- `GREEN_ION_V17_WINDOWS.zip` and `GREEN_ION_V17_WINDOWS.sha256`, with accompanying Windows manifest
- `GREEN_ION_V17_WEB.zip` for the offline PWA/dashboard
- `Green_Ion_V17.ino.bin` — ESP32 **application** image, never a merged full-flash/bootloader image
- `Ground_Receiver_V17.ino.hex` — Uno AVR receiver firmware
- `GREEN_ION_V17_SOURCE.zip` and `SHA256_MANIFEST.json` (all release artifacts are SHA-256 indexed)

The source sketch folders deliberately remain `Green_Ion_V16_3/` and `Ground_Receiver_V16_3/` for Arduino build compatibility, **but the runtime version of each is 17.0.0**. The Windows update package has a V17 download filename and V17 version metadata; its extracted inner folder keeps the compatible legacy `GREEN_ION_V16_WINDOWS` name.

Do not install firmware merely because a version number looks newer: verify the SHA-256 manifest, board target, image type and correct application partition.

## Windows app installation

1. Download and verify the V17 Windows ZIP and .sha256 file.
2. Extract `GREEN_ION_V17_WINDOWS.zip` and launch `START_GREEN_ION.cmd`.
3. With the Feather powered on, connect Windows to the optional `GREEN-ION-V17` access point; it may instead connect through home Wi-Fi if configured.
4. Plug in the receive-only Uno by USB. Windows rediscovers the correct port.
5. In Windows, `UPDATE_GREEN_ION.cmd` fetches official release ZIP + checksum and installs complete packages side by side; `ROLLBACK_GREEN_ION.cmd` restores the prior complete app.
6. Confirm V17 in `Ground_Station/version.json` and keep your existing flights under `%USERPROFILE%\GREEN_ION_Data`. The updater must never delete this data.

## Feather firmware: OTA or USB

1. **Remove the motor** and keep the rocket on a stable bench. Attach a correct antenna before powering the LoRa radio, and use stable USB power and a sufficiently charged LiPo.
2. Finish flight recording, Home Test and any other maintenance. Connect to the `GREEN-ION-V17` access point (default local address `http://192.168.4.1`) or a previously configured permitted home-network route.
3. In the local dashboard open **Settings & Advanced → Firmware update**. Select the official `Green_Ion_V17.ino.bin` after SHA-256 verification.
4. Enter the explicit bench-only update/maintenance flow; never upload while a flight is active. **No firmware update is automatic.**
5. Do not interrupt power. Reconnect after restart and verify the firmware reports **17.0.0**, then perform the full Home Test and inspect the resulting SD file.
6. If OTA is unavailable, compile the active `Green_Ion_V16_3/` sketch for **Adafruit Feather ESP32 V2** using the documented Arduino CLI libraries and flash it over USB. The sketch is V17 despite its legacy folder name.

For Feather rollback use only the deliberate bench maintenance/rollback control or reinstall a previously verified historical firmware image by USB. Windows app rollback does **not** rollback the Feather.

## Arduino Uno ground receiver

1. Connect the receiver on the bench, with appropriate 3.3 V-safe RFM95W wiring and the correct antenna.
2. Use Arduino Uno AVR tooling to flash `Ground_Receiver_V17.ino.hex` (or compile `Ground_Receiver_V16_3/` with the RadioHead and GreenIonProtocol libraries).
3. Reconnect the USB receiver, verify version **17.0.0**, and check real 70-byte protocol-v2 LoRa frames, GPS fix freshness and receiver packet count.
4. Coordinate the operating radio frequency at your launch site.

## Mandatory prelaunch checks

Run **Prepare for Flight → Run Full Home Test** with motors removed. Save actual evidence of sustained 50 Hz SD writing during simultaneous LoRa activity, Wi-Fi disconnect/reconnect, power/battery stability, GPS operation, clean file/checkpoint recovery, receiver reception, safe maintenance/rollback, and correct mechanical mounting and recovery configuration. Have an appropriate launch-safety officer inspect the actual vehicle.

**Passing CI, downloading a release or flashing firmware does not authorize launch.** See the [V17 validation checklist](V17_RELEASE_CANDIDATE.md) and [full V16.3 test reference](V16_3_HOME_TEST.md).
