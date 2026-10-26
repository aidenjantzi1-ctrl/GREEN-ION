# Install GREEN ION V9.1 from a phone

## Download

[Download the current phone firmware: `Green_Ion_V9_1.ino.bin`](https://raw.githubusercontent.com/aidenjantzi1-ctrl/GREEN-ION/main/Green_Ion_V9_1/Green_Ion_V9_1.ino.bin)

Download it before connecting to GREEN-ION because the recorder's Wi-Fi network does not provide internet access.

## Install

1. Keep the Feather connected to reliable power and confirm it is not armed or recording.
2. Connect the phone to Wi-Fi network `GREEN-ION` using password `GreenIon2026`.
3. Stay connected when the phone warns that this network has no internet.
4. Open `http://192.168.4.1` and choose **System**.
5. Under **Install firmware from this phone**, select `Green_Ion_V9_1.ino.bin`.
6. Tap **Install Firmware & Restart** and do not remove power.
7. Reconnect to GREEN-ION after it restarts and confirm the System tab reports V9.1.
8. Run **Prepare & Calibrate All** and a safe ground test before flight use.

Use only the file ending in `.ino.bin`. Do not select bootloader, partitions, or merged binary files.
