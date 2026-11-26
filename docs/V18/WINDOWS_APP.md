# GREEN ION V18 Windows desktop application

Architecture: PyInstaller bundles the existing local bridge and V18 web assets into GREEN_ION.exe. The UI opens in Edge or Chrome app mode so the Uno Recovery tab keeps normal browser Web Serial support; embedded WebView2 was not selected because equivalent Web Serial behavior cannot be assumed.

GREEN_ION_Setup.exe is a per-user Inno Setup installer for Windows 10/11 x64. It installs under %LOCALAPPDATA%\Programs\GREEN ION, creates Start menu and optional desktop shortcuts, and requests no administrator privileges. Flight data remains in %USERPROFILE%\GREEN_ION_Data. Settings, updates, and rollback state remain under %LOCALAPPDATA%\GREEN_ION and are not deleted by normal uninstall.

The local server prefers 127.0.0.1:8765 and falls back to another loopback port if occupied. A Windows named mutex prevents duplicate app instances. Edge is preferred, Chrome second, system browser fallback third. The interface remains navy and has exactly Flight | Recovery | Data | Settings.

Updates live inside Settings. They accept only non-draft, non-prerelease v18.x.y assets from official GitHub Releases, never branch heads, PRs, commits, or Actions artifacts. The complete installer is downloaded before replacement, checked against its published SHA-256, optionally bound to GREEN_ION_Windows_manifest.json, and Authenticode-checked when present. Invalid signatures are rejected; unsigned releases are clearly reported and only allowed when the manifest does not require signing.

Install and rollback fail closed while Feather state is unknown/stale or while recording, armed, preparing, firmware diagnostics, or local data import/export are active. Before install the current packaged executable is copied to %LOCALAPPDATA%\GREEN_ION\rollback and SHA-256 recorded. A detached helper waits for the app to close, runs the verified installer, restores the previous executable if installation exits unsuccessfully, and restarts GREEN ION. Rollback re-verifies the saved executable before replacement. Firmware is never flashed by the updater.

Maintenance commands: GREEN_ION.exe --version, --check-update, --update, --rollback. The update command checks/downloads/verifies; final install and rollback are intentionally started from Settings so fresh idle hardware state can be verified.

The Windows Actions workflow runs the existing Python/Node suites, builds and smokes the executable, checks duplicate startup, builds Inno Setup, performs fresh install/uninstall smoke, creates checksum files and a version/build manifest, and uploads development artifacts only. The release workflow is manual and fail-closed; this branch does not publish a release.

Automated CI cannot prove actual Edge/Chrome permission prompts, physical Uno Web Serial, Feather Wi-Fi, antivirus reputation, signed-publisher trust without a certificate, RF/GPS, SD hardware, battery endurance, or flight readiness. Those remain manual or physical tests.
