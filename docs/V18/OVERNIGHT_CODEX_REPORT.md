# GREEN ION V18 overnight engineering report

Date: 2026-11-14 Pacific. This is software evidence, not hardware flight qualification.

## Baseline, ownership, and Git trail

- Starting `main` SHA: `17f517f275be7b52e3e71a1f2d656edbc65a08ea` (V17.0.1 / merged PR #55).
- Dedicated branch: `worker/v18-codex-overnight`, based directly on that SHA. First completed code commit: `77ffa6c`.
- Ending implementation SHA before the CI-status report update: `d9a5762333d595e526e1d6c66bf83d2407210157`. The report commit follows; use PR #62 head for the branch's final SHA.
- Exact changed files: `tools/v18_rkt4_audit.py`, `tests/test_v18_rkt4_audit.py`, `docs/V18/OVERNIGHT_CODEX_REPORT.md`.
- PR: [#62](https://github.com/aidenjantzi1-ctrl/GREEN-ION/pull/62), draft, targeting `worker/v18-integration`.
- Latest remote fetch found `worker/v18-firmware` at `14d2d6c` (save verification and Wi-Fi policy code), `worker/v18-offline-app` at `7fa01b5`, `worker/v18-integration` at `901caf5`, and `worker/v18-hardware` at the main SHA. These tips may continue moving; fetch before integration.

## Completed work and bugs fixed

Added a read-only offline RKT4 v2 audit command. It scans one BIN or a directory of BINs, emits JSON with original-byte SHA-256, counts complete records and partial trailing bytes, validates the immutable 32-byte header/16-byte records, flags invalid sample rates/physical fields and sensor validity codes, and reports timestamp gaps. It handles the 32-bit microsecond clock wrap. It checks TXT completion evidence and `.JRA`/`.JRB`/`.RCV` recovery evidence per flight without changing or deleting any original files. Multiple flights are reported separately.

The checker distinguishes `invalid`, `incomplete`, `unverified` and `structurally_complete`. It now verifies the V17 completion TXT marker, status, file identity, byte count and record count against the BIN, catching truncation even at a complete-record boundary. Matching metadata does **not** prove all raw bytes are correct: RKT4 v2 has no per-record checksum. The result includes `no_per_record_checksum` warning instead of inventing a clean integrity result. An initial regression caught an incorrectly classified invalid header; fixed before the first code commit.

Run locally with `python tools/v18_rkt4_audit.py <BIN-or-directory>`. Exit status 0 requires every supplied file to be structurally complete; exit status 1 means further review is needed. The command does not alter BIN, TXT, journal or recovery files.

## Tests and compilation

| Check | Result | Scope |
| --- | --- | --- |
| Python `unittest` focused suite (`test_v18_rkt4_audit`, `test_rkt4_recovery`, `test_v17_flight_readiness`) | PASS: 29 tests | Synthetic RKT4 edge cases and existing recovery/readiness contracts |
| `node tests/js/test_rkt4.mjs` | PASS | Existing browser RKT4 parsing/analysis smoke |
| `node tests/v17_flight_readiness.test.mjs` | PASS: 4 tests | Existing one-press readiness UI contracts |
| Full Python `unittest discover`, UTF-8 enabled with loopback access | PASS: 229 tests | Default restricted Windows run first hit cp1252 decoding and socket access errors; rerun with UTF-8 and local loopback succeeded. |
| `node --test tests` | PASS: 126 tests | All Node tests on this main-based branch. |
| Firmware / Uno compilation | NOT RUN | Arduino CLI, C++ compiler and target boards unavailable in this environment. CI must compile exact PR head. |
| V18 offline app tests on detached Secondary head `378b824` | PASS: 11/11 | Initial `7fa01b5` run failed 2 narrow source-string assertions. Findings sent to [PR #58 comment](https://github.com/aidenjantzi1-ctrl/GREEN-ION/pull/58#issuecomment-6890032927); Secondary fixed them and the later head passes. No worker files edited here. |
| Windows packaging | NOT RUN | No package build performed in this checkout. |
| Hardware, real BIN, SD, power, RF | NOT RUN | No physical device or original ROCstock BIN files in this checkout. |

CI checks at implementation SHA `d9a5762333d595e526e1d6c66bf83d2407210157` and subsequent report SHA `2fb5433`: Vercel combined status **success**; GitHub returned **no Actions workflow runs** for either SHA. Recheck the final PR head after this report commit. The existing main, PR #58 and a prior integration branch SHA also returned a successful Vercel status; that status does not establish firmware or hardware results.

## Remaining P0/P1 work and hardware blockers

- Primary-owned firmware branch now has initial save verification and Wi-Fi policy changes. SD buffering, checkpoint finalization/readback, autonomous sampling, PREPARE & ARM, phase detection and full Wi-Fi/power transitions remain to be implemented or independently verified. No firmware source was edited here to avoid conflicting ownership.
- Secondary-owned offline UI and Web Serial implementation needs compatibility review with the exact firmware and Uno serial contracts before merge. PR #58 documents its existing V17-compatible contract. Its prior test assertion failures were corrected at `378b824`.
- An independent checksum or expected byte count captured at recording time would be needed to verify content corruption beyond structural checks. Keep any such evidence in a sidecar/API; do not alter RKT4 v2 records or invent missing samples.
- Real BIN files C and D, interruption traces, SD hardware, properly vented barometer, IMU, 350 mAh battery measurements, button, Uno/LoRa antenna and serial hardware, and two sequential flights require physical validation. The reported 163 m D altitude is unverified; the unvented Green Eggs bay prevents treating its pressure altitude as a known truth.
- No software test here establishes LoRa range, antenna reliability, battery endurance, So Long fit/stability or flight readiness.

## Branch and PR cleanup recommendations (review only)

- Keep `main`, `worker/v18-firmware`, `worker/v18-offline-app` (#58), `worker/v18-integration` (#60) and `worker/v18-hardware` (#61) active. Keep this overnight branch until its PR is integrated.
- PR #53 is a broad open V17 draft, currently reported unmergeable and overlapping merged V17 functionality. Compare its unique commits/files with main before deciding whether to retain as archive/reference or close. Do not merge wholesale.
- PR #56 is a stacked, open V17 Quick Fly draft based on the old PR #55 branch, currently reported unmergeable. Its unique `next_sequence` and save-readback ideas may be valuable, but reconcile them with main and V18 firmware/API contracts before cherry-picking.
- PR #57 is documentation-only and open; Integration Manager should decide whether its prompts remain useful. Preserve it until reviewed.
- Remote inventory includes many historical branches; several are ancestors of `main`, but ancestry alone does not prove every branch is safe to delete. Map unique commits, PR links, CI and current users before proposing individual archive/deletion actions. No branches or PRs were deleted, closed, rebased or force-pushed.

## Exact next actions for Integration Manager

1. Review this branch's audit output semantics and PR checks, then decide whether the read-only checker should join the V18 integration test gate.
2. Review Primary Coder's branch and agree the firmware status, save/readback and Uno packet contracts with Secondary before merging #58. Recheck Secondary CI and browser behavior on the exact combined head.
3. Run firmware/Uno builds and all CI jobs on the exact combined head, and test two distinct flight files plus interrupted-write recovery on disposable SD hardware.
4. Locate and preserve original C and D BIN/TXT/sidecars, then run the checker on copies and compare its byte hashes to originals. Record pressure-port limitation explicitly.
5. Complete branch-by-branch unique-value audit; bring any cleanup proposal to the user for approval. Keep release and main merge gated on review and hardware evidence.

## Summary

**Completed:** independent, read-only RKT4 v2 integrity audit; nine new synthetic regression cases; full Python and main-based Node suites; current Secondary V18 offline app tests. **Partial:** V18 integration and CI on the combined head. **Untested:** hardware recorder/receiver, actual flights, firmware compilation, battery endurance and RF performance.
