# Safe update: GREEN ION V12 → V13

1. **Back up first.** Remove rocket motors and use stable power. Keep the existing V12 app/firmware as a rollback copy. Never erase/format microSD: BIN/SEN/TXT flights are still stored there.
2. **Download once.** Download **GREEN_ION_V13_COMPLETE.zip** from GitHub Releases or a successful V13 GitHub Actions run. Extract to a NEW folder, not on top of the old V12 folder.
3. **Preserve logs.** Back up any old **Ground_Station/logs/** CSVs. New V13 logs are stored outside the replaceable app at **%USERPROFILE%\GREEN_ION_Data\logs**. Old CSVs found within the active Ground_Station/logs are copied without deleting the originals.
4. **Feather USB upload.** In Arduino IDE 2 install esp32 boards, Adafruit BMP3XX, LSM6DS, GPS, RadioHead and both packaged GreenIon libraries. Select **Adafruit Feather ESP32 V2**, use an OTA-capable partition layout and open **Feather_Firmware/Past_Versions/V13/Green_Ion_V13/Green_Ion_V13.ino**. Compile and upload.
5. **Feather OTA alternative (BENCH ONLY).** While V12 is working, use its Wi-Fi System → firmware maintenance with no recording, stable power and correct OTA partition. Upload **READY_TO_UPLOAD/FEATHER/Green_Ion_V13.ino.bin**. Do not disconnect power during an update. Afterwards reconnect to **GREEN-ION-V13** (same Wi-Fi password and 192.168.4.1 address).
6. **Uno.** Existing V12 LoRa receiver firmware remains protocol-compatible. To reinstall, open **Uno_Receiver/Past_Versions/V12/Ground_Receiver/Ground_Receiver.ino** in Arduino IDE with correct board/port and RadioHead and GreenIonProtocol libraries. Do not use the Uno HEX as a Feather update.
7. **Computer.** Install Python 3 and (if Uno connected) one-time **py -3 -m pip install pyserial**. Join Feather Wi-Fi, connect Uno USB and close Serial Monitor. Open **Ground_Station/OPEN_GROUND_STATION.cmd**, enter COM5/actual port or blank for Wi-Fi only.
8. **Verify without rocket motors.** Check full dashboard at **127.0.0.1:8765**, sensor/SD tests, actual receiver-side LoRa packets, loss of Wi-Fi followed by LoRa-only tracking, then reconnect Wi-Fi. Recheck flight recording on the SD card before ever launching.

The ZIP does not automatically flash boards, change OTA partitions, or erase files. Firmware changes are deliberate manual actions. Software CI is not proof of physical hardware or flight readiness.
