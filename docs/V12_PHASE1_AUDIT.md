# Phase 1 V11 and V12 feature audit

Code inspected at V12 development commit `2b2d2d8`; this table tracks the new eight-phase plan, not the older roadmap numbering. `Past_Versions/Green_Ion_V11` is a read-only reference. A source-level feature is not a physical test.

| Requirement | V11 | Current V12 | Missing work | Tests | Status |
| --- | --- | --- | --- | --- | --- |
| Blue responsive dashboard | `Dashboard.h` blue CSS and mobile layout | Feather `Dashboard.h` port and laptop `Ground_Station/index.html` blue layout | Browser validation of both interfaces on actual devices | `test_v12_dashboard_restore.py`; laptop Chromium desktop/mobile smoke | Partial |
| Six laptop destinations | V11 had seven Feather tabs; no six-page laptop design | Laptop now has Preflight, Mission Control, Recovery, Analysis, Diagnostics, Settings | Deeper content in phases 2–7 | `test_v12_ground_station.py`; laptop Chromium navigation | Foundation complete |
| Autonomous recording | V11 RKT4 recorder | V12 Feather recorder core, prebuffer, checkpoints and recovery modules | Hardware timing, power-loss and SD fault validation | `test_v12_rkt4.py`, native recorder suites | Software present; hardware unverified |
| Wi-Fi and LoRa telemetry | V11 Wi-Fi dashboard | Feather Wi-Fi status, Uno LoRa bridge, source-aware laptop merge | Physical dual-link operation and range | `test_v12_ground_station.py`, `test_v12_telemetry.py` | Software present; hardware unverified |
| GPS recovery | V11 field/recovery display | Last valid laptop position and offline relative track | User position, bearing and map services | Source merge tests | Partial |
| Offline flight analysis | V11 RKT4 browser analysis, CSV/PDF | V12 Feather browser port; laptop BIN library, CSV/JSON/report exports | Synchronized SEN/BIN analysis, replay, comparison and backup flow | `test_v12_flight_library.py`, `tests/js/test_rkt4.mjs` | Partial |
| Recorder controls | V11 recording controls | Feather recorder-safe stop and SD endpoints | Laptop recorder control flow and detailed state | `test_v12_dashboard_restore.py` | Partial |
| Preflight diagnostics | V11 hardware testing | Laptop preflight backend and dedicated page; Feather sensor and SD tests | Full disconnect test, calibration workflow and measured readiness | `test_v12_ground_station.py`, native bench suites | Partial |
| OTA, rollback, backup/restore | V11 controls and routes | V12 port labels unsupported controls as disabled | Compatible backend and safe restore flow | `test_v12_dashboard_restore.py` | Missing in V12 |
| Unified field semantics | No laptop merge contract | `field_meta` for each accepted Ground Station field | Full field-by-field verification against live Feather and Uno packets | `test_v12_ground_station.py`; `V12_TELEMETRY_MODEL.md` | Foundation complete |

The Ground Station was started locally with Wi-Fi and serial disabled. Chromium loaded and navigated all six destinations at 1280×800 and 390×844 with no page errors or horizontal overflow. This was a software UI check; no rocket hardware was connected.
