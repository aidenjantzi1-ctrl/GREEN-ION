# GREEN ION V16 Phase 1 — Windows reliability

## Goal

The Windows ground station must stay open and keep the mission session intact through short latency spikes, prolonged Feather Wi-Fi loss, file transfers and automatic reconnection. A temporary missing response must not create the old repeating ONLINE/OFFLINE effect.

## Connection states

The local bridge now reports four user-facing states while preserving a stricter command gate:

- **Connected** — a fresh successful Feather status response exists and no current status error is recorded.
- **Delayed** — a recent good response exists, but a poll failed or the Feather is busy with another bounded request.
- **Reconnecting** — the last successful response is older and the bridge is retrying automatically.
- **Disconnected** — no successful response has existed for the longer disconnect window.

The latest successful response age is exposed separately. Last-known Feather measurements remain available after a link interruption as `wifi_stale`; they are not labeled live and cannot authorize controls.

## Request coordination

- The Python bridge still serializes access to the ESP32 HTTP server.
- Background status polling now waits only briefly for the upstream slot. If a download or another request owns the slot, the poll is **deferred**, not recorded as a failed Wi-Fi probe.
- Browser-side status polling is coalesced: simultaneous `/api/status` callers share one local request instead of generating duplicate upstream work.
- File/control relays fail fast when the Feather is busy instead of forming an unbounded queue.
- Socket timeouts remain bounded. Phase 1 does not solve congestion by simply making every timeout larger.

## Safety and data behavior

- Visual connection state and command authorization are deliberately different.
- Commands still require a recent verified Feather response inside the stricter control-freshness window.
- LoRa remains receive-only.
- Stale Feather measurements are retained for context but marked stale.
- Original SD files, RKT4 v2 and GIS1 are unchanged.
- The localhost dashboard remains available even when Feather Wi-Fi disappears.

## Automated reliability coverage

Phase 1 adds checks for:

- one-hour simulated polling without state/memory accumulation in the bridge state model;
- transient failed status polls;
- delayed → reconnecting → disconnected progression;
- reconnect clearing failure state;
- long/active upstream work deferring status polling without creating a false failure;
- strict control freshness remaining shorter than display freshness;
- simultaneous browser status calls being coalesced;
- stale cached Feather status not becoming command-authorized Wi-Fi;
- explicit Wi-Fi state/age diagnostics in the V16 UI.

Existing receiver tests continue to cover packet gaps, duplicates, malformed data and receiver reconnect behavior.

## Hardware validation still required

No physical Feather, Uno, RFM9x, antenna, GPS, SD card or LiPo was exercised by this software phase. Real Windows/Feather Wi-Fi behavior must still be bench-tested later before claiming flight reliability.
