# GREEN ION V8.7

GREEN ION V8.7 is experimental firmware for an Adafruit ESP32 Feather V2 flight-data recorder. It reads a BMP388 barometric sensor, serves a phone-friendly dashboard from its own Wi-Fi access point, and writes RKT4-compatible binary flight logs to an SPI microSD card.

> **Safety scope:** GREEN ION is a data recorder only. It must never control ignition, staging, parachute deployment, or any recovery system.

## Current phone dashboard changes

The **Storage** page shows saved flight summaries and statistics directly, without a download. **View stats** opens a selected flight; the latest saved flight is shown automatically. The Overview tab has a recorder snapshot and a direct shortcut to the saved flights. The **Detect & Test SD Card** button automatically requests control when available and displays a specific success or failure message; it writes and removes only a temporary test file. Use this test before each flight. On the documented wired microSD breakout the firmware now explicitly uses SCK GPIO5, MISO GPIO21, MOSI GPIO19, and CS GPIO33.

**Before flying:** confirm your physical SPI wiring, wait for a successful GitHub Actions build, install the new firmware, and verify SD read/write, test recording, CSV/TXT downloads and battery power on the actual Feather. Source changes alone do not verify hardware operation.

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

Open `Green_Ion_V8_7/Green_Ion_V8_7.ino`, select the correct serial port, and upload. Open Serial Monitor at **115200 baud** for startup and diagnostic messages. The first installation of V8.5 or newer must be performed over USB; afterward, later GitHub-built updates can be installed from the dashboard on a phone.

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

1. Power the Feather and wait for `GREEN ION V8.7` in Serial Monitor.
2. On a phone, tablet, or computer, open Wi-Fi settings.
3. Join **GREEN-ION**.
4. Enter the password **GreenIon2026**.
5. If the device warns that the network has no internet, choose to stay connected. This is expected: GREEN ION is a local device network and does not provide internet access.
6. Open **http://192.168.4.1** in a browser.

## Preflight and recording

1. Confirm that the BMP388 is healthy and the dashboard is receiving live readings.
2. Press **Calibrate** while the recorder is stationary. The firmware averages valid pressure samples and sets the current height to zero.
3. Confirm the calibration success message and verify that live altitude is near zero.
4. If recording, insert the microSD card and run the SD diagnostic. Confirm that storage is verified and that the battery is not critical.
5. Press **Arm**.
6. Select Flight or Test for the next automatic recording label, choose a motor designation, select 10, 25, or 50 samples per second, and save the settings before arming. Use 25 Hz as the normal balance; use 50 Hz when fast-motion detail matters and the SD wiring has tested reliably.
7. Press **Start Recording**. The firmware creates an RKT4 binary log and metadata summary on the card.
8. Press **Stop & Save** before removing power or the card. From Storage, download the original BIN, a CSV for spreadsheet analysis, and the text summary.

Keep a separate, independently tested recovery system. Test the recorder on the ground before every use.

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

- [Full version index (V1–V8.7)](VERSIONS.md) — lists each version and original source availability.
- [Complete changelog](CHANGELOG.md) — documented changes and evidence limitations.
- [Historical records V1–V7](history/) — descriptive archive only; no original executable source is available.

### Preserved firmware sketches

- [V8 source](Green_Ion_V8/Green_Ion_V8.ino) — preserved Arduino sketch
- [V8.1 source](Green_Ion_V8_1/Green_Ion_V8_1.ino) — preserved Arduino sketch
- [V8.2 source](Green_Ion_V8_2/Green_Ion_V8_2.ino) — preserved Arduino sketch
- [V8.3 source](Green_Ion_V8_3/Green_Ion_V8_3.ino) — preserved Arduino sketch
- [V8.4 source](Green_Ion_V8_4/Green_Ion_V8_4.ino) — preserved Arduino sketch
- [V8.5 source](Green_Ion_V8_5/Green_Ion_V8_5.ino) — preserved Arduino sketch
- [V8.6 source](Green_Ion_V8_6/Green_Ion_V8_6.ino) — preserved Arduino sketch
- [V8.7 source](Green_Ion_V8_7/Green_Ion_V8_7.ino) — preserved Arduino sketch (latest)

**Important:** Source snapshots begin at V8. Earlier historical records must not be confused with working firmware. GitHub commit dates in October 2026 are preservation dates, not necessarily prototype development dates.

## View flight statistics on your phone

After a recording has been saved to microSD, connect to the `GREEN-ION` Wi-Fi dashboard and open **Storage → Flight files**. Tap **View stats** next to a recording to see the saved flight summary directly on the dashboard: Flight/Test label, motor, duration, peak altitude, sample rate, samples and quality counts, pressure and temperature ranges, and starting battery voltage when available. The **BIN**, **CSV**, and **TXT** download options remain available. A summary is available only if the recording's companion `.TXT` metadata file was successfully saved; this viewer does not reconstruct missing summaries. Firmware compilation and hardware testing are still required before flight use.

## GitHub builds and phone downloads

Every firmware change on `main` runs the **Build Green Ion firmware** GitHub Actions workflow. Open the latest successful workflow run and download the `green-ion-v8.7-phone-update` artifact. It contains the complete `.ino` source and the main `Green_Ion_V8_7.ino.bin` firmware image.

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
