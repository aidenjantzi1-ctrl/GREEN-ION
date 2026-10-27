# S08 — GREEN ION V11 precise Codex implementation protocol
**Target version: V11. Current released baseline: V10.** Approved as V11 documentation only; not implemented or released.

## Execution rule
**Do not implement all S01–S10 in one task.** Work in dependency order from `suggestions/README.md` and finish one stage with tests + concise handoff before starting next. When asked to implement a stage, do not ask to redo repository-wide discovery already described here.

## Stage gates
- **Gate A (baseline):** S06 golden V10 fixtures, original test run, documented baseline endpoints/record schema, existing firmware compilation evidence.
- **Gate B (format):** S04 compatibility spec, browser decoder fixtures and transfer integrity tests.
- **Gate C (client analysis):** S02 pure analysis tests; S03 import/offline test on browser; no Feather processing removed yet.
- **Gate D (slim firmware):** S01 migrate one computation category at a time while retaining API compatibility; repeat Gate A tests.
- **Gate E (features):** S05 reports; S07 profiling; S09 history; S10 PWA, each independently scoped.

## How each Codex task should proceed
1. State the requested stage, exact files to modify (maximum scope ~one subsystem), functions/routes under study, and expected outcome in 5–10 lines.
2. Read its S0x file plus files actually required. Search symbols narrowly (e.g. `rg -n 'RKT4|handleClient|FLUSH_MS' Green_Ion_V10 tests`). Avoid large repeated searches or rewriting all historical releases.
3. Implement smallest functional changes preserving V10 raw logs and documented baseline APIs.
4. Run closest tests first, then entire existing Python test suite, dashboard check and firmware compile where available; if a command is unavailable, record why once.
5. Report changed paths, test command/results, quantitative performance evidence if relevant, browser/hardware NOT TESTED items and follow-up required. Stop.

## Release handling
Do not touch `Past_Versions/`, `CHANGELOG.md`, `VERSIONS.md`, compiled `.bin` or version numbering during early stage work unless an explicit release is requested. For the approved V11 release, create the new root release folder `Green_Ion_V11/`, update version/build identifiers and documentation, move the former V10 release into `Past_Versions/` only at the release stage, and follow `CODEX_HANDOFF.md`, preserve previous root release and verify artifact provenance. Avoid prematurely replacing the published V10 firmware.

## Decisions requiring explicit escalation
Breaking RKT4 format, weakening recovery/protections, removing existing OTA functionality, changing autonomous flight-recording semantics, insecure PWA-to-device transport, or hardware assumptions unsupported by bench tests.

## Shared constraints
- GREEN ION logs data only: **never** control ignition, staging, parachutes or recovery deployment.
- Recording and power-loss recovery must remain independent of Wi-Fi or a connected browser.
- Preserve RKT4 header/16-byte record layout, timestamp meaning, calibration metadata, OTA rollback and original BIN files.
- Label pressure-derived acceleration, G and phase events as estimates, not direct measurements.
- Compilation/simulation does not establish hardware or flight readiness.
