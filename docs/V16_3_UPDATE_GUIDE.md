# GREEN ION V16.3 — update and rollback guide

V16.3 has three separately updated pieces: the **Windows app**, the **Feather ESP32 V2 firmware**, and the **Arduino Uno receiver firmware**. Updating one never silently updates the others.

## 1. Windows app

1. Download `GREEN_ION_V16_WINDOWS.zip` and `GREEN_ION_V16_WINDOWS.sha256` from the official `v16.3.0` GitHub release.
2. Verify the ZIP checksum if your download tool does not do this automatically.
3. Extract the ZIP.
4. Double-click `START_GREEN_ION.cmd`.
5. The app reports **GREEN ION V16.3** in Settings & Advanced.
6. Your flight archive remains under `%USERPROFILE%\GREEN_ION_Data`; the package updater does not delete it.

For a later app update, use `UPDATE_GREEN_ION.cmd`. To return to the previous verified side-by-side app package, use `ROLLBACK_GREEN_ION.cmd`.

## 2. Feather V16.3

Use the official `Green_Ion_V16_3.ino.bin` from the same release.

Before any firmware work:

- remove every rocket motor;
- put the payload on a stable bench;
- attach the correct antenna before intentional LoRa transmission;
- use stable USB power and a charged/protected LiPo if that is part of the intended setup;
- finish any recording or Full Home Test first.

### Browser OTA

1. Connect to the Feather's `GREEN-ION-V16-3` Wi-Fi.
2. Open Settings & Advanced → firmware/update tools.
3. Select the official V16.3 Feather application image, or use the verified GitHub update flow when internet access and browser networking permit it.
4. GREEN ION verifies the official image/manifest before the normal OTA workflow.
5. Confirm the separate maintenance/update prompt. Firmware is never flashed automatically.
6. Do not interrupt power.
7. Reconnect after reboot and verify the reported firmware is **GREEN ION Feather V16.3 / 16.3.0**.
8. Run **Run Full Home Test** before relying on the new firmware.

If OTA is unavailable, install the V16.3 sketch/source using the supported ESP32/Arduino USB workflow. Do not use a merged/full-flash image where an application `.ino.bin` is expected.

### Feather rollback

Windows rollback does not roll back the Feather. Use the existing deliberate Feather rollback control while safely on the bench, or reinstall the previously verified V16.2 Feather application/source by USB. Never roll back firmware during a mission.

## 3. Arduino Uno receiver V16.3

The release includes `Ground_Receiver_V16_3.ino.hex` and complete source.

1. Remove the receiver from any rocket/flight setup; this is a ground receiver.
2. Attach the correct RFM9x antenna before RF testing.
3. Select **Arduino Uno** and the correct serial port in Arduino tooling.
4. Upload/flash the V16.3 receiver source or HEX using a trusted AVR flashing workflow.
5. Reconnect USB to the Windows ground station.
6. System Health should identify receiver firmware **16.3.0**.
7. Run Full Home Test with the Feather and Uno present to collect actual receiver-side bench-packet evidence.

The Uno is 5 V logic while the RFM9x is a 3.3 V radio. Use the documented 3.3 V-safe interfacing; software cannot verify level shifting or perfboard wiring.

## What does not change

- RKT4 v2 BIN format.
- GIS1 SEN format.
- 70-byte LoRa protocol v2.
- receive-only ground LoRa.
- autonomous Feather recording.
- Windows `GREEN_ION_Data`.
- optional Supabase backups.
- exactly five blue application tabs.

A successful compile/update is not proof of RF range, SD timing on the assembled board, GPS reception, battery margin, wiring correctness or flight readiness.
