# GREEN ION V16.2 — physical validation still required

Automated tests and CI can verify code paths, file compatibility, packet parsing, packaging and compilation. They cannot certify physical hardware.

Before treating V16.2 as launch-ready for a specific payload, record measured evidence for:

1. Windows + Feather reconnect soak on the actual laptop/Feather.
2. Autonomous SD recording through real Wi-Fi loss; recover and checksum the original.
3. BMP388 and LSM6DSO32 assembled bench behavior and orientation/mounting check.
4. Outdoor GPS fix/current/stale behavior.
5. Receiver-side LoRa range/integrity: CRC/reject, gaps, duplicates/out-of-order, sender reset, RSSI/SNR and reconnection.
6. Shared-SPI sustained SD + LoRa operation.
7. Protected 1S LiPo voltage/brownout behavior under the assembled load.
8. Feather OTA update, interruption handling and rollback on stable bench power.
9. Mechanical mounting, strain relief and antenna placement in the actual rocket.
10. Supervised real flight and recovery evidence.

Optional phone/tablet checks should cover direct Feather Wi-Fi, offline history, touch 3D/map fallback and file download/import.

Use NOT TESTED / PASS / FAIL / INCONCLUSIVE. A software PASS is not a physical PASS and never authorizes launch.
