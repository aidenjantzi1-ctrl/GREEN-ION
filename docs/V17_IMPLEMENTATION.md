# GREEN ION V17 implementation status

V17 is the integration release built from the validated V16.3 recorder baseline plus Codex 1 AP+STA Wi-Fi and Codex 2 visualization/replay/recovery work.

## Preserved safety contracts

- 50 Hz sensor recording.
- Three-second prelaunch buffer.
- RKT4 v2 and GIS1 compatibility.
- Recorder independence from Wi-Fi/dashboard availability.
- microSD and LoRa share SPI with separate chip selects and recorder-first ownership.
- No ignition, staging, pyrotechnic, recovery-deployment or guidance control.

## Integrated feature set

- AP+STA home Wi-Fi with protected credential management.
- Shared-SPI reliability and home verification.
- Bounded live graphs.
- WebGL live/replay visualization.
- Timestamp-synchronized replay.
- Multi-flight comparison.
- GPS freshness and offline recovery tooling.
- IndexedDB history and offline PWA behavior.
- Windows ground station, OTA/rollback and packaged release flow.
- Comparison filename injection regression protection.

## Release gate

The exact V17 commit must pass Python, Node/browser, native C++, WebGL rendering, Feather ESP32 V2 compile, Uno compile, Windows packaging and V17 release smoke tests before merge to main.

Physical validation remains separate and required.
