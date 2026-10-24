# GREEN ION V8.3

GREEN ION V8.3 is experimental firmware for an Adafruit ESP32 Feather V2 flight-data recorder. It reads a BMP388 barometric sensor, serves a phone-friendly dashboard from its own Wi-Fi access point, and writes RKT4-compatible binary flight logs to an SPI microSD card.

> **Safety scope:** GREEN ION is a data recorder only. It must never control ignition, staging, parachute deployment, or any recovery system.

## Features

- Non-blocking pressure calibration with live progress feedback
- Dashboard shutdown control using ESP32 deep sleep; press physical RESET to restart
- Sequential `FLT0001` through `FLT9999` files with no four-flight limit
- Flight labels and motor designations saved in companion summaries
- Original BIN, analysis-ready CSV, and summary downloads directly to a phone
- Postflight peak, pressure, temperature, duration, and sample-quality statistics
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

Open `Green_Ion_V8_3/Green_Ion_V8_3.ino`, select the correct serial port, and upload. Open Serial Monitor at **115200 baud** for startup and diagnostic messages.

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

1. Power the Feather and wait for `GREEN ION V8.3` in Serial Monitor.
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
6. Enter a flight label and motor designation, then save the flight details.
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

## Files

- `Green_Ion_V8_3/Green_Ion_V8_3.ino` — latest V8.3 firmware and embedded dashboard
- `Green_Ion_V8_2/Green_Ion_V8_2.ino` — archived V8.2 firmware
- `Green_Ion_V8_1/Green_Ion_V8_1.ino` — archived V8.1 firmware
- `Green_Ion_V8/Green_Ion_V8.ino` — archived V8 firmware

## GitHub builds and phone downloads

Every firmware change on `main` runs the **Build Green Ion firmware** GitHub Actions workflow. Open the latest successful workflow run on GitHub and download its firmware artifact to obtain both the `.ino` source and compiled `.bin` files. V8.3 still requires a USB/Arduino upload. A later release adds the dashboard endpoint that can install the compiled `.bin` from a phone.

## License and use

No license has been granted unless a separate license file is added. Use this experimental recorder at your own risk and follow applicable rocketry safety codes and local rules.
