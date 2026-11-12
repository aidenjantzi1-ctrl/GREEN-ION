# GREEN ION V16.2 implementation

Baseline: `d7a0943835cd2908b0b335869f211d60760b55c4` (V16.1 RC2)

V16.2 preserves the autonomous SD recorder, RKT4 v2 BIN, GIS1 SEN, receive-only ground LoRa, existing cloud accounts/backups, Windows `GREEN_ION_Data`, OTA/rollback, the #081420 visual identity, and exactly five visible tabs.

## Ordered implementation checklist

- [x] Phase 0 — audit current main, recent PRs, firmware, Windows bridge, five-tab UI, recovery, 3D, packaging and validation boundaries.
- [ ] Phase 1 — harden Windows/browser reliability and long-duration recovery tests.
- [ ] Phase 2 — create clearly versioned Feather V16.2 firmware generation and truthful diagnostics without changing RKT4/GIS1 or LoRa airtime contract.
- [ ] Phase 3 — simplify everyday UI; Balanced default; preserve five tabs and essential warnings.
- [ ] Phase 4 — add Dashboard live/replay 2D rocket visualization with measured/estimated/illustrative labels and stale-data freeze.
- [ ] Phase 5 — replace flat trajectory projection with genuine interactive WebGL 3D scene, timestamp playback, camera modes, phase markers and fallback.
- [ ] Phase 6 — embed optional Google Maps JavaScript recovery map with keyless/offline fallback and no committed credentials.
- [ ] Phase 7 — expand SD manager with backup/verification evidence before deletion while preserving exact-name confirmation and BIN-last deletion.
- [ ] Phase 8 — improve flight library filtering/comparison/report entry points without mutating originals.
- [ ] Phase 9 — make Run Full Home Test the primary testing workflow and expand human-readable diagnostics.
- [ ] Phase 10 — preserve one-click Windows startup/update/rollback and improve phone/touch behavior.
- [ ] Phase 11 — regression-lock local-first/cloud-optional integrity.
- [ ] Phase 12 — add mission summary, one-click report, Dashboard replay, missing-data explanations, data-quality scorecard, profiles, first-run setup and accessibility improvements where validated.
- [ ] Phase 13 — apply visual-honesty labels consistently.
- [ ] Phase 14 — extend automated firmware/Windows/UI/3D/reliability coverage.
- [ ] Phase 15 — update beginner-facing setup/update/map/3D/firmware/validation docs.
- [ ] Phase 16 — full CI, PR, merge, v16.2.0 tag and verified release only if all mandatory gates pass.
- [ ] Phase 17 — final broken-reference/API/resource/credential/rollback cleanup.

## Compatibility decisions

1. Keep LoRa protocol v2 / 70 bytes for V16.2 unless a measured need forces a protocol revision. V16.2 adds diagnostics around the packet rather than consuming more airtime.
2. Keep RKT4 v2 and GIS1 byte layouts unchanged.
3. Keep the existing evidence reconstruction worker and replace only the display layer for true 3D.
4. Google Maps is optional. No API key is committed; without a configured restricted browser key or internet, Recovery retains the offline plot, coordinates, distance/bearing and external Maps shortcut.
5. Visual flame, smoke, parachute and ground contact are illustrative unless a recorded measurement specifically supports the event. Relative gyro orientation is estimated, never absolute attitude.
6. Physical RF range, power integrity, sensor accuracy, SD timing, mechanical fit, OTA interruption behavior and real-flight validation remain NOT TESTED until measured on hardware.
