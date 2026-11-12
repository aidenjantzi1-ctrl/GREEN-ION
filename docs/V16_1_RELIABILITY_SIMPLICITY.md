# GREEN ION V16.1 — Reliability + simplicity

V16.1 keeps the V16 blue interface, the same five visible tabs and the same autonomous recorder/file formats. It is a reversible reliability/usability layer, not a flight-control redesign.

## What changed

### Automatic recovery

- Windows keeps watching for the Arduino Uno even when it is absent at startup.
- If the Uno is unplugged and returns on a different COM port, automatic discovery starts again.
- Feather Wi-Fi reconnection keeps using the V16 delayed/reconnecting/disconnected state model.
- Finished-flight transfer retries remain content/checksum safe; partial transfers are rejected and retried rather than saved.
- A one-click diagnostic report captures software/link evidence without claiming hardware validation.

### First run

Normal Windows use is:

1. Extract the verified V16 Windows package.
2. Double-click `START_GREEN_ION.cmd`.
3. Leave the terminal window open.
4. Plug the Uno in whenever needed; GREEN ION watches USB automatically.
5. Join the Feather GREEN ION Wi-Fi when live Feather status, tests or SD downloads are needed.

The launcher checks Python 3.10+, attempts the pinned pyserial setup, and can fall back to Wi-Fi-only operation if USB serial support cannot be installed.

### Simple diagnostics

Settings & Advanced starts with a compact health card for:

- app/runtime;
- Feather Wi-Fi;
- Uno USB;
- receive-only LoRa;
- GPS evidence;
- SD readiness;
- optional cloud backup;
- local archive.

Detailed diagnostics remain downloadable JSON.

### Guided testing

Testing starts with one guided front door to the existing real one-button home/preflight test. It never simulates a flight or converts missing physical evidence into PASS.

### LoRa recovery contract

The existing CRC-protected 70-byte protocol is deliberately retained. It already carries critical recovery/flight data:

- GPS latitude/longitude and fix age;
- barometric altitude and pressure;
- GPS altitude and speed;
- battery;
- acceleration/rotation;
- satellites;
- flight state;
- health/flags;
- sequence/uptime and CRC.

The Uno adds receiver-side RSSI, SNR, packet-loss/gap/duplicate/reset evidence. Ground LoRa remains receive-only.

### Finder

Recovery now uses confirmed received GPS fixes only.

- Google Maps opens the last confirmed rocket position.
- With browser location permission, GREEN ION compares the rocket fix with the user's current position.
- It shows straight-line distance and bearing.
- Google Maps directions can open from current browser location to the last confirmed rocket fix.
- User current location is not persisted by the finder.
- The offline schematic remains available without internet.

### Flight analysis

- pressure events require sustained evidence;
- launch/apogee/descent/landing get confidence scores;
- IMU/GPS can corroborate pressure events;
- a likely burnout transition is only shown when IMU evidence supports it and is explicitly labeled an estimate;
- GPS altitude change is compared against pressure-derived relative altitude;
- the first report view stays concise while raw/high-rate evidence remains in originals/exports.

### 3D replay

- pressure and GIS1 sensor timelines are aligned using their recorded timestamps;
- horizontal display uses light smoothing while raw GPS remains untouched;
- estimated flight-phase markers appear on the path;
- replay uses animation frames rather than an independent redraw interval;
- recovery/finder continues to use raw confirmed GPS fixes;
- orientation remains relative gyro integration, not absolute attitude.

### Flight library

- search;
- favorites;
- tags;
- notes;
- obvious INCOMPLETE and REVIEW/collision status.

These annotations are stored separately from immutable flight originals.

### Updates and rollback

- Windows update shows release notes before installation.
- Verified packages install side-by-side.
- `ROLLBACK_GREEN_ION.cmd` switches to the previous complete app package.
- `GREEN_ION_Data` is never replaced by app update/rollback.
- Feather firmware update remains a separate bench-only workflow.

### Release automation

A `v16.*` tag runs the V16 release workflow. It must pass application tests, browser tests, JavaScript syntax, web build, Feather compile, Uno compile and Windows package verification before publishing release assets.

## Unchanged safety boundaries

V16.1 does not add ignition, staging, deployment, guidance or LoRa command control. The primary SD recorder remains independent of Windows, browser, LoRa, cloud and 3D visualization.

Real hardware testing is still required for RF range, power integrity, sensor accuracy, SD timing, OTA interruption recovery, mechanical installation and actual flight behavior.
