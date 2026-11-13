# GREEN ION V16.3

GREEN ION V16.3 finishes the shared-SPI reliability work for the Feather ESP32 V2 perfboard while preserving the V16.2 application experience.

## Highlights

- **Feather V16.3:** shared microSD + RFM9x SPI ownership, deferred DIO0 servicing, recorder-first scheduling, bounded radio recovery and stronger reset/SD/radio diagnostics.
- **Uno V16.3:** receive-only protocol-v2 receiver with packet integrity, RSSI/SNR, bench evidence and explicit receiver version.
- **Full Home Test:** real 50 Hz temporary RKT4 recording, SD readback/cleanup, bounded concurrent LoRa traffic and actual Uno receiver evidence when connected.
- **Truthful radio results:** no receiver means NOT TESTED; transmitter activity alone is never receiver PASS.
- **Existing V16.2 features preserved:** clean five-tab #081420 UI, live rocket visualization, genuine WebGL 3D replay, embedded Maps/offline recovery, flight history/comparison, backup-aware SD cleanup, optional Supabase and Windows auto-reconnect.
- **Verified updater:** official V16.3 Feather application image + SHA manifest are checked before the separate deliberate bench OTA prompt.
- **Packaging:** Windows ZIP/SHA/manifest, offline web ZIP, Feather V16.3 BIN, Uno V16.3 HEX, complete V16.3 source ZIP and release SHA manifest.

## Compatibility

- RKT4 v2 BIN unchanged.
- GIS1 SEN unchanged.
- LoRa protocol v2 remains 70 bytes.
- Ground LoRa remains receive-only.
- Windows data stays under `%USERPROFILE%\GREEN_ION_Data`.
- Existing V16.2 recordings, history and cloud data remain compatible.
- Windows update/rollback and Feather firmware update/rollback remain separate operations.

## Physical validation still required

CI and compilation do not prove the assembled perfboard, RF range, GPS performance, LiPo/brownout margin, mechanical installation, OTA interruption recovery or real-flight behavior. Follow `docs/V16_3_SHARED_SPI_PERFBOARD.md` and `docs/V16_3_HARDWARE_VALIDATION.md` before treating hardware as validated.
