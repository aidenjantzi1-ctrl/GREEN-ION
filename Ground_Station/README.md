# GREEN ION V12 Ground Station

The Ground Station is the offline Windows application that remains on the laptop. It combines Feather Wi-Fi telemetry and Arduino Uno LoRa telemetry into one session, keeps the dashboard available when either connection disappears, and writes every accepted raw event incrementally. It is a recorder and recovery aid only; it never controls ignition, staging, arming, or deployment.

## First-time Windows setup

1. Install Python 3 from python.org and enable **Add Python to PATH**.
2. Double-click `INSTALL_GROUND_STATION.cmd` once. It installs `pyserial`, the only non-standard dependency.
3. Upload `Ground_Receiver/Ground_Receiver.ino` to the Arduino Uno.
4. Connect suitable antennas before powering either RFM95W radio.

## Normal launch-day startup

1. Connect the Uno receiver to the laptop by USB.
2. Double-click `OPEN_GROUND_STATION.cmd`.
3. The browser opens `http://127.0.0.1:8765`. Keep the command window open.
4. In **Settings**, select the Uno COM port if automatic detection is inconclusive.
5. Connect Windows Wi-Fi to the Feather access point. The default Feather address is `http://192.168.4.1`.
6. Use **Preflight** for readiness checks, **Mission Control** for live status, **Recovery** for GPS, **Analysis** for recordings and sessions, and **Diagnostics** for connection evidence.

No internet, phone, cloud account, Bluetooth, or hotspot is needed. The server continues running when Feather Wi-Fi is lost and automatically uses valid LoRa packets. If both links fail, graphs and the last valid GPS position remain visible and become explicitly stale.

## Final navigation

- **Preflight**: connected hardware readiness checks and results.
- **Mission Control**: the live-flight screen with primary measurements and link health.
- **Recovery**: last valid position, fix age, LoRa age, RSSI, and offline relative track.
- **Analysis**: append-only ground telemetry sessions, validated manual BIN import, Feather SD downloads, checksum deduplication, CSV/JSON exports and printable reports. Original RKT4 files are retained byte-for-byte.
- **Diagnostics**: source errors, duplicates, sequence gaps, rejected out-of-order packets, and parse errors.
- **Settings**: Uno COM port and Feather address. Unsupported configuration is not presented as functional.

This grouping avoids separate History, Analysis, Reports, Map, Receiver and Connection pages while retaining one clear location for each workflow.

## Data integrity and source merging

- Wi-Fi is preferred while fresh because it contains higher-rate fields.
- LoRa takes over automatically when Wi-Fi becomes stale.
- Packets are tracked independently by source and sequence number.
- Duplicates and out-of-order packets are rejected; sequence gaps are counted.
- Valid GPS coordinates are retained separately from the selected source.
- Stale coordinates are never labeled as a live fix.
- Accepted events append to `data/session-*.jsonl`; `data/current-session.json` is an atomic restart snapshot.
- Connection changes never start, stop, or restart the Feather SD recorder.

## Explicit simulation

Simulation never starts by default. For a software-only demonstration:

```powershell
py -3 ground_station.py --sample
```

The page labels the active source `SIMULATION`. Simulation and automated tests are not hardware evidence.

## Full-resolution sensor export

After recovery, copy the matching `.SEN` and `.BIN` files from the SD card and run:

```powershell
py -3 sensor_log.py V12D007.SEN --rkt4 V12D007.BIN --csv V12D007-sensors.csv
```

The exporter validates the GIS1 header, matching RKT4 header CRC, record CRCs, sequence gaps and torn tails. Originals remain unchanged.

The **Analysis** screen can also import an RKT4 BIN directly or download one from the Feather after recovery. A download is accepted only after its byte count and RKT4 v2 header/layout validate. SHA-256 identifies duplicates, and the original BIN is stored unchanged. CSV, JSON and printable HTML/PDF reports are derived copies with an explicit analysis-version and pressure-estimate disclaimer.

## Verification boundary

Automated tests exercise merging, stale handling, sequence gaps, duplicate rejection, GPS retention and persistence. Feather/Uno compilation is verified in CI. Physical radio range, packet-loss behavior, USB reconnect timing, GPS accuracy, SD power-loss survival and full launch-day operation still require measured bench and field tests.
