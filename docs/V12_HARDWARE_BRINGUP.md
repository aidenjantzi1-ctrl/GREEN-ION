# GREEN ION V12 hardware bring-up

Do not start with the full stack. Use current limiting where appropriate, keep the radio antenna attached, and record board/library versions plus every measured result.

1. **Visual and continuity inspection** — confirm orientation, solder joints, common ground, shorts, chip selects, transistor pinout, and antenna connection before power.
2. **Feather only** — upload a minimal serial sketch, confirm USB stability and onboard LiPo reading against a multimeter.
3. **I2C power and scan** — enable GPIO2, scan the bus, then add BMP388, LSM6DSO32, and PA1010D one at a time. Expected normal addresses are 0x76/0x77, 0x6A/0x6B, and 0x10.
4. **Sensor examples** — run the vendor examples before V12. Verify pressure response, each IMU axis/range, and an outdoor GPS fix with valid age/time/satellite data.
5. **SD only** — sustain logging above the target rate, measure write/flush latency, and inspect the resulting file after clean shutdown.
6. **LoRa pair only** — compile the receiver, verify CRC-valid sequence progression at short range and conservative power, then introduce packet loss deliberately.
7. **Shared SPI** — log continuously while transmitting at 1 Hz. Measure sample gaps, write failures, radio errors, supply voltage, and SPI chip-select behavior.
8. **Buzzer driver** — verify current and waveform through the transistor stage. Confirm activation does not alter sample cadence or corrupt storage.
9. **Fault injection** — disconnect GPS/radio/sensors only in a controlled bench setup; test SD removal and power interruption with expendable data and a documented procedure.
10. **Integrated endurance** — run longer than the intended mission duration, review every health counter and file, then repeat on battery power.

Stop on overheating, unexpected current, brownout resets, corrupt files, unexplained sample gaps, or unsafe RF behavior. None of these steps has been physically performed as part of the pre-hardware implementation.

