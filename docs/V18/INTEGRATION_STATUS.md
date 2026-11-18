# GREEN ION V18 — Integration status

Updated 2026-11-14. Status: **planning only; no V18 release or hardware flight qualification**.

## Verified baseline / PR inventory
- main V17.0.1 commit: `17f517f275be7b52e3e71a1f2d656edbc65a08ea`; PR #55 merged.
- PR #53: open draft, unmergeable, head `cc5842f62c19ac71f30174e7c48b58e3a6f71745`, base main; contains 72 changed files including shared firmware/web. Preserve and review before reuse.
- PR #56: open draft, unmergeable, head `2a0161364407bbbc999d93f34034df472e57c69a`, base `v17/flight-readiness-upgrade` (old PR #55 branch); includes Quick Fly API, caching, filename and UI changes. Needs reconciliation with current main.
- PR #57: open, non-draft, reported mergeable, head `2702a0d229fbf0d57bf8edc5efe0c213c5c6d516`; documentation-only role prompts.
- `worker/v18-offline-app`: compared 18 commits ahead, zero behind main; new web/v18 modules and `docs/V18_FRONTEND_CONTRACT.md`; preserve work, check branch tip, tests and CI separately.
- Branch search returned 67 historic/active branches. No branch deletion, closure, archival tag or bulk modification performed. Unique SHA mappings, dependencies, review status, CI and other branch tips require a follow-up audit.
- `worker/v18-integration` created from V17.0.1 baseline for this planning log; `worker/v18-firmware` was not found in initial branch inventory. Re-check because collaborators may create it.

## Status board
| Workstream | State | Next action |
|---|---|---|
| Master V18 planning files | In progress | Review docs/V18/REQUIREMENTS.md, ROCSTOCK_LESSONS.md, DECISIONS.md and this board via draft PR |
| Primary firmware | Not integrated | Agree status/state/Wi-Fi/power/SD contracts and verify branch/PR |
| Secondary offline app | Work observed; not integrated | Review frontend contract, serial schema, UI/tests, avoid overlapping edits |
| So Long Build Helper | Evidence pending | Obtain manual, approved motors, actual dimensions, mass, CG, static ports, sled, antennas and recovery clearance |
| Historic branch preservation | Audit incomplete | Map unique commits and branch dependencies; archive only after assessment; seek approval before deletion/PR closure |
| Software test/CI evidence | Not run by Integration Manager | Record actual commands and CI URLs for firmware, Uno, Python, JS, parser, RF/serial, packaging, two-flight, interrupted writes |
| Hardware validation | Unverified | SD/reboot, real sensors, pressure venting, measured battery, physical antenna/LoRa/GPS, receiver, fit/stability and launch safety |
| Release | Blocked | No deployment, merge into main, or V18 publication before appropriate review/checks; release requires user approval |

## Contract blockers and principles
RKT4 v2 serialization and autonomous nominal 50 Hz SD recorder remain immutable. Optional GPS/LoRa absence is advisory; critical SD/barometer/IMU/unsafe battery block preparation. PREPARE & ARM should be firmware-owned, safe/idempotent, with verified save and distinct FLY AGAIN filename. UI must never infer success from a stale response. The V18 Windows Chrome/Edge four-tab app must function locally with manual BIN import, no cloud. Wi-Fi transitions, five-minute recovery interval, three-minute hibernation TX intervals, 350 mAh power policy, physical wake, LoRa packet and Uno Web Serial format require coordinated contract and physical tests.

## Approval boundaries
No force-push, automatic branch deletion, PR closure, historical source removal, V18 deployment or release publication. Preserve all historical versions in Past_Versions and maintain active V16_3 build paths pending verified migration.


