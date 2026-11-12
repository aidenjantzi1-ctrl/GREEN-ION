# GREEN ION V16 Phase 5 — Data and cloud

## Goal

Make original flight data easier to trust, group, verify, export and back up without making cloud availability part of recording, recovery or analysis.

Local originals remain authoritative.

## Flight packages

V16 groups files by flight identity:

- `.BIN` — immutable RKT4 pressure/battery recording.
- `.SEN` — immutable GIS1 IMU/GPS companion when available.
- `.TXT` — metadata/recovery notes when available.

A package can be:

- **COMPLETE** — BIN + SEN + TXT are present.
- **RECORDED_PARTIAL** — a BIN is present but one or more companions are missing.
- **COMPANION_ONLY** — companion data exists without the BIN.
- **COLLISION** — two different checksums claim the same flight/kind; V16 refuses to silently replace one.

Missing companions do not invalidate a valid original BIN.

## Local verification

The Data tab can reread every stored original and recompute SHA-256. Verification does not rewrite the file.

The package manifest records:

- original filename;
- kind;
- SHA-256;
- byte size;
- origin;
- local key;
- package completeness;
- missing companions.

The exported manifest contains metadata/checksums, not a replacement for the original files.

## Cloud state

Supabase remains optional private backup. V16 exposes explicit sync state:

- SIGNED_OUT
- UNAVAILABLE
- OFFLINE
- IDLE
- PENDING
- RETRYING
- SYNCED
- ERROR

The state includes pending count, last successful sync and retry information when available. Offline or failed cloud backup never marks local originals bad and never blocks mission recording or local analysis.

## Safety and compatibility

- Existing SHA-256 deduplication remains.
- Cloud restore still verifies size and SHA-256 before saving.
- Existing files are never overwritten.
- RKT4/GIS1 formats are unchanged.
- Cloud cannot command the rocket, arm the recorder or perform OTA.
- Browser/local disk originals remain useful without an account or internet.

## Validation boundary

Checksum verification establishes byte integrity of the stored copy. It does not establish sensor accuracy, GPS quality, RF range, battery safety, launch safety or actual flight validity.
