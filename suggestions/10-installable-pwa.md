# S10 — Installable offline web app
**Status:** Approved; not implemented. **Dependencies:** S03/S04 and browser-origin design decision.

## Technical constraint
A PWA/service worker generally requires a secure context (HTTPS or permitted localhost origins). The Feather AP at `http://192.168.4.1` is not a generally installable service-worker origin. Do not claim that adding `manifest.json` to that AP alone provides reliable offline installation.

## Implementation plan
1. Distribute standalone analyzer over a stable HTTPS origin or as a locally served desktop app; use a web app manifest, icons and scoped service worker to precache only static shell/assets. Keep versioned update strategy and offline banner.
2. Enable user-selected local BIN/TXT import, browser analysis, history and PDF/CSV export without connecting to Feather.
3. Keep a separate optional **device connection** workflow; investigate iOS/Android/desktop browser restrictions on HTTPS-to-HTTP private-network requests, CORS, mixed content and local AP access. Do not assume fetch to `192.168.4.1` succeeds from HTTPS.
4. If direct PWA-to-Feather transfers are blocked, provide supported fallback: use Feather's own local web UI to download BIN into Files, then import it into the standalone analyzer. Avoid unsafe transport workarounds.
5. Verify install/uninstall, version update, cold offline start and file picker/export on iPhone/iPad Safari, Android Chrome and desktop Chrome/Edge where devices are available.

## Acceptance
App installs from supported secure origin, opens offline after cold restart, analyzes locally imported V10 BIN files, exports reports, and documents any device transfer restrictions accurately.

## Shared constraints
- GREEN ION logs data only: **never** control ignition, staging, parachutes or recovery deployment.
- Recording and power-loss recovery must remain independent of Wi-Fi or a connected browser.
- Preserve RKT4 header/16-byte record layout, timestamp meaning, calibration metadata, OTA rollback and original BIN files.
- Label pressure-derived acceleration, G and phase events as estimates, not direct measurements.
- Compilation/simulation does not establish hardware or flight readiness.
