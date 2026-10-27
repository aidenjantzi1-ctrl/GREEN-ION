# GREEN ION V11 — approved implementation plan (S01–S10)
**Target release: GREEN ION V11.** Current released baseline: V10. **Status:** Approved as V11 planning instructions on 2026-10-27; implementation, testing and release are still pending. Creating these documents does not release V11.

All S01–S10 belong to the V11 redesign. Codex must implement them in stages, preserve the V10 baseline, and use the V11 release/version name only when a tested release is explicitly authorized.

## Goal
Feather V2 + BMP388 + microSD should perform only essential **sampling, timestamping, calibration, recording, storage/recovery, health monitoring, lightweight control and updates**. Phone/iPad/computer performs heavy flight analytics, graphing, comparisons, reporting and history locally in its browser.

## Exact existing code to inspect
- `Green_Ion_V10/Green_Ion_V10.ino`: logger, server routes and binary storage
- `Green_Ion_V10/Dashboard.h`: inline browser UI/JS
- `CODEX_HANDOFF.md`: release discipline and hardware cautions
- `README.md`: RKT4, wiring, controls and behavior
- `.github/workflows/build-firmware.yml`: test/build pipeline
- `tests/`: fixture and regression tests

## Documents / implementation order
1. **S06** `06-reliability-testing.md`: freeze baseline, build test harness.
2. **S04** `04-data-format-and-transfer.md`: lock format/API contract and raw transfer.
3. **S02** `02-browser-analysis-engine.md`: make offline deterministic analysis work with V10 logs.
4. **S03** `03-offline-dashboard.md`: use analysis module in browser from imported/saved BIN.
5. **S01** `01-recorder-only-firmware.md`: remove redundant Feather analytics *only after* browser parity.
6. **S05** `05-flight-reports.md`: reports/graphs/replay/motor comparisons.
7. **S07** `07-performance-and-battery.md`: benchmark and tune, without reducing safety.
8. **S09** `09-flight-history-and-trends.md`: local flight library.
9. **S10** `10-installable-pwa.md`: installable offline experience, subject to HTTPS/origin limitations.
10. **S08** `08-codex-implementation-plan.md` applies to *every* stage.

## Codex credit-saving rules (mandatory)
1. Read **only** the relevant S0x file, this index and the exact linked code files/functions needed for the current step; do not repeatedly audit the entire repo.
2. Implement **one narrowly-scoped stage per task/PR**. No unrequested features, cosmetic refactors or rewrites.
3. Search the code once for the target symbols/routes; record results in the PR description. Reuse previous findings.
4. Prefer adapting existing working code to adding libraries/frameworks. Preserve existing user workflows.
5. Before editing, state a short plan: affected files, expected behavior and tests. If scope exceeds one stage, stop after the agreed stage.
6. Run targeted tests first, full suite/firmware compile once before completion. Do not loop on builds without diagnosing error output.
7. Report changed files, exact tests run and pass/fail, hardware checks NOT TESTED, and remaining issues in concise form.
8. Never create new version folders, move historical releases or commit binaries until the implementation stage specifically calls for a release.
9. Stop and ask for a decision only if preserving log compatibility or physical-device safety requires a breaking choice.


## Shared constraints
- GREEN ION logs data only: **never** control ignition, staging, parachutes or recovery deployment.
- Recording and power-loss recovery must remain independent of Wi-Fi or a connected browser.
- Preserve RKT4 header/16-byte record layout, timestamp meaning, calibration metadata, OTA rollback and original BIN files.
- Label pressure-derived acceleration, G and phase events as estimates, not direct measurements.
- Compilation/simulation does not establish hardware or flight readiness.
