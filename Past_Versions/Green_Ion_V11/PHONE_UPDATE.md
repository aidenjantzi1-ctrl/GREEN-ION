# GREEN ION V11 — Phone firmware update

[Download V11 firmware (`.ino.bin`)](https://raw.githubusercontent.com/aidenjantzi1-ctrl/GREEN-ION/main/Green_Ion_V11/Green_Ion_V11.ino.bin) · [Check build status](https://github.com/aidenjantzi1-ctrl/GREEN-ION/actions)

Download the binary **before** connecting to `GREEN-ION` Wi-Fi, which does not provide internet.

1. Ensure the Feather has stable power and is **not armed or recording**.
2. Join `GREEN-ION` Wi-Fi (password: `GreenIon2026`).
3. Open **http://192.168.4.1 → System**.
4. Select the downloaded `Green_Ion_V11.ino.bin` and tap **Install Firmware & Restart**.
5. Keep power connected; reconnect after restart and verify version **11.0-rc1**.
6. Calibrate, test the SD card, and run a ground recording before flight.

Use the application `.ino.bin`, **not** a bootloader, partition, or merged binary. The first firmware installation requires USB.
