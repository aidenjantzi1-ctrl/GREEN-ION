# GREEN ION V18 DEVELOPMENT TEST PREVIEW — beginner guide

> **Development testing only. NOT a release, flight approval, firmware update, or hardware safety certification.** Keep all motors and igniters disconnected for bench testing. Never arm a live rocket as part of a software test. Stable V17 remains the normal/default app.

## 1. Start on Windows (Chrome or Edge)

1. Obtain the current **worker/v18-integration** source checkout or a Windows package built from that exact branch. Do not use an unrelated V17 ZIP and assume it includes V18.
2. Keep the extracted folder intact (including `Ground_Station` and `web`). On Windows, open `Ground_Station` and double-click **START_V18_PREVIEW.cmd**. It delegates to `START_GREEN_ION.cmd --v18-offline` and starts the local bridge in V18 Wi-Fi-only mode.
3. If Windows asks for permission to run the local Python/PowerShell launcher, review the source and allow only if you trust this checkout. Follow any console error instead of closing the window. Do not flash either board.
4. Chrome/Edge should open a local `/v18-offline.html#flight` page with a **dark blue/navy background** and four top tabs: **Flight / Recovery / Data / Settings**. This is an opt-in preview; the normal launcher still opens V17.
5. The app and its local bridge do not require an account, cloud upload, or subscription. For an offline-only check, disconnect the computer from the internet *after* the local files are present; do not mistake a failed Feather connection for failed offline support.

If the launcher fails: run `Ground_Station/START_V18_PREVIEW.cmd` from a Windows Command Prompt to keep error text visible; check that the entire extracted folder is present and the required Python runtime is installed. If the browser does not open, read the local address printed in the console and open that address in Chrome/Edge. Do not substitute an internet-hosted preview when verifying offline behavior.

## 2. No hardware? Test the Data tab first

1. Open **Data**. Choose **Manual BIN import**, and select a *copy* of an existing GREEN ION RKT4 v2 `.BIN` flight file.
2. Confirm that the app reports an import result; inspect its analysis, warnings and any available export. Repeat import to check duplicate handling. Keep the original file and any companion `.TXT` untouched.
3. Try a separate **copy** of a deliberately truncated or invalid BIN and confirm that the app rejects or warns about it rather than claiming a saved, valid flight.
4. Close and reopen the local app without internet and verify whether the imported flight remains available. Record any failure as **FAIL**; unsupported or unperformed checks are **NOT TESTED**.
5. The independent PR #62 RKT4 CLI audit is **not yet an accepted release gate**. Do not interpret file-size/header/TXT checks as cryptographic verification of every sample.

## 3. Feather bench checks (motor-free)

1. Disconnect all motors/igniters. Place the Feather, microSD and sensors securely on a stationary, nonflammable work surface; use a safe power source and supervise the LiPo.
2. Connect to the Feather using the documented local connection method and check the **Flight** tab. Confirm that status and battery readings are fresh; an absent or unknown battery value is **NOT TESTED**, not PASS.
3. Verify SD presence, barometer/IMU status, and calibration only through firmware-supported diagnostics. A temporary-file write/read/delete test is permitted **only if the firmware explicitly supports a TEMP diagnostic**; never delete `.BIN`, `.TXT`, or recovery journals.
4. **Do not press PREPARE & ARM solely to test a screen.** It is a real firmware-controlled command. Only use an explicitly documented stationary, non-arming simulation if the firmware owner confirms its safety. Otherwise mark arming and two sequential test flights **NOT TESTED**.
5. For a completed *authorized* motor-free test, preserve both distinct original BINs and completion metadata, inspect saved-state evidence, and note Wi-Fi-off/on behavior. A simulated UI state is not hardware evidence.
6. If any SD fault, unsafe battery condition, unexpected arming, heating, or sensor failure occurs, stop, disconnect safely, preserve files, and report **FAIL**.

## 4. Uno / LoRa / GPS receiver checks (separate from SD)

1. In Chrome/Edge open **Recovery** and select **CONNECT RECEIVER**. Choose the **Arduino Uno USB serial port** in the browser's permission dialog. The app expects newline-delimited JSON at **115200 baud**. Do not let another serial program or the Python bridge simultaneously own the Uno COM port.
2. Verify disconnect/reconnect and invalid or stale packet warnings. A displayed last-known coordinate is not necessarily a current fix; check packet age and GPS fix age.
3. Only report radio reception, GPS location, RF range, antenna health or recovery timing as **PASS** after a real, safely configured hardware measurement. Do not transmit with damaged/disconnected antennas; leave these **NOT TESTED** if hardware is absent.
4. Use the separate motor-free physical checklist at [PHYSICAL_DEV_TEST_CHECKLIST.md](PHYSICAL_DEV_TEST_CHECKLIST.md) once reviewed/staged from PR #61. It covers battery endurance, antenna safety, So Long fit/stability and wake/recovery checks. Software CI cannot establish these.

## 5. Record and share evidence

For each check write **PASS / FAIL / NOT TESTED**, exact action, observed result, date, device, firmware version, receiver version, source commit SHA, and any error text. Keep original BIN/TXT/recovery files unchanged; make copies for analysis. Save screenshots of the four-tab UI and export diagnostics only where the current app supports them. Post reproducible software defects on [Integration PR #60](https://github.com/aidenjantzi1-ctrl/GREEN-ION/pull/60), with the SHA and steps; do not include private location coordinates.

**Current limitations:** A one-click preview launcher exists, but a full in-app **TEST / CHECK EVERYTHING** flow, validated diagnostic API, integrated RKT4 audit and complete physical bench evidence must be verified on their final exact heads before this preview can be called **DEV-TEST READY**. Missing capabilities must remain **NOT TESTED**, never silently marked PASS. No main merge, release, deployment, hardware flash or flight approval is authorized.
