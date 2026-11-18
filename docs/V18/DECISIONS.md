# V18 decisions, ownership and interface contracts

## Locked user decisions (2026-11-14 planning)
1. V18 is a coherent SD-first logger/recovery system, independent of network/UI availability. RKT4 v2 and 50 Hz baseline must survive.
2. Four top-level tabs: **Flight / Recovery / Data / Settings**; adaptive, beginner-friendly one-press PREPARE & ARM, retries and post-save FLY AGAIN.
3. Preflight Wi-Fi ON, in-flight OFF, restore after verified save. Do not gate autonomous recording on radios.
4. Local-only Windows Chrome/Edge offline app; manual BIN import; no cloud account, subscription or remote flight upload.
5. GPS and LoRa recovery use an Arduino Uno + separate LoRa module with USB Web Serial.
6. Five-minute recovery window then three-minute hibernation transmissions; physical wake button; optional charging; optimize existing 350 mAh LiPo first.
7. Correct ROCstock file association: 163 m D, other C; altitude estimates are not validated; replace tape with measured So Long sled and design static pressure ports.
8. No unreviewed merge, force-push, bulk branch deletion, PR closure or release publication.

## Ownership and coordination
- **Primary** `worker/v18-firmware`: Feather state machine, SD-first invariants, phase estimates, power/Wi-Fi scheduling, recovery timing, physical wake input and firmware API contracts; receiver firmware alterations only with explicit coordination.
- **Secondary** `worker/v18-offline-app`: four-tab offline UI, Web Serial/receiver parsing, manual immutable BIN import, local storage/PWA/package and UI tests. Existing `docs/V18_FRONTEND_CONTRACT.md` belongs to this worker until integrated.
- **Integration Manager** `worker/v18-integration`: requirements, audit, contracts, compatibility gates, independent test evidence, version labels, release candidate status and review. So Long Build Helper delivers measurements/manual/mounting recommendations as documentation to this owner. GitHub PR descriptions/review comments are the cross-chat coordination record; no direct chat messaging is assumed.
- Do not edit another worker's owned files concurrently; submit suggested edits via PR comments or dedicated integration commits after agreed handoff.

## Proposed versioned contracts — review before implementation lock
**Firmware status/events:** immutable schema version and monotonically increasing boot/session ID + status timestamp; flight ID/filename; `phase`, `phase_confidence`, `flight_prep_state`, `flight_prep_step`, progress, `blocking_errors[]`, `warnings[]`, `battery_mv`/battery validity, `sd_ready`, `sd_save_verified`, GPS fix/age, LoRa hardware/link status, `wifi_state`. Phase must distinguish ESTIMATED apogee from measured altitude. Use stable machine-readable error codes plus human-readable remedies, with age/staleness and no false READY/SAVED. Backward-compatible optional fields until paired releases ship.

**Commands:** idempotent POST PREPARE & ARM with single-flight transaction/attempt ID; reject while recording/saving/OTA, do not arm on cached/stale UI state; response includes firmware authoritative state. FLY AGAIN requires verified save and allocates a new recorder-owned unique flight file. SD diagnostic creates only bounded temporary files and confirms deletion; never delete flight originals.

**RKT4 v2:** original binary structure, timing, sample scaling and checkpoint conventions remain immutable; parser must prove byte-level compatibility with V17.0.1 samples and older archives. Do not modify BIN format for extra data; sidecars and versioned APIs may carry new fields. SAVED requires completion metadata plus independent readback/size-integrity checks.

**LoRa and Uno USB serial:** retain deployed **70-byte protocol-v2 LoRa** compatibility unless a separately negotiated protocol upgrade and tests are agreed. Receiver serial messages need explicit line framing, version, packet length/checksum, flight/session identity or sequence, GPS validity/coordinates, RSSI, receive timestamp/age and reject/unknown-version states. Missing radio = warning, not simulated connected. No transmission with missing/damaged antenna; receiver remains receive-only. Exact JSON/line schema is **TBD pending actual firmware and receiver source review**; Secondary must not guess it.

**Wi-Fi/power:** states `PRE_FLIGHT_ON`, `IN_FLIGHT_OFF`, `POST_SAVE_ON`, `HIBERNATING` are *proposed observable semantics*, not existing API values. Wi-Fi shutdown must not interrupt launch detection or writes. Save verified before restart; five-minute post-landing recovery period and three-minute hibernation TX interval are user-approved goals; trigger instant/interval, wake behavior, safe low-battery policy and antenna interlocks require bench-verified exact firmware specification.

**Errors:** categorize critical recorder/sensor/unsafe power blockers versus optional GPS/LoRa warnings, UI freshness failures and unrecoverable corruption. No success based solely on a button click; preserve event/state evidence for test traces.

**Conflict policy:** inspect PR #53 / #56 legacy dashboard, Quick Fly, caching, flight file naming and readiness state before cherry-picking. Preserve merged PR #55. Version bump only once integration passes; update firmware/receiver/UI/service-worker manifests, README, CHANGELOG, VERSIONS, package and OTA metadata coherently. No rename of active legacy V16_3 sketch paths without build/path migration tests.
