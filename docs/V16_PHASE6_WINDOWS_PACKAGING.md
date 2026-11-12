# GREEN ION V16 Phase 6 — Windows packaging and updates

## Goal

Make the Windows ground station easy to start, easy to replace with a newer package, and difficult to accidentally lose flight data.

The application folder is disposable. User data is not.

## One-click startup

Use `START_GREEN_ION.cmd` from the package root.

It:

1. redirects an old launcher to the currently installed side-by-side version when one exists;
2. finds Python 3.10+ using `py -3` or `python`;
3. keeps data under `%USERPROFILE%\GREEN_ION_Data`;
4. checks for the pinned `pyserial==3.5` USB dependency;
5. attempts to install pyserial if it is missing;
6. falls back to Feather Wi-Fi-only mode if pyserial still cannot be used;
7. starts the local server and lets it auto-discover one unambiguous Arduino Uno.

There is no normal COM-port prompt.

`OPEN_GROUND_STATION.cmd` remains as a compatibility alias to the same launcher.

## Package artifact

CI builds `GREEN_ION_V16_WINDOWS.zip` after the web application tests pass.

The ZIP contains:

- the built offline web application;
- Ground_Station Python source;
- launch/update scripts;
- pinned Windows requirement;
- V16 documentation;
- package identity/manifest.

CI also produces:

- `GREEN_ION_V16_WINDOWS.sha256`;
- `GREEN_ION_V16_WINDOWS_MANIFEST.json`.

The CI job recomputes the ZIP SHA-256 and inspects required files before publishing the Actions artifact.

## Update behavior

`UPDATE_GREEN_ION.cmd` checks official GitHub releases for a V16 Windows package and matching SHA-256 asset.

When a package is available, the PowerShell updater:

1. downloads the ZIP and checksum;
2. validates the checksum format;
3. recomputes SHA-256;
4. refuses a mismatch;
5. extracts to a temporary directory;
6. verifies required GREEN ION files;
7. installs side-by-side under `%LOCALAPPDATA%\GREEN_ION\App\<tag>-<sha12>`;
8. updates a small `current.txt` pointer only after validation.

It does **not** delete or migrate `GREEN_ION_Data`.

An existing content-addressed package directory is never deleted or replaced. A currently running server is not forcibly killed. The verified package becomes active the next time the launcher is started.

## Runtime diagnostics

The local server exposes `/api/app/health` only through its loopback-origin protections. The System tab shows:

- Windows package version/channel;
- Python version;
- current receiver mode and detected Uno port;
- data directory;
- confirmation that user data lives outside the app.

## What this updater does not do

The Windows package updater does not flash the Feather or Uno. Firmware OTA remains a separate bench-only flow with its own safety checks and confirmation.

## Validation boundary

CI verifies package contents, hashes, scripts and source behavior. It does not prove that a particular Windows machine has Python installed, that corporate antivirus permits PowerShell, that USB drivers work, or that a physical Uno/Feather reconnects correctly.
