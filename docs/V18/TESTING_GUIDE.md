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
5. The PR #62 read-only RKT4 CLI audit is now staged in the V18 integration branch, but it is **not a flight-safety or release gate**. To audit copied files, run `python tools/v18_rkt4_audit.py "C:\\path\\to\\copied-flights"` from the extracted repository root. An empty folder returns a nonzero result; structural PASS does not prove every record's content. Do not interpret file-size/header/TXT checks as cryptographic verification of every sample.

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
4. For beginner assembly order and a blank measurement worksheet, read [SO_LONG_ELECTRONICS_INSTALL_STEPS.md](SO_LONG_ELECTRONICS_INSTALL_STEPS.md); do not treat proposed dimensions or component placements as measured. Use the separate motor-free physical checklist at [PHYSICAL_DEV_TEST_CHECKLIST.md](PHYSICAL_DEV_TEST_CHECKLIST.md), now staged from PR #61. It covers battery endurance, antenna safety, So Long fit/stability and wake/recovery checks. Software CI cannot establish these.

## 5. Record and share evidence

For each check write **PASS / FAIL / NOT TESTED**, exact action, observed result, date, device, firmware version, receiver version, source commit SHA, and any error text. Keep original BIN/TXT/recovery files unchanged; make copies for analysis. Save screenshots of the four-tab UI and export diagnostics only where the current app supports them. Post reproducible software defects on [Integration PR #60](https://github.com/aidenjantzi1-ctrl/GREEN-ION/pull/60), with the SHA and steps; do not include private location coordinates.

**Current limitations:** The guided **TEST / CHECK EVERYTHING** panel, safe firmware diagnostics, read-only RKT4 audit and physical checklist are now staged on `worker/v18-integration`. Use **Settings → TEST / CHECK EVERYTHING** for read-only evidence; run hardware diagnostics only by an explicit separate button. The combined integration head must still pass its own CI, Windows ZIP launch and actual device checks before this preview can be called **DEV-TEST READY**. Missing capabilities must remain **NOT TESTED**, never silently marked PASS. No main merge, release, deployment, hardware flash or flight approval is authorized.


## 6. Easier one-click Windows ZIP (integration development only; 2026-11-18)

The GitHub Actions **V18 development preview ZIP (no release)** workflow creates an artifact named **`GREEN_ION_V18_DEV_TEST_PREVIEW`**, containing a ZIP and companion SHA-256 text file. Open the latest **successful run for the exact integration commit you want to test** from [the integration Actions page](https://github.com/aidenjantzi1-ctrl/GREEN-ION/actions/workflows/v18-dev-preview.yml). In its Artifacts section download `GREEN_ION_V18_DEV_TEST_PREVIEW`, extract the outer artifact download and then fully extract `GREEN_ION_V18_DEV_TEST_PREVIEW.zip`. **Do not execute from inside either compressed ZIP.**

1. Confirm `GREEN_ION_V18_DEV_TEST_PREVIEW/PREVIEW_MANIFEST.json` exists; its `commit` is the exact source SHA under test, and `hardwareTestsVerified` must be `false`. The companion `.sha256` can be verified using PowerShell `Get-FileHash -Algorithm SHA256 .\\GREEN_ION_V18_DEV_TEST_PREVIEW.zip` from the artifact folder; its output must match the hex string in the `.sha256` file.
2. Inside the fully extracted `GREEN_ION_V18_DEV_TEST_PREVIEW` folder, **double-click the root `START_V18_PREVIEW.cmd`**. The script passes `-NoRedirect --v18-offline` to avoid silently opening a separately installed V17 copy. V17 remains the normal/default launcher.
3. If required, install Python 3.10+; use Windows Chrome or Edge. The preview should open `http://127.0.0.1:8765/v18-offline.html#flight`. Keep the terminal running. If a prior ground station occupies port 8765, close that instance rather than opening two bridge services. Avoid Arduino Serial Monitor and other COM readers when using browser Web Serial.
4. **Without hardware**, use **Data → Manual BIN import** on a copy, then **Settings → TEST / CHECK EVERYTHING**. Missing Feather/Uno and hardware-only checks should display **NOT TESTED**, not PASS. **With hardware**, use the motor-free steps above; do not press PREPARE & ARM just to prove a button works. The separate firmware diagnostic requires an explicit safe user action.
5. To share a reproducible bug, record the manifest commit SHA, Windows/browser versions, PASS/FAIL/NOT TESTED rows, terminal errors, and screenshot. Use the **COPY PRIVATE TEST RESULTS** feature only if present in the exact downloaded build, and inspect the report before sharing. Do not publicly post raw GPS coordinates, COM identifiers or complete private paths.

**CI evidence scope:** An automated ZIP SHA/manifest check and even a Windows runner's extracted-ZIP localhost smoke prove software packaging and local HTTP serving only. They **do not** exercise a human double-click, Windows antivirus/permissions, real offline cache reload, actual Feather/Uno serial permissions, physical SD, RF or launch safety. If the extracted-package smoke or exact-head CI has not passed, mark the new preview **DEV-TEST BLOCKED**, even if prior commits were green. Stable `main` stays V17.0.1.

## P0 save-state caution — next-build regression gate (2026-11-21)

**The present V18 development preview is NOT approved for Flight-tab save/fly-again acceptance.** A verified software regression found that a completed file-list entry plus *any* HTTP-success completion metadata response could mark SAVED even if the TXT describes a different or incomplete flight. Do not treat an on-screen **SAVED**, **Verification**, or **FLY AGAIN** indicator alone as proof of successful SD finalization, and do not start another flight based on that indicator. Preserve all flight originals. The validation fix belongs to Secondary PR #58 and must pass Heavy PR #62's independent metadata-binding probe before acceptance.

**Still useful without flight hardware:** use the **Data** tab to import a *copy* of an existing BIN, inspect pressure-derived plots and warnings, export a local report, and keep the original BIN/TXT intact. Follow the motor-free checklists for any subsequent real Feather/Uno work only after the coordinator publishes a SHA with the explicit save-state regression gate green. Real USB/antenna/battery/wake and So Long stability remain NOT TESTED by CI.

This caution supersedes earlier software-preview readiness wording for the save-state feature and is not a V18 release.
