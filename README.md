# GREEN ION — Rocket Flight Recorder

**Current: V14 Beta 4 (software beta, physical testing pending).**

[**Open the blue flight dashboard**](https://green-ion-beta.vercel.app/) · [**Download V14 releases**](https://github.com/aidenjantzi1-ctrl/GREEN-ION/releases) · [**Automated tests**](https://github.com/aidenjantzi1-ctrl/GREEN-ION/actions/workflows/build-v14.yml)

GREEN ION records a rocket's pressure, motion, GPS and battery measurements using the **Adafruit Feather ESP32 V2**. It records flight data independently on an SD card. A Windows PC, iPhone or iPad can open the **same original blue dashboard and tabs**. Windows can also receive **Uno USB / LoRa** telemetry; the phone works without LoRa.

## Choose a guide

| What you want | Where to go |
|---|---|
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

The original **Status, Flight, Field, Testing, Utilities, Storage and System** tabs remain. **Connections, History, Cloud and Updates** add optional Windows LoRa, offline RKT4/GIS1 analysis, private Supabase backups and user-confirmed GitHub update downloads.

You can disable **Sensor Lab, historical SEN analysis, automatic cloud uploads, compact spacing or imperial display** in System without changing the onboard recorder. Original BIN/SEN/TXT files, checksum-verified restore, plots, comparison, replay and manually exported GPS GeoJSON work offline.

**Hardware commands:** Use the Feather's Wi-Fi and local **http://192.168.4.1** backup dashboard when direct connection from the hosted HTTPS website is blocked. The iPhone website cannot read Uno USB serial. Downloads from GitHub **never flash automatically**; bench-only firmware OTA requires separate confirmation.

## Software vs hardware validation

GitHub Actions compiles both firmware targets and tests the web app, but **no real launch, GPS/RF range, SD, LiPo, iPhone local-network, account-based cloud restore or OTA rollback test can be certified by CI**. Complete the supervised [hardware validation guide](docs/V14_QUALITY_PLAN.md) before relying on V14 for a launch.

Sources: [Adafruit Feather V2 pins](https://learn.adafruit.com/adafruit-esp32-feather-v2/pinouts), [Supabase security](https://supabase.com/docs/guides/storage/security/access-control), [Vercel Git deployments](https://vercel.com/docs/git), [Web Serial compatibility](https://developer.mozilla.org/en-US/docs/Web/API/Web_Serial_API).
