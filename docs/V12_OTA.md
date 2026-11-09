# GREEN ION V12 — Phone firmware update and rollback

**Software implementation only.** ESP32 target compilation, actual phone uploads, unexpected power loss, partition rollback, and long-duration recorder tests must be completed on the Feather before rocket use. Do not use a newly updated firmware in flight based only on CI results.

## Before updating
1. Recover and download any completed flight files from the microSD card.
2. Put the rocket on a stable surface, away from a launch operation. **Never attempt firmware maintenance during a flight.**
3. Keep a reliable external power source connected; firmware requires a measured LiPo reading of **at least 3.65 V**. If no battery is detected, OTA is blocked even on USB.
4. Verify the board is **Adafruit Feather ESP32 V2**, with an OTA-capable partition scheme, and use only its application **`.ino.bin`** from the matching GitHub Actions build. Do not upload a bootloader, partitions image, merged binary, Arduino Uno sketch, or arbitrary firmware.
5. Connect to **GREEN-ION-V12** Wi-Fi, password **GreenIon2026**, and open **http://192.168.4.1/**.

## Update from phone, tablet or computer
1. Open **System → Maintenance access → Take Control**. Only one browser can hold a 60-second maintenance lease. Retake control if it expires before starting the transfer.
2. Check the maintenance status: OTA ready, battery OK, and no active recording or hardware tests.
3. Under **Install firmware from this phone**, select the verified **`.ino.bin`**.
4. Confirm the operation. The progress indicator tracks file transfer, **not** validation or boot success.
5. After the HTTP response reports a verified image, the Feather restarts. Reconnect to **GREEN-ION-V12** (Wi-Fi may briefly disappear), verify the version, run real sensor/SD tests, and check logs before use.
6. If the transfer fails, the firmware aborts the update and restarts stationary calibration, leaving the currently running application selected. Inspect the reported error. Do not disconnect power during flash programming.

## Roll back
1. Stop all recordings and tests. Ensure reliable power.
2. Choose **Take Control** again.
3. If **Previous image AVAILABLE** is displayed, choose **Roll Back to Previous Firmware** and confirm.
4. The boot partition is switched only if ESP-IDF can read a valid application description in the inactive OTA slot. The board restarts after replying to the browser.
5. Reconnect using the previous firmware's Wi-Fi details. Older releases may use the SSID **GREEN-ION**, not **GREEN-ION-V12**. Test all actual hardware and verify flight file compatibility.

A device flashed only by USB may have **no valid previous OTA image** to roll back to. Rollback is a boot partition selection, **not** a complete backup of SD flight files, app configuration or sensor calibration. Use USB recovery with a verified build if neither OTA slot boots.

## Safety boundaries
- The update and rollback endpoints require the local Wi-Fi network and a one-browser controller token. A password printed in public source code is **not** strong network authentication; keep the network physically controlled.
- The firmware refuses updates when logging, when hardware tests are active, or if the measured battery is under 3.65 V. During an accepted upload, launch watching, SD writes, sampling and LoRa dispatch are suspended. The system must not be near a live launch during maintenance.
- The firmware checks the ESP32 image header and uses the Arduino Update library's validation. This does **not** cryptographically authenticate firmware provenance; only use a build you trust.
- Boot confirmation is attempted after the firmware initializes its dashboard. This is not a substitute for full sensor, SD, GPS, radio and rollback testing.
- Sample rate and other V12 settings remain fixed in firmware. **Download Device Settings** is read-only; configuration restore is still unavailable.
- V12 uses the **GREEN-ION-V12** network. The legacy V11 dashboard may use a different SSID.
- OTA and rollback do not control ignition, staging or rocket recovery deployment.

## Required physical verification
- [ ] Flash V12 to the Feather by USB and establish whether the OTA partition table supports two application slots.
- [ ] Test control ownership from two browsers, lease expiry and unauthorized upload rejection.
- [ ] Reject malformed and wrong-named firmware, cancelled upload, low-battery upload and upload during recording.
- [ ] Install an actual valid application over Wi-Fi; reboot and verify version, radio, sensors, SD and recording.
- [ ] Perform rollback to an actual previous valid application, check data retention and reconnect.
- [ ] Simulate interruption on **expendable** hardware/data in a controlled bench setup, then recover over USB.
- [ ] Verify serial output at 115200 baud and confirm quiet mode does not compromise sample timing.

Sources within this repository: `Green_Ion_V12/Maintenance.h`, `Green_Ion_V12/Green_Ion_V12.ino`, `Past_Versions/Green_Ion_V11/Green_Ion_V11.ino`, and `docs/V12_TESTING.md`.
