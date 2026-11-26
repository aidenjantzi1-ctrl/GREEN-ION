# V18 GitHub branch preservation audit

Snapshot: 2026-11-14/15 UTC; `main` at `17f517f275be7b52e3e71a1f2d656edbc65a08ea`. The initial head inventory contains **69** branches. This table maps **exact live head SHAs as audited**. Heads may move; recheck before any archival tagging/cleanup.

**Interpretation:** `ancestor of main` means GitHub compare reported ahead=0 at audit. `diverged/unmerged` means ahead>0 relative to main (even when equivalent code was already squash/cherry-pick merged). It does **not** automatically imply missing functionality. No branch has been deleted or archived in this audit.

| Branch | Observed SHA | Preservation disposition |
|---|---|---|
| `chatgpt/v12-six-upgrades-hardening` | `dfd6984dc262507b162478b7a8b85d49f01d4726` | Potential cleanup *after approval* — ancestor of main; still check PR dependencies/tags |
| `codex/green-ion-v12` | `a35927e4b83067f4683b4b368e6cba9bc2efce98` | KEEP pending unique commit / PR dependency review — not proven disposable |
| `codex/v12-guided-home-verification` | `10de9e21906379040d0e6df7478af0e526854821` | Potential cleanup *after approval* — ancestor of main; still check PR dependencies/tags |
| `codex/v12-ota-dashboard-groundstation` | `a10ea4108c967ecb2636e356395a0cfdb7d61993` | KEEP pending unique commit / PR dependency review — not proven disposable |
| `codex/v12-reliability-preflight-profiles` | `619b044d1b823089f44d450b0133dfc0d4d96714` | KEEP pending unique commit / PR dependency review — not proven disposable |
| `codex2/v16-3-visual-replay-recovery-20261113` | `20ff18452b0549d0d75f4acf5ad2e83869fe7bac` | Potential cleanup *after approval* — ancestor of main; still check PR dependencies/tags |
| `docs/chatgpt-role-prompts-v17` | `2702a0d229fbf0d57bf8edc5efe0c213c5c6d516` | KEEP pending unique commit / PR dependency review — not proven disposable |
| `feature/v12-ota-dashboard-console-20261109` | `538508ab26a649f2d54178ebd2499aae53825063` | KEEP pending unique commit / PR dependency review — not proven disposable |
| `fix/windows-feather-http-connection` | `914699df7e3d2e88d67ad432b43d4df5b3ae5a15` | KEEP pending unique commit / PR dependency review — not proven disposable |
| `improve/flight-data-reliability-and-ux-20261024` | `bb9194ad4a04bf1ffe7c8aa11b10ea718bf312af` | KEEP pending unique commit / PR dependency review — not proven disposable |
| `improve/sd-retest-and-preflight-guidance-20261024` | `7f5401e1820e0c63560b4f1f70305516c380997f` | KEEP pending unique commit / PR dependency review — not proven disposable |
| `improve/v8-8-reliability-and-clear-preflight-20261024` | `353b9207a9b8d4d09ee75e160c7369c483d36996` | KEEP pending unique commit / PR dependency review — not proven disposable |
| `improve/v88-report-gaps-and-fourth-backlog-20261024` | `a69aa38f7a57049a879f8a95f999114a02b87802` | Potential cleanup *after approval* — ancestor of main; still check PR dependencies/tags |
| `improve/v9-bin-only-review-safe-delete-thirteenth-60-20261025` | `fed9b14b49ac55125901ddbdfe9df07a75cf4912` | KEEP pending unique commit / PR dependency review — not proven disposable |
| `improve/v9-clock-sd-phase-safety-and-seventh-backlog-20261024` | `c59d0e1cccdabb3951e33986839592fcae63fc4a` | KEEP pending unique commit / PR dependency review — not proven disposable |
| `improve/v9-comparison-visibility-and-honest-values-20261025` | `3a4dd55bf2433d2f2d2019f410560e9de25dcc56` | KEEP pending unique commit / PR dependency review — not proven disposable |
| `improve/v9-file-integrity-labels-and-twelfth-backlog-20261025` | `ae78a85857569ca6118314ea5a8cd3a79620a763` | KEEP pending unique commit / PR dependency review — not proven disposable |
| `improve/v9-ground-test-file-safety-and-eighth-backlog-20261024` | `f40ea3af071cdf192475021dbb3640e53bf503e3` | KEEP pending unique commit / PR dependency review — not proven disposable |
| `improve/v9-note-safety-and-fourteenth-backlog-20261025` | `40501cab2dc9b28b6dec2339293c5f90b3ea8a68` | KEEP pending unique commit / PR dependency review — not proven disposable |
| `improve/v9-report-gaps-and-fourth-backlog-20261024` | `a8baa4fa03b1f8ddbac1329a9ac883fc6d37435a` | KEEP pending unique commit / PR dependency review — not proven disposable |
| `improve/v9-report-provenance-and-storage-focus-seventeenth-60-20261025` | `57b52c5f4712fcc566861baf677cdeba275c6f14` | KEEP pending unique commit / PR dependency review — not proven disposable |
| `improve/v9-sd-reconnect-and-report-consistency-20261025` | `e853f2c89baf5444fe70e34f99883e22435728df` | KEEP pending unique commit / PR dependency review — not proven disposable |
| `improve/v9-state-guards-and-fifth-backlog-20261024` | `64ff7ea54a7714f80c3d78d70adde4764215b141` | KEEP pending unique commit / PR dependency review — not proven disposable |
| `improve/v9-storage-and-async-report-safety-20261025` | `749453f4b85d091651136b137ec3b6fa0dd273a3` | KEEP pending unique commit / PR dependency review — not proven disposable |
| `improve/v9-storage-refresh-and-note-validation-fifteenth-60-20261025` | `21310acc7bedd9ea36db7808f98b1dcd16ba54d5` | KEEP pending unique commit / PR dependency review — not proven disposable |
| `improve/v9-storage-validation-bounded-notes-sixteenth-60-20261025` | `367d2d49ac7cad7730557fb90fd1594378d7cf11` | KEEP pending unique commit / PR dependency review — not proven disposable |
| `improve/v9-strict-settings-and-gap-aware-analysis-20261024` | `2c9bb28cb7d24359c2668b756e8ec266167c7096` | KEEP pending unique commit / PR dependency review — not proven disposable |
| `integration/v12` | `7ade0bef2f99ea77f37c360460f6e69841169771` | Potential cleanup *after approval* — ancestor of main; still check PR dependencies/tags |
| `main` | `17f517f275be7b52e3e71a1f2d656edbc65a08ea` | KEEP — stable baseline |
| `maintenance/archived-workflow-scope` | `796a30da6911756d831fa276e3feb2c5387ae689` | KEEP pending unique commit / PR dependency review — not proven disposable |
| `release/v17` | `cc5842f62c19ac71f30174e7c48b58e3a6f71745` | KEEP pending unique commit / PR dependency review — not proven disposable |
| `release/v17-final-archive` | `f3d809f116bde922fe0365d1ee9613f620a38d44` | Potential cleanup *after approval* — ancestor of main; still check PR dependencies/tags |
| `v12/six-upgrades-ci-verification` | `dccf6959b8fea37cdcbb1994c24f89e747aced65` | Potential cleanup *after approval* — ancestor of main; still check PR dependencies/tags |
| `v13/unified-dashboard-downloads` | `1307c5659f7835ad6cd2a324f562c5f4b647991d` | KEEP pending unique commit / PR dependency review — not proven disposable |
| `v14/dashboard-layout-reliability` | `78b6dea047e6a9148204d943d2bee958633bb167` | KEEP pending unique commit / PR dependency review — not proven disposable |
| `v14/direct-feather-ota-download` | `8e76fef1b19563c9c148bded104dfcc1023ec350` | KEEP pending unique commit / PR dependency review — not proven disposable |
| `v14/offline-analysis-and-polish` | `4c07d1e4271d6d28c2644797ad272f3936dbfe0b` | KEEP pending unique commit / PR dependency review — not proven disposable |
| `v14/phone-first-cloud-dashboard` | `21f29921d051f586e2894d87e055017df1627243` | KEEP pending unique commit / PR dependency review — not proven disposable |
| `v14/sensor-cloud-responsive-upgrade` | `05fb516b421909b4dd09b88a39611a281339945c` | KEEP pending unique commit / PR dependency review — not proven disposable |
| `v15/hosted-feather-local-network` | `837effe8abdfdcea88b99ee2eca53b22bbbb0b3e` | KEEP pending unique commit / PR dependency review — not proven disposable |
| `v15/local-first-ground-operations` | `f22723e5dd9fcb2725d4e1885c1afd4269032b5b` | KEEP pending unique commit / PR dependency review — not proven disposable |
| `v16.1/reliability-simplicity` | `fd2491e0af027514a5875f7128af7813c8b9f560` | KEEP pending unique commit / PR dependency review — not proven disposable |
| `v16.1/sd-delete-layout-hardening` | `7f7e8375be7e9a7a5a1e34895c4d05130c3b3ee5` | KEEP pending unique commit / PR dependency review — not proven disposable |
| `v16.2/full-upgrade` | `096a3770322b297253bade20f37b4e3faa3d18db` | KEEP pending unique commit / PR dependency review — not proven disposable |
| `v16.3/final-readiness-doc-sync` | `6aa6dfd6eec422b53ba923b44b2e0cca4e1fb935` | KEEP pending unique commit / PR dependency review — not proven disposable |
| `v16.3/home-wifi-ap-sta` | `b002c360826faef44de6df06ab73436931e5736c` | Potential cleanup *after approval* — ancestor of main; still check PR dependencies/tags |
| `v16.3/shared-spi-reliability` | `a7dbbc2eb4fc5057176d0a692d5e8a3695acb2c3` | Potential cleanup *after approval* — ancestor of main; still check PR dependencies/tags |
| `v16/final-docs-cleanup` | `664e3ae58ba378cbb2e5ed17cdc749012016a816` | KEEP pending unique commit / PR dependency review — not proven disposable |
| `v16/phase-1-windows-reliability` | `b51cd5fb11ed962603b7e986dcfb97b25359c1a3` | KEEP pending unique commit / PR dependency review — not proven disposable |
| `v16/phase-2-unified-telemetry` | `d93789265b937bf851531ffaa889a64a9f00570a` | KEEP pending unique commit / PR dependency review — not proven disposable |
| `v16/phase-3-mission-workflow` | `d46b2fb7f32ef190cc1bc051de63e51b22bf7353` | KEEP pending unique commit / PR dependency review — not proven disposable |
| `v16/phase-4-five-tab-interface` | `79b42f756c492170111ff701102c20986dfbffe8` | KEEP pending unique commit / PR dependency review — not proven disposable |
| `v16/phase-5-data-cloud` | `03f36dcdebc69deb6f6a464648a6bdbfdca595ec` | KEEP pending unique commit / PR dependency review — not proven disposable |
| `v16/phase-5-data-cloud-chatgpt` | `53f9f478ac358ea42c81546ddd8738df832383b8` | KEEP pending unique commit / PR dependency review — not proven disposable |
| `v16/phase-5-data-cloud-final` | `bd8a78a42fd32b24df555a2ebbf7665cd459028d` | KEEP pending unique commit / PR dependency review — not proven disposable |
| `v16/phase-5-hardening` | `3827ad270ac41c91e5af177687f462572be54b5f` | KEEP pending unique commit / PR dependency review — not proven disposable |
| `v16/phase-5-hardening-current` | `7f1300c5fa763441261621577c7a1a2c06d4ac56` | KEEP pending unique commit / PR dependency review — not proven disposable |
| `v16/phase-5-hardening-final` | `90a2236f8e95d07dcda3a99d47d193c2cf401b3d` | KEEP pending unique commit / PR dependency review — not proven disposable |
| `v16/phase-6-launcher-hotfix` | `193b0cecfd0599a7efae8a0f3aa09047b8f40c5d` | KEEP pending unique commit / PR dependency review — not proven disposable |
| `v16/phase-6-windows-packaging-final` | `761a11a6f22f42910a098602f77b51e80d245cfd` | KEEP pending unique commit / PR dependency review — not proven disposable |
| `v16/phase-6-windows-packaging-updates` | `836e064f6d4a61af2cdd299404bbfcc24dea13b3` | KEEP pending unique commit / PR dependency review — not proven disposable |
| `v16/phase-7-advanced-visualization` | `4376ea831d61adeb6ffdc2f57ada58795d5c3d07` | KEEP pending unique commit / PR dependency review — not proven disposable |
| `v16/phase-7-advanced-visualization-final` | `33a223b0490b7987ca559f5b33a9a127f5376b20` | KEEP pending unique commit / PR dependency review — not proven disposable |
| `v16/phase-8-validation-evidence` | `ba1d633b301ec7ad55d9485fff4dfd4bf053edc3` | KEEP pending unique commit / PR dependency review — not proven disposable |
| `v17/flight-readiness-upgrade` | `090c178704745cfdd62323e14b9f19b2668b459f` | KEEP pending unique commit / PR dependency review — not proven disposable |
| `v17/quick-fly-dashboard` | `2a0161364407bbbc999d93f34034df472e57c69a` | KEEP pending unique commit / PR dependency review — not proven disposable |
| `worker/v18-hardware` | `17f517f275be7b52e3e71a1f2d656edbc65a08ea` | KEEP — V18 worker |
| `worker/v18-integration` | `c2d7a9ed00aaa87ec7b1d95982cf05f124bcadbc` | KEEP — V18 worker |
| `worker/v18-offline-app` | `9134089367323fe445ff26e043a7a380defd5cbc` | KEEP — V18 worker |

