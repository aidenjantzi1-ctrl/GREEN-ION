# GREEN ION V16.2 — LoRa telemetry contract

Ground LoRa remains receive-only. The primary SD recorder is independent of radio success.

V16.2 keeps:

- protocol version: **2**;
- fixed packet size: **70 bytes**;
- CRC protection;
- packet sequence and uptime evidence;
- flight state;
- pressure/barometric altitude;
- GPS coordinates, altitude, speed, age and satellites;
- battery;
- selected acceleration/rotation values;
- flags and health field.

V16.2 adds diagnostic health meanings in currently available health bits without changing packet size:

- bit 8 — SD initialization error;
- bit 9 — GPS runtime error;
- bit 10 — IMU runtime lost;
- bit 11 — high-rate sensor-log records dropped.

Receiver-side Windows evidence continues to track packet gaps, duplicates, out-of-order frames, sender resets, reconnects, RSSI and SNR.

No dynamic schedule is allowed to delay SD sampling. V16.2 therefore keeps the proven bounded radio queue/scheduling instead of increasing transmit rate. The existing configured frequency is deployment/site dependent and must be coordinated with local rules and launch-site practices; software compilation is not RF approval.
