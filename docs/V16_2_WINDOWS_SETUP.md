# GREEN ION V16.2 — Windows setup and update

## First start

1. Download the official `GREEN_ION_V16_WINDOWS.zip` and its `.sha256` file from the V16.2 GitHub release.
2. Verify the SHA-256 if you downloaded manually. The built-in updater also verifies it before installation.
3. Extract the ZIP.
4. Double-click `START_GREEN_ION.cmd`.
5. Leave the terminal window open. The blue dashboard opens at `http://127.0.0.1:8765`.
6. Plug the Arduino Uno receiver in before or after startup. GREEN ION continuously looks for one unambiguous Uno and reconnects after unplug/replug or a COM-port change.
7. Join the Feather `GREEN-ION-V16-2` Wi-Fi when live Feather status, guided tests or SD transfer is needed. The Feather access point does not provide internet.

Python 3.10+ is required. The launcher tries to install the pinned `pyserial==3.5` dependency. If that fails, the app still starts in Wi-Fi-only mode.

## Data safety

User data remains under `%USERPROFILE%\GREEN_ION_Data`, outside replaceable app versions. Updates and rollbacks do not delete this folder.

## Update

Run `UPDATE_GREEN_ION.cmd`. The updater:

- checks official `v16.*` GitHub releases;
- downloads the Windows ZIP and SHA-256 file;
- verifies the package;
- installs side-by-side under `%LOCALAPPDATA%\GREEN_ION\App\<tag>-<sha12>`;
- keeps the previous complete app for rollback;
- does not flash the Feather or Uno.

Close the currently running ground-station window before starting the newly selected app version.

## Rollback

Run `ROLLBACK_GREEN_ION.cmd`. It switches the app pointer back to the previous retained package. It does not change Feather firmware and does not delete `GREEN_ION_Data`.

## Google Maps

The embedded Recovery map is optional. Set a restricted browser key before launching:

```bat
set GOOGLE_MAPS_BROWSER_KEY=YOUR_RESTRICTED_BROWSER_KEY
START_GREEN_ION.cmd
```

Without it, Recovery keeps the offline GPS plot, coordinates, distance/bearing and external Google Maps link.
