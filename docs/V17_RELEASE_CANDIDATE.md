# GREEN ION V17.0 — official software release; hardware qualification pending

Version: **17.0.0 software release**, with CI-gated package integrity checks. **Not authorized for launch until actual hardware and rocket safety verification is complete.**

## Included
- Feather AP+STA home Wi-Fi, physical management authorization, bounded retry and direct AP fallback.
- Live bounded sensor graphs, WebGL orientation and recorded-sample replay, flight comparison and GPS recovery enhancements.
- Filenames are rendered as text in comparison tables; security regression tests added.
- Shared microSD/LoRa SPI scheduler, separate SD/LoRa CS, and interrupt-safe LoRa handling preserved.
- Feather/Uno 17.0.0 runtime strings and Windows 17.0.0 software-release metadata; V16.3 historical source directory paths retained for build/package compatibility.

## Nonnegotiable preserved interfaces
- RKT4 v2 and GIS1 formats; 50 Hz recording and 3 s prelaunch buffer; autonomous SD recording; receive-only Uno receiver.
- 70-byte LoRa telemetry protocol, shared SPI SCK 5 / MOSI 19 / MISO 21; SD CS 33; LoRa CS 27, RST 15, DIO0 32.
- No ignition, staging, active flight control, or deployment actions.

## Physical launch hold — must be completed on actual hardware
1. Inspect and electrically verify wiring, SPI voltage levels, no shorts, antennas, power/battery margins and independent physical rocket safety.
2. Flash Feather and Uno, verify version/status, collect full *actual* Home Test results including sustained 50 Hz SD, barometer and IMU data, LoRa packet reception through Uno, disconnected Wi-Fi recording, battery behavior and clean file recovery.
3. Verify GPS fix and recovery on real receiver, RF frequency coordination at launch site, OTA rollback on bench, and SD power-interruption/recovery test.
4. Inspect propulsion, retention, recovery system, CG/CP and motors with an authorized launch safety officer and follow applicable field/club rules.
5. Record pass/fail evidence using [hardware validation](V16_3_HARDWARE_VALIDATION.md). Until passed, call this **software CI verified**, never flight ready.

## Software verification
GitHub Actions [build and tests](https://github.com/aidenjantzi1-ctrl/GREEN-ION/actions/workflows/build-v14.yml). V17 release must pass CI on its exact commit before a release artifact or public stable tag is made. V16.3 published release remains intact.
