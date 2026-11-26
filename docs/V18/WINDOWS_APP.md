# GREEN ION V18 Windows desktop application

## Install and run

1. Download the `GREEN_ION_V18_WINDOWS_APP` development artifact from the successful Windows Actions run and extract it.
2. Run `GREEN_ION_Setup.exe` for the normal installation. It is per-user and does not need administrator privileges.
3. Launch GREEN ION from the desktop shortcut or Start menu. The navy UI remains exactly **Flight | Recovery | Data | Settings**.
4. Edge is preferred, Chrome is second, and the system browser is the fallback so Recovery can retain Web Serial access to the Arduino Uno.
5. The portable `GREEN_ION.exe` can also run directly; no separate Python installation or terminal is required.

The installer targets Windows 10/11 x64 under `%LOCALAPPDATA%\Programs\GREEN ION`. Flight data stays in `%USERPROFILE%\GREEN_ION_Data`. Desktop settings, staged updates, and rollback state stay under `%LOCALAPPDATA%\GREEN_ION`. Normal uninstall does not delete those locations.

## Architecture and hardware compatibility

PyInstaller bundles the existing local bridge and V18 web assets. The local server prefers `127.0.0.1:8765` and falls back to another loopback port if occupied. A Windows named mutex prevents duplicate app instances. The packaged Python process never claims the Uno COM port; Recovery continues to use browser Web Serial at 115200 baud. Feather Wi-Fi continues through the existing loopback bridge.

Embedded WebView2 was not selected because equivalent Web Serial behavior cannot be assumed. Hardware connectivity takes priority over hiding the browser renderer.

## Updates and rollback

Update controls live inside **Settings**. Normal operation is offline. Internet is used only for an explicit update check/download or when automatic startup checks are enabled.

The updater accepts only non-draft, non-prerelease `v18.x.y` assets from official GitHub Releases for `aidenjantzi1-ctrl/GREEN-ION`. It never installs branch heads, pull requests, commits, or Actions artifacts. It selects the highest valid V18 version, downloads the complete installer first, verifies the published SHA-256, optionally binds `GREEN_ION_Windows_manifest.json`, and checks Authenticode when present. Invalid signatures are rejected. Unsigned releases are clearly reported and are allowed only when the manifest does not require signing; SHA-256 then proves integrity against the published asset but not Windows publisher identity.

Install and rollback fail closed while Feather state is unknown/stale or while recording, armed, preparing, firmware diagnostics, or a marked data operation is active. The update installer uses Inno Setup silent-with-progress mode so Windows displays installation progress. Before installation the current packaged executable is saved under `%LOCALAPPDATA%\GREEN_ION\rollback` with its SHA-256. A failed installer exit triggers restoration of the prior executable. Rollback re-verifies that backup before replacement. The updater never flashes Feather or Uno firmware.

Maintenance commands: `GREEN_ION.exe --version`, `--check-update`, `--update`, `--rollback`. Final install and rollback stay Settings-gated so current hardware safety is rechecked.

## CI verification boundary

The development workflow runs the newest V18 Python/Node gates, builds the offline web cache, builds and starts the packaged EXE, verifies the four-tab page and updater endpoint, checks duplicate launch behavior, builds Inno Setup, performs fresh install/launch/uninstall, verifies user-data/settings sentinels survive uninstall, creates SHA-256 files and a build manifest, and uploads development artifacts.

Updater tests cover available/no-update/offline responses, development-to-stable promotion, interrupted download cleanup, checksum mismatch, manifest mismatch, safe-state deferral, verified download staging, rollback hash validation, and the installer failure-recovery helper path.

The approved-release workflow is manual only: it requires the exact authorization phrase, an existing published non-prerelease V18 release/tag, reruns V18 gates, builds from that tag, and only then attaches assets. This branch does not publish a release.

Automated CI does **not** prove actual Edge/Chrome permission prompts, physical Uno Web Serial traffic, real Feather Wi-Fi, antivirus/SmartScreen reputation, Authenticode trust without a signing certificate, RF/GPS, SD hardware, battery endurance, or flight readiness. Those remain interactive/physical tests.
