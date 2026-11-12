# GREEN ION V16 Phase 8 — Real-hardware validation evidence

## Status

**Validation framework implemented. Physical evidence is still NOT TESTED unless someone performs the test on the assembled hardware and records the measured result.**

Software tests and successful compilation are not physical validation. This document and the Testing-tab evidence ledger exist specifically to prevent those categories from being mixed.

The evidence ledger can end in **READY_FOR_HUMAN_REVIEW_NOT_FLIGHT_AUTHORIZATION**. It never returns a launch authorization.

## Required physical evidence

### 1. Windows + Feather reconnect soak

Goal: verify the Phase 1 connection fix on the real Windows computer and Feather.

Record:

- Windows version and GREEN ION V16 package version;
- Feather firmware build/version;
- test duration;
- number/duration of intentional Wi-Fi losses;
- one or more long file transfers during the run;
- whether the localhost dashboard remained open;
- whether status changed through Connected / Delayed / Reconnecting / Disconnected truthfully;
- whether fresh status returned automatically;
- whether any false repeating ONLINE/OFFLINE flapping occurred.

A software simulation cannot PASS this item.

### 2. SD recording through network loss

On the real Feather and SD card:

1. start the existing safe test/recording workflow;
2. remove phone/computer Wi-Fi access while recording continues;
3. reconnect afterward;
4. retrieve the original recording;
5. verify RKT4/GIS1 parsing, byte size/checksum, sample timing and completion/recovery metadata.

The browser/network must not be required for autonomous SD recording.

### 3. IMU + barometer

Run the existing guided hardware test on the assembled sensors.

Retain measured:

- BMP388 readiness/pressure behavior;
- LSM6DSO32 detection and configured ±32 g range;
- stationary acceleration/gyro evidence;
- controlled movement response;
- any read/recovery failures.

Do not convert an INCONCLUSIVE stationary test into PASS manually.

### 4. Outdoor GPS

With clear sky:

- obtain a real current fix;
- record satellites, fix age and coordinates;
- block/remove reception and confirm live → stale behavior;
- verify last-known coordinates remain available but never become current again without a new fix.

### 5. Receiver-side LoRa range/integrity

Use antennas on both radios. Coordinate frequency use with the launch site and applicable rules before field transmission.

Record actual Uno-receiver evidence:

- received valid packets;
- CRC/rejected packets;
- duplicates/out-of-order packets;
- estimated sequence gaps/loss;
- sender restarts;
- RSSI and SNR;
- link loss and automatic recovery;
- test distances/environment.

Sender-side “five packets sent” is not sufficient evidence of receiver success.

### 6. Shared SPI under load

Record real SD data while radio traffic occurs.

Verify:

- original flight/test file remains readable;
- recorder timing/sample-miss evidence is acceptable;
- radio failure cannot stop the primary recorder;
- SD/radio coexistence does not corrupt files.

### 7. LiPo / power

Using the intended protected 1S LiPo and appropriate measurement equipment:

- compare GREEN ION battery reading with an external meter;
- observe voltage sag and stability during realistic activity;
- verify no unexpected reset/brownout;
- document power source and test duration.

### 8. OTA + rollback

Bench only, stable power, no launch preparation.

Verify:

- expected firmware image/version/checksum;
- alternate OTA partition behavior;
- reboot into expected image;
- rollback behavior;
- controlled interruption/failure behavior;
- cleanup of maintenance state;
- complete guided home test after update.

Until this is measured, OTA remains software/compile-tested only.

### 9. Mechanical / RF installation

With the real rocket/payload:

- payload fit and retention;
- wire/connector strain relief;
- SD retention;
- antenna mounting/routing;
- no conductive shorts or pin conflicts;
- actual mass/CG review appropriate to the rocket;
- recovery hardware and access.

Software cannot validate these items.

### 10. Supervised real flight

Only after normal launch-site safety review and the physical bench evidence has been reviewed.

Preserve:

- original BIN/SEN/TXT;
- Windows/receiver log when available;
- GPS recovery evidence;
- mission evidence JSON;
- validation evidence JSON;
- notes about any link loss, recovery or data-quality anomalies.

A successful flight is evidence for that configuration and flight; it is not a blanket certification of every future configuration.

## Optional phone/tablet evidence

Verify the installed/offline browser history and analysis on the actual phone/tablet. Document whether hosted local-network access works in that browser; use the direct Feather fallback when it does not.

## Testing-tab evidence ledger

V16 stores a local evidence ledger with four permitted statuses:

- NOT_TESTED
- PASS
- FAIL
- INCONCLUSIVE

Every physical item defaults to NOT_TESTED.

“Capture current diagnostics” can save local software/runtime/telemetry snapshots, but **never changes a physical test to PASS**.

The exported evidence JSON includes the requirement definitions, result notes/references, diagnostic snapshots and an overall advisory state.

## Acceptance rule

Do not describe GREEN ION V16 as physically validated or flight-tested until measured evidence exists for the applicable items above.

Even when all required ledger items are marked PASS, the application reports **READY_FOR_HUMAN_REVIEW_NOT_FLIGHT_AUTHORIZATION**. Normal rocket, motor, range and launch-site safety procedures remain independent.
