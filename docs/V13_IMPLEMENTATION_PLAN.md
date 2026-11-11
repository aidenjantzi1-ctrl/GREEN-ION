# GREEN ION V13 — Unified Dashboard Implementation Plan

**Goal:** One Windows dashboard at 127.0.0.1:8765 using the actual Feather 192.168.4.1 dashboard layout, with Wi-Fi controls and Uno/LoRa receive-only telemetry.

## Changes

1. Compile a separate V13 Feather sketch derived from V12, preserving existing RKT4/GIS1 recorder, recovery system, LoRa protocol and V12D### file naming.
2. Host the blue Feather dashboard directly on the Windows ground station. Keep the phone/iPad dashboard running on the Feather independently.
3. Python monitors both Feather Wi-Fi and Uno serial in parallel. API status uses the full Feather result when fresh, otherwise a radio-derived subset only. A dedicated ground-link endpoint exposes packet age, source, RSSI, receiver status and last GPS.
4. When Wi-Fi drops, hide Wi-Fi-only tests, settings, controls, downloads and OTA; keep actual LoRa measurements and last valid GPS. On both-link loss, label data stale. Restore full functions upon Wi-Fi return.
5. Only proxy HTTP API calls to the fixed Feather 192.168.4.1 host. Never send control or update commands over LoRa; avoid guessing SD recording success, GPS live status or vertical speed.
6. Save computer CSV logs under the user profile, not inside the replaceable app download folder.
7. Build both boards and Python/JS tests in GitHub Actions, assemble one ZIP (app, firmware sources/binaries, libraries, instructions). Publish first prerelease after successful main build.

## Verification

Host tests, firmware compilation and packaging are software checks only. Real Feather/Uno wiring, simultaneous links, receiver range, SD recording and recovery, GPS, battery, antennas, RF compliance and flight safety still require physical testing.

**Status:** V13 software development; flight certification is NOT claimed.