## Subsequent live checks (same audit)
- **PR #58** is the Secondary Coder V18 offline app, open draft, reported mergeable, head `9134089367323fe445ff26e043a7a380defd5cbc`, 7 changed files. Its recorded Actions run [#430](https://github.com/aidenjantzi1-ctrl/GREEN-ION/actions/runs/43673094793) completed **success**; the commit's Vercel status is success. Neither proves its new V18 UI is mutually compatible with unreleased firmware or physical hardware. No PR reviews were returned for #58.
- **PR #60** is this planning PR, open draft, head originally `045c7bd15dcbe96f879528bdc9e877d227737588`, initially reported mergeable. Workflow run [#431](https://github.com/aidenjantzi1-ctrl/GREEN-ION/actions/runs/43682105895) was **in progress** at audit; do not assume success. No PR reviews returned.
- `docs/V18_FRONTEND_CONTRACT.md` on the Secondary branch describes actual V17.0.1 fields, same-origin Windows bridge, direct Web Serial at 115200 baud, newline JSON protocol 2 and RKT4 v2 32-byte header/16-byte records. The integration manager's proposed extra fields are **not** currently implemented or agreed; reconcile before worker merge.
- Initial branch inventory contained no `worker/v18-firmware`; this may change as Primary Coder creates it. PR #59 was not confirmed by connector (no reliable metadata returned), so no status is asserted.


## Integration-manager update — later audit and safe staging
- Stable `main` verified unchanged at `17f517f275be7b52e3e71a1f2d656edbc65a08ea`. Latest GitHub refs inventory: **69 branches**, with exact SHA mapping and conservative preservation/disposition in [BRANCH_AUDIT.md](BRANCH_AUDIT.md). There is still no `worker/v18-firmware` branch in that live snapshot; unpushed Codex changes remain unknown.
- PR #58 exact seven frontend files staged **only** on `worker/v18-integration` at `07119fd99a13edccfe730f13db2283481f328dc9`, leaving worker branch untouched. Targeted GPS null-value and stale-fix regressions patched on integration branch in `935b89ef181517749b7de5e2b9419fef9b591656` and `c2d7a9ed00aaa87ec7b1d95982cf05f124bcadbc`.
- Pre-release-candidate evidence report: [RC_READINESS.md](RC_READINESS.md). README has an explicit V18 planning navigation section; V17 stays the stable software identity.
- CI original frontend PR #58: [run #430](https://github.com/aidenjantzi1-ctrl/GREEN-ION/actions/runs/43673094793) SUCCESS. Earlier integration docs head: [run #432](https://github.com/aidenjantzi1-ctrl/GREEN-ION/actions/runs/43682144782) SUCCESS. Combined staged/fixed-source runs [staging](https://github.com/aidenjantzi1-ctrl/GREEN-ION/actions/runs/43682679441) / [GPS fixes](https://github.com/aidenjantzi1-ctrl/GREEN-ION/actions/runs/43682783246) / [audit](https://github.com/aidenjantzi1-ctrl/GREEN-ION/actions/runs/43682809771) were pending at the time of this note. Do not infer pass from queued/in-progress.
- Critical clarification from firmware source: current `/api/files` returns `recording_complete` when TXT exists and no journal/recovery sidecars; this flag alone does NOT independently verify every BIN sample/checkpoint. Full BIN integrity/readback and power-loss regressions remain release gates.
- V18 frontend is a standalone `web/v18-offline.html` entry point. Existing Windows launcher still identifies as V17 and opens existing default dashboard; V18 standalone package/default Windows launch has NOT been qualified.
- No V18 firmware branch, V18 power/Wi-Fi transition implementation, measured 350 mAh endurance, physical wake, LoRa RF repair, So Long structural fit, or hardware acceptance has been verified.
- No main merge, branch deletion/closure, tagged release, deployment, flashing or force-push performed.


## Firmware and Windows preview integration update
- Primary firmware PR #63 exact reviewed source head `70fc016080ee7a1d733456880a2f9e189cb6c2ac` was staged on integration at `1f3fa497e0cb828f2a14c2961d380c9f5f7155f0`. The Primary branch itself was **not** changed. This includes RKT4 structural save/readback checks, Wi-Fi-off-after-recorder-start, Wi-Fi-on-after-verified-save, and short-flight pending-flag guard; full physical and combined CI remain required. It is not a complete five-minute recovery/three-minute hibernation/wake implementation.
- Hardware plan PR #61 was staged on integration in `0b7fc8d043d01523ac2db1c6ddbe360b390209f1`. All manufacturer and proposed wiring dimensions remain subject to physical verification.
- Secondary frontend latest source/tests was reconciled at `22e5ec81c81b4b9744dc6d46114ec695ed62c8f6` / `6b5e32f20c761951edb1531ccc037544ac484dc3`; null-coordinate fix retained. Existing live PR #58 may move again: recheck before final review.
- Integration-only opt-in V18 Windows preview added to `Ground_Station/ground_station.py`, `Ground_Station/START_V18_PREVIEW.cmd`, README and `tests/test_v18_windows_preview.py`. Stable V17 default page unchanged. Preview is neither a V18 installer nor hardware authorization.
- Overnight PR #62 remains draft, not integrated pending no-input false-success and completion metadata review. Recent PR #63 also remains draft and user approval for `main` / release has not been granted.
- Combined exact-head CI must succeed after the last staged firmware/preview commit; earlier passing CI does not cover the new combination.


