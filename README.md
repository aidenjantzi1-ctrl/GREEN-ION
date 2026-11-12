# GREEN ION — Rocket Flight Recorder

**Current target: GREEN ION V16.2.** V16.2 keeps the exact dark-blue `#081420` identity and exactly five visible tabs: **Dashboard, Flight, Flights & Analysis, Testing, Settings & Advanced**. It adds a versioned Feather/Uno firmware generation, stronger reconnect behavior, an honest live/replay rocket visualization, genuine WebGL 3D replay, optional embedded Google Maps recovery, backup-aware SD cleanup, improved flight comparison, guided testing and clearer diagnostics.

GREEN ION is a **flight recorder and recovery aid**, not a flight-control computer. It does not control ignition, staging, deployment, guidance or pyrotechnics. Primary RKT4/GIS1 recording remains autonomous on the Feather and does not depend on Windows, LoRa, maps, cloud or visualization.

[**Open the blue dashboard**](https://green-ion-beta.vercel.app/) · [**GitHub releases**](https://github.com/aidenjantzi1-ctrl/GREEN-ION/releases) · [**Automated tests**](https://github.com/aidenjantzi1-ctrl/GREEN-ION/actions/workflows/build-v14.yml)

## Start here

| Goal | Guide |
|---|---|
| Windows install, update and rollback | [V16.2 Windows setup](docs/V16_2_WINDOWS_SETUP.md) |
| Flash/update the Feather | [V16.2 Feather update](docs/V16_2_FIRMWARE_UPDATE.md) |
| Update the Uno receiver | [V16.2 Uno receiver](docs/V16_2_UNO_RECEIVER.md) |
| Understand receive-only radio data | [V16.2 LoRa protocol](docs/V16_2_LORA_PROTOCOL.md) |
| Download or safely delete SD flights | [V16.2 SD file management](docs/V16_2_SD_FILE_MANAGEMENT.md) |
| Configure the embedded recovery map | [V16.2 Google Maps](docs/V16_2_GOOGLE_MAPS.md) |
| Use the interactive 3D viewer | [V16.2 3D viewer](docs/V16_2_3D_VIEWER.md) |
| Understand the live rocket animation | [V16.2 live visualization](docs/V16_2_LIVE_VISUALIZATION.md) |
| See implementation status | [V16.2 implementation checklist](docs/V16_2_IMPLEMENTATION.md) |
| Record physical validation evidence | [V16.2 hardware validation](docs/V16_2_HARDWARE_VALIDATION.md) |

## Everyday workflow

1. Run `START_GREEN_ION.cmd` on Windows.
2. Plug the Uno receiver in when needed; GREEN ION auto-detects/reconnects it.
3. Join Feather `GREEN-ION-V16-2` Wi-Fi for live hardware status, tests and SD transfer.
4. Select the rocket/motor profile and use **Prepare for Flight**.
5. Use **Run Full Home Test** on the bench with motors removed.
6. During flight, the Feather records autonomously. Windows may show Wi-Fi/LoRa telemetry, but a link loss does not stop SD recording.
7. After landing, use Recovery. The embedded Google Map is optional; the offline GPS plot always remains.
8. Reconnect Feather Wi-Fi, download/verify the flight, then use reports, comparison and the WebGL 3D replay.
9. Verify browser/Windows/cloud backup evidence before deleting a completed Feather SD package.

## V16.2 compatibility

- RKT4 v2 BIN remains unchanged.
- GIS1 SEN remains unchanged.
- Existing recorded flights remain readable.
- LoRa stays protocol v2, fixed 70 bytes and receive-only on the ground.
- Windows data remains under `%USERPROFILE%\GREEN_ION_Data`.
- Supabase remains optional; originals are local first.
- Windows app update/rollback and Feather OTA remain separate operations.
- The direct Feather dashboard at `http://192.168.4.1` remains a fallback when hosted browser networking is restricted.

## Visual honesty

The Dashboard animation and 3D viewer distinguish evidence types. GPS, pressure, battery and recorded IMU data are measured when valid. Barometric altitude, velocity, phase and gyro-integrated relative attitude are estimates. Flame, smoke, parachute and ground contact are illustrative unless separate physical evidence supports them. Stale telemetry freezes instead of inventing an ongoing trajectory.

## Existing hardware/reference guides

These older guides remain useful for the current hardware and are intentionally retained:

- [Wiring & soldering](docs/V14_WIRING_SOLDER.md)
- [Sensor capabilities & formats](docs/V14_SENSOR_CAPABILITIES.md)
- [Devices & connectivity](docs/V14_DEVICES.md)
- [Private cloud backups](docs/V14_CLOUD.md)
- [Improvement architecture](docs/V14_IMPROVEMENT_PLAN.md)
- [Developer guide](docs/V14_DEVELOPMENT.md)
- [Quality / physical validation](docs/V14_QUALITY_PLAN.md)
- [Version archive](Past_Versions/README.md)

## Validation boundary

CI can test software, compile the Feather/Uno targets, render the WebGL smoke scene, build the offline web app, verify Windows packaging and validate release hashes. It **cannot** prove actual RF range, GPS reception, SD timing on a physical card, battery/brownout behavior, wiring, antenna installation, OTA interruption recovery or a real flight. Those remain measured physical tasks in the [V16.2 hardware validation guide](docs/V16_2_HARDWARE_VALIDATION.md).

Sources: [Adafruit Feather ESP32 V2 pins](https://learn.adafruit.com/adafruit-esp32-feather-v2/pinouts), [Google Maps JavaScript API](https://developers.google.com/maps/documentation/javascript/overview), [Supabase storage security](https://supabase.com/docs/guides/storage/security/access-control), [WebGL](https://developer.mozilla.org/en-US/docs/Web/API/WebGL_API).
