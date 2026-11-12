# GREEN ION V16 Phase 2 — Unified telemetry

## Goal

Create one stable mission telemetry model from two independent communication paths:

- Feather Wi-Fi for full live device state and command-authorized local control.
- Uno/LoRa for compact receive-only backup telemetry and recovery evidence.

The UI should show only the few readings needed during a mission. Full-resolution sensor streams stay in immutable RKT4 BIN / GIS1 SEN recordings for later analysis.

## Unified telemetry contract

Windows exposes `/api/v16/telemetry` with schema `GREEN-ION-V16-TELEMETRY-1`.

Each selected live reading includes:

- value;
- source (`wifi`, `lora`, or `none`);
- source age;
- freshness (`live`, `stale`, or `missing`).

Wi-Fi and LoRa freshness are tracked independently. Wi-Fi has priority when it is live because it carries the complete Feather status. A live LoRa field can fill a Wi-Fi field that is unavailable, and LoRa becomes primary when Wi-Fi disappears. Stale data stays visible only with an explicit stale label.

## Packet and session identity

Receiver-side evidence now retains:

- one random Windows receiver-session ID per ground-station process;
- sender-session index that increments when a corroborated Feather reboot is observed;
- packet ID built from receiver session + sender session + sequence;
- sender uptime, sequence and reception UTC;
- duplicate, gap, out-of-order and sender-restart classification;
- packet/accepted/missing counts;
- receiver reconnect count;
- RSSI and SNR when provided by the receiver.

The existing 70-byte telemetry packet is unchanged. The Uno adds protocol and SNR to its USB JSON output only.

## Deduplication and history

CRC-valid duplicates and out-of-order frames remain receiver evidence, but they do not replace the current live sample. A bounded in-memory radio history retains the latest 240 received telemetry events, and the unified endpoint returns the latest 120. This prevents unbounded memory growth during long sessions.

## Receiver discovery

Double-clicking `OPEN_GROUND_STATION.cmd` no longer asks for a COM port every time. The Python ground station safely auto-selects a recognizable Arduino Uno when it can identify one unambiguously.

If automatic discovery cannot safely choose a receiver, Windows continues Wi-Fi-only rather than guessing. Manual overrides remain available:

- `OPEN_GROUND_STATION.cmd --port COM5`
- `OPEN_GROUND_STATION.cmd --wifi-only`
- environment variable `GREEN_ION_UNO_PORT`

Demo remains explicit with `OPEN_GROUND_STATION.cmd DEMO`.

## Browser behavior

The Windows link panel now reads the single fused endpoint instead of separately polling link and telemetry endpoints. It shows:

- simultaneous link availability;
- unified primary source;
- concise Wi-Fi state;
- telemetry integrity summary;
- LoRa packet counts/loss;
- RSSI/SNR;
- last packet age;
- receiver state.

Detailed sensor values remain in Sensor Lab/history/analysis instead of flooding the live mission view.

## Safety and compatibility

- LoRa remains receive-only.
- No radio control fallback exists.
- Stale Wi-Fi data cannot authorize commands.
- Autonomous SD recording remains independent of the Windows app.
- RKT4 v2, GIS1 and the 70-byte telemetry packet remain unchanged.
- No dedicated firmware mission UUID exists yet, so Phase 2 does not fabricate one. Active/finished log filename is exposed only as a file identity when available.

## Automated acceptance coverage

Phase 2 tests:

- Wi-Fi only;
- LoRa only;
- both links simultaneously;
- stale link behavior;
- protocol mismatch;
- duplicate packets;
- out-of-order packets;
- sequence gaps;
- sender reboot/session rollover;
- receiver disconnect/reconnect;
- bounded radio history;
- browser telemetry validation/deduplication;
- Uno protocol/SNR source contract;
- unchanged 70-byte radio packet format.

Physical RF range, real USB enumeration, real Wi-Fi loss/recovery and antenna performance still require hardware validation.
