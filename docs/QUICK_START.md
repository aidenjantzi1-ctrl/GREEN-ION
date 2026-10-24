# GREEN ION — quick start

**Purpose:** Record barometric flight data on an ESP32 Feather V2, then review it on a phone, tablet or computer. **Not a flight controller:** do not connect it to ignition, staging, parachute deployment or recovery hardware.

## Before a ground test

1. Power the Feather with USB or a suitable charged LiPo. Confirm the BMP388 and 3.3 V microSD module are wired as described in [the README](../README.md).
2. Connect your phone, iPad or computer to Wi-Fi **GREEN-ION** (password **GreenIon2026**). Open **http://192.168.4.1** in a browser. This local network may report “no internet”; that is normal.
3. Tap **Take Control** if needed. Only one browser can control the recorder at a time; other devices can still view it.
4. In **Storage**, tap **Detect & Test SD Card**. Wait for the read/write check to pass. Use a noncritical FAT/FAT32 card for initial tests.
5. Keep the recorder still and run **Calibrate**. Wait until calibration completes and sensor, battery and SD indicators are acceptable.

## Record and review

6. Set a mission name, motor label and sample rate (10/25/50 Hz). Select **Save Settings & Arm Logger**. Check the displayed status before continuing.
7. Tap **Start Recording** *before* the event you want to record. For a first test, leave the recorder stationary for 15–30 seconds. GREEN ION only records data; it never launches a rocket.
8. After the ground test, tap **Stop & Save**. During an actual flight, stop only after the rocket and logger are safely recovered.
9. Open **Storage**, select the saved flight and inspect its summary and graph. Download **BIN** (original data), **CSV** (analysis), and **TXT** (metadata). Use the printable report button and your browser's **Print / Save PDF** option.

## If something does not work

- **Cannot open dashboard:** reconnect to GREEN-ION Wi-Fi and open the local address above. Turn off VPN/cellular switching if your device keeps leaving the local network.
- **Another browser controls it:** release control in the other browser or wait for its lease to expire, then tap **Take Control**.
- **SD test fails:** stop recording and disarm, reseat the card with power off, check 3.3 V wiring and FAT/FAT32 formatting, then retry with a disposable card. Do not erase a card containing important flights.
- **CSV contains empty cells:** missing/invalid measurements are intentionally left blank; they are **not zero readings**.
- **Recovered interrupted recording:** inspect its TXT and CSV. Some buffered samples may have been lost at power interruption; the BIN is retained.
- **Firmware update:** use the compiled `.ino.bin` artifact from a successful GitHub Actions build, not the `.ino` source file. Test on the ground after updating.

## Before flight use

A successful firmware compile is **not** a hardware test. Verify sensor calibration, battery power, SD read/write, normal stop, interrupted recording recovery and downloads on the actual device. Use an independently tested recovery system. Barometric acceleration/G and phase markers are approximate and must not be used for safety-critical decisions.

For deeper setup and wiring details, see [README](../README.md). For proposed follow-ups, see [60 improvement recommendations](IMPROVEMENT_BACKLOG_60.md).
