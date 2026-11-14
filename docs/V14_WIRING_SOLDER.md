# V14 Wiring and Soldering Guide

**Disconnect USB and LiPo before touching wires. Verify voltage and pins against the exact breakouts before soldering.** These are the V14 firmware assignments, not a physically validated harness. An experienced adult/mentor should inspect your finished work.

## Rocket: Adafruit ESP32 Feather V2

| Component | Connect to Feather | Method |
|---|---|---|
| BMP388 barometer (VIN/GND/SDA/SCL) | **3V / GND / SDA GPIO22 / SCL GPIO20** | STEMMA QT cable preferred; solder only if needed |
| LSM6DSO32 IMU (VIN/GND/SDA/SCL) | Same **3V / GND / GPIO22 / GPIO20** | STEMMA QT/daisy chain or hub |
| PA1010D GPS (VIN/GND/SDA/SCL) | Same **3V / GND / GPIO22 / GPIO20** | STEMMA QT/daisy chain or hub |
| microSD SPI breakout SCK/MOSI/MISO | **SCK GPIO5 / MO GPIO19 / MI GPIO21** | Solder or secured SPI connector |
| microSD CS | **D33 / GPIO33** | Dedicated chip select |
| Adafruit RFM95W SPI SCK/MOSI/MISO | **GPIO5 / GPIO19 / GPIO21** | Shares SPI with SD |
| Adafruit RFM95W CS | **D27 / GPIO27** | Dedicated chip select |
| Adafruit RFM95W RST | **D15 / GPIO15** | Solder/headers |
| Adafruit RFM95W G0/DIO0 | **D32 / GPIO32** | Solder/headers |
| Adafruit RFM95W VIN/GND | **3V / GND only if that exact breakout supports 3.3 V power** | Confirm module specifications |
| Optional buzzer driver input | **D14 / GPIO14** through correctly rated transistor/resistor stage | Never connect buzzer directly to GPIO |
| LiPo | Feather rated battery JST socket | Do not solder battery power to GPIO |

The three I²C sensors share one SDA/SCL bus. A compatible STEMMA QT hub or daisy-chain replaces multiple soldered data wires. All components require a common ground. Firmware enables the switched STEMMA QT rail using NEOPIXEL_I2C_POWER / GPIO2. **Verify combined 3.3 V regulator load before powering several accessories.**

**Do not apply 5 V Uno logic to a bare 3.3 V RFM95W module.** Bare radio modules and Adafruit breakouts with regulator/level shifting are not interchangeable.

## Ground receiver: Arduino Uno R3 + Adafruit RFM95W breakout

| Adafruit RFM95W breakout | Uno |
|---|---|
| VIN | 5V **only for the Adafruit regulated breakout** |
| GND | GND |
| SCK | D13 |
| MOSI | D11 |
| MISO | D12 |
| CS | D4 |
| RST | D2 |
| G0 / DIO0 | D3 (hardware interrupt) |

The Uno connects to your Windows computer through USB. It is **not wired to the rocket**. Attach a proper antenna before radio transmission. Current firmware uses provisional 920.5 MHz; coordinate frequencies and legal power levels with the launch club before operation.

## Safe assembly order

1. Verify breakout model numbers and each voltage pin with the official pinout. With power disconnected, check no shorts between 3V and GND.
2. Test BMP388, LSM6DSO32 and PA1010D **one at a time** on the I²C bus. Expected default addresses: BMP388 0x76/0x77; IMU 0x6A/0x6B; GPS 0x10.
3. Solder the microSD lines and dedicated **CS D33**. Test SD read/write without radio first.
4. Solder the rocket LoRa SPI and **CS D27, RST D15, DIO0 D32**. Keep both chip-select lines separate and inactive when the other device uses SPI.
5. Wire/solder the *separate* Uno breakout per the table. Confirm the breakout includes correct logic-level shifting and regulator.
6. If included, build the buzzer transistor driver only after checking the actual buzzer's voltage and current; use a flyback diode if the buzzer is inductive.
7. Inspect polarity, joint bridges, strain relief and insulation. Secure wires against acceleration, vibration and metal contact.
8. Bench-test sensor readings, stable LiPo voltage, GPS fix, LoRa reception, SD write/flush, recording during Wi-Fi loss, OTA rollback and full home tests before launching.

**Not provided by these instructions:** a validated buzzer resistor/transistor value (depends on exact buzzer), antenna certification, current budget, or proof of mechanical flight safety. No ignition or deployment wiring is part of GREEN ION.

Sources: [Adafruit Feather V2 pinouts](https://learn.adafruit.com/adafruit-esp32-feather-v2/pinouts), [Adafruit LSM6DSO32](https://learn.adafruit.com/lsm6dsox-and-ism330dhc-6-dof-imu/arduino), [PA1010D](https://learn.adafruit.com/adafruit-mini-gps-pa1010d-module), [Adafruit LoRa breakout](https://learn.adafruit.com/adafruit-rfm69hcw-and-rfm96-rfm95-rfm98-lora-packet-padio-breakouts/arduino-wiring), [V14 Config.h](../Past_Versions/V14/Green_Ion_V14/Config.h).
