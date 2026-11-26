# GREEN ION V18 Windows application integration handoff

Source: `worker/v18-windows-app`. Target: `worker/v18-integration`.

This branch already incorporates integration commit `88e6d646bc517f502211d9637d47e196de9d6d34` and its latest V18 contract/smoke gates. Merge only after the final **V18 Windows desktop application (no release)** run is green.

Windows-specific source:
- `Ground_Station/desktop_app.py`: packaged launcher, dynamic localhost port, duplicate guard, Edge/Chrome app mode, updater endpoints, safety gate.
- `Ground_Station/update_manager.py`: GitHub Releases-only check/download/verification/update/rollback.
- `Ground_Station/desktop_version.json`: desktop build metadata.
- `packaging/GREEN_ION.iss`: per-user installer.
- `tools/make_windows_icon.py`, `tools/package_v18_windows.py`: icon, hashes, manifest.
- `.github/workflows/v18-windows-app.yml`: development artifact build.
- `.github/workflows/v18-windows-release.yml`: explicitly authorized existing-release asset attachment.
- `tests/test_v18_windows_app.py`, `docs/V18/WINDOWS_APP*.md`.

Browser changes are limited to the Settings update card, desktop heartbeat/critical-operation markers, cache version bump, and matching regression expectation. Exactly four top tabs remain **Flight | Recovery | Data | Settings**.

Primary Coder: do not move desktop update behavior into firmware; never let the updater flash Feather or Uno. Preserve firmware ownership of PREPARE & ARM, recording, save verification, phase state, and SD-first behavior. Keep the install gate fail-closed if status fields change. Do not replace browser Web Serial with Python COM ownership; Recovery intentionally owns the Uno through Web Serial. Preserve the existing Feather Wi-Fi bridge contract.

Before an approved release, perform the still-unverified physical tests on a sacrificial Windows machine: Edge/Chrome permission/reconnect with the actual Uno, Feather Wi-Fi connect/disconnect/recovery, real flight BIN import, update deferral while actual hardware is active, and real rollback after an intentionally failed/superseded test install. Do not publish from this worker branch.
