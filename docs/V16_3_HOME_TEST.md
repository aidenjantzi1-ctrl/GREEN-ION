# GREEN ION V16.3 — Full Home Test

Use **Testing → Run Full Home Test** as the normal bench check. Remove all rocket motors before starting.

The test intentionally separates what software can measure from what still needs a human or physical test.

## What the one-button workflow checks

Where hardware/evidence is available, GREEN ION checks:

- fresh Feather connection and measured battery voltage;
- BMP388/barometer status and stationary calibration;
- LSM6DSO32 communication and stationary IMU/gyro evidence;
- GPS communication and a current fix when available outdoors;
- microSD write/read/delete verification using reserved temporary files;
- RFM9x initialization;
- bounded LoRa bench-packet transmission;
- **actual** Arduino Uno receiver packets when the Windows receiver is connected;
- RSSI/SNR and receiver packet evidence when present;
- a 30-second 50 Hz temporary RKT4 recording;
- concurrent temporary SD recording + bounded LoRa scheduling on the shared SPI bus;
- sample timing/missed-period counters;
- temporary-file cleanup;
- restart of stationary calibration after the test.

## Receiver truth rule

Feather transmission by itself is not receiver PASS.

- **PASS** requires actual expected bench packets observed through the connected Uno/Windows receiver.
- **NOT TESTED** is shown when the receiver is absent.
- **INCONCLUSIVE** is used when transmission ran but expected receiver evidence was not confirmed.
- RF range is a separate physical measurement even if the local bench receiver passes.

## Shared-SPI truth rule

A software shared-SPI PASS means the scheduler/temporary-recording evidence met its software checks. It does not prove perfboard soldering, electrical margins or RF performance.

For the assembled-hardware stress test, follow [V16.3 shared SPI perfboard validation](V16_3_SHARED_SPI_PERFBOARD.md).

## If something fails

- **SD unavailable:** check card seating, power and SD CS/wiring. Do not launch without primary recording.
- **LoRa unavailable:** check both antennas, RFM9x power, CS/reset/DIO0 wiring and matching settings. The recorder remains independent.
- **Uno not detected:** reconnect USB, close other serial-monitor programs and let the Windows app rediscover it.
- **GPS no fix:** communication may still work; take the GPS outdoors with a clear sky and repeat.
- **IMU/barometer failure:** inspect the STEMMA QT/I2C path and rerun while stationary.
- **Missed samples / SPI ownership error / radio timeout:** treat the run as a failure or investigation item; do not average it away.

Home Test is bench evidence, not launch authorization.
