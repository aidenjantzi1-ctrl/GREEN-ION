# GREEN ION V13 — Unified Wi-Fi + LoRa Rocket Flight Recorder

**Status: V13 software beta; real Feather/Uno wiring, radio and rocket flight validation pending.** Automated tests and firmware compilation do not approve a launch. No ignition, staging or deployment control is provided.

## DOWNLOAD THE COMPLETE V13 PACKAGE

**[One ZIP download — GREEN ION V13 Releases](https://github.com/aidenjantzi1-ctrl/GREEN-ION/releases)**

Download **GREEN_ION_V13_COMPLETE.zip** from the V13 beta release after its successful main-branch build. During development, open the latest successful [V13 Actions run](https://github.com/aidenjantzi1-ctrl/GREEN-ION/actions/workflows/build-v13.yml) and select the **GREEN_ION_V13_COMPLETE** artifact (GitHub sign-in may be required).

**[Start here](docs/V13_START_HERE.md)** · **[How to update V11/V12 safely](docs/V13_UPDATE_GUIDE.md)** · **[V13 technical plan](docs/V13_IMPLEMENTATION_PLAN.md)**

Extract the ZIP to one folder. **Double-click Ground_Station/OPEN_GROUND_STATION.cmd** on Windows. Press Enter for Wi-Fi-only, or enter your Uno COM port for simultaneous Wi-Fi and LoRa. The computer's full blue Feather dashboard opens at **http://127.0.0.1:8765/**.

| State | Windows app shows |
|---|---|
| Feather Wi-Fi available | Full Feather dashboard and all existing Wi-Fi controls, testing, SD downloads, bench OTA |
| Feather Wi-Fi lost; Uno/LoRa live | Received radio altitude, acceleration, rotation, GPS, battery and signal quality; Wi-Fi-only controls unavailable |
| Both links lost | STALE measurements, last-known LoRa GPS and its age, no false live data |
| Feather Wi-Fi restored | Full dashboard and controls automatically return (Windows may need manual Wi-Fi reconnect) |

The Feather's direct **http://192.168.4.1** dashboard also remains usable independently for phone/iPad. The Uno is connected to the laptop by USB, **not** wired to the rocket Feather.

**V13 Feather source:** [Green_Ion_V13](Green_Ion_V13/) · **Uno source:** [Ground_Receiver](Ground_Receiver/) · **Windows app:** [Ground_Station](Ground_Station/) · **V12 reference:** [Green_Ion_V12](Green_Ion_V12/) · **Earlier versions:** [Past_Versions](Past_Versions/)

V13 preserves the V12 RKT4 v2, GIS1 and **V12D###.BIN / .SEN / .TXT** storage formats. New PC CSV logs are stored outside the downloaded folder under your user profile.

---

# GREEN ION V12 — Engineering reference (retained for existing hardware and recordings)

GREEN ION is a model-rocket flight recorder. An **Adafruit Feather ESP32 V2** in the rocket records barometer and synchronized IMU/GPS data to microSD, serves a local phone dashboard, and optionally sends LoRa radio packets to a separate **Arduino Uno R3** receiver connected to a Windows laptop.

**Status: software development, hardware validation pending.** Automated tests and compilation are not proof that your actual wiring, GPS, SD card, radio, power supply, mounting, or flight are safe or reliable. GREEN ION never controls ignition, staging, deployment or other energetic devices.

**Continuing with another ChatGPT?** Read [CHATGPT_TRANSFER.txt](CHATGPT_TRANSFER.txt) for the project context, engineering rules, development workflow, and prioritized improvement ideas.

## Three programs — what to use

| Location | File | Purpose |
|---|---|---|
| Inside rocket: ESP32 Feather V2 | [Green_Ion_V12/Green_Ion_V12.ino](Green_Ion_V12/Green_Ion_V12.ino) | Recorder, calibration, launch watching, phone app, optional LoRa transmitter |
| Ground: Arduino Uno R3 | [Ground_Receiver/Ground_Receiver.ino](Ground_Receiver/Ground_Receiver.ino) | Receives and validates LoRa, sends JSON to computer |
| Windows laptop | [Ground_Station/OPEN_GROUND_STATION.cmd](Ground_Station/OPEN_GROUND_STATION.cmd) | Opens live receiver dashboard, saves telemetry CSV, or runs labeled demo |

Primary files on microSD: **V12D###.BIN** (barometric RKT4), matching **V12D###.SEN** (full-resolution IMU/GPS), and **V12D###.TXT** (metadata). SD recording continues without radio, internet, or a connected phone.

## Parts and tools required

**Rocket payload:** Adafruit Feather ESP32 V2 (not the original older Feather ESP32), BMP388 barometer breakout, LSM6DSO32 ±32 g IMU, PA1010D GPS, compatible 3.3 V SPI microSD breakout and microSD card, protected 1-cell LiPo with correctly polarized JST connector, mounting hardware, insulation and strain relief. Optional: transmitting RFM95W breakout with **correct 900/915 MHz antenna**, and an active buzzer plus a suitable transistor-driver circuit. The particular breakout part numbers matter for power connections.

**Ground receiver:** Arduino Uno R3, separate Adafruit RFM95W breakout **with 5 V-compatible regulator/logic-level shifting**, suitable matching antenna, USB data cable, Windows computer.

**Tools:** soldering iron and solder, wire stripper/cutter, multimeter, insulated wire, solderable headers, heat-shrink, Arduino IDE 2, Python 3, and (for the real receiver dashboard) pyserial. Prefer a current-limited bench supply when appropriate.

## SOLDERING AND WIRING — all connections

**Before soldering:** Disconnect LiPo and USB. Verify each board's part number, voltage rating and printed pin labels. The pin numbers here are specifically for the **Feather ESP32 V2**. On the Feather, GPIO27/33/15/32/14 are generally marked **D27/D33/D15/D32/D14**; SPI is marked **SCK, MO, MI**. The older original Feather has a DIFFERENT I2C/SPI arrangement. Double-check against [official Feather V2 pinouts](https://learn.adafruit.com/adafruit-esp32-feather-v2/pinouts).

### A. Feather → BMP388, LSM6DSO32 and PA1010D (shared I2C)

If your boards have **STEMMA QT**, plug in compatible QT cables and daisy-chain them or use a QT splitter. The Feather's QT socket provides the four signals below; soldering those four signal wires is not needed with appropriate QT connectors. If a breakout lacks QT, attach soldered headers or wires:

| Feather V2 | BMP388 | LSM6DSO32 | PA1010D GPS |
|---|---|---|---|
| 3V regulated output | 3V/VIN **only if your breakout supports this supply** | 3V/VIN per breakout specification | 3V/VIN per breakout specification |
| GND | GND | GND | GND |
| SDA (GPIO22) | SDA | SDA | SDA |
| SCL (GPIO20) | SCL | SCL | SCL |

Usual I2C addresses: BMP388 **0x76 or 0x77**; IMU **0x6A or 0x6B**; GPS **0x10**. The Feather V2 QT power rail is controlled by GPIO2 (NEOPIXEL_I2C_POWER); firmware powers it before I2C starts. **VIN labels are not interchangeable between manufacturers**; confirm each breakout's datasheet.

### B. Feather → SPI microSD module (3.3 V-compatible)

| Feather V2 | microSD breakout | Meaning |
|---|---|---|
| 3V | VCC/3V, **only if module accepts 3.3 V power** | Power |
| GND | GND | Common ground |
| SCK / GPIO5 | SCK/CLK | SPI clock |
| MO / GPIO19 | MOSI/DI | Feather to SD data |
| MI / GPIO21 | MISO/DO | SD to Feather data |
| D33 / GPIO33 | CS/SS | SD selection |

Test the SD module **alone** before attaching radio SPI wires. Never hot-unplug it while powered or writing. Confirm the SD breakout's logic and power requirements: not all modules use the same VCC voltage.

### C. Feather → optional transmitting RFM95W breakout (shares SPI with SD)

| Feather V2 | RFM95W breakout | Meaning |
|---|---|---|
| 3V | Radio 3V/VIN **if breakout supports 3.3 V input** | Power |
| GND | GND | Common ground |
| SCK / GPIO5 | SCK | Shared SPI clock |
| MO / GPIO19 | MOSI | Shared SPI data out |
| MI / GPIO21 | MISO | Shared SPI data in |
| D27 / GPIO27 | CS | **Radio CS, different from SD CS** |
| D15 / GPIO15 | RST | Radio reset |
| D32 / GPIO32 | G0/DIO0 | Radio interrupt |

**Attach an antenna appropriate for this 900/915 MHz radio BEFORE transmitting**; never transmit from an RFM95W without one. Verify connector, power rail headroom, SPI chip-select timing and SD write behavior with the actual assembly. Secure the antenna so it cannot snag recovery components.

### D. Feather → optional active buzzer (driver required)

| Feather V2 | Where it goes |
|---|---|
| D14 / GPIO14 | A suitable transistor-driver input **through the appropriate resistor** |
| GND | Driver and buzzer supply common ground |
| Driver output | Connect to buzzer according to its rated voltage, polarity and transistor circuit |

**NEVER drive a buzzer directly from GPIO14.** Resistor values, transistor orientation, supply and flyback diode depend on the exact buzzer and transistor; do not guess them. A flyback diode is required for inductive sounders. Buzzer hardware and acoustic recovery are not validated yet.

### E. Battery and physical installation

Use a suitable protected **single-cell 3.7 V nominal LiPo** through the Feather battery connector. **Verify JST polarity with a multimeter** before plugging it in; JST-style connectors can have different polarity. Never solder the battery itself while connected. Do not connect battery positive to Feather 3V or Uno 5V.

Use properly secured, strain-relieved and insulated wiring. Loose breadboard jumper contacts inside a flying rocket can disconnect under acceleration. Secure battery and SD and verify nothing interferes with payload section, recovery separation or staging. Check **both** the Green Eggs and the narrow So Long independently for fit, weight, balance and center of gravity.

### F. Arduino Uno R3 → second ground RFM95W breakout

**These exact power connections are ONLY for the Adafruit RFM95W breakout that includes an onboard regulator and voltage level shifting.** A **bare** RFM95W is a 3.3 V device and must not have Uno 5 V logic connected directly.

| Arduino Uno R3 | Adafruit level-shifted RFM95W breakout |
|---|---|
| 5V | VIN |
| GND | GND |
| D13 | SCK |
| D11 | MOSI |
| D12 | MISO |
| D4 | CS |
| D2 | RST |
| D3 | G0/DIO0 |

Attach a suitable 900/915 MHz antenna to the Uno's radio, too. **The Arduino Uno is the ground receiver, not part of the rocket payload.** Plug the Uno into your Windows computer over USB.

### G. Shared frequency and ROC rules

The Feather and Uno use the same provisional settings: **920.5 MHz**, 125 kHz bandwidth, spreading factor 7, coding rate 4/5, 13 dBm. Defined in [GreenIonRadioConfig.h](libraries/GreenIonProtocol/src/GreenIonRadioConfig.h). **Coordinate the final frequency with ROC** before ROCstock, and verify FCC authorization/equipment and interference rules. Being in the 902–928 MHz band alone does not establish legality or a free channel. Do not change only one radio. References: [ROC tracker guidance](https://rocstock.org/rocket-tracking/), [Adafruit RFM95 hookup](https://learn.adafruit.com/adafruit-rfm69hcw-and-rfm96-rfm95-rfm98-lora-packet-padio-breakouts/arduino-wiring), [V12_WIRING](docs/V12_WIRING.md).

## Upload Feather V12 (first installation by USB)

1. Install **Arduino IDE 2** and **esp32 by Espressif Systems** board package.
2. Select board **Adafruit Feather ESP32 V2** and select its real USB COM port. For web firmware updates, use an **OTA-capable two-app-slot partition scheme**, not a huge/no-OTA scheme.
3. Install Arduino libraries: **Adafruit BMP3XX Library, Adafruit LSM6DS, Adafruit GPS Library, RadioHead, Adafruit BusIO, Adafruit Unified Sensor**. Install repo-local **libraries/GreenIonProtocol** and **libraries/GreenIonRecorder** as Arduino libraries.
4. Open [Green_Ion_V12.ino](Green_Ion_V12/Green_Ion_V12.ino), compile and upload to Feather over USB.
5. Set Serial Monitor to **115200 baud** with **newline** line ending. It prints readable status messages rather than flooding the screen at startup. Enter **help** for commands, **status** for a quick overview, or **bench imu**, **bench gps**, **bench all** for detailed 1 Hz readings. Enter **bench off** to stop verbose output.
6. Power down between wiring changes. Verify each sensor alone, then add devices one at a time.

**Phone/iPad app:** Join Wi-Fi **GREEN-ION-V12**, password **GreenIon2026**. Open **http://192.168.4.1/** in browser. It works without internet. Keep the device Wi-Fi password private.

## Upload Uno ground receiver

1. Attach the correct level-shifted RFM95W breakout and antenna, power off for wiring.
2. In Arduino IDE, select **Arduino Uno** and its USB COM port.
3. Install **RadioHead** and the repository's **GreenIonProtocol** library.
4. Open [Ground_Receiver.ino](Ground_Receiver/Ground_Receiver.ino), compile and upload.
5. Open Uno Serial Monitor at **115200 baud**. Boot prints JSON receiver_status, and incoming valid radio packets print JSON telemetry.
6. Type **human on** for additional readable one-line radio summaries; **human off** hides them; **status** shows received, invalid and estimated missed packets. Machine-readable JSON stays available in both modes.
7. On the Feather, use Serial Monitor commands **radio status** then **radio test**. The Uno should receive five CRC-checked radio_bench messages with increasing sequence numbers. Antennas are required before any transmission.

## Windows ground-station computer

1. Install Python 3 on Windows and connect the Uno USB data cable.
2. For live Uno use, install serial library once:

       py -3 -m pip install pyserial

3. In Windows **Device Manager → Ports (COM & LPT)**, identify the Uno COM port. **Close Arduino Serial Monitor** before opening the Python app because the COM port cannot be shared.
4. Double-click [OPEN_GROUND_STATION.cmd](Ground_Station/OPEN_GROUND_STATION.cmd). It asks for a COM port such as **COM5**. Enter it for **LIVE RECEIVER**, or press Enter for explicitly labeled **DEMO telemetry** with no hardware.
5. The app opens a local browser at **http://127.0.0.1:8765**. Keep its window open; it writes CSV logs in **Ground_Station/logs/**. The computer display is separate from the Feather's local phone app.

Manual commands:

    cd Ground_Station
    py -3 -m pip install pyserial
    py -3 ground_station.py --port COM5
    py -3 ground_station.py --sample

The Uno does not control flight. The computer provides approximate received telemetry, last-known coordinates, and a CSV log; the primary data remains on microSD.

## V12 reliability hardening (home tests and LoRa ground evidence)

The guided home test now accepts IMU and GPS results **only from tests started during the current home session**. An earlier PASS cannot be reused. When starting a session, launch watching is cancelled **before** SD cleanup or nonvolatile-memory writes; failed setup restarts stationary calibration rather than leaving an uncertain launch-watch state.

The Windows ground-station CSV now has **fixed columns** across normal telemetry and radio-bench packets. It includes the full original JSON in `raw_packet_json` so newer firmware fields are not lost. Receiver-side five-packet bench events are timestamped and saved to the same downloadable CSV, rather than disappearing when the ground station closes. A Feather TX count alone is still **not** proof of Uno reception.

## Full temporary home verification (Testing tab)

Open **Testing → Full home verification — guided and temporary** on the Feather dashboard. This is a step-by-step real bench test. **Disconnect all rocket motors; never run it on a launch pad.**

1. Inspect LiPo polarity, wiring, antennas, SD mounting, insulation and strain relief. Confirm this manual check.
2. Read actual battery voltage, barometric pressure and stationary-calibration baseline; implausible values fail.
3. Run the built-in five-second IMU test, keeping the Feather motionless.
4. Run the 12-second GPS check outdoors. NMEA reception without a fresh position fix is **INCONCLUSIVE**.
5. Perform a temporary SD write/read/compare/delete verification.
6. With antennas connected, transmit five LoRa bench packets and manually confirm **all five packets were received on the Uno**. Sending alone does not prove receiving or range.
7. With a protected LiPo installed, briefly unplug USB to check battery-only operation, then reconnect USB. This is a manually confirmed check.
8. Run **30 seconds of actual BMP388 recording at 50 Hz**. Briefly disconnect the **phone's Wi-Fi**, then reconnect GREEN-ION-V12. The Feather writes reserved **/GIHOME.BIN**, checks the RKT4 header, bytes, timestamps, valid samples and missed sample periods, then deletes the test file. Wi-Fi disconnection is reported separately, and remains **not verified** if no disconnection was detected.
9. Review physical mounting, balance and recovery clearance for both Green Eggs and So Long. Press **END & DELETE TEMP DATA** and leave the Feather still while automatic calibration restarts.

The app shows the next instruction, results and progress while tests run. Use **MARK NOT VERIFIED** when a step cannot be completed; this is not a PASS. Download the **JSON test report** at the end. Ending the session removes only the reserved temporary test file; no V12D flight recordings are deleted. After a power interruption, starting a fresh session attempts to clean up that reserved file.

**Limitations:** The bench test does not simulate real launch/staging/landing, verify recovery hardware, test LoRa range, prove power-loss recovery, check OTA rollback, certify sensor accuracy, or establish flight readiness. Hardware checks still require real devices and a documented review.

## Phone app — flight, home test, saved files

- V12 calibrates while stationary after boot and **automatically watches for launch** when calibrated. There is no aircraft/airplane simulation test mode. **After one flight is saved, reboot/reset the Feather and recheck calibration before the next flight** (for example before switching from Green Eggs to So Long).
- **Testing**: Test IMU, Test GPS, Test All Sensors, and Integrated Preflight Bench Check; run **Detect & Test SD Card**. A GPS communication test without a fresh outdoor fix can legitimately show **INCONCLUSIVE**.
- **Flight**: launch detection writes a primary RKT4 file and synchronized IMU/GPS companion file. Recording operates without continuing Wi-Fi access. **Do not press Stop & Save during flight.**
- **After landing**: open Storage and download **BIN** (primary), **SEN** (IMU/GPS), **TXT** (metadata), or **CSV** (derived browser export). Keep the matching BIN/SEN pair.
- Export the companion log on Windows with:

       cd Ground_Station
       py -3 sensor_log.py V12D007.SEN --rkt4 V12D007.BIN --csv V12D007-sensors.csv

The exporter validates binding, header/record CRCs, sequence gaps and torn tails; corrupted input is refused unless deliberately permitted for recovery.

## OTA firmware update and rollback — bench only

1. **Never perform firmware updates on the launch pad or during a flight.** Download existing flight files first. Secure the payload on a safe bench. Connect stable **USB power and a charged LiPo**. V12 maintenance currently requires onboard LiPo measurement **at least 3.60 V**; this is not a battery-health certification.
2. In GitHub **Actions**, open a successful V12 build, download the **green-ion-v12-development-binaries** artifact and unzip it. Select the Feather's main firmware file ending in **.ino.bin**, **NOT** the bootloader, partitions binary, merged image or Uno HEX.
3. Connect phone to **GREEN-ION-V12** Wi-Fi and open **http://192.168.4.1/**. Go to **System → Install firmware from this phone**, select the .ino.bin file, then tap **INSTALL FIRMWARE & RESTART**.
4. Confirm the warning. The app enters **MAINTENANCE**, which **DISABLES autonomous launch watching**. Keep USB and LiPo power connected while upload/verification/restart proceeds.
5. Rejoin Wi-Fi after reboot, confirm firmware build in System diagnostics and repeat sensor, SD and LoRa bench tests.
6. To go back, choose **ROLL BACK TO PREVIOUS FIRMWARE** in System. This works **only** if the alternate OTA slot contains a valid previously flashed app. Some recovery scenarios still require USB flashing.
7. If you cancel or an update fails, select **EXIT MAINTENANCE / RECALIBRATE**. If the page was reloaded and the maintenance token was lost, restart Feather physically and wait for calibration. **Do not launch while maintenance is active.**
8. Bench-test real OTA and rollback on your assembled hardware. A successful source build does not prove OTA reliability under power interruption.

## Arduino Serial Monitor commands

| Feather command | Meaning |
|---|---|
| help | Human-readable list of commands |
| status | Quick current sensor, SD, battery, recorder summary |
| test imu / test gps / test all | Finite sensor tests |
| test status | Print results |
| bench imu / bench gps / bench all | Detailed 1 Hz sensor readings |
| bench off | Stop periodic verbose bench readings (default) |
| radio status / radio test / radio retry | Radio status, 5-packet bench check, retry after wiring correction |

Uno commands: **human on**, **human off**, **status**. JSON packet lines are retained for the ground-station parser.

## V12 reliability upgrade: home tests, preflight, receiver and rocket profiles

The current development firmware adds an advisory preflight evaluator, non-overwriting filename allocation, on-device home-test step evidence, a saved per-rocket profile, and better ground-receiver diagnostics. **Every hardware test below still requires the real assembled electronics.** There is no automated authorization to launch.

### Flight profile — save before launch

1. With the Feather safely secured on the bench, open its Wi-Fi dashboard at `http://192.168.4.1/`.
2. Open **Flight → Rocket flight profile**. Select **Green Eggs**, **So Long**, or **Another rocket**. Type an optional motor designation (e.g. `D12-5`).
3. Press **SAVE ROCKET PROFILE & RECALIBRATE**. The name/motor are saved in Feather nonvolatile storage. Launch watching is cancelled until pressure calibration restarts and completes.
4. Check the actual rocket name and preflight warning list. The selected rocket/motor are appended to each new flight's **V12D###.TXT** as `rocket=` / `motor=` lines. Binary **BIN/SEN formats are unchanged**.
5. After changing from Green Eggs to So Long, save a new profile and separately check physical fit, payload mass, center of gravity, motor/staging and recovery.

No rocket is assumed by default. If an older flight TXT has no profile lines, the dashboard displays “Rocket not recorded in this flight” rather than attaching today's selection to historical data.

### Automatic preflight — advisory, not launch approval

The Status/Flight recorder-health card updates from actual `/api/status` data. The firmware returns a classification plus a bitmask of reasons. **BLOCKED** means an essential software preflight requirement is missing, **CHECK_OPTIONAL** means core recorder conditions are met but optional sensors, GPS, radio, historical errors, calibration age, or the rocket profile need attention, and **READY_FOR_REVIEW** means the software checks currently have no flags. None of these proves that the rocket is safe or permitted to launch.

Essential conditions include SD availability, barometer health, voltage measurement above the conservative software threshold (3.60 V; verify your divider with a multimeter), valid pressure calibration, automatic watcher active, no OTA/home-test mode, no outstanding recovery evidence, and no interrupted home-test cleanup. Optional checks cover IMU range, current GPS fix, LoRa readiness, missed samples, companion errors, 30-minute calibration age, and rocket selection.

Use **DOWNLOAD PREFLIGHT REPORT** for a snapshot JSON report. It explicitly states `hardware_approved: false`. A missing GPS fix does not disable pressure-only recording but is reported as a warning. **A radio being initialized is not proof that the Uno receives it.** Always complete physical inspection and range safety review separately.

### Guided home test now saves step results on the Feather

The **Testing → Full home verification** wizard writes completed step result codes into ESP32 nonvolatile storage, not just the current browser page. If a phone disconnects or reloads, the wizard restores only steps already recorded by the Feather. Automatic measurements are re-evaluated **on the Feather** rather than accepting a claimed PASS from JavaScript; user-checklist and Uno-reception steps are labeled **CONFIRMED**, not electronically measured.

The 30-second 50 Hz test remains a TEMPORARY file `/GIHOME.BIN`; it checks actual RKT4 bytes and deletes its own temporary data only. A Wi-Fi disconnect must actually be observed to mark the complete recording + disconnect step PASS. Incomplete steps are **NOT VERIFIED** or **INCONCLUSIVE**. If the Feather reboots mid-session, it marks the previous test **INTERRUPTED** rather than resuming a potentially unsafe recording, and requires a fresh session and cleanup. Ending a session restarts stationary calibration.

**Important:** The device stores step **result codes**, not a full raw time series or every browser-written narrative. The downloadable browser JSON includes observed details when available; restored historical codes are clearly identified. Tests still cannot establish physical flight, launch/landing detection, vibration, RF range, OTA reliability, independent battery operation without manual confirmation, or safe mounting.

### Flight data protection

Flight filenames now check for pre-existing `.BIN`, `.SEN`, `.TXT`, journal and recovery-report siblings before allocating a new `V12D###` sequence. An orphaned companion/recovery file will no longer be casually overwritten when the primary BIN is missing. Existing checkpointing and RKT4/GIS1 bytes remain unchanged.

SD write/flush, interrupted recording recovery and file-integrity behavior must still be measured on hardware. Do not interpret collision protection alone as proof against power interruption, worn flash, or failing SD cards.

### LoRa receiver and Windows ground station

The Arduino Uno firmware reports CRC/rejected counts, valid packets, estimated missing packets, duplicates, out-of-order packets, sender reboots and a 5-second receiver status heartbeat. Valid bench packets are tagged `radio_bench` and **not counted as flight telemetry** by the Windows program. The computer shows serial receiver state, RSSI average, estimated packet loss, receiver-side five-packet count, GPS freshness, and last-known position.

To use real radio data, connect **both correct antennas**, plug the Uno into your Windows computer, close Arduino Serial Monitor and run `Ground_Station/OPEN_GROUND_STATION.cmd`, entering the Uno COM port. The Python program reconnects after USB/serial loss and records telemetry to `Ground_Station/logs/`. Select **Download telemetry CSV** in the local computer dashboard or open `http://127.0.0.1:8765/api/export`. Empty COM input runs explicitly labeled **DEMO**, which is never hardware evidence.

Before the next launch, verify actual LoRa range, link reliability, CRC diagnostics, both antenna connections and the ROC-approved/legally usable channel. The firmware's configured 920.5 MHz is provisional and not itself clearance.

## Before ROCstock (both Green Eggs and So Long)

- [ ] Confirm electronics and complete battery/antenna fit, mass, balance and center of gravity individually for **each** rocket.
- [ ] Make a secure sled/retention system; no loose jumpers, exposed conductors, pinched wiring or blocked recovery/staging hardware.
- [ ] Power-on inspection and multimeter check: no heating, shorts, low voltage, resets or unexpected current.
- [ ] Sensor I2C tests and real **outdoor GPS fix**. Determine last-known behavior when signal is lost.
- [ ] SD tests, real 50 Hz flight-like recordings, corrupt/torn-tail inspection, recovery after controlled power loss.
- [ ] Radio + SD at the same time, packet gaps, RSSI, GPS recovery, range and frequency coordination.
- [ ] Check **WATCHING FOR LAUNCH**, **not MAINTENANCE**, and no unresolved health faults. Reset and recalibrate between flights.
- [ ] Review the finished hardware and rocket configuration with Rolling Robots mentor and ROC range safety officer. Follow kit instructions and launch-field rules.

## Detailed sources, code and validation

- [First-day hardware tests](docs/V12_FIRST_DAY_TEST.md)
- [Hardware bring-up](docs/V12_HARDWARE_BRINGUP.md)
- [V12 wiring](docs/V12_WIRING.md)
- [Testing checklist: code tests versus untested hardware](docs/V12_TESTING.md)
- [V12 roadmap](docs/V12_ROADMAP.md)
- [Development log](docs/V12_DEVELOPMENT_LOG.md)
- [GitHub Actions builds](https://github.com/aidenjantzi1-ctrl/GREEN-ION/actions)
- [Adafruit ESP32 Feather V2 pinout](https://learn.adafruit.com/adafruit-esp32-feather-v2/pinouts)
- [Adafruit RFM95 breakout hookup](https://learn.adafruit.com/adafruit-rfm69hcw-and-rfm96-rfm95-rfm98-lora-packet-padio-breakouts/arduino-wiring)
- [ROC tracking coordination](https://rocstock.org/rocket-tracking/)
