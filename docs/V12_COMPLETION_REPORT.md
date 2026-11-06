# GREEN ION V12 completion and readiness report

Audit date: 2026-11-05 Pacific. Development branch: `codex/green-ion-v12`. Audited pushed code head before the Phase 8 documentation update: `c56d176`; Phase 8 documentation checkpoint: `7a36415`. Run `git rev-parse HEAD` on the branch for the latest documentation commit. No merge to `main` occurred.

## Implemented software

| Phase | Status and evidence |
| --- | --- |
| 1. Architecture | Six blue Ground Station destinations, V11 audit and telemetry contract. See `V12_PHASE1_AUDIT.md` and `V12_TELEMETRY_MODEL.md`. |
| 2. Sensors and recording | Feather BMP388, LSM6DSO32, PA1010D, battery, SD and optional LoRa telemetry paths; separate GPS and barometric altitude. Autonomous RKT4 v2 BIN and GIS1 SEN recording remains independent of laptop links. See `V12_PHASE2_AUDIT.md`. |
| 3. Mission Control | Live source-aware measurements, 12-series graph, maps, recorder status and guarded controls; stale/missing data remains explicit. |
| 4. Preflight and Diagnostics | Hardware-evidence checks, stationary IMU/GPS tests, SD readback, recorder watcher checks, report history and motor-free bench workflow. See `V12_PHASE4_AUDIT.md` and `V12_HOME_BENCH_TEST.md`. |
| 5. Recovery | Last valid coordinate with age/quality, phone/manual position, straight-line bearing/distance, offline trail, online street/satellite imagery and reconnection handoff. See `V12_PHASE5_AUDIT.md`. |
| 6. Analysis | RKT4/SEN pairing, JSONL sessions, library details/filters, bounded graph and replay, comparisons, current GPS peak, apogee elapsed time, bounded raw/CSV/JSON downloads, print report, checksum verification, ZIP backup/restore and reversible laptop Trash. See `V12_PHASE6_AUDIT.md`. |
| 7. iPhone Companion | Specific private LAN bind with HTTPS/password, expiring session, rate limit, same-origin POST guard, protected routes, phone-sized shared dashboard and mobile Recovery/Analysis. See `V12_PHASE7_AUDIT.md` and `V12_IPHONE_COMPANION.md`. |
| 8. Integration and documentation | Software regression gate and native recorder tests pass; build, wiring, physical validation and launch-day instructions are linked below. Physical acceptance remains open. |

All phases have cloud-verifiable software paths. Phase 6 Feather SD deletion is deliberately **not offered** in V12; only laptop copies have guarded, reversible Trash. A future onboard deletion flow would need explicit confirmation and real hardware validation. Combined barometric/GPS apogee, precise launch/landing event times and linkage of laptop connection-loss events to a specific onboard BIN are not claimed without sufficient recorded evidence.

## Verification at this checkpoint

- Python: `python3 -m unittest discover -s tests -q` — **183 passed** on Python 3.12 after the bounded GIS1 CSV export follow-up.
- Native recorder/protocol: all **17** `tests/cpp/test_v12_*.cpp` programs compiled with `g++ -std=c++17 -Wall -Wextra -Werror` and executed successfully in this run, including RKT4, sensor log, calibration, watcher, landing, recovery, radio and coordinated failure paths.
- JavaScript: `node tests/js/test_rkt4.mjs` passed; extracted Ground Station script passed `node --check`.
- Chromium: `tests/browser/recovery_smoke.py` passed at 390 pixels for Recovery, Analysis, satellite request/failure fallback, recorder bench flow, backup and Trash; `tests/browser/companion_smoke.py` passed over local HTTPS for pairing gate, protected API, cross-origin denial, mobile layout and logout. These are fixture/synthetic checks, not real rocket or iPhone tests.
- Firmware target compilation: **PASS in the current cloud run** using Arduino CLI 1.5.1, ESP32 core 2.0.17 and AVR core 1.8.7. Feather ESP32 V2 used 1,220,837 flash bytes (36%) and 69,928 RAM bytes (21%). Uno R3 used 14,370 flash bytes (44%) and 712 RAM bytes (34%). Vendor library versions are recorded in `V12_BUILD_GUIDE.md`. The existing package installation printed missing discovery-tool warnings and the Adafruit GPS library printed ESP32 Wire overload notes; both compile commands exited 0. This is software build evidence, not physical hardware evidence.
- Physical hardware, Windows launcher, trusted iPhone Safari, GPS permission, cellular maps and radio range: **NOT TESTED** in Codex Cloud. See `V12_HARDWARE_VALIDATION.md` for explicit results and procedures.

## Important files and installation

- Feather and receiver firmware: `Green_Ion_V12/Green_Ion_V12.ino`, `Ground_Receiver/Ground_Receiver.ino`, `Green_Ion_V12/Config.h`, `libraries/GreenIonRecorder/`, `libraries/GreenIonProtocol/`.
- Laptop/phone server: `Ground_Station/ground_station.py`, `Ground_Station/companion_security.py`, `Ground_Station/index.html`, `Ground_Station/flight_library.py`, `Ground_Station/session_library.py`, `Ground_Station/library_backup.py`.
- Install and operate: `V12_BUILD_GUIDE.md`, `V12_WIRING.md`, `V12_FIRST_DAY_TEST.md`, `V12_IPHONE_COMPANION.md`, `V12_HARDWARE_VALIDATION.md`, `V12_LAUNCH_DAY.md` and `Ground_Station/README.md`.
- The complete V11 source archive stays under `Past_Versions/Green_Ion_V11/`. The V11 `.ino.bin` there is a firmware image, not a flight recording.

## Readiness and next actions

**Software integration is ready for controlled bench validation; flight readiness is NOT ESTABLISHED.** Do not infer a physical PASS from CI, simulation or the browser smokes. The next concrete work is to run the hardware checklist with the actual Feather sensors, LiPo, SD and GPS; physically disconnect/reconnect Wi-Fi during a disposable `TESTnnn.BIN` recording; validate power-loss recovery and actual BIN/SEN timing; connect the Uno/RFM95W safely with antennas and measure packets/range; then pair a trusted iPhone over the selected LAN and test Safari GPS and cellular imagery. Record PASS/WARNING/FAIL/NOT TESTED for each item and repair any failures before a field readiness decision.

Post-report integrity follow-up: imported-session and flight-metadata corruption checks were tightened without changing raw originals; backup creation now rejects incomplete imported-session Trash. The 170-test Python suite passed.

Current known limitations: the Feather development Wi-Fi credential is visible in the repository and is separate from laptop Companion pairing; no physical SD, sensor, battery, LoRa or Windows/iPhone outcome; no onboard SD delete endpoint; mobile satellite tile and cellular delivery depend on external service/network; trusted certificate installation on iPhone must be checked. Codex usage balance and reset timing were not exposed by available tools, so no reset schedule was inferred.
