# GREEN ION V12

**ESP32 model-rocket flight recorder and recovery aid — hardware validation in progress**

GREEN ION V12 records pressure and synchronized full-resolution IMU/GPS data to microSD, provides a local phone dashboard, and can send selected CRC-protected telemetry to an Arduino Uno ground receiver. It is strictly a recorder and recovery aid: it never controls ignition, staging, arming, pyrotechnics, or deployment.

V12 host tests and target compilation pass. Physical sensor, SD/power-loss, shared-SPI, radio-range, recovery, and flight testing are still required before use in a rocket.

## Current projects

- [`Green_Ion_V12/Green_Ion_V12.ino`](Green_Ion_V12/Green_Ion_V12.ino) — Adafruit ESP32 Feather V2 recorder firmware.
- [`Ground_Receiver/Ground_Receiver.ino`](Ground_Receiver/Ground_Receiver.ino) — **Arduino Uno R3 receiver firmware**.
- [`Ground_Station/`](Ground_Station/) — Windows/Python telemetry dashboard, simulation, logging, and GIS1 export tools.
- [`libraries/GreenIonProtocol/`](libraries/GreenIonProtocol/) — shared packet and radio contract.
- [`libraries/GreenIonRecorder/`](libraries/GreenIonRecorder/) — portable recorder, recovery, format, and hardware-test modules.
- [`Past_Versions/Green_Ion_V11/`](Past_Versions/Green_Ion_V11/) — complete archived V11 release candidate for rollback.

## Feather hardware

- Adafruit ESP32 Feather V2
- BMP388 barometer
- LSM6DSO32 accelerometer/gyroscope on STEMMA QT/I²C
- PA1010D GPS at I²C address `0x10`
- 3.3 V SPI microSD module
- Optional RFM95W LoRa radio with a suitable antenna

The firmware raises the Feather's `NEOPIXEL_I2C_POWER` rail before starting I²C. See [`docs/V12_WIRING.md`](docs/V12_WIRING.md) before connecting hardware.

## Upload V12 to the Feather

1. Install Arduino IDE 2 and the **esp32 by Espressif Systems** board package.
2. Select **Adafruit Feather ESP32 V2** and the Feather's COM port.
3. Install Adafruit BMP3XX, Adafruit LSM6DS, Adafruit GPS, RadioHead, Adafruit BusIO, and Adafruit Unified Sensor.
4. Add the complete `libraries/GreenIonProtocol` and `libraries/GreenIonRecorder` folders to the Arduino library search path.
5. Open [`Green_Ion_V12/Green_Ion_V12.ino`](Green_Ion_V12/Green_Ion_V12.ino) and upload over USB.
6. Open Serial Monitor at **115200 baud**.

## Test IMU and GPS without LoRa

### Phone or laptop

1. Power the Feather by USB.
2. Join Wi-Fi **`GREEN-ION-V12`** using password **`GreenIon2026`**.
3. Open **http://192.168.4.1/**.
4. Choose **Test IMU**, **Test GPS**, or **Test All Sensors**.

Keep the Feather motionless for the five-second IMU test. The GPS test distinguishes I²C/NMEA communication from obtaining a current outdoor fix; last-known coordinates are labeled stale after signal loss. Tests cannot run during active recording.

### USB Serial

Enter `test imu`, `test gps`, `test all`, or `test status`. Use `bench imu`, `bench gps`, or `bench all` for continuous live values. No LoRa radio or Uno is required for these sensor tests.

Detailed instructions: [`docs/V12_FIRST_DAY_TEST.md`](docs/V12_FIRST_DAY_TEST.md).

## Arduino Uno receiver

The Uno code is included at [`Ground_Receiver/Ground_Receiver.ino`](Ground_Receiver/Ground_Receiver.ino). Select **Arduino Uno**, install RadioHead and the local `GreenIonProtocol` library, then upload. Attach suitable antennas before any radio transmission. The shared 920.5 MHz setting remains provisional and must be coordinated with the Rocketry Organization of California before field use; the frequency alone does not establish FCC compliance or freedom from interference.

## Build and evidence

GitHub Actions compiles both the Feather V2 recorder and Arduino Uno receiver and runs the Python/native C++ regression suites. A successful build is software evidence, not a physical hardware test.

- [GitHub Actions](https://github.com/aidenjantzi1-ctrl/GREEN-ION/actions)
- [`docs/V12_TESTING.md`](docs/V12_TESTING.md)
- [`docs/V12_DEVELOPMENT_LOG.md`](docs/V12_DEVELOPMENT_LOG.md)
- [`VERSIONS.md`](VERSIONS.md)

## Safety

Secure all wiring, batteries, SD cards, and antennas mechanically. Stop testing if anything becomes hot or USB repeatedly disconnects. Perform measured ground tests and a documented readiness review before flight. Follow the rocket manufacturer's instructions and applicable NAR safety rules.