## Proposed branch cleanup groups (NOT authorized)
- **Always retain:** `main`, `worker/v18-integration`, `worker/v18-offline-app`, and `worker/v18-firmware` when created. Any still-running Codex/heavy-coder branch must remain.
- **Eligible for human-reviewed cleanup only after explicit approval and dependency review:** `chatgpt/v12-six-upgrades-hardening`, `codex/v12-guided-home-verification`, `codex2/v16-3-visual-replay-recovery-20261113`, `improve/v88-report-gaps-and-fourth-backlog-20261024`, `integration/v12`, `release/v17-final-archive`, `v12/six-upgrades-ci-verification`, `v16.3/home-wifi-ap-sta`, `v16.3/shared-spi-reliability`. These were behind main with no ahead commits at this snapshot; verify no active stacked PR still names them.
- **Preserve/assess divergences before cleanup:** all other rows in the table. Snapshot each important head into an `archive/pre-v18/<sanitized-name>` ref or tag **after** identifying meaningful unique changes and PR relationships, record original SHA/ref and archive ref. No archive tag/ref has been created yet. Do not assume a mere merged PR means exact branch head is already on main.
- **Open PRs requiring separate user approval before closure:** #1–#17 historical V8/V9 draft stack, #19 historical V12, #53 V17 integration (draft/conflicted), #56 V17 Quick Fly (draft/stacked), #57 prompts, #58 V18 frontend, #60 V18 integration. Check all open PRs live before any closure; keep #58/#60 while integration is in progress. Historical branch deletion may break open PRs.
- **Special**: `codex/green-ion-v12`, `codex/v12-ota-dashboard-groundstation`, `codex/v12-reliability-preflight-profiles` and related `improve/v9-*` heads have unmerged ancestry per comparison, so do not put them in a safe-delete bucket.
- No bulk deletion, force-push, PR closure or history rewrite without explicit user approval, and no action affecting a still-active heavy coder.

