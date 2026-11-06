# GREEN ION V12 physical validation record

Fill this in on the actual Feather ESP32 V2, rocket payload, Windows laptop, optional Uno/RFM95W receiver and iPhone. Each result must be **PASS**, **WARNING**, **FAIL** or **NOT TESTED**. A compile, unit test, synthetic browser page or simulation is not a physical PASS. Keep dated evidence, device/firmware versions, rocket profile, battery and SD card identity with each run. The current cloud result for every physical item below is **NOT TESTED**.

| Physical check | Procedure and evidence to record | Current result |
| --- | --- | --- |
| Power and wiring | Power off; inspect 3.3 V logic, ground, SPI chip selects, I²C bus, LiPo polarity, strain relief and antenna. Record the exact RFM95W board variant. | NOT TESTED |
| BMP388 detection and pressure | Run Preflight or Feather bench status; record detection, pressure, temperature and stationary noise. Compare with a known local pressure reference if available. | NOT TESTED |
| LSM6DSO32 IMU | Run the stationary five-second and 30-second checks; record mean magnitude, maximum deviation, gyro bias, sample count, then move each axis and confirm response. | NOT TESTED |
| PA1010D GPS outdoor fix | Outdoors with clear sky, record satellite count, fix quality, coordinates, UTC, age, GPS altitude and loss/reacquisition behavior. Compare coordinates and elevation with a known reference. | NOT TESTED |
| Barometric and GPS altitude | Record both independently at rest and during a measured low-risk elevation change; document baseline drift and disagreement. Do not treat a fused estimate as validated. | NOT TESTED |
| LiPo voltage | Compare telemetry with a meter at a safe stationary voltage; check under radio/SD load and after recharge. Record supply and load conditions. | NOT TESTED |
| SD initialization and readback | Run the idle SD test; check capacity, test-file write/read/cleanup and a real BIN/SEN download with byte count and SHA-256. Preserve originals. | NOT TESTED |
| Autonomous recording and Wi-Fi loss | Run the separate motor-free `TESTnnn.BIN` bench procedure; physically disconnect Wi-Fi while the Feather remains powered, reconnect, stop/save, then validate RKT4 records and any SEN companion. | NOT TESTED |
| Power interruption and recovery | On a dedicated disposable test recording, interrupt power under a controlled bench setup; inspect original, recovered/torn-tail status and journal/report behavior. Never use a real flight file as the test target. | NOT TESTED |
| Feather Wi-Fi | Join `GREEN-ION-V12`, confirm live status on the laptop, then observe disconnect and return without falsely converting stale data to live. | NOT TESTED |
| Uno USB serial | Upload receiver firmware, identify COM port, observe JSON boot/protocol status, then disconnect/reconnect USB and record recovery. | NOT TESTED |
| RFM95W LoRa link | Attach suitable antennas before any transmission. Confirm matched frequency/modem configuration, bench packets, sequence, RSSI, dropout/recovery and range under the permitted site setup. Receiver hardware is currently not connected. | NOT TESTED |
| iPhone HTTPS Companion | Use the documented private LAN arrangement and trusted certificate; pair, inspect Mission Control/Recovery/Analysis, sign out and check unauthorized access. | NOT TESTED |
| iPhone GPS, cellular and maps | Grant then deny Safari location, verify manual fallback, street/satellite imagery and offline coordinates. Record whether cellular internet works while the local laptop link remains live. | NOT TESTED |
| Flight-file download and backup | Download the actual SD BIN/SEN, verify transferred bytes, inspect Analysis graphs/replay/exports, and create and restore a ZIP backup to a separate location. | NOT TESTED |

If any item fails, stop the affected test, preserve logs/raw files, and troubleshoot before a launch. Preflight PASS represents the check it names at that moment; it is not a substitute for physical integration and field validation. No automatic ignition, staging or deployment control exists in GREEN ION.
