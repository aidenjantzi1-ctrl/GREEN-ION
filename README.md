# GREEN ION V17.0 — Rocket Flight Recorder

> **V18 development (not released):** [Requirements](docs/V18/REQUIREMENTS.md) · [ROCstock lessons](docs/V18/ROCSTOCK_LESSONS.md) · [Decisions/contracts](docs/V18/DECISIONS.md) · [Integration status](docs/V18/INTEGRATION_STATUS.md) · [Pre-RC readiness](docs/V18/RC_READINESS.md) · [Branch preservation audit](docs/V18/BRANCH_AUDIT.md). The V18 offline frontend is staged on `worker/v18-integration` for testing; existing V17.0.1 remains stable and hardware acceptance is still required. No V18 release or approved flight qualification exists.\n\n> **CURRENT SOFTWARE VERSION: GREEN ION V17.0 (`17.0.1`)**
> **V18 preview on Windows (integration branch only):** Run `Ground_Station/START_V18_PREVIEW.cmd`, then open `http://127.0.0.1:8765/v18-offline.html#flight` if your browser does not open automatically. This uses the same local Windows server but shows the experimental four-tab V18 app. It does not flash firmware or change the stable V17 launcher default. Use Chrome or Edge for Web Serial, and do not treat preview status as flight authorization.

>
> **SD-FIRST FLIGHT READINESS:** V17.0.1 adds firmware-owned one-press preparation. microSD/RKT4 recording is authoritative; missing GPS or LoRa is a warning, not a recording blocker.
>
> **Feather ESP32 V2 firmware:** [Current V17 source](Green_Ion_V16_3/) · **Arduino Uno receiver:** [Current V17 source](Ground_Receiver_V16_3/) · **Windows + offline dashboard:** [Ground Station](Ground_Station/) and [web](web/) · **Release downloads:** [v17.0.1](https://github.com/aidenjantzi1-ctrl/GREEN-ION/releases/tag/v17.0.1)
>
> **Software CI verification is not hardware launch verification.** V17 has not been certified flight-ready; see the [hardware evidence checklist](docs/V17_RELEASE_CANDIDATE.md).

The active V17 sketches retain the directory names `Green_Ion_V16_3` and `Ground_Receiver_V16_3` deliberately: Arduino builds, package scripts and OTA/source compatibility depend on those exact source paths. The **runtime firmware and receiver versions are 17.0.1**, and `web/v14_utils.mjs` reports V17.0. Historic V12, V13, V14 and V16.2 source trees are archived under [Past_Versions](Past_Versions/README.md).

GREEN ION is a **flight recorder and recovery aid only**. It does not ignite, stage, steer or deploy anything. Autonomous **50 Hz microSD recording**, a **3-second prelaunch buffer**, **RKT4 v2 / GIS1** file compatibility, and the unchanged **protocol-v2 70-byte LoRa payload** remain the authoritative flight data path.

## V17.0 features

- Secure bench-managed dual **AP+STA Wi-Fi**, home-network connection with direct `GREEN-ION-V17` access point fallback, and bounded reconnection independent of recording.
- **Live sensor dashboard** with acceleration/gyro, barometric altitude and pressure, battery, microSD, GPS, radio and freshness/health indicators, plus bounded live graphs.
- **WebGL 3D attitude and sample-based replay**, historical flight comparisons, GPS/LoRa recovery map, and stale-data warning behavior.
- Shared microSD/LoRa **SPI ownership** (SD CS 33, LoRa CS 27, LoRa RESET 15, DIO0 32; SCK 5, MOSI 19, MISO 21) that keeps recorder and checkpoint operations ahead of radio work.
- **Home verification**, offline flight analysis, Windows packages and explicit OTA/rollback with checksum-verified firmware assets.

The application retains the dark-blue `#081420` theme and five main tabs: **Dashboard**, **Flight**, **Flights & Analysis**, **Testing**, and **Settings & Advanced**.

## Get started

1. Download the [V17 release](https://github.com/aidenjantzi1-ctrl/GREEN-ION/releases/tag/v17.0.1), verify its `SHA256_MANIFEST.json`, and use the [V17 installation guide](docs/V17_INSTALL_UPDATE.md).
2. Power the Feather safely with its antenna attached and connect to **GREEN-ION-V17** at `http://192.168.4.1`; home Wi-Fi is optional.
3. Open the Windows app with `START_GREEN_ION.cmd`, or use the offline web dashboard.
4. Run **Prepare for Flight** and **Run Full Home Test** on the bench with the rocket motor removed. Save actual microSD, radio, GPS and battery evidence.
5. Have your rocket/recovery/launch site checked by the responsible launch safety team before any flight.

## Guides, history and boundaries

- [V17 installation and update/rollback](docs/V17_INSTALL_UPDATE.md)
- [V17 release notes and physical acceptance gates](docs/V17_RELEASE_CANDIDATE.md)
- [V16.3 reference: full home test](docs/V16_3_HOME_TEST.md)
- [V16.3 reference: shared SPI wiring](docs/V16_3_SHARED_SPI_PERFBOARD.md)
- [V16.3 reference: hardware evidence](docs/V16_3_HARDWARE_VALIDATION.md)
- [Recovery map and GPS](docs/V16_2_GOOGLE_MAPS.md)
- [Historical versions index](Past_Versions/README.md)
- [Version history](VERSIONS.md) · [Changelog](CHANGELOG.md) · [GitHub Actions](https://github.com/aidenjantzi1-ctrl/GREEN-ION/actions)
- [Historical wiring guide](docs/V14_WIRING_SOLDER.md) · [Sensor capabilities](docs/V14_SENSOR_CAPABILITIES.md) · [Devices](docs/V14_DEVICES.md)
- [Historical cloud guide](docs/V14_CLOUD.md) · [Improvement plan](docs/V14_IMPROVEMENT_PLAN.md) · [Development reference](docs/V14_DEVELOPMENT.md) · [Quality plan](docs/V14_QUALITY_PLAN.md)

Historical guides in `docs/` are retained where current tests, user help or recovery references still link to them; a version number in a filename is not proof the file is unused. Older tags and releases remain published for rollback. Flight readiness always requires independent mechanical, electrical and RF checks on the actual assembled rocket.
