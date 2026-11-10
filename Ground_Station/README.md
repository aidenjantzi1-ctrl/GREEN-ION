# GREEN ION V12 Ground Station

The ground station is the equipment and software that stays on the ground, receives the rocket recorder's radio telemetry, displays it, and saves a separate recovery log. It never controls ignition, staging, arming, or deployment.

## Open it

Double-click `OPEN_GROUND_STATION.cmd`. It **asks for the Arduino Uno COM port** (for example `COM5`) to start the live radio ground station. Press Enter without entering a port for explicitly labeled simulated/demo data. The website opens automatically; keep the command window open. Close Arduino Serial Monitor before starting the live mode, since both programs cannot share the COM port.

Demo mode uses only Python's standard library. It plots an offline relative GPS trajectory and saves every row to `logs/*.csv`. It is not hardware evidence. **Live mode requires the Arduino Uno receiver, its connected RFM95W radio/antenna, and `pyserial`.**

The equivalent manual command is optional:

```powershell
cd Ground_Station
py -3 ground_station.py
```

The dashboard opens at `http://127.0.0.1:8765` and distinguishes live, stale, and missing link/GPS data.

Later, when using the physical Arduino Uno receiver, install `pyserial` and provide its port. This is not needed to open or demonstrate the website:

```powershell
py -3 -m pip install pyserial
py -3 ground_station.py --port COM5
```

The relative map does not need an internet connection or paid map service. It intentionally omits street/satellite tiles; coordinates remain available for copying to a separate navigation app.

## Export a full-resolution flight log

Copy the matching `V12Dxxx.SEN` and `V12Dxxx.BIN` files from the SD card, then run:

```powershell
py -3 sensor_log.py V12D007.SEN --rkt4 V12D007.BIN --csv V12D007-sensors.csv
```

The exporter validates the GIS1 header, flight filename, matching RKT4-header CRC,
every complete record CRC, sequence gaps, and any torn tail. GPS rows are labeled
`live`, `stale`, or `missing`; stale last-known coordinates are never labeled live.
Corrupt or torn input is not exported unless `--allow-partial` is supplied, and a
mismatched RKT4 binding is never overridden. The original `.SEN` and `.BIN` files
are read-only and remain unchanged.


## Live link quality and reconnect behavior

Start **LIVE** mode by entering the Uno's COM port in `OPEN_GROUND_STATION.cmd`. Python now automatically retries after USB/serial disconnection, shows actual Uno receiver status/error messages, counts `radio_bench` packets separately from `telemetry` packets, estimates sequence loss, duplicates, out-of-order packets and Feather reboots, and maintains an exponentially weighted received RSSI estimate.

The receiver sends a status heartbeat about every five seconds. The dashboard shows receiver-side bench packet count: **Feather TX does not prove Uno RX**. Confirm five actual valid `radio_bench` packets on this computer before checking the corresponding manual home-test step. The loss percentage is a sequence-gap estimate, not a calibrated radio-range probability.

A current GPS fix must come from a fresh radio packet marked live. When LoRa packets stop, the computer downgrades displayed link/GPS status to stale and preserves last-known coordinates with an age; repeated stale positions do not reset the last-known timestamp.

Click **Download telemetry CSV** in the web dashboard, or open `http://127.0.0.1:8765/api/export`. No CSV exists until actual telemetry has been received (or demo samples generated). Receiver bench markers and JSON status lines are deliberately excluded from the normal flight CSV.

### Known limits

- Does not authenticate sender identity beyond the existing shared packet format/CRC; do not use it for flight-critical actions.
- Receiver counters are based on what the Uno actually receives; periods with no radio packets cannot reveal the true number lost except by sequence gaps.
- Moving the same hardware to a new rocket does not transmit its local Feather rocket profile over the 70-byte LoRa packet.
- Physical antenna/range tests, device overheating, regulated frequency coordination and battery measurements remain to be done.

