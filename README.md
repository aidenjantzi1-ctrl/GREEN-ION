# GREEN ION — Rocket Flight Recorder

**Current application: V16.1 RC2 — safe SD management + reliability.** The original blue five-tab layout is preserved. RC2 adds verified Wi-Fi SD deletion, removes stale storage controls, hardens critical SD access, and improves Flights & Analysis layout. Feather/Uno hardware firmware remains the V14 generation. Real hardware/flight validation is still pending.

**Hosted V16 application / V14 firmware Wi-Fi connection:** compatible Chromium browsers can connect to the Feather after its trusted-origin firmware update and local network permission. Safari/iPhone uses the direct Feather dashboard plus History import and cloud sync after internet returns. [Phone/computer setup and limitations](docs/V15_HOSTED_PHONE_CONNECTION.md).

[**Open the blue flight dashboard**](https://green-ion-beta.vercel.app/) · [**Download V14 releases**](https://github.com/aidenjantzi1-ctrl/GREEN-ION/releases) · [**Automated tests**](https://github.com/aidenjantzi1-ctrl/GREEN-ION/actions/workflows/build-v14.yml)

GREEN ION records pressure, motion, GPS and battery data independently to SD. Windows, iPhone and iPad use the same blue dashboard; Windows can also receive Uno USB / LoRa telemetry.

[**V16.1 reliability + simplicity**](docs/V16_1_RELIABILITY_SIMPLICITY.md) · [**V16 implementation checklist**](docs/V16_IMPLEMENTATION_CHECKLIST.md) · [**V16 five-tab interface**](docs/V16_PHASE4_FIVE_TAB_INTERFACE.md) · [**V16 data/cloud integrity**](docs/V16_PHASE5_DATA_CLOUD.md) · [**V16 Windows packaging**](docs/V16_PHASE6_WINDOWS_PACKAGING.md) · [**V16 3D/replay**](docs/V16_PHASE7_ADVANCED_VISUALIZATION.md) · [**V16 hardware evidence**](docs/V16_PHASE8_HARDWARE_VALIDATION.md) · [**V16 Phase 3 Mission Workflow**](docs/V16_PHASE3_MISSION_WORKFLOW.md) · [**Phase 2 Unified Telemetry**](docs/V16_PHASE2_UNIFIED_TELEMETRY.md) · [**Phase 1 Windows reliability**](docs/V16_PHASE1_WINDOWS_RELIABILITY.md) · [**Version history**](VERSIONS.md)

## Choose a guide

| What you want | Where to go |
|---|---|
| **Current V16 development status and phase order** | [**V16 Implementation Checklist**](docs/V16_IMPLEMENTATION_CHECKLIST.md) |
| **Five-tab Dashboard / Flight / Analysis / Testing / Advanced layout** | [**V16 Phase 4 Five-Tab Interface**](docs/V16_PHASE4_FIVE_TAB_INTERFACE.md) |
| **One mission from preflight through recovery and analysis** | [**V16 Phase 3 Mission Workflow**](docs/V16_PHASE3_MISSION_WORKFLOW.md) |
| **Unified Wi-Fi + LoRa telemetry and receiver setup** | [**V16 Phase 2 Unified Telemetry**](docs/V16_PHASE2_UNIFIED_TELEMETRY.md) |
| **Windows connection reliability changes** | [**V16 Phase 1 Reliability**](docs/V16_PHASE1_WINDOWS_RELIABILITY.md) |
| **Existing launch pad workflow: preflight, Recovery, archive, cloud sync and Help** | [**V15 Local-First Guide**](docs/V15_LOCAL_FIRST.md) |
| **3D postflight trajectory and relative digital twin** | [V16 3D + Replay](docs/V16_PHASE7_ADVANCED_VISUALIZATION.md) |
| Start the website and use the blue tabs | [Start Here](docs/V14_START_HERE.md) |
| **What to solder and exactly where** | [**Wiring & Soldering**](docs/V14_WIRING_SOLDER.md) |
| Every BMP388, IMU, GPS, battery and radio measurement | [Sensor Capabilities & Recorded Formats](docs/V14_SENSOR_CAPABILITIES.md) |
| Different iPhone, iPad, tablet and Windows layouts | [Devices and Connectivity](docs/V14_DEVICES.md) |
| Create a cloud account, sync or restore flights | [Private Cloud Backups](docs/V14_CLOUD.md) |
| New ideas and removable optional modules | [V14 Improvement Plan](docs/V14_IMPROVEMENT_PLAN.md) |
| Testing and requirements before launch | [Quality & Hardware Validation](docs/V14_QUALITY_PLAN.md) |
| GitHub folders, code changes, builds and releases | [Developer Guide](docs/V14_DEVELOPMENT.md) |
| Previous software, source and rollback packages | [Version Archive](Past_Versions/README.md) · [V13 release](https://github.com/aidenjantzi1-ctrl/GREEN-ION/releases/tag/v13.0.0-beta.1) |

## Dashboard

V16 exposes exactly five normal top-level tabs: **Dashboard, Flight, Flights & Analysis, Testing, and Settings & Advanced**. Existing advanced tools are preserved underneath those destinations rather than deleted. Recovery is part of Flight; storage/history are combined under Flights & Analysis; radio/cloud/updates/help live under expandable Settings & Advanced sections.

Optional Sensor Lab, SEN analysis, cloud backup and display settings live under Settings & Advanced. Original BIN/SEN/TXT analysis and export work offline.

**Hardware commands:** Use the Feather's Wi-Fi and local **http://192.168.4.1** backup dashboard when direct connection from the hosted HTTPS website is blocked. The iPhone website cannot read Uno USB serial. Downloads from GitHub **never flash automatically**; bench-only firmware OTA requires separate confirmation.

**Windows local server:** Run `START_GREEN_ION.cmd` for the current blue localhost dashboard with Feather Wi-Fi, Uno LoRa, automatic SHA-256 computer archives and optional signed-in cloud backup when internet returns. **Delete from SD** removes only the Feather copy; Windows/browser/cloud backups remain. Firmware RKT4/GIS1 formats remain unchanged.

## Software vs hardware validation

GitHub Actions compiles both firmware targets and tests the web app, but **no real launch, GPS/RF range, SD, LiPo, iPhone local-network, account-based cloud restore or OTA rollback test can be certified by CI**. Complete the supervised [hardware validation guide](docs/V14_QUALITY_PLAN.md) before relying on V14 for a launch.

Sources: [Adafruit Feather V2 pins](https://learn.adafruit.com/adafruit-esp32-feather-v2/pinouts), [Supabase security](https://supabase.com/docs/guides/storage/security/access-control), [Vercel Git deployments](https://vercel.com/docs/git), [Web Serial compatibility](https://developer.mozilla.org/en-US/docs/Web/API/Web_Serial_API).
