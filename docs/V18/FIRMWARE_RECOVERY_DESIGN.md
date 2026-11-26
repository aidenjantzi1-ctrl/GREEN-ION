# GREEN ION V18 firmware recovery and power design

**Branch:** `worker/v18-firmware`  
**Scope:** recorder-safe recovery scheduling and evidence for bench development.  
**Safety status:** software model only for automatic recovery transmission/hibernation. No automatic LoRa TX, deep sleep, GPIO wake, ignition, staging, deployment or flight-readiness claim is enabled by this work.

## 1. Actual behavior found before V18 recovery changes

The Feather firmware records the authoritative RKT4 v2 BIN at 50 Hz. Launch detection, the prebuffer, SD writes, flush/checkpoint service and finalization do not require GPS, LoRa, Wi-Fi or the Windows app. GPS and LoRa remain warnings/recovery aids. The current protocol is GreenIonProtocol v2 and `TelemetryPacket` remains 70 bytes.

A flight receives a new `V12D###.BIN` sequence by scanning for an unused package. On landing/STOP, firmware stops and flushes the recorder, writes completion metadata, reopens the BIN/TXT for structural package verification, then removes JRA/JRB recovery journals. Only after those steps does it declare `RECORDER_COMPLETE`, set LANDED and request Wi-Fi restoration. A failed stop, completion write, structural verification or journal cleanup leaves a recorder fault and does not start the V18 recovery policy.

Wi-Fi flight suppression is requested only after the primary recorder starts. The transition is delayed/guarded so it stays subordinate to SD service. A short flight can finalize before Wi-Fi actually turns off; that pending state is cancelled safely. Wi-Fi comes back only after verified save.

LoRa uses the existing protocol-v2 packet and shared SPI scheduler. Transmit authorization is RAM-only and false after every reboot. Bench/home radio transmission requires explicit antenna confirmation. **The SX127x cannot tell firmware whether a physical antenna is attached or damaged**, so software must not infer antenna safety.

The existing management button is Feather ESP32 V2 **SW38 / GPIO38**, used for a short physical Wi-Fi-management authorization window. Hardware documentation explicitly says GPIO38 deep-sleep wake has not been physically confirmed in the V18 installation. This branch therefore does not enable deep sleep or treat SW38 as a verified wake source.

## 2. V18 post-save recovery policy

The new `GreenIonRecoveryPolicy.h` is a deterministic, non-blocking policy model. It has no Arduino, Wi-Fi, SD, GPS or RadioHead dependency and can be tested with a fake clock.

### Time origin and states

Recovery time origin is the exact `nowMs` at which all of the following are already true:

1. primary RKT4 recorder stopped successfully;
2. completion TXT was written;
3. reopened BIN/TXT passed structural package/readback verification;
4. JRA and JRB journal cleanup succeeded.

Until then state is `LOCKED`.

After verified save:
- **0 through < 5 minutes:** `ACTIVE_RECOVERY`.
- **at 5 minutes:** `HIBERNATING` *policy state*. This name means reduced scheduling only; the ESP32 is **not** put into deep sleep.
- **home Wi-Fi reconnect:** `WIFI_CONNECTED`; recovery TX scheduling is disarmed while connected.
- **battery unknown or below 3500 mV:** `BATTERY_BLOCKED`.

A newly started authoritative flight resets prior recovery state only after `RecorderCore::start()` succeeds, preventing failed start attempts from destroying useful previous recovery evidence.

### Transmission opportunity rules

The policy only reports whether a transmission **would be due**. Runtime automatic recovery TX remains disabled pending Integration + hardware evidence.

Proposed/default opportunity timing:
- active recovery: at most one opportunity per 1 second;
- hibernating policy: one opportunity every 180 seconds (3 minutes);
- first hibernation opportunity is not immediate at the 5-minute boundary; if no prior successful TX exists, it is 3 minutes after hibernation starts.