## 2026-11-18 — manual START resume, Windows dev-test preview
Latest coordinator [START] is `V18-MANUAL-RESUME-20261118` on PR #60. This is **manual only**; no new schedule was created. Main remains approved V17.0.1 SHA `17f517f275be7b52e3e71a1f2d656edbc65a08ea`. The Integration Manager owns only `worker/v18-integration`; other workers may still be active.

- Staged reviewed, CI-passing Secondary #58 UX2 snapshot `ec5545da7bca53e01cf220e07ce36a536fd1d156` as `218ec2da05e2a1887ba408fb2b5bca18807a424f`, preserving integration-only parser/bridge fixes. Newer Secondary head appeared concurrently and is **not** automatically staged.
- Prevented V18 preview entrypoint from redirecting to older installed V17: root package launcher → `Ground_Station/START_V18_PREVIEW.cmd` → standard launcher `-NoRedirect --v18-offline`. Normal V17 startup stays unchanged.
- Added Windows-runner extracted preview ZIP smoke script `tools/smoke_v18_dev_preview.py`; it SHA-checks, extracts, starts localhost service and downloads real page+JS+service-worker from the **extracted package**. Combined with existing ZIP contents and PowerShell checks, exact source/guide head `96291b0661fe6df25df52cdd4b7998a03911c071` passed [V18 preview workflow 44345589814](https://github.com/aidenjantzi1-ctrl/GREEN-ION/actions/runs/44345589814). This does **not** prove a human double-click, offline restart or hardware.
- Updated novice [testing guide](TESTING_GUIDE.md) with artifact download/extraction, manifest commit/sha256 verification, BIN-only offline testing, optional motor-free Feather/Uno tests and privacy-safe evidence sharing.
- Current status: **software-only Windows development preview CI PASS; V18 RC and physical acceptance remain BLOCKED**. Cross-worker newer Secondary UX and firmware changes require their own source review and exact-head tests before further staging. No main merge, release, tag, deploy, flash, force-push, branch deletion or PR closure.


## 2026-11-18 continued manual integration (Secondary [DONE] snapshot)
- Secondary #58 exact worker source `5f14c2656eba4f9802cb64969e2765b716161716`, [CI success 44344824745](https://github.com/aidenjantzi1-ctrl/GREEN-ION/actions/runs/44344824745), staged in `8b7637d707781117e47f45c939a9bc4bf097a21f` without changing the worker branch. Source's latest behavioral privacy/serial/unknown-state tests and canonical private report label handling are included.
- Integration-only `finite` GPS field presence guard plus separate Node null/stale regressions committed as `27e57ae97dae18f60d6cecdf565f94636a024bba`.
- [V18 extracted-package preview CI 44346524012](https://github.com/aidenjantzi1-ctrl/GREEN-ION/actions/runs/44346524012) passed, uploaded the opt-in ZIP artifact `GREEN_ION_V18_DEV_TEST_PREVIEW` (id 14446256821). Full [combined CI 44346541272](https://github.com/aidenjantzi1-ctrl/GREEN-ION/actions/runs/44346541272) was pending Feather/Uno compile, while app-tests and Windows package passed. No hardware was executed.
- **Software-only dev-test package is testable in CI; manual Windows UX and hardware qualification remain blocked/not tested.** Never infer actual radio/GPS/battery/wake/So Long fitness or flight approval. Stable V17.0.1 main unchanged.
