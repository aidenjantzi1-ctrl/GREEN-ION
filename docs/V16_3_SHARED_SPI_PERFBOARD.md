# GREEN ION V16.3 — shared SPI perfboard and physical validation

**Status:** software architecture implemented; assembled perfboard validation is **REQUIRES PHYSICAL TEST**.

GREEN ION uses one ESP32 SPI bus for the microSD card and the RFM9x LoRa transmitter. That is intentional. SCK, MOSI and MISO are shared; each peripheral has its own chip-select signal. The barometer, IMU and GPS are on the separate I²C bus and do not use these SPI data wires.

## Verified configured Feather pins

| Function | Feather ESP32 V2 GPIO |
|---|---:|
| SPI SCK | 5 |
| SPI MOSI | 19 |
| SPI MISO | 21 |
| microSD CS | 33 |
| RFM9x CS / NSS | 27 |
| RFM9x RESET | 15 |
| RFM9x DIO0 | 32 |

V16.3 does **not** change these assignments. They come from `Green_Ion_V16_3/Config.h`.

## What “shared SPI” means

- **SCK, MOSI and MISO are common bus wires.** Both SD and RFM9x connect to the same three Feather signals.
- **CS is separate.** GPIO 33 selects the SD card. GPIO 27 selects the RFM9x.
- **Only one SPI peripheral may own bus transactions at a time.** The inactive device is deselected while the active device’s library controls its own chip-select and SPI transaction details.
- **RFM9x DIO0 is not an SPI data line.** It is the radio interrupt/status line on GPIO 32.
- **I²C is separate.** BMP388, LSM6DSO32 and the configured GPS use SDA/SCL and are not selected with these SPI chip-select pins.

## V16.3 software coordination

Recording is the highest priority.

1. Sensor sampling and pending samples.
2. Primary RKT4 BIN write/flush.
3. Required recovery checkpoint.
4. GIS1/SEN companion work.
5. LoRa telemetry.
6. Background diagnostics and maintenance.

V16.3 adds a small shared-bus coordinator with SD and RADIO ownership scopes. It is intentionally nonblocking: if radio work would collide with SD ownership or approach the next sensor deadline, radio work is deferred rather than delaying the recorder.

The RFM9x DIO0 interrupt is also changed from the normal RadioHead behavior. The interrupt routine only records that DIO0 fired. It does **not** perform SPI. RadioHead’s protected interrupt handler is serviced later from the normal Feather loop while the RADIO scope owns the bus. This prevents asynchronous RFM9x register access from cutting into an SD transaction.

RadioHead’s own RH_RF95 source documents that its normal DIO0 handler reads/writes radio registers through SPI, and `send()` starts transmit before TxDone completes. That is why V16.3 separates **TX started** from **TX completed** and only counts a bench packet as sent after TxDone is serviced.

## Perfboard electrical inspection — before power

This is a checklist for you; the repository cannot inspect the physical board.

- Motors are removed from the rocket/test setup.
- Feather power is disconnected.
- Check common ground between Feather, SD breakout and RFM9x.
- Confirm SCK from both peripherals reaches GPIO 5.
- Confirm MOSI from both peripherals reaches GPIO 19.
- Confirm MISO from both peripherals reaches GPIO 21.
- Confirm SD CS goes only to GPIO 33.
- Confirm RFM9x CS/NSS goes only to GPIO 27.
- Confirm RFM9x RESET goes to GPIO 15.
- Confirm RFM9x DIO0 goes to GPIO 32.
- Check for solder bridges between adjacent perfboard pads.
- Check that no chip-select line is shorted to SCK/MOSI/MISO.
- Check power polarity before connecting the LiPo or USB.
- The RFM9x is a 3.3 V device. Do not drive it from unshifted 5 V logic.
- The Arduino Uno is 5 V logic; the Uno-side RFM9x wiring must use appropriate 3.3 V-safe interfacing where required.
- Both radios must have suitable antennas attached before intentional transmission.
- Add strain relief so antenna/coax and perfboard wires cannot pull on solder joints.

