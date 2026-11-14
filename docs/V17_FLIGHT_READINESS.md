# GREEN ION V17.0.1 — Flight readiness and SD-first operation

GREEN ION is a flight-data recorder and recovery aid only. It never controls ignition, staging, guidance or recovery deployment.

## Priority rule

The **50 Hz RKT4 v2 microSD recorder is authoritative**. Launch detection and local recording do not depend on Wi-Fi, GPS, LoRa, the Arduino Uno, Vercel, Supabase or an active phone.

A missing GPS fix or missing/disabled LoRa link is a **warning**, not a local-recording failure. A missing/unverified microSD card, BMP388, IMU, unsafe battery voltage, unresolved recovery evidence, failed SD integrity test, failed calibration, failed synthetic recorder/detector self-test, or failure to arm the launch watcher is **NOT READY**.

## PREPARE FOR FLIGHT

Use this on a safe bench with all rocket motors removed and GREEN ION stationary.

1. Press **PREPARE FOR FLIGHT** at the top of Dashboard.
2. Firmware checks critical local hardware and recovery state.
3. It creates reserved temporary file `/GIPREFLT.BIN`, writes an RKT4 v2 header plus 100 synthetic 50 Hz records, flushes, reopens, verifies size/header/timestamps/readback, then deletes the file.
4. It runs the existing IMU diagnostic and GPS diagnostic. IMU is critical; missing GPS is advisory.
5. It performs a passive LoRa chip/SPI/init diagnostic. Missing LoRa is advisory.
6. It performs a fresh stationary pressure calibration.
7. It runs a bounded RAM-only `RecorderCore` RKT4 simulation plus synthetic launch and landing detector trajectory. This does **not** create a real `V12D###` flight.
8. It rechecks the critical conditions and arms the real pressure-based launch watcher.
9. Dashboard shows **ARMED — READY FOR LAUNCH** only when the critical local recorder path passes.

Actual launch creates a new unique `V12D###.BIN` flight package. Existing RKT4 v2/GIS1 formats, 3-second prelaunch buffer, 500 ms primary flushes, 5-second recovery checkpoints, automatic landing finalization and boot recovery remain in use.

## LoRa diagnostics

Feather RFM9x wiring used by current firmware:

| Signal | Feather ESP32 V2 |
| --- | ---: |
| SCK | GPIO 5 |
| MOSI | GPIO 19 |
| MISO | GPIO 21 |
| microSD CS | GPIO 33 |
| LoRa CS / NSS | GPIO 27 |
| LoRa RESET | GPIO 15 |
| LoRa DIO0 | GPIO 32 |

Uno receiver wiring:

| Signal | Arduino Uno |
| --- | ---: |
| CS | D4 |
| RESET | D2 |
| DIO0 | D3 |
| SCK | D13 |
| MOSI | D11 |
| MISO | D12 |

Both programs use the shared `GreenIonRadioConfig.h` radio settings. The Uno remains receive-only.

The passive Feather diagnostic reads SX127x `RegVersion`:

- **0x12:** expected SX127x/RFM95-family chip ID. Firmware then checks RadioHead initialization.
- **0x00:** likely no low-level SPI response; inspect radio power/ground, MISO, SCK and CS/NSS.
- **0xFF:** MISO may be floating/high, CS may not select the radio, or radio power may be absent.
- **Other value:** unexpected chip/model or corrupt SPI communication.
- **0x12 + init failure:** inspect RESET, DIO0, CS and radio configuration.

These are probable causes, not proof of a specific broken wire.

### Antenna transmission lock

Software cannot reliably detect whether an antenna is physically attached. Therefore **all Feather LoRa transmission is locked after every reboot**.

To enable LoRa transmission for that boot:

1. Physically attach a suitable antenna to the Feather radio and the correct antenna to the Uno receiver.
2. Open Dashboard → Flight readiness → LoRa diagnostics.
3. Run the passive diagnostic first.
4. Choose **TRANSMIT 3 BENCH PACKETS** and explicitly confirm both antennas.
5. Verify actual packet reception on the Uno. This unlocks normal Feather telemetry for the current boot only.

Automatic preflight and automatic Home Test never infer antenna attachment. Without antenna confirmation, LoRa remains a warning and the SD recorder can still become ready.

## During flight

After arming:

- launch detection is autonomous;
- the new flight file is created only when launch is detected;
- primary RKT4 recording remains first priority;
- SD flush/checkpoint work is serviced before companion logs, radio telemetry, Wi-Fi or dashboard work;
- SD browsing/downloads are disabled while the launch watcher is armed and while recording;
- temporary GPS/LoRa/phone/Wi-Fi loss does not stop the local recorder;
- ascent, coast, apogee/descent and landing status are estimated from onboard measurements;
- landing automatically finalizes the flight;
- actual flight files are never automatically deleted;
- interrupted files retain recovery evidence and are examined on the next boot.

## Still required on real hardware

CI and simulated tests do **not** prove a safe flight. Before launch, physically verify:

- microSD retention and a real Full Home Test;
- Feather/SD/sensor wiring and strain relief;
- LiPo condition, voltage/current margin and reset/brownout behavior;
- RFM95 wiring, antennas, actual Uno reception and RF range if LoRa will be used;
- GPS outdoors if recovery GPS is desired;
- payload mounting, rocket structure, motor installation and retention;
- recovery system operation and clearance;
- rocket stability and launch-range/club safety inspection;
- operating radio frequency coordination.

If LoRa remains `MISSING`, launch-data recording can still be software-ready **only if all critical local recorder checks pass**. Treat recovery/tracking capability separately.
