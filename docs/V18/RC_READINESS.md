# GREEN ION V18 — Release-candidate readiness report

**Assessment: NOT READY FOR RELEASE CANDIDATE APPROVAL.** This is a pre-RC integration audit, not an official release, deployment, firmware update or launch authorization.

## Exact refs and integration trail
- Stable main: `17f517f275be7b52e3e71a1f2d656edbc65a08ea` (merged PR #55, V17.0.1 SD-first baseline).
- PR #58 (V18 standalone offline frontend): `9134089367323fe445ff26e043a7a380defd5cbc`, open draft. Seven exact source files staged on integration branch in commit `07119fd99a13edccfe730f13db2283481f328dc9`. Worker branch not changed and PR not merged.
- Targeted integration-only GPS validity/staleness fixes and two regression tests: commits `935b89ef181517749b7de5e2b9419fef9b591656` and `c2d7a9ed00aaa87ec7b1d95982cf05f124bcadbc`. The fix prevents null coordinates from being treated as numeric zero and prevents aged packets from displaying a live fix.
- PR #60: integration/planning branch `worker/v18-integration` (draft). Current exact head should be obtained live before review or merge.
- PR #53 (historic V17 stack) `cc5842f62c19ac71f30174e7c48b58e3a6f71745`, draft/conflicted; PR #56 (old Quick Fly stack) `2a0161364407bbbc999d93f34034df472e57c69a`, draft/conflicted. Do not merge wholesale over PR #55.
- PR #57 (V17 role prompts) `2702a0d229fbf0d57bf8edc5efe0c213c5c6d516`, open, documentation-only.
- **No** `worker/v18-firmware` branch or V18 Primary PR was found in the 69-branch head inventory at audit. Heavy coders may have local/unpushed work: do not overwrite or assume complete.
- Exact full branch-to-SHA preservation snapshot: [BRANCH_AUDIT.md](BRANCH_AUDIT.md). **No branches deleted, refs force-updated, PRs closed or release tags created.**

## Confirmed software CI evidence (historical completed heads, not final combined candidate)
- Frontend PR #58 head `9134089367323fe445ff26e043a7a380defd5cbc`: [Actions #430](https://github.com/aidenjantzi1-ctrl/GREEN-ION/actions/runs/43673094793) **success**, including Python regression, native C++ SD/SPI/Wi-Fi, Node browser tests, WebGL smoke, JS syntax, web assets, Feather ESP32 V2 compile, Uno compile, Windows V16 packaging and V17 smoke artifacts. Publish steps skipped.
- Prior integration documentation head `3c29cbbf58c05f4cdd2b555f2af97c08b5dd8403`: [Actions](https://github.com/aidenjantzi1-ctrl/GREEN-ION/actions/runs/43682144782) **success** with the same workflow classes. This was before frontend staging.
- Combined staging head `07119fd99a13edccfe730f13db2283481f328dc9`: [Actions](https://github.com/aidenjantzi1-ctrl/GREEN-ION/actions/runs/43682679441) **in progress** at audit.
- Integration fix head `c2d7a9ed00aaa87ec7b1d95982cf05f124bcadbc`: [Actions](https://github.com/aidenjantzi1-ctrl/GREEN-ION/actions/runs/43682783246) **in progress** at audit.
- Branch-audit head `bd06424340981ce11e99a47b10dbc25085e1bf85`: [Actions](https://github.com/aidenjantzi1-ctrl/GREEN-ION/actions/runs/43682809771) **queued** at audit.
- Example exact commands from verified CI job logs:
  - `python3 -m unittest discover -s tests -p 'test_*.py' -v`
  - `node --test tests/*.test.mjs`
  - `arduino-cli compile --fqbn esp32:esp32:adafruit_feather_esp32_v2 --library libraries/GreenIonProtocol --library libraries/GreenIonRecorder --output-dir build/feather Green_Ion_V16_3`
  - `arduino-cli compile --fqbn arduino:avr:uno --library libraries/GreenIonProtocol --output-dir build/uno Ground_Receiver_V16_3`
  - `python3 tools/build_v16_windows_package.py --website built-web --output windows-release --commit "$GITHUB_SHA"`
  - Native test job used `g++ -std=c++17 -Wall -Wextra -pedantic` (full flags should be copied from workflow).
  These commands were observed from GitHub Actions logs, **not executed locally by the Integration Manager**.

## Feature readiness and unresolved blockers
| Feature | Repository evidence | RC gate |
| --- | --- | --- |
| V17 SD-first 50 Hz RKT4 v2, autonomous log | Existing main firmware | Preserve in final regression/physical SD write, readback, brownout/reboot |
| PREPARE & ARM | Existing V17 API + V18 frontend call | Confirm no duplicate command, clean retry, non-destructive temp checks, sequential flights |
| Flight phases/SAVED | V18 displays V17 recorder status; save evidence uses completion flag + TXT fetch | Verify exact V18 firmware phase/error mapping, no stale save acceptance, independent BIN readback |
| Wi-Fi ON/off/on | Documented V18 requirement only | Missing firmware branch/implementation and real transition proof |
| 5-minute recovery and 3-minute hibernation TX | Documented V18 requirement only | Missing firmware behavior, RF-safe TX and measured power proof |
| Physical wake, safe charging/350 mAh endurance | Documented V18 requirement only | Missing electrical/hardware evidence |
| Offline four-tab frontend | PR #58 staged in isolation; CI passed on original PR | Verify integrated source CI and actual Windows launcher/packaged entry point |
| Windows package | Existing CI builds **V16/V17** launcher/package; starter script says V17 | V18 entrypoint + actual offline package launch/install not demonstrated |
| Manual BIN import | PR #58 source uses original ArrayBuffer, parser and local storage | Test true original-byte round-trip, malformed/truncated files, dedup and storage clear/restore |
| Uno Web Serial | PR #58 uses 115200-baud Chrome/Edge API; prior Uno protocol-v2 newline JSON contract | Actual receiver + USB permission/disconnect/hardware RF evidence |
| GPS/LoRa | Frontend parsing and warning semantics; RF previously nonfunctional | Replace damaged antennas, confirm physical radios/pins/range with safe RF setup |
| So Long bay/stability/recovery | No measured payload/motor evidence supplied | Official manual, measured dimensions/mass, venting, CG/CP, stability, staging/recovery acceptance |

## Priority decisions
1. Do **not** merge PR #53/#56 wholesale or declare V18 firmware complete. Preserve RKT4 serialization and active Arduino sketch/build paths.
2. Treat PR #58 as staged for independent integration testing, not production; reconcile with the Primary firmware/receiver contract as soon as that branch is available. Existing V18 frontend consumes V17 status fields and protocol-v2 serial JSON; no V18-only firmware fields are assumed.
3. Do not label current Windows V16/V17 packaging as a completed V18 installer. Document how V18 is accessed locally; create versioned V18 launcher/package changes only after contracts and smoke tests pass.
4. After final software CI, perform hardware SD/sensor/cell current/low-voltage, GPS/LoRa antenna/range, Web Serial, pressure-port response, payload mounting, CG/stability, stage coupling and recovery checks with logged actual measurements. CI is not flight authorization.
5. Recheck main and workers for concurrent movement, require reviewed diffs/checks, and request explicit user approval before a V18 release publication/deployment or any bulk branch/PR cleanup.

**Explicitly not performed:** release/tag publishing, deploying, flashing Feather/Uno, main merge, user-environment hardware tests, branch deletion or PR closure.


## Concurrent worker handoffs and review results (later live audit)
- **PR #61** `worker/v18-hardware`, head `36c0d9d6695c3418794a7b3db7e87d25c54b613f`, adds only `docs/V18/HARDWARE_PLAN.md`. Reviewed and staged documentation on integration at commit `0b7fc8d043d01523ac2db1c6ddbe360b390209f1`. Manufacturer specifications are not a substitute for measured usable tube ID/length, loaded stability and actual recovery clearance. [CI run](https://github.com/aidenjantzi1-ctrl/GREEN-ION/actions/runs/43682911875) succeeded on its doc-only head.
- **PR #63** `worker/v18-firmware`, initial reviewed head `486418d6619fcd8f7cb5b357c3c18158c23866f1` (open draft): adds deterministic optional Wi-Fi shutdown after SD recorder starts, verified-save Wi-Fi restore, header/file-size/TXT structural readback and tests while preserving RKT4 v2 and LoRa v2. **NOT staged/merged**: review comment flags an unresolved short-flight/never-suppressed pending Wi-Fi state that can inhibit future service, a truncated metadata token edge case, and overstated full-byte verification. [PR review request](https://github.com/aidenjantzi1-ctrl/GREEN-ION/pull/63). CI [run](https://github.com/aidenjantzi1-ctrl/GREEN-ION/actions/runs/43683057170) was in progress at inspection.
- **PR #62** `worker/v18-codex-overnight` (open draft) adds read-only RKT4 audit, focused Python tests and Codex report. **NOT staged/merged**: review requests explicit nonzero failure for zero found BINs, and valid completion metadata/filename checks rather than treating arbitrary existing TXT as verified completion. Preserve heavy-coder branch until resolved. [PR #62](https://github.com/aidenjantzi1-ctrl/GREEN-ION/pull/62).
- **PR #58 Secondary follow-up**: new UI settings, automatic serial reconnect and phase timeline reconciled on integration in `22e5ec81c81b4b9744dc6d46114ec695ed62c8f6`; updated tests reconciled in `6b5e32f20c761951edb1531ccc037544ac484dc3`. Kept the integration-only missing GPS coordinate fix, and worker branch untouched. Final combined CI still required.
- Original staged frontend integration [Actions run](https://github.com/aidenjantzi1-ctrl/GREEN-ION/actions/runs/43682679441) completed **SUCCESS**, while the latest integration commits must have their own completed CI. No active release/deployment or `main` merge was performed.
- Late branch snapshot counted **71** branches; original 69-head audit and late additions are in [BRANCH_AUDIT.md](BRANCH_AUDIT.md). The user has not approved branch deletion or PR closure.

**Revised stop-ship:** resolve Primary PR #63 state edge case and clarify save verification scope, review PR #62 audit correctness, obtain final independent CI on the **combined exact head**, verify Windows V18 package/default entry point, then complete physical SD/antenna/GPS/battery/So Long acceptance. This document intentionally does not label V18 RC ready.


## Latest integration staging revision
- Firmware PR #63 head `70fc016080ee7a1d733456880a2f9e189cb6c2ac` was **staged only to integration** as `1f3fa497e0cb828f2a14c2961d380c9f5f7155f0` after short-flight Wi-Fi flag and metadata-buffer fixes were reviewed. The original Primary branch/PR remains unmerged. Firmware code still needs software CI on the exact combined head and real SD/ESP32 tests; readback remains structural, not cryptographic sample-by-sample proof.
- Preview launcher code and tests added on integration: `Ground_Station/ground_station.py --v18-offline` opens `/v18-offline.html#flight`; `Ground_Station/START_V18_PREVIEW.cmd` is an opt-in Windows entry point. The normal V17 dashboard stays the default. Web site build copies V18 source assets to `web/dist`; Windows packaging copies the website and Ground_Station tree. This is a compatibility preview, **not** official V18 Windows release branding or physical qualification.
- PR #62 RKT4 audit remains draft awaiting no-files/metadata correctness and a passing integrated test. No RKT4 format changes made.
- No evidence yet for five-minute recovery transmitter window, every-three-minute hibernation transmissions, working wake button, measured 350 mAh mission endurance, repaired LoRa antennas/range, measured So Long fit/stability or true barometric altitude. These are P0 hardware/feature blockers to release.
- **Status remains NOT RC-READY**, no deployment, no V18 release publication, no main merge, no branch deletion or PR closure.


## 2026-11-15 UX2 development preview integration — current authoritative status

**DEV-TEST BLOCKED pending exact combined-head CI and a real Windows launch/package smoke. NOT a release candidate and NOT flight ready.** This section supersedes stale historical branch-status statements above; retain earlier entries as an audit trail.

- Stable `main`: `17f517f275be7b52e3e71a1f2d656edbc65a08ea` (unchanged).
- Read-only RKT4 audit from PR #62 exact worker `e08412cd8ed32a1c179ad12ca53ac108185ccbda` plus physical motor-free checklist from PR #61 `36c80296671f45464e8ee20139dbb1de32e6af7d`: staged in integration commit `9f7b0b7304101e97de6f658262485e38b5363975`. The audit has source-worker reported 232/232 Python PASS; do not mistake this for combined CI or full BIN sample checksum.
- Firmware diagnostics from PR #63 exact worker `e80e90203bdab5bff361d9c8f1d66dfed67b6a53`: staged in integration commit `990de0db1d8e869a78327c440229059c94c48979`; [source exact-head CI success](https://github.com/aidenjantzi1-ctrl/GREEN-ION/actions/runs/43737455168). Includes guarded reserved-temp SD test and diagnostic status; no new endpoint or destructive flight-file test.
- Blue/navy four-tab guided test UI from PR #58 exact worker `3f13564a174d7be8011fd3e907e332045a31e510`: staged in integration commit `2ad658ad268645b349eff12a5ef379cca4349e0a`; [source exact-head CI success](https://github.com/aidenjantzi1-ctrl/GREEN-ION/actions/runs/43737633465). The source core's GPS coordinate-null and fix-age checks were inspected against the earlier integration-only implementation before staging; do not infer full behavioral regression PASS without combined tests.
- Combined integration CI for `2ad658ad268645b349eff12a5ef379cca4349e0a` must pass Python, Node, native C++, browser/asset checks, Feather ESP32 V2 and Uno compile, and Windows packaging. A previous successful source or intermediate integration run does **not** qualify the combined head. Check [integration Actions](https://github.com/aidenjantzi1-ctrl/GREEN-ION/actions?query=branch%3Aworker%2Fv18-integration) and record exact run ID/results before marking DEV-TEST READY.
- Still unverified: actual extracted Windows ZIP launch, Chrome/Edge permissions and local bridge connectivity, real SD temporary readback, physical battery/SD/IMU/barometer, Uno/LoRa/GPS RF, 5-minute recovery/3-minute hibernation/physical wake, So Long dimensions/CG/static ports and flight safety. These remain **NOT TESTED**.

**Next gate:** inspect exact-head CI and package ZIP contents, exercise the opt-in `Ground_Station/START_V18_PREVIEW.cmd` on a real Windows Chrome/Edge machine, then update beginner guide and this status with exact artifacts and real test evidence. No release/deploy/main merge/flash/force push/deletion/PR closure.
