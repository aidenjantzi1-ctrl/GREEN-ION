# GREEN ION — Rocket Flight Recorder

> **LATEST PUBLISHED RELEASE: GREEN ION V16.3 (`v16.3.0`)** · **V17 software integration candidate on `main` (hardware checks pending)**  
> **V17 source (legacy folder path):** [`Green_Ion_V16_3/`](Green_Ion_V16_3/) · **Current Uno receiver:** [`Ground_Receiver_V16_3/`](Ground_Receiver_V16_3/) · **Downloads:** [V16.3.0 release](https://github.com/aidenjantzi1-ctrl/GREEN-ION/releases/tag/v16.3.0)
>
> Older V12/V13/V14/V16.2 folders remain for regression tests, rollback/reference, and historical build workflows. They are **not the current flight version**.

GREEN ION V17 preserves the exact dark-blue `#081420` interface and exactly five visible tabs: **Dashboard, Flight, Flights & Analysis, Testing, Settings & Advanced**.

GREEN ION is a **flight recorder and recovery aid**, not a flight-control computer. It does not control ignition, staging, deployment, guidance or pyrotechnics. Primary RKT4/GIS1 recording remains autonomous on the Feather and does not depend on Windows, LoRa, maps, cloud or visualization.

[**GitHub releases**](https://github.com/aidenjantzi1-ctrl/GREEN-ION/releases) · [**Automated tests**](https://github.com/aidenjantzi1-ctrl/GREEN-ION/actions/workflows/build-v14.yml)

## V17 software integration (based on V16.3)

V16.3 focuses on the real Feather perfboard where the microSD card and RFM9x share SPI. It adds explicit bus ownership, defers radio DIO0 SPI work out of interrupt context, gives sampling/SD/checkpoints priority over radio, bounds radio recovery, adds shared-SPI diagnostics, and extends **Run Full Home Test** with concurrent temporary 50 Hz SD + LoRa evidence and actual Uno receiver confirmation when connected.

It preserves:

- RKT4 v2 BIN and GIS1 SEN formats;
- protocol-v2 70-byte LoRa packets;
- receive-only ground LoRa;
- V16.2 live rocket visualization, WebGL 3D replay, Maps recovery, flight history/comparison and optional Supabase backup;
- Windows data under `%USERPROFILE%\GREEN_ION_Data`;
- separate Windows app update/rollback and deliberate Feather firmware update/rollback.

Software validation does **not** prove actual perfboard wiring, RF range, GPS reception, battery margin, SD timing on hardware or flight readiness.

## V17 integrated visualization and home Wi-Fi (software candidate)

V17 integrates AP+STA home Wi-Fi with physical authorization and bounded reconnection, bounded live sensor graphs, WebGL orientation, GPS recovery, sample-based 3D replay, and multi-flight overlays. The V17 software candidate includes Feather and Uno firmware version 17.0.0, but **hardware validation and launch clearance remain outstanding**. See [Codex 2 candidate installation and validation](docs/V16_3_CODEX2_VISUAL_RECOVERY.md).

## Start here

| Goal | Guide |
|---|---|
| Install/update Windows, Feather and Uno | [V16.3 update & rollback](docs/V16_3_UPDATE_GUIDE.md) |
| Run the one-button bench test | [V16.3 Full Home Test](docs/V16_3_HOME_TEST.md) |
| Wire/test shared SD + LoRa SPI | [V16.3 shared-SPI perfboard](docs/V16_3_SHARED_SPI_PERFBOARD.md) |
| Record remaining physical validation | [V16.3 hardware validation](docs/V16_3_HARDWARE_VALIDATION.md) |
| See implementation status | [V16.3 implementation checklist](docs/V16_3_IMPLEMENTATION.md) |
| Configure the recovery map | [V16.2 Google Maps](docs/V16_2_GOOGLE_MAPS.md) |
| Use the WebGL 3D viewer | [V16.2 3D viewer](docs/V16_2_3D_VIEWER.md) |
| Safely manage Feather SD flights | [V16.2 SD management](docs/V16_2_SD_FILE_MANAGEMENT.md) |

## Everyday workflow

1. Run `START_GREEN_ION.cmd`.
2. Plug in the Uno receiver; Windows auto-discovers/reconnects it.
3. Join Feather `GREEN-ION-V17` Wi-Fi for live status, tests, SD transfer or deliberate OTA.
4. Use **Prepare for Flight**, then **Run Full Home Test** on the bench with motors removed.
5. During flight the Feather records autonomously; Wi-Fi/LoRa loss does not stop primary SD recording.
6. After landing use Recovery, reconnect Feather Wi-Fi, import/verify the flight, then open reports/comparison/WebGL replay.
7. Verify backup evidence before deleting a completed Feather SD package.

## Visual/data honesty

GPS, pressure, battery and recorded IMU values are measured when valid. Barometric altitude, filtered velocity, flight phase and gyro-integrated relative attitude are estimates. Flame, smoke, parachute and ground contact are illustrative unless separate physical evidence supports them. Stale telemetry freezes rather than inventing an ongoing trajectory.

## Existing hardware/reference guides

- [Wiring & soldering](docs/V14_WIRING_SOLDER.md)
- [Sensor capabilities & formats](docs/V14_SENSOR_CAPABILITIES.md)
- [Devices & connectivity](docs/V14_DEVICES.md)
- [Private cloud backups](docs/V14_CLOUD.md)
- [Improvement architecture](docs/V14_IMPROVEMENT_PLAN.md)
- [Developer guide](docs/V14_DEVELOPMENT.md)
- [Quality / physical validation](docs/V14_QUALITY_PLAN.md)
- [Version archive](Past_Versions/README.md)

Historical V16.2 documentation and release source remain in the repository for rollback/reference.


[**V17 release candidate and physical acceptance checklist**](docs/V17_RELEASE_CANDIDATE.md)
