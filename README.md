# GREEN ION

**ESP32-based model rocket flight recorder**

GREEN ION records flight data to a microSD card and provides a local Wi-Fi dashboard for setup, monitoring, and reviewing flights. The current firmware is **V11 (release candidate; hardware validation pending)**.

**GREEN ION is a data recorder only.** It does not control ignition, staging, or recovery deployment. Test the hardware and verify the rocket's stability and recovery system before flight.

## Current hardware (V11)

- Adafruit ESP32 Feather V2
- BMP388/BMP390 barometric pressure sensor (I²C)
- 3.3 V-compatible SPI microSD module and FAT/FAT32 card
- Single-cell 3.7 V LiPo battery

## What V11 does

- Measures pressure, temperature, and barometric altitude.
- Records flight data to microSD with selectable 10, 25, or 50 Hz sampling.
- Serves a phone-friendly dashboard over its own `GREEN-ION` Wi-Fi network.
- Provides calibration, recorder controls, SD-card checks, saved-flight statistics, and BIN/CSV/TXT downloads.
- Supports printable flight reports, simulated dashboard tests, and firmware updates from the dashboard.
- Estimates acceleration from barometric altitude; **it does not currently measure true 3-axis acceleration or GPS position**.

The firmware is experimental. A successful build does not establish flight readiness.

## Get started

1. Install the [Arduino IDE](https://www.arduino.cc/en/software) and the Espressif ESP32 board package.
2. In Boards Manager, select **Adafruit ESP32 Feather V2**.
3. Install **Adafruit BMP3XX Library**, **Adafruit Unified Sensor**, and **Adafruit NeoPixel** from Library Manager.
4. Open [`Green_Ion_V11/Green_Ion_V11.ino`](Green_Ion_V11/Green_Ion_V11.ino), keeping `Dashboard.h` in the same folder.
5. Connect the Feather by USB, select its port, and upload. Use Serial Monitor at **115200 baud** for diagnostics.
6. Insert the SD card, connect to the GREEN ION dashboard, and perform an SD-card test and a short ground recording before relying on the device.

### Wiring

**BMP388/BMP390 — I²C**

| Sensor | Feather V2 |
| --- | --- |
| VIN | 3V |
| GND | GND |
| SCL / SCK | SCL |
| SDA / SDI | SDA |

The firmware checks sensor addresses `0x77` and `0x76`.

**microSD — SPI**

| microSD module | Feather V2 |
| --- | --- |
| VCC | 3V |
| GND | GND |
| SCK | SCK (GPIO 5) |
| MOSI | MOSI (GPIO 19) |
| MISO | MISO (GPIO 21) |
| CS | GPIO 33 |

Use a module compatible with **3.3 V logic**. Secure the card, battery, and all wiring mechanically for flight.

## Connect to the dashboard

1. Power on GREEN ION.
2. Connect your phone or computer to the **`GREEN-ION`** Wi-Fi network.
3. Open **http://192.168.4.1**.
4. Use the dashboard to check the sensor and SD card, calibrate altitude, choose a recording rate, arm the **recorder**, and start recording.

The network has no internet access; this is normal. The recorder continues running when the phone disconnects.

After recovering the rocket, stop and save the recording if it has not stopped automatically. Open **Storage** to view statistics and download logs.

### Phone firmware update

[Download the V11 firmware binary](https://raw.githubusercontent.com/aidenjantzi1-ctrl/GREEN-ION/main/Green_Ion_V11/Green_Ion_V11.ino.bin) after confirming the matching [GitHub Actions build](https://github.com/aidenjantzi1-ctrl/GREEN-ION/actions) succeeded.

While connected to the dashboard, open **System**, select the downloaded `.ino.bin` file, then choose **Install Firmware & Restart**. Use stable power; do not update while recording. First-time installation requires USB.

## GREEN ION V12 — planned

V12 is an upgrade project, **not a verified or flight-ready release**. Planned additions are:

| Component | Purpose |
| --- | --- |
| Adafruit LSM6DSO32 | Three-axis acceleration and rotation measurements |
| Adafruit PA1010D GPS | Position data for recovery |
| Adafruit RFM95W LoRa radios (two) | Send selected live telemetry to a ground receiver |
| Arduino Uno R3 | Receive LoRa data and forward it to a laptop by USB |
| Active buzzer | Help locate the rocket after landing |

The plan is to keep detailed measurements on microSD while using LoRa for selected pressure/IMU telemetry and GPS recovery information. Hardware fit, antenna placement, battery retention, radio compliance, stability, and ground/flight testing are still required. None of these planned features should be treated as working until implemented and validated.

## Repository

- [V11 source](Green_Ion_V11/) — current release candidate
- [Past versions](Past_Versions/) — archived firmware
- [Version history](VERSIONS.md)
- [Changelog](CHANGELOG.md)
- [GitHub Actions](https://github.com/aidenjantzi1-ctrl/GREEN-ION/actions) — build status

## Safety

GREEN ION must remain independent of motor ignition, staging, and recovery deployment. Follow the rocket manufacturer's instructions and applicable [NAR safety codes](https://www.nar.org/), and have the finished flight configuration inspected before launch.

Hardware testing, secure installation, and correct recovery equipment are required regardless of software test results.