An opportunity is blocked unless all are true:
- verified save and journals cleared;
- Wi-Fi is not reconnected;
- battery measurement exists and is at least 3500 mV;
- LoRa initialized;
- RAM-only antenna/TX authorization is true;
- a position exists and its age is no more than 30 seconds.

No GPS fix yields `POSITION_MISSING`; an older last-known fix yields `POSITION_STALE`. The policy does not invent coordinates or claim RF success.

All time math uses unsigned 32-bit subtraction and is covered across `millis()` rollover.

## 3. Machine-readable evidence

Existing `GET /api/status` remains backward-compatible and gains additive fields:

- `recovery_policy_state`
- `recovery_tx_due`
- `recovery_tx_block`
- `recovery_elapsed_since_save_ms`
- `recovery_next_due_ms`
- `recovery_runtime_tx_enabled=false`
- `recovery_deep_sleep_enabled=false`
- `recovery_wake_hardware_verified=false`

These fields are bench evidence only. `recovery_tx_due=true` means the pure policy would permit an opportunity; **it does not mean a packet was transmitted**.

## 4. Hardware features deliberately blocked

### Automatic recovery LoRa TX — BLOCKED: HARDWARE EVIDENCE / INTEGRATION REVIEW

No direct automatic TX call is added. Before enabling it:
- both Feather and Uno antennas must be physically inspected/repaired and confirmed appropriate;
- bench packet receipt and range evidence must be recorded;
- battery current during TX must be measured;
- Integration Manager must review how a due opportunity enters the existing telemetry queue without starving SD/shared SPI.

RF frequency, power, spreading factor, bandwidth, coding rate and v2 packet layout are unchanged.

### Deep sleep / physical wake — BLOCKED: HARDWARE EVIDENCE

No ESP32 deep-sleep call or wake GPIO is configured. SW38/GPIO38 is already used for Wi-Fi management, but its V18 wake behavior and installed accessibility are not verified. A future enablement must demonstrate that sleep is impossible during calibration/watching/recording/finalization and that wake cannot corrupt SD or unexpectedly authorize RF.

Safer alternatives until then:
1. keep the ESP32 awake and use the reduced 3-minute *scheduling policy* only;
2. after current measurements, consider modem/light sleep if it preserves timer/GPS/radio behavior;
3. consider deep sleep only after board/pin/wake-current testing and a reviewed reboot/recovery design.

## 5. 350 mAh LiPo measurement procedure

The 3.7 V 350 mAh LiPo remains the baseline. The firmware does **not** estimate remaining runtime from nominal capacity.

Motor-free bench procedure:
1. use the actual Feather, SD, GPS, LoRa, antenna and 350 mAh battery intended for the rocket;
2. use an inline USB/LiPo power meter or suitable ammeter with enough peak-current bandwidth;
3. record voltage/current for boot, Wi-Fi idle, calibration/watching, SD 50 Hz recording, GPS acquisition, verified post-save idle, and LoRa TX only after antenna safety authorization;
4. repeat the transition into the 5-minute/3-minute policy states with automatic TX still disabled, then separately measure controlled bench TX;
5. run a supervised endurance soak covering the Integration Manager-approved worst-case pad wait + flight + recovery-search duration;
6. record minimum voltage, average/peak current, brownouts/resets and total runtime;
7. require at least the hardware checklist's agreed reserve (currently target: >= 2x approved worst-case mission duration) before any battery optimization is called successful.

The 3500 mV policy threshold is the existing conservative bench threshold, **not** a validated remaining-capacity model. Hardware evidence may justify changing it only through reviewed configuration.

## 6. Bench-safe evidence without motors or RF transmission

Software-only:
- native fake-clock recovery policy test;
- preflight/SD/Wi-Fi/finalization regressions;
- `GET /api/status` recovery fields;
- existing reserved-file SD diagnostic.

Motor-free hardware:
- run existing SD/home tests with real SD;
- verify status changes after a safely generated completed recording;
- verify Wi-Fi reconnect changes policy state to `WIFI_CONNECTED`;
- verify no automatic radio packet occurs when TX authorization is false;
- physically measure battery behavior.

