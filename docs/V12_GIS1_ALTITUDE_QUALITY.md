# GIS1 optional GPS altitude freshness marker

GIS1 version 1 keeps its 32-byte header, 68-byte record, field offsets, and CRC. V12 now uses two previously zeroed record bytes to describe the freshness of the GPS altitude that accompanies a position:

| Record byte | Value | Meaning |
| --- | --- | --- |
| 62 | `0x80` or `0x81` | `0x80` means quality marker present, altitude not current. Bit 0 adds altitude current when the GPS position is also current. |
| 63 | `0xA5` | Identifies this optional marker. |

The record CRC at bytes 64–67 covers both bytes. Earlier files have zero in these positions, which means altitude freshness is **unknown**. Earlier GIS1 readers ignore these reserved bytes and continue reading the original coordinates, altitude, speed and fix age. The current readers preserve that legacy interpretation. No RKT4 v2 or existing raw file is rewritten.

The Feather sets the altitude-current bit only when the recorded GPS position is current, the latest parsed GGA altitude was valid, and that altitude is at most three seconds old. Analysis excludes an explicitly noncurrent altitude from its GPS peak and replay graph. It retains legacy altitude values with the label **quality unverified**. This bit reports firmware freshness; it does not establish GPS accuracy, calibration, or a verified landing location.

GIS1 still requires finite GPS altitude and speed to store a position in a companion record. A valid position can therefore be absent from GIS1 until both fields are available. The Feather's live status and autonomous RKT4 pressure recording are separate from that GIS1 limitation. Confirm real PA1010D sentence timing and BIN/SEN behavior during the physical checklist.

The Feather status now reports `sensor_log_gps_position_omitted`: a saturating, boot-lifetime count of samples offered to the GIS1 prebuffer or active log that had valid coordinates but lacked finite GPS altitude or speed. Ground Station shows it beside `sensor_log_dropped`, which instead counts active queue overflow. The omission count does not establish how many records were written, because the prebuffer can replace older samples before recording starts. LoRa-only telemetry does not carry this counter; Ground Station marks it unavailable when Wi-Fi status is not current. No placeholder altitude or speed is written to GIS1.
