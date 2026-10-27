# GREEN ION V11

## Download the phone update

**[Download `Green_Ion_V10.ino.bin` directly](https://raw.githubusercontent.com/aidenjantzi1-ctrl/GREEN-ION/main/Green_Ion_V10/Green_Ion_V10.ino.bin)**

Download the file while your phone still has internet access. Then connect to the `GREEN-ION` Wi-Fi network, open `http://192.168.4.1`, go to **System**, choose the downloaded `.ino.bin`, and tap **Install Firmware & Restart**. The GREEN-ION network itself does not provide internet access.

GREEN ION V10 is experimental firmware for an Adafruit ESP32 Feather V2 flight-data recorder. It reads a BMP388 barometric sensor, serves a phone-friendly dashboard from its own Wi-Fi access point, and writes RKT4-compatible binary flight logs to an SPI microSD card. It never controls ignition or recovery deployment.

> **Safety scope:** GREEN ION is a data recorder only. It must never control ignition, staging, parachute deployment, or any recovery system.

## V10 field-resilient workflow

V10 is the current root-level source in `Green_Ion_V10/`. Download the `green-ion-v10-phone-update` artifact only after its GitHub Actions run succeeds. V9.2 and every earlier release are preserved under `Past_Versions/`.

V10 adds dual checkpoint recovery, emergency RAM buffering, adaptive flight filters and landing detection, pressure-spike recovery, SD profiling, stored health snapshots, post-flight reports, manifests, integrity scans, Field mode, audible phone cues, configuration backup, maintenance reminders, and guarded OTA rollback. Raw pressure remains unchanged in RKT4 records.

## Repository layout

- The latest firmware version is the only firmware folder kept at the repository root.
- Every older firmware snapshot and historical record is stored under [`Past_Versions/`](Past_Versions/).
- When a new version is released, the formerly current root folder must be moved into `Past_Versions/` before the new version is added at the root.

V8.8 adds a real 10-second ground test, altitude-noise and frozen-sensor checks, SD flush timing and heartbeat, configurable 2/5/10/20-minute recording limits, phone-provided date/time, firmware/build metadata, battery sag tracking, a postflight confidence score, an FNV-1a log checksum, protected flights, recovery notes, and a downloadable diagnostic report.

- Buffered RKT4 samples continue to flush approximately every 500 ms. A checkpoint journal is updated about every 5 seconds while recording, recording the last flushed file size and sample count.
- After a reset, firmware scans for outstanding `FLTxxxx.JRN` files and appends `status=recovered_interrupted` to the companion TXT metadata. It reports the count of complete 16-byte samples, partial trailing bytes, and whether the RKT4 header is valid. Original BIN files are preserved; CSV export ignores partial trailing records.
- If a completed flight has a leftover journal from an interruption during cleanup, the recovery scan removes that journal without relabeling the completed flight.
- The Storage diagnostics display recording integrity, recovered flights and sample count, and ESP32 reset reason code. Reset reason is a numeric ESP32 enum, not an explanation of the physical cause.
- Synthetic RKT4 regression tests and dashboard syntax checks run in GitHub Actions before firmware compilation.

**Limits:** The journal is a recovery marker, not a transactional filesystem. Power loss can still lose buffered or unwritten data; physical SD card damage is not repairable by firmware. A recording with an incomplete final sample remains a raw BIN with trailing bytes. Test interruption recovery with a disposable ground recording before relying on it in flight. This version does not alter ignition or recovery hardware.

### V10 ground-test protocol (before flight use)

1. Install the compiled firmware only after its GitHub Actions run succeeds. Keep the ESP32 powered from a stable source.
2. Insert a noncritical FAT/FAT32 microSD card, connect to the dashboard, and run **Detect & Test SD Card**. Confirm the read/write check passes.
3. Create a **Test** recording for 15–30 seconds and stop it normally. Confirm the new BIN and TXT files appear and that the TXT ends with `status=complete`. Download the CSV and inspect the sample timestamps.
4. Start another disposable Test recording, wait at least ten seconds for checkpoints, then interrupt power. Restore power and reconnect. In Storage, confirm **Recovered interrupted flights** increased and inspect the TXT for `status=recovered_interrupted`, `recovered_samples`, and `trailing_partial_bytes`. Confirm the BIN remains downloadable and its CSV opens.
5. Repeat at different points during recording, including soon after starting. Expect the most recent buffered samples to be lost after abrupt power loss; a very early interruption may leave only a header or a small recording.
6. Perform a third normal stop and restart to ensure completed recordings are **not** incorrectly relabeled as interrupted. Check that other flight files are untouched.
7. Test on battery power, then verify sensor calibration, logging rates, SD write errors, battery voltage, PDF export, and firmware update functionality. Only consider flight use after all tests pass.

## Current dashboard — guided recording and analysis

The dashboard works in modern browsers on phones, tablets, and computers over the Feather's local **GREEN-ION** Wi-Fi network. The Flight tab provides preparation, calibration, arming, launch watch, cancellation, and safe manual stop controls. Status displays the live filtered measurements and graphs. These controls operate the recorder only and do not control ignition or recovery.

The interface includes an optional **Sunlight mode**, responsive two-column tablet/desktop layouts, large touch controls, clearer explanations, and a built-in Help section in System. One browser at a time holds the controller lease; all other devices may view status and exported files.

Storage shows recorded flight summaries, automatically loads the newest saved flight without unexpectedly scrolling away from the SD controls, and offers **BIN**, **CSV**, **TXT**, and printable PDF reports. Reports include flight metadata, estimated launch/apogee/landing markers, derived climb/descent statistics, sample-quality calculations, battery voltage, acceleration/G estimates, and seven graphs. The motor comparison PDF includes side-by-side flight statistics and comparison graphs. On iPad/iPhone choose **Print / Save PDF**, then share the print preview to Files; computers may print to PDF.

**Sample-rate and mission changes:** The dashboard preserves unsaved form fields during status refreshes, and **SAVE SETTINGS & ARM LOGGER** applies outstanding motor/mission/sample-rate edits before arming. The recorder's selected 10/25/50 Hz rate is saved in ESP32 Preferences.

**Hardware and data limitations:** Flight acceleration and apparent vertical G are calculated from barometric altitude, not a true accelerometer. The original 16-byte RKT4 binary records remain unchanged and contain sampled battery voltage, pressure and temperature. CSV conversion adds estimated acceleration and G columns. Exact ignition/burnout/deployment events, absolute position and true 3D acceleration are not measurable with this sensor set. A separate, independently tested recovery system is essential.

**Before flying:** Wait for a successful GitHub Actions build; install the new image; verify wiring and SD read/write on the physical Feather; record a ground test; download CSV and TXT; generate and inspect a PDF; and confirm battery operation. Compilation does not verify real hardware operation.

## Features

- Non-blocking pressure calibration with live progress feedback
- Dashboard shutdown control using ESP32 deep sleep; press physical RESET to restart
- Sequential `FLT0001` through `FLT9999` files with no four-flight limit
- Flight labels and motor designations saved in companion summaries
- Original BIN, analysis-ready CSV, and summary downloads directly to a phone
- Postflight peak, pressure, temperature, duration, and sample-quality statistics
- Adaptive altitude smoothing that damps stationary jitter and responds faster to large flight changes
- Dashboard-selectable 10, 25, or 50 Hz logging, stored in each RKT4 header and retained across ESP32 reboots
- Phone firmware updates using the main `.ino.bin` produced by GitHub Actions
- Multiple simultaneous viewers with one renewable 30-second controller lease
- Automatic Flight/Test labels with monotonically increasing recording numbers
- Searchable motor designation suggestions with custom entry
- Responsive control buttons and additional phone-sized padding
- Live relative altitude after calibration, even when the recorder is not armed
- Estimated LiPo battery percentage and measured voltage
- BMP388, battery, and microSD preflight status
- Calibrate, Arm, Start Recording, and Stop & Save controls
- Metric/Imperial switching, including Celsius/Fahrenheit and hPa/inHg
- Absolute pressure in PSI
- Live altitude, pressure, and temperature graphs
- Simulated flight mode for dashboard checks
- microSD diagnostics, file listing, downloads, and deletion
- RGB NeoPixel color, brightness, effect, Off, and automatic status controls
- RKT4 binary log header and 16-byte sample records

## Hardware

- Adafruit ESP32 Feather V2
- Adafruit BMP388 or BMP390 breakout configured for I2C
- Optional 3.3 V-compatible SPI microSD module and FAT-formatted card
- Optional 3.7 V LiPo battery connected to the Feather battery socket
- USB-C cable for programming and power

## Arduino setup

Install the current Arduino IDE, then add Espressif's ESP32 board package through Boards Manager. Select **Adafruit ESP32 Feather V2** as the target board.

Install these libraries through Arduino Library Manager:

- Adafruit BMP3XX Library
- Adafruit Unified Sensor (dependency)
- Adafruit NeoPixel

The ESP32 board package supplies `WiFi`, `WebServer`, `Wire`, `SPI`, and `SD`.

Open `Green_Ion_V10/Green_Ion_V10.ino`, keep `Dashboard.h` beside it, select the correct serial port, and upload. Open Serial Monitor at **115200 baud** for startup and diagnostic messages. The first installation must be performed over USB; afterward, later GitHub-built updates can be installed from the dashboard on a phone.

## Wiring

### BMP388 over I2C

| BMP388 | Adafruit ESP32 Feather V2 |
| --- | --- |
| VIN | 3V |
| GND | GND |
| SCK / SCL | SCL |
| SDI / SDA | SDA |

Use the breakout's I2C mode. The firmware tries BMP3XX addresses `0x77` and `0x76`.

### Optional SPI microSD

| microSD module | Adafruit ESP32 Feather V2 |
| --- | --- |
| VCC | 3V |
| GND | GND |
| SCK / CLK | SCK |
| MOSI / DI | MOSI |
| MISO / DO | MISO |
| CS | GPIO 33 |

Use a module that is electrically compatible with 3.3 V logic. The firmware can display live sensor data without a card, but arming and flight recording require a card that passes the built-in read/write test.

## Connect to the dashboard

1. Power the Feather and wait for `GREEN ION V10` in Serial Monitor.
2. On a phone, tablet, or computer, open Wi-Fi settings.
3. Join **GREEN-ION**.
4. Enter the password **GreenIon2026**.
5. If the device warns that the network has no internet, choose to stay connected. This is expected: GREEN ION is a local device network and does not provide internet access.
6. Open **http://192.168.4.1** in a browser.

## Full launch workflow

1. **Install and connect:** Verify that the Feather, BMP388, microSD module, and battery are secured, then join **GREEN-ION** Wi-Fi and open **http://192.168.4.1** in your browser. It is normal for this Wi-Fi to have no internet.
2. **Take control:** Open Overview and press **TAKE CONTROL**. Other connected devices may view the status but cannot change settings without taking the control lease.
3. **Check health:** Confirm fresh BMP388 readings and a suitable battery reading. Open Storage and press **DETECT & TEST SD CARD**; confirm a successful read/write verification and sufficient space.
4. **Set the recording:** Choose Flight or Test, type/select the motor label, and select 10, 25, or 50 samples per second. At 25 Hz you typically get a useful compromise between detail and file size. Editing these fields marks them unsaved; **SAVE SETTINGS & ARM LOGGER** applies the changes before arming.
5. **Calibrate:** With the rocket stationary near its launch point and the pressure vent unobstructed, run **CALIBRATE**. Verify successful calibration and altitude near zero. Follow motor and launch-range safety procedures separately.
6. **Arm the logger and start logging:** Arm only after preflight checks pass. Press **START RECORDING** before the physical launch and check the file and sample count. GREEN-ION **does not** initiate ignition or control any recovery equipment.
7. **During flight:** The onboard recorder operates independently of the phone or Wi-Fi signal. Wi-Fi disconnects or range loss must not be treated as an SD failure; do not rely on a live connection to run a flight.
8. **After recovery:** If it has not already stopped automatically, press **STOP & SAVE** after recovering the rocket. Do not remove power or the card while a recording is active.
9. **Review and export:** In Storage select **View stats**, inspect phase markers, confidence and integrity results, then download the complete selected-flight package. Keep the original BIN file for later reanalysis.

## LED status

Automatic status colors take priority during safety-critical recorder states. The dashboard's LED page can select a manual color, effect, and brightness, restore automatic status, or turn the RGB LED off when safety overrides are inactive. The dashboard contains the full automatic color guide used by this firmware.

## Troubleshooting

- **Dashboard does not open:** Confirm the device is still connected to `GREEN-ION`, disable mobile-data auto-switching if needed, and enter `http://192.168.4.1` exactly.
- **Calibration fails:** Keep the BMP388 still, check I2C power and wiring, and verify that the sensor uses address `0x77` or `0x76`.
- **Altitude says CALIBRATE:** Complete a successful calibration. Live relative altitude is available immediately afterward and does not require arming.
- **Arm is blocked:** Check calibration, sensor freshness, battery state, SD verification, SD free space, and any displayed error.
- **SD test fails:** Power down before rewiring, confirm CS is GPIO 33, use 3.3 V logic, and reformat the card as FAT/FAT32.
- **Battery reading is unknown or inaccurate:** The firmware estimates a single-cell LiPo percentage from voltage. USB power, load, ADC tolerance, and battery chemistry affect the estimate.
- **Upload fails:** Confirm the Feather V2 board and serial port, close other serial programs, and retry while holding BOOT if the board does not enter its bootloader automatically.

## Complete version archive

- [Full version index (V1–V10)](VERSIONS.md) — lists each version and original source availability.
- [Complete changelog](CHANGELOG.md) — documented changes and evidence limitations.
- [All past versions and historical records](Past_Versions/) — archived firmware snapshots plus descriptive early-version records.

### Preserved firmware sketches

- [V8 source](Past_Versions/Green_Ion_V8/Green_Ion_V8.ino) — preserved Arduino sketch
- [V8.1 source](Past_Versions/Green_Ion_V8_1/Green_Ion_V8_1.ino) — preserved Arduino sketch
- [V8.2 source](Past_Versions/Green_Ion_V8_2/Green_Ion_V8_2.ino) — preserved Arduino sketch
- [V8.3 source](Past_Versions/Green_Ion_V8_3/Green_Ion_V8_3.ino) — preserved Arduino sketch
- [V8.4 source](Past_Versions/Green_Ion_V8_4/Green_Ion_V8_4.ino) — preserved Arduino sketch
- [V8.5 source](Past_Versions/Green_Ion_V8_5/Green_Ion_V8_5.ino) — preserved Arduino sketch
- [V8.6 source](Past_Versions/Green_Ion_V8_6/Green_Ion_V8_6.ino) — preserved Arduino sketch
- [V8.8 source](Past_Versions/Green_Ion_V8_8/Green_Ion_V8_8.ino) — archived previous release
- [V10 source](Green_Ion_V10/Green_Ion_V10.ino) — latest buildable Arduino sketch
- [V8.7 source](Past_Versions/Green_Ion_V8_7/Green_Ion_V8_7.ino) — previous preserved snapshot

**Important:** Source snapshots begin at V8. Earlier historical records must not be confused with working firmware. GitHub commit dates in October 2026 are preservation dates, not necessarily prototype development dates.

## View flight statistics on your phone

After a recording has been saved to microSD, connect to the `GREEN-ION` Wi-Fi dashboard and open **Storage → Flight files**. Tap **View stats** next to a recording to see the saved flight summary directly on the dashboard: Flight/Test label, motor, duration, peak altitude, sample rate, samples and quality counts, pressure and temperature ranges, and starting battery voltage when available. The **BIN**, **CSV**, and **TXT** download options remain available. A summary is available only if the recording's companion `.TXT` metadata file was successfully saved; this viewer does not reconstruct missing summaries. Firmware compilation and hardware testing are still required before flight use.

## GitHub builds and phone downloads

Every firmware change on `main` runs the **Build Green Ion firmware** GitHub Actions workflow. The verified `Green_Ion_V10.ino.bin` is kept directly beside the source for easy phone downloads, and the successful workflow also creates the `green-ion-v10-phone-update` backup artifact. Preserve the `.ino` and `.h` together if recompiling from source.

### Install a later update from a phone

1. Install V8.5 or newer over USB once. Earlier firmware does not contain the update endpoint.
2. On the phone, open this repository while connected to the internet.
3. Open **Actions**, select the latest successful **Build Green Ion firmware** run, and download its artifact.
4. Extract the ZIP and keep the file ending exactly in `.ino.bin`. Do not select a bootloader, partitions, or merged binary.
5. Connect the phone to `GREEN-ION`, open `http://192.168.4.1`, and select **System**.
6. Under **Install firmware from this phone**, choose the `.ino.bin` file and press **Install Firmware & Restart**.
7. Keep stable power connected. Wait for the Wi-Fi network to disappear and return, reconnect, and confirm the displayed version.

Firmware installation is blocked while armed, recording, calibrating, simulating, shutting down, or reporting a critical battery. ESP32 OTA writes the inactive application partition first; the running firmware is switched only after the uploaded image finishes successfully.

## License and use

No license has been granted unless a separate license file is added. Use this experimental recorder at your own risk and follow applicable rocketry safety codes and local rules.
