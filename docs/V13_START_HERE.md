# GREEN ION V13 — START HERE

**Software beta: physical hardware and flight validation remain required. Remove motors for bench tests.**

## ONE ZIP, ONE Windows launcher

1. Go to GitHub **Releases**, choose GREEN ION V13 beta 1 and download **GREEN_ION_V13_COMPLETE.zip**. While V13 is in development, use **Actions → Build GREEN ION V13 and one ZIP download → latest successful run → GREEN_ION_V13_COMPLETE** (GitHub login may be required for Actions downloads).
2. Extract the ZIP into a **GREEN_ION_V13** folder. Keep its internal folders together.
3. Connect Windows to Feather Wi-Fi **GREEN-ION-V13** (use GREEN-ION-V12 until Feather V13 is installed).
4. If using the Uno LoRa receiver, plug it into USB and close Arduino Serial Monitor.
5. Double-click **Ground_Station/OPEN_GROUND_STATION.cmd**, enter the Uno's COM port (e.g. COM5), press Enter for Wi-Fi only, or type DEMO for simulated data.
6. Your full Feather dashboard opens at **http://127.0.0.1:8765/**. It shows a Wi-Fi and LoRa status strip. When Wi-Fi disconnects, the same page uses received LoRa telemetry. Rejoining Feather Wi-Fi restores full controls.

Windows needs Python 3. Live Uno reception also requires a one-time **py -3 -m pip install pyserial**. No Internet is required during operations.

## Folder contents

- **Ground_Station/** — one launcher, the full blue Feather dashboard, Wi-Fi/LoRa bridge, radio-only view at /radio.html
- **Feather_Firmware/Past_Versions/V13/Green_Ion_V13/** — Feather ESP32 V2 source
- **Uno_Receiver/Past_Versions/V12/Ground_Receiver/** — Uno receiver source
- **Arduino_Libraries/** — both repository-local GreenIon libraries
- **READY_TO_UPLOAD/FEATHER/** — compiled .ino.bin OTA app
- **READY_TO_UPLOAD/UNO/** — compiled Uno HEX for suitable AVR flashing tools
- **HOW_TO_UPDATE.md** — step-by-step old version replacement

Arduino source installation needs Arduino IDE 2, ESP32 board package, RadioHead, Adafruit BMP3XX, LSM6DS and GPS libraries.

**Wi-Fi live:** full settings, testing, file downloads, calibration and on-bench OTA.

**Only LoRa live:** received altitude, pressure, acceleration, rotation, battery and GPS. Wi-Fi-only controls unavailable; GPS ground speed is not vertical speed.

**Both offline:** stale readings and last known GPS with age. LoRa never sends rocket commands. Autonomous SD recording does not depend on the computer.

**Compatibility:** V13 keeps V12D### BIN/SEN/TXT flight file names and RKT4/GIS1 data formats. Do not format the microSD. Verify hardware, antennas, radio frequency, flight safety and recovery independently.