## Follow-up verification
Before proposing any precise deletion execution, confirm per-branch unique commit list and changed files; map open and stacked PR bases/heads, historical releases/tags and CI references; confirm archived refs exist and exactly match captured SHA; recheck latest head and approval; then delete only explicitly approved names.


## Live addendum — concurrent V18 branches
The original snapshot counted 69 branches; a subsequent live ref inventory counted **71** because two more worker branches were created while the audit was in progress. The original exact SHA table remains a historical snapshot; do not use its tips for deletion without refreshing.

| Newly observed / moved branch | Most recently observed SHA | Disposition |
|---|---|---|
| `worker/v18-firmware` | `486418d6619fcd8f7cb5b357c3c18158c23866f1` | KEEP; Primary draft PR #63; active and unmerged |
| `worker/v18-hardware` | `36c0d9d6695c3418794a7b3db7e87d25c54b613f` | KEEP pending PR #61 documentation integration/approval |
| `worker/v18-codex-overnight` | `d9a5762333d595e526e1d6c66bf83d2407210157` | KEEP; heavy coder PR #62 is draft and may be moving |
| `worker/v18-offline-app` | `378b824bfc33c40532a6f23d5c64c9f5aabbac80` | KEEP; recent Secondary commits, staged in integration separately |
| `worker/v18-integration` | `6b5e32f20c761951edb1531ccc037544ac484dc3` | KEEP; this branch is moving as audit/integration progresses |

No archive or deletion actions have been performed. The cleanup plan must be regenerated against live refs after worker activity stops.
