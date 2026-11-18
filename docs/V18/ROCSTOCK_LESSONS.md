# ROCstock lessons — user report versus verification

This records the user's observations and required follow-up, not a reconstructed telemetry analysis. Obtain original BIN/metadata, photos and physical measurements before elevating a report to a measured instrument conclusion.

| Observation | Status / confidence | V18 action |
| --- | --- | --- |
| Two GREEN ION payload flights recorded | User-reported; two files not independently opened here | Preserve both original BIN + metadata; parse independently and compare timelines |
| **163 m** flight corresponds to file **D** | User correction to previous labeling; altitude value reported, instrument accuracy **unverified** | Keep D mapping; inspect raw pressure/baseline, compare simulation/tracking if available |
| Other recorded flight corresponds to file **C** | User correction; actual altitude not independently established | Preserve C mapping; do not swap C/D in reports |
| Altitude concerns | User observation; true apogee **unverified** | Test vented vs unvented pressure response and stationary baseline; label estimated apogee |
| Payload compartment had **no static pressure ports** | User-reported physical configuration | Design/verify appropriate static ports without compromising structure/recovery; bench airflow/pressure response |
| Electronics mounted with **tape** | User-reported | Design secured removable insulated sled with battery restraint, shock protection, connector strain relief |
| LoRa antennas broken; LoRa never functional | User-reported; cause and radio condition unknown | STOP transmission with damaged/disconnected antenna; inspect/replace approved antenna path, verify wiring/SPI/chip, then RF bench/range test |
| 350 mAh battery drained quickly | User report; current/endurance unmeasured | Measure current in all phases and LiPo safety margins before adjusting |
| Preflight confusing | User experience | One PREPARE & ARM with sequential checks and actionable error guidance |
| Too many test files | User experience; counts unknown | Temporary SD integrity tests must clean up only own fixtures and never remove genuine flights |

No precise flight altitude, maximum acceleration, sensor accuracy, actual power consumption, RF link distance, rocket fit, two-stage stability or launch readiness is verified by this document. Record raw evidence, measurement method, date, device/firmware build and uncertainty as hardware tests become available.
