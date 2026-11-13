# GREEN ION V16.3 — hardware validation boundary

CI verifies source behavior, protocol/file compatibility, browser rendering, package integrity and compilation. It cannot validate the assembled rocket electronics.

## Required physical tests before treating hardware as validated

- shared microSD + RFM9x SPI stress on the actual perfboard;
- valid downloaded BIN/SEN originals and SHA-256 evidence after that stress;
- real Uno receiver packet evidence, RSSI/SNR and packet-loss behavior;
- RF range at the intended site/frequency configuration;
- outdoor GPS acquisition/recovery behavior;
- LiPo, USB transition, reset/brownout and power-margin behavior;
- physical wiring, 3.3 V radio protection, antennas and strain relief;
- OTA update/rollback interruption behavior using safe bench power;
- mechanical installation and RF placement in the actual rocket;
- supervised real flight.

The exact shared-SPI procedure and pinout are in [V16.3 shared SPI perfboard validation](V16_3_SHARED_SPI_PERFBOARD.md). The broader V16.2 validation ledger remains compatible background evidence in [V16.2 hardware validation](V16_2_HARDWARE_VALIDATION.md).

Do not convert CI PASS into a physical PASS. Use PASS / FAIL / INCONCLUSIVE / NOT TESTED based on retained measurements.
