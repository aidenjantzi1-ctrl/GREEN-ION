# S03 — Offline flight analyzer UI
**Status:** Approved; not implemented. **Dependencies:** S02, S04.

## Scope
Reuse existing UI vocabulary from `Green_Ion_V10/Dashboard.h` but separate **live device controls** from **offline analysis**. Phone/tablet/computer must analyze files when Feather is powered off.

## Implementation
1. Add obvious modes: **Recorder connected** (status, arm, stop, download) and **Analyze offline** (import BIN/TXT, charts, reports). Never show inactive recorder actions as available offline.
2. Support native file picker and drag/drop (desktop), robust mobile file import; store original byte array without mutation. Decode via S02 worker.
3. Cache or bundle analyzer modules separately from firmware; use responsive layout, accessible button labels, metric/imperial units, mobile-safe graphs and progress/error states.
4. If same interface is Feather-served, JavaScript still runs on client. Plan separate local/static app distribution so offline analysis never depends on access to `192.168.4.1`.
5. Explain browser storage retention; exports must work even when storage is unavailable; recover gracefully from rejected/quota-limited local storage.

## Acceptance
Open imported V10 BIN offline on phone/iPad/desktop; analyze and export without Feather or internet; clear distinction between live and archived values; no impact on active recordings.

## Shared constraints
- GREEN ION logs data only: **never** control ignition, staging, parachutes or recovery deployment.
- Recording and power-loss recovery must remain independent of Wi-Fi or a connected browser.
- Preserve RKT4 header/16-byte record layout, timestamp meaning, calibration metadata, OTA rollback and original BIN files.
- Label pressure-derived acceleration, G and phase events as estimates, not direct measurements.
- Compilation/simulation does not establish hardware or flight readiness.
