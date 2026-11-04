# GREEN ION V12 first-day hardware test

These are bench tests, not flight approval. Stop if a board, wire, radio, or battery becomes hot; if USB repeatedly disconnects; or if supply polarity is uncertain. Disconnect power before changing wiring. Use a common ground and check for shorts before every power-up.

## 1. Feather, IMU, and GPS first

1. Install the ESP32 board package and select **Adafruit Feather ESP32 V2**. Install **Adafruit BMP3XX Library**, **Adafruit LSM6DS**, **Adafruit GPS Library**, **RadioHead**, **Adafruit BusIO**, and **Adafruit Unified Sensor**. Add the complete local `GreenIonProtocol` and `GreenIonRecorder` libraries.
2. With all external power disconnected, connect only the Feather by USB and upload `Green_Ion_V12/Green_Ion_V12.ino`. Open Serial Monitor at **115200 baud**. The firmware tolerates missing devices and prints a line for each missing component.
3. Unplug USB. Connect the LSM6DSO32 by STEMMA QT to the Feather STEMMA QT socket. This cable test needs no soldering. Reconnect USB. The address should be `0x6A` or `0x6B`.
4. Enter `bench imu`. Once per second, expect `BENCH IMU status=OK range=+-32g rate_hz=104`, acceleration in both m/s² and g, and gyro in both rad/s and degrees/s. Stationary magnitude should be near 1 g; rotate one axis at a time and confirm the corresponding values change. `INIT_FAILED` means check power/cable/address. `READ_FAILED` means initialization succeeded but a later read was invalid; power down and reseat the cable.
5. Unplug USB. Connect the PA1010D on the same I²C/STEMMA QT bus at `0x10`; use a QT splitter or chained QT connector if available. Do not force or reverse a connector. Reconnect USB and enter `bench gps`.
6. Indoors, `BENCH GPS status=NO_FIX` is normal. Move the antenna outdoors with a clear sky view. Success is `status=LIVE fix=1` with satellites, age, latitude, longitude, altitude, and UTC. After fix loss, the last position remains visible as `status=STALE fix=0`; it is never labeled live. `INIT_FAILED` means the I²C device was not detected.

The IMU and GPS cable tests require no permanent soldering. Perfboard mounting, strain relief, and the RFM95W header/antenna connections do require suitable soldered connections before vehicle use.

## 2. First LoRa link

Never power or transmit from an RFM95W without a suitable 915 MHz antenna firmly attached. The provisional 920.5 MHz setting must be coordinated with ROC and checked for legal/interference constraints before field use.

1. Power everything off. Solder the two RFM95W breakouts and their antenna connectors correctly. On the Feather radio use shared SPI SCK 5, MOSI 19, MISO 21, plus CS 27, RST 15, DIO0 32, 3.3 V, and GND. SD uses the same three SPI lines but its own CS 33. Check that both CS lines are distinct and high when idle.
2. Connect the Arduino Uno receiver radio according to `docs/V12_WIRING.md`, attach its antenna, upload `Ground_Receiver/Ground_Receiver.ino`, and open its Serial Monitor at 115200 baud.
3. Attach the Feather radio antenna, upload the V12 Feather sketch, and keep the boards a short bench distance apart. The Uno should print CRC-valid JSON packets with increasing sequence numbers. IMU, pressure, altitude, GPS status, and packet-loss fields should be present even when a sensor is missing.
4. In the Feather Serial Monitor, enter `radio status` and confirm the displayed modem settings match the Uno startup JSON. Then enter `radio test`. The Feather sends five CRC-protected packets marked as a bench test. Success is five Uno lines with `"type":"radio_bench"`, increasing sequence numbers, and an RSSI value.
5. Enter `radio status` again. Confirm `queued=0`, `dropped=0`, and `send_failures=0`. Any nonzero value is diagnostic evidence to record, not a reason to bypass recorder safeguards.
6. If no packet arrives, power down before checking 3.3 V power, grounds, antenna connections, SPI pins, CS/RST/DIO0, and the matching modem settings. Do not increase transmit power as a first troubleshooting step.

## 3. Before combining everything

Test SD logging alone, then LoRa alone, then both together. During the shared-SPI test, confirm that recording continues, sample-miss counters do not climb unexpectedly, files remain readable, and radio failure does not stop the recorder. Physical SD removal, power interruption, GPS performance, sensor accuracy, radio range, and shared-bus timing remain **NOT TESTED** until measured on the actual hardware.
