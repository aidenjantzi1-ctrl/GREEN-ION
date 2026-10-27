# S04 — RKT4 format and reliable file transfer
**Status:** Approved; not implemented. **Dependency:** S06 fixtures.

## Implementation
1. Document actual RKT4 header and all 16-byte field offsets, endianness, timestamp scale, valid ranges and metadata source by reading `Green_Ion_V10/Green_Ion_V10.ino`. Do **not** invent offsets.
2. Create golden fixture BIN/TXT pairs from known V10 behavior plus intentionally truncated, empty and corrupt cases; keep raw originals immutable.
3. Preserve existing endpoints during migration. Provide streaming/chunked file reads with bounded RAM, correct content length/type, disconnection handling and zero changes to active flight files. Avoid download workloads while recording if they disturb sampling; expose a safe busy response.
4. Browser verifies downloaded length/checksum when available and warns on incomplete transfer. Never report file integrity as verified based only on HTTP success.
5. Support resumption *only if* range semantics are tested end-to-end; otherwise retry full download. Include flight ID, log version, calibration reference, sample rate and UTC/relative-time caveats in companion metadata.
6. Define future log versioning without changing the current RKT4 records. If extensions are unavoidable, use companion files or an explicitly versioned forward-compatible format.

## Acceptance
V10 BIN survives byte-identical round-trip; large download under limited RAM; phone disconnect/retry and partial corruption detected; logging not blocked; legacy CSV/TXT behavior covered during transition.

## Shared constraints
- GREEN ION logs data only: **never** control ignition, staging, parachutes or recovery deployment.
- Recording and power-loss recovery must remain independent of Wi-Fi or a connected browser.
- Preserve RKT4 header/16-byte record layout, timestamp meaning, calibration metadata, OTA rollback and original BIN files.
- Label pressure-derived acceleration, G and phase events as estimates, not direct measurements.
- Compilation/simulation does not establish hardware or flight readiness.
