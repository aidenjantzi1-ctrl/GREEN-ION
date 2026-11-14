# GREEN ION V16.3 integration handoff

This is the integration working record, not a release announcement or a hardware approval.
The repository is already at **V16.3**; do not revert to V12 to satisfy an older
integration-branch name.

## Baseline (13 November 2026, initial inspection)

- Base branch: `main`.
- Base commit: `9d5d789c98c9b92c3d7f2a47a666399f822e4d98`.
- Main CI: [run 43453198414](https://github.com/aidenjantzi1-ctrl/GREEN-ION/actions/runs/43453198414) completed successfully for app-tests, firmware-build, Windows packaging, release smoke and existing publish gate.
- Published historical tag: [v16.3.0](https://github.com/aidenjantzi1-ctrl/GREEN-ION/releases/tag/v16.3.0). Preserve it unchanged.
- Main firmware: `Green_Ion_V16_3/`; receiver: `Ground_Receiver_V16_3/`.
- The current `main` baseline is **software/CI verified only**. No real-hardware flight-ready claim.

## Branch ownership and integration order

| Source | Role | Inspection status |
| --- | --- | --- |
| `integration/v12` | Integration / CI guardrails | Owned by integration manager; do not put unfinished features here |
| `v16.3/home-wifi-ap-sta` | Codex 1, home Wi-Fi/AP+STA | [PR #47](https://github.com/aidenjantzi1-ctrl/GREEN-ION/pull/47); inspect updated head/CI before merging |
| `codex2/v16-3-visual-replay-recovery-20261113` | Codex 2, live graphs/replay/comparison/recovery | Work observed progressing; inspect updated head and create/review PR before merging |
| `v16.3/shared-spi-reliability` | Historical SPI reliability work | Already behind main at initial inspection; do not merge indiscriminately |
| Prior V9/V12 PRs | Historical/stacked proposals | Do not merge wholesale into V16.3 |

Use committed changes and PR discussion as the handoff mechanism. Do not change
another agent's branch or working files. The two feature branches can advance
during review; earlier checks do not certify a later head.

## Integration-owned guardrails

- The CI build workflow now covers `integration/**` and `codex2/**` push
  branches and discovers all `tests/*.test.mjs` tests.
- A regression test now rejects duplicate numeric Feather GPIO assignments
  among I2C, SPI, SD, LoRa, buzzer and battery roles.
- Current configured SD/LoRa pins: SCK=5, MOSI=19, MISO=21,
  SD CS=33, LoRa CS=27, RESET=15 and DIO0=32.
- Do not change SPI chip selects, LoRa packet length (70 bytes), RKT4 v2
  or GIS1 binary layouts as a side effect of Wi-Fi or UI integration.

## Review finding requiring resolution

- [Issue #49](https://github.com/aidenjantzi1-ctrl/GREEN-ION/issues/49):
  comparison headings currently interpolate imported flight filenames into
  `innerHTML` (both baseline and Codex 2 branch). Use `textContent` for
  each dynamic heading and test with an HTML-like filename before integration.
  Do not overwrite Codex 2's unfinished comparison work.

## Gate before any feature merge to main

1. Read the source branch's latest commit, PR diff, reviews and latest check runs.
2. Confirm Wi-Fi credentials never leak through status, logs, exports, or unauthenticated management endpoints.
3. Confirm Wi-Fi connect/reconnect/scanning cannot block the 50 Hz acquisition loop or alter an active recording.
4. Confirm visualization/replay uses actual sample timestamps, respects stale GPS/telemetry and bounds graphs/memory.
5. Review overlapping file changes deliberately, notably firmware dashboard, hosted dashboard, and CI workflow.
6. Run the merged commit's Python, JS, native C++, WebGL, firmware/Uno build, Windows package and release smoke checks.
7. Preserve the existing V16.3 tag/release. Use a new candidate identifier for a new build; do not republish a stable tag.
8. Open/review an integration PR; merge only if code review, required checks and branch protections permit it.

**Hardware remains NOT TESTED through repository inspection alone.**
Before any flight-ready claim, perform the physical microSD+LoRa bus stress test,
50 Hz timing/log recovery, real Uno receipt, outdoor GPS, power/brownout,
OTA/rollback, RF/antenna and supervised flight checks in
[V16.3 hardware validation](V16_3_HARDWARE_VALIDATION.md) and
[shared SPI perfboard validation](V16_3_SHARED_SPI_PERFBOARD.md).

## What should not happen

- No force-push, reset of another agent's branch, or branch deletion.
- No relocation of Arduino sketches or long-lived archives solely for appearance.
- No ignition, staging, pyrotechnics, or recovery deployment controls.
- No release or main merge based on an older CI run or simulated hardware proof.