References:
- Adafruit Feather ESP32 V2 pinouts: https://learn.adafruit.com/adafruit-esp32-feather-v2/pinouts
- Adafruit RFM9x guide: https://learn.adafruit.com/adafruit-rfm69hcw-and-rfm9x-lora-packet-radio-breakouts
- Arduino SPI reference: https://docs.arduino.cc/language-reference/en/functions/communication/SPI/

## Full Home Test shared-bus check

Use **Testing → Run Full Home Test** with motors removed.

V16.3’s 30-second temporary recorder step:

1. Creates the reserved `/GIHOME.BIN`.
2. Records real pressure samples at 50 Hz.
3. Schedules a bounded set of eight RFM9x bench telemetry packets during the same recording interval when the Feather radio initialized successfully.
4. Keeps primary SD work ahead of radio service.
5. Reads the temporary RKT4 file back.
6. Checks header/timestamps/sample count/valid-sample ratio.
7. Checks missed sample periods against the existing test tolerance.
8. Checks software shared-bus ownership errors and radio TxDone timeouts.
9. Deletes the temporary file when verification succeeds.
10. Restarts stationary calibration after the home-test session.

On the Windows app, actual Uno receiver evidence is separate. A receiver PASS requires actual received bench packets. If no receiver is connected, the app must show **NOT TESTED — LoRa receiver not connected** rather than PASS.

A Feather-side shared-SPI PASS means the software-observed concurrent scheduler/temporary recording test passed. It does **not** prove RF range, electrical signal integrity, solder quality or power margin.

## Required physical shared-SPI stress test

Do this only on a safe bench with motors removed, suitable antennas installed and stable power.

### Setup

- Feather ESP32 V2 with the actual perfboard.
- The actual microSD card intended for use.
- The actual Feather-side RFM9x.
- Arduino Uno + matching RFM9x receiver.
- Windows ground station with Uno USB connected.
- Stable USB power plus the intended protected 1S LiPo arrangement.
- Keep the payload stationary unless a step explicitly asks for movement.

### Test A — one-button concurrent check

Run **Run Full Home Test**.

Record:

- `home_shared_spi_result`
- temporary recording sample count
- valid sample count
- missed sample periods
- shared-SPI ownership errors
- radio Tx start/completion/timeout counts
- telemetry queue drops
- actual Uno bench packets received
- RSSI and SNR

Do not mark the receiver PASS unless the Uno actually received the expected packets.

### Test B — sustained recorder + radio

Perform a controlled bench recording long enough to exercise:

- 50 Hz sensor acquisition
- BIN writes
- SEN writes
- periodic BIN flushes
- checkpoints
- normal LoRa telemetry
- temporary Windows/Feather Wi-Fi loss
- Uno USB disconnect/reconnect once, if safe

Afterward collect:

- original BIN and SEN files
- SHA-256 of originals after download
- RKT4/GIS1 parser results
- total and valid samples
- timestamp gaps / missed sample periods
- sensor-log dropped records
- SD/checkpoint/finalization failures
- radio packets accepted
- inferred packet gaps
- duplicates/out-of-order packets
- RSSI/SNR
- radio recovery attempts/successes
- reset reason / boot-session change
- any brownout evidence

### Test C — fault isolation

With motors removed and only when safe:

- Start with receiver absent: SD recording must still work.
- Start with Feather radio unavailable: SD recording must still be the primary behavior and radio should report unavailable.
- Reconnect/recover the radio only when the firmware allows a conservative retry.
- Test an SD-unavailable startup separately: the app must clearly report that primary recording is unavailable; it must never pretend recording succeeded.
- Do **not** remove an SD card while a real recording file is open merely to force a failure.

## Acceptance evidence

Software/CI alone cannot mark the physical shared-SPI requirement PASS.

A human reviewer can consider the assembled shared-bus test successful only after retained evidence shows, at minimum:

- valid original flight/test files
- expected checksums after transfer
- acceptable sample timing for the chosen test
- no unexplained ownership errors
- no reset/brownout during the test
- receiver-side packet evidence when LoRa is being evaluated
- no evidence that radio recovery damaged or restarted the recorder

Any unexplained corruption, reset, sustained sample loss or SD failure is a **FAIL / investigate**, not something to average away.

## Rollback

V16.2 source and release remain available as a separate versioned generation. Windows app rollback is independent of Feather OTA rollback. Never update firmware automatically during a mission.
