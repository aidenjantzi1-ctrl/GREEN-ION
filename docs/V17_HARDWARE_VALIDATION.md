# GREEN ION V17 — required physical validation

V17 CI can validate software behavior, source contracts, browser rendering, packaging and compilation. It cannot certify the assembled flight hardware.

Before calling GREEN ION flight-ready, record PASS evidence for:

- actual Feather ESP32 V2 boot and sensor initialization;
- BMP388, LSM6DSO32 and PA1010D operation on the installed I2C wiring;
- actual microSD + RFM95W shared-SPI stress on the perfboard at 50 Hz;
- no SD/LoRa chip-select collision and valid downloaded RKT4 v2 / GIS1 files;
- three-second prelaunch buffer and launch/landing state behavior on the real device;
- real Uno receiver packets, RSSI/SNR and link-loss/recovery behavior;
- outdoor GPS fix, freshness, last-known recovery and antenna placement;
- AP-only and AP+STA operation, bad-password handling, router loss/recovery and direct AP fallback;
- LiPo/USB transitions, brownout/reset evidence and power margin;
- OTA update, interrupted-update recovery and rollback on safe bench power;
- mechanical mounting, strain relief, antenna clearance and SD retention;
- supervised launch and post-flight data recovery.

Use the detailed V16.3 shared-SPI procedure in `docs/V16_3_SHARED_SPI_PERFBOARD.md`; the hardware topology is unchanged in V17.

Do not convert CI PASS into physical PASS.