Real RF testing is separate and requires known-good antennas. CI, simulation and status fields can never mark physical RF/range/wake/endurance PASS.


## 7. Exact save-readback contract for V18 UI/integration

Firmware-authoritative save success is fail-closed. A flight is not considered structurally complete merely because a TXT sidecar exists.

The reopened package must satisfy all of these conditions:

1. BIN byte length equals the authoritative saved length and has no partial trailing RKT4 record.
2. RKT4 header is valid and decodes at the configured 50 Hz sample rate.
3. At least one complete RKT4 record exists.
4. Exactly one completion block marker `GREEN-ION-COMPLETION-1` exists.
5. Inside that completion block, exactly one `status=complete` line exists.
6. Inside that completion block, exactly one `file=V12D###.BIN` line matches the selected BIN.
7. Exactly one `file_bytes=N` line equals the reopened BIN byte length.
8. Exactly one `complete_records=N` line equals the RKT4 record count derived from the reopened BIN.
9. No JRA/JRB/RCV recovery evidence remains before `recording_complete=true` is exposed.

Blank, missing, duplicate, wrong-flight, byte-mismatched, record-mismatched, malformed-header, wrong-rate, zero-record or unreadable evidence fails closed. `GET /api/files` exposes additive `completion_verification` text and keeps `recording_complete=false` for these cases. The delete endpoint uses the same strict evidence and refuses to delete an invalid/incomplete package.

This is **structural package verification only**. RKT4 v2 does not contain a per-record cryptographic digest, so a structural PASS is not proof that every sample byte is historically unmodified.

## 8. Motor-free bench test card for Primary firmware

Keep motors and igniters physically removed. Leave automatic recovery TX, deep sleep and wake disabled. Do not authorize LoRa transmission unless both antennas have already passed the hardware inspection gate.

Record the exact firmware SHA, date, SD card, battery voltage and evidence files/screenshots for every step.

1. **Reserved SD probe:** run the guided SD test / `POST /api/sdtest`. PASS requires `success=true`, `cleanup_verified=true`, and no `V12SD.TMP` remaining afterward. If a previous reset left `V12SD.TMP`, the next explicit probe must remove it before writing. Any cleanup failure is FAIL and the card should be preserved for inspection.
2. **Stationary barometer calibration:** with the payload still and vented to room air, run calibration. Record baseline pressure, noise and age. A software-complete calibration does not prove the rocket's static-port design.
3. **IMU diagnostic:** run the explicit IMU hardware diagnostic while stationary, then gently rotate the electronics by hand. Record sample count/result and confirm no read-failure growth. GPS remains optional to SD logging.
4. **Guided home recording:** use the existing home test to record the reserved `GIHOME.BIN` at 50 Hz, then finish the test. PASS requires the firmware's sample/format checks and cleanup evidence. The reserved home file is not a real flight.
5. **Motor-free save package check:** only if the tester intentionally runs the existing safe simulated/manual recording path, complete and save it without arming a real launch. Confirm a new distinct `V12D###.BIN` appears, `recording_complete=true`, `completion_verification=VALID`, and its TXT contains the exact marker/status/file/byte/record binding above. A second cycle must use a different filename and preserve the first package.
6. **Failure evidence:** disconnecting Wi-Fi must not stop SD recording. Do not intentionally pull power during a valuable flight file; use a dedicated bench card if interruption recovery is being tested. Any JRA/JRB/RCV artifact means the package is not a clean completed save.
7. **Recovery status only:** after a verified save, inspect the recovery policy fields. They are simulation/status evidence; no automatic LoRa packet or deep sleep should occur on this branch.

A missing measurement is **NOT TESTED**, never PASS. Physical SD vibration reliability, 350 mAh endurance, real GPS sky view, RF range/antenna safety, static-pressure ports and So Long fit/CG remain separate hardware gates.
