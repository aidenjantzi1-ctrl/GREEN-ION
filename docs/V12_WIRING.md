# GREEN ION V12 preliminary wiring

This is the starting design, not a hardware-validated harness. Keep I2C leads short, use a common ground, add strain relief, and bench-test every device individually before combining them.

## Rocket recorder — Adafruit ESP32 Feather V2

| Component | Connection |
|---|---|
| BMP388 | I2C SDA GPIO22, SCL GPIO20, 3.3 V, GND |
| LSM6DSO32 | I2C SDA GPIO22, SCL GPIO20, 3.3 V, GND |
| PA1010D | I2C SDA GPIO22, SCL GPIO20, 3.3 V, GND |
| microSD | SCK GPIO5, MOSI GPIO19, MISO GPIO21, CS GPIO33 |
| RFM95W breakout | SCK GPIO5, MOSI GPIO19, MISO GPIO21, CS GPIO27, RST GPIO15, G0/DIO0 GPIO32 |
| active buzzer driver | GPIO14 to transistor base/gate network; buzzer uses an appropriate supply and common ground |
| LiPo monitor | onboard divider at GPIO35 |

The Feather V2 STEMMA QT rail is switched by `NEOPIXEL_I2C_POWER`/GPIO2. V12 drives it high before starting `Wire`. The board's documented hardware buses are SDA 22/SCL 20 and SCK 5/MOSI 19/MISO 21. The proposed I2C devices do not conflict at their normal addresses: BMP388 normally 0x76/0x77, LSM6DSO32 0x6A/0x6B, PA1010D 0x10.

SD and LoRa share hardware SPI but have separate chip selects. Both CS lines are configured as outputs and driven high before SPI initialization. Each transaction must leave the inactive device deselected. Reliable coexistence, card MISO tri-state behavior, supply droop, and radio interference still require oscilloscope/logic-analyzer and physical stress testing.

Do not power an active buzzer directly from GPIO14. Use a transistor driver, current-limiting/base resistor as appropriate, and a flyback diode if the selected sounder is inductive. The buzzer default is controlled by configuration and must never block logging.

## Arduino Uno R3 receiver

These power connections apply specifically to the Adafruit RFM95W breakout with onboard regulator and level shifting, not a bare radio module.

| RFM95W breakout | Uno R3 |
|---|---|
| VIN | 5 V |
| GND | GND |
| SCK | D13 |
| MOSI | D11 |
| MISO | D12 |
| CS | D4 |
| RST | D2 |
| G0/DIO0 | D3 (interrupt capable) |

Attach a correct 900 MHz antenna before transmitting. Never transmit without an antenna.

Both boards import one shared radio contract from `libraries/GreenIonProtocol/src/GreenIonRadioConfig.h`: provisional 920.5 MHz center frequency, 125 kHz bandwidth, spreading factor 7, coding rate 4/5, and 13 dBm transmitter power. Do not change one board independently. The receiver applies the same modem and power fields so configuration drift is visible in source even though it does not normally transmit.

## Source verification

- Adafruit Feather ESP32 V2 pinout: https://learn.adafruit.com/adafruit-esp32-feather-v2/pinouts
- Adafruit LSM6DSO32 Arduino guide: https://learn.adafruit.com/lsm6dsox-and-ism330dhc-6-dof-imu/arduino
- Adafruit PA1010D guide: https://learn.adafruit.com/adafruit-mini-gps-pa1010d-module
- Adafruit RFM9x Arduino wiring: https://learn.adafruit.com/adafruit-rfm69hcw-and-rfm96-rfm95-rfm98-lora-packet-padio-breakouts/arduino-wiring

Before field use, verify frequency, power, antenna, bandwidth, spreading factor, coding rate, airtime, duty cycle, and operator/location requirements against current applicable radio rules. ROC's Lucerne guidance currently asks unlicensed low-power ISM/LoRa users to operate from 918 through 926.875 MHz; 920.5 MHz is only a provisional development choice within that range. Coordinate with ROC before launch. Being inside the 902–928 MHz band does not by itself establish FCC compliance, authorization, or an interference-free channel.

- ROC rocket tracking coordination: https://rocstock.org/rocket-tracking/
- FCC equipment authorization overview: https://www.fcc.gov/engineering-technology/laboratory-division/general/equipment-authorization

