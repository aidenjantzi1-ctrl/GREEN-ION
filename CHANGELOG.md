# V17.0 — official software release and historical archive — November 13, 2026

- Promoted Feather, Uno receiver, Windows and offline dashboard runtime identities to **17.0.0**. Kept their original V16.3 Arduino sketch source directory names for build/update compatibility.
- Integrated physically authorized AP+STA home Wi-Fi, bounded live sensor graphs, WebGL/3D replay, GPS/LoRa recovery and multi-flight comparison features.
- Hardened V17 OTA release manifest checking against mismatched firmware generations; preserved SHA-256 validation and deliberate bench-only firmware updates.
- Archived historical V12/V13/V14/V16.2 firmware and Uno source trees under `Past_Versions/`, plus existing V11 and V15 historical design notes; updated legacy tests, workflows and links.
- Added V17-tagged firmware, receiver, source, web and Windows distribution artifacts, with verified release manifest and one-time CI-gated publication.
- Preserved RKT4 v2, GIS1, 50 Hz autonomous recording, 3 s prebuffer, separate SD/LoRa chip selects, 70-byte LoRa v2 and receive-only Uno.
- **Hardware launch checks are still outstanding.** Software verification is not flight certification.

---

# V16.3 AP+STA home Wi-Fi extension — November 13, 2026

- Added optional simultaneous direct AP + 2.4 GHz home-router station mode while retaining `GREEN-ION-V16-3` at `192.168.4.1`.
- Added bounded nonblocking station attempts/retries and asynchronous scans that are disabled during recorder-critical states.
- Added NVS-backed home credentials and optional direct-AP password without exposing saved passwords through APIs, logs or reports.
- Added physical SW38 + temporary management session + CSRF protection for credential-changing endpoints.
- Added mobile Wi-Fi Settings UI for scanning, hidden/manual SSID entry, connect/disconnect/forget, RSSI/mode and both dashboard addresses.
- Added C++/Python/JavaScript regression tests. Physical AP+STA/router/iPhone behavior remains **NOT VERIFIED** until real Feather bench validation.

# V16.3 — Shared-SPI reliability and full home verification — November 12, 2026

- Added GREEN ION Feather V16.3 and Uno receiver V16.3 source/build identities while preserving RKT4 v2, GIS1 and protocol-v2 70-byte receive-only LoRa.
- Added explicit shared-SPI ownership for the Feather perfboard: SD and RFM9x keep independent CS lines; radio SPI is deferred when SD/recording work owns the bus.
- Removed RadioHead SPI work from DIO0 interrupt context; the ISR records an event and loop-time servicing honors sample-deadline margin.
- Prioritized sampling, primary BIN flush/checkpoints and SEN work ahead of bounded radio dispatch/recovery.
- Added shared-SPI/radio counters, TX start/completion/timeout evidence, reset/brownout diagnostics and safe recovery counters.
- Extended Full Home Test with a real 30-second 50 Hz temporary RKT4 recording plus bounded LoRa activity and actual Uno packet evidence when connected.
- Receiver PASS requires actual received bench packets; absent receiver remains NOT TESTED and missing evidence is never promoted to PASS.
- Preserved V16.2 live Dashboard rocket, true WebGL 3D viewer, Maps recovery, flight library/comparison, backup-aware SD management, cloud integrity and five-tab #081420 design.
- Fixed duplicated hidden tab semantics that caused the V16.3 browser regression while retaining historical internal route compatibility.
- Updated the verified browser firmware updater to recognize versioned V16.3 Feather assets and the V16 release SHA manifest.
- Added native shared-SPI scheduler tests, Python fault/contract tests, browser/WebGL regression coverage, Windows packaging and V16.3 release/source packaging.
- Physical perfboard concurrency, RF range, GPS, power/brownout, mechanical install, OTA interruption and real-flight validation remain pending.

---

# V16.2 — Full application + firmware upgrade — November 12, 2026

- Added explicit GREEN ION Feather V16.2 and Uno V16.2 source generations while preserving RKT4 v2, GIS1 and protocol-v2 70-byte receive-only LoRa.
- Added boot/session/reset evidence, sensor/SD recovery counters, read-age diagnostics and safe idle peripheral/card reinitialization.
- Separated brief Wi-Fi display continuity from strict command freshness so transient delays do not erase mission context or authorize stale controls.
- Added Balanced/Simple/Advanced presentation with Balanced as default; exact five tabs and #081420 visual identity remain.
- Added a lightweight Dashboard rocket animation with stale-data freeze, saved-flight replay and explicit measured/estimated/illustrative labeling.
- Replaced the old flat Canvas trajectory display in normal use with a genuine WebGL scene: depth, perspective, 3D rocket geometry, orbit/pan/zoom, camera modes, actual timestamp playback, phase jumps, point picking and graph synchronization.
- Added optional embedded Google Maps Recovery with satellite/terrain/roadmap/hybrid modes, confirmed GPS trail, optional user position and keyless/offline fallback.
- Expanded SD cleanup to show completion plus browser/Windows/cloud backup evidence before typed-filename deletion; interrupted/recovery evidence is retained.
- Added rocket/motor/date/status/backup flight-library filters and 2–4 flight comparison with launch-relative graphs and export.
- Made Run Full Home Test the primary Testing action and expanded System Health/troubleshooting.
- Added mission next-action summary, first-run helper, rocket display/mounting profiles, one-click postflight report entry and data-quality scorecard.
- Added actual headless-browser WebGL render smoke testing, extended reconnect soak coverage, V16.2 firmware compilation and verified release/source packaging.
- Physical RF/power/GPS/SD/mechanical/OTA-interruption/real-flight validation remains pending.

---

# V16.1 RC2 — Safe SD management + layout hardening — November 12, 2026

- Added app-driven deletion of completed Feather SD flight packages over verified Wi-Fi.
- Restricted deletion to exact `V12D###.BIN` identities with matching filename confirmation.
- Delete SEN/TXT/JRA/JRB/RCV companions first and the primary BIN last; verify removal before reporting success.
- Block SD deletion, listing and downloads during recording, home verification and firmware maintenance/OTA work.
- Pause launch watching for SD deletion and restart stationary calibration afterward.
- Keep browser, Windows and cloud backups untouched when deleting from Feather SD.
- Removed broken legacy Protect/Add note, manifest and integrity-scan controls that no longer had V14 backend routes.
- Restored saved-flight quick stats through current V14 TXT metadata.
- Simplified selected-flight package download to available BIN + SEN + TXT files.
- Improved Flights & Analysis hierarchy and desktop file-row layout while preserving the original blue background and exact five visible tabs.
- Updated stale Help guidance for Windows startup, recovery maps, 3D replay and SD deletion.

---

# V16.1 RC1 — Reliability + simplicity — November 11, 2026

- Added continuous Uno USB rediscovery and reconnect, including COM-port changes after unplug/replug.
- Added concise software/link diagnostics plus downloadable troubleshooting evidence.
- Added a simple guided front door to the existing real one-button home/preflight test.
- Added Google Maps recovery links, optional current-location comparison, straight-line distance/bearing and Google Maps directions.
- Locked critical GPS, altitude, battery, flight-state, health and receiver-quality fields into the compact receive-only LoRa regression contract.
- Improved sustained pressure phase detection, confidence scores, IMU/GPS corroboration and pressure-vs-GPS altitude comparison.
- Improved offline 3D time alignment, light display smoothing, phase markers and animation-frame replay while keeping recovery GPS raw.
- Added flight-library search, favorites, tags and notes without modifying original BIN/SEN/TXT files.
- Added release-note-aware Windows updates and one-click rollback to the prior complete app package; GREEN_ION_Data is untouched.
- Kept the blue five-tab layout and reduced always-visible Dashboard readings.
- Added a verified `v16.*` release workflow for web, Windows, Feather and Uno artifacts.

---

# V16 RC.2 — Windows launcher hotfix — November 11, 2026

- Fixed the packaged PowerShell launcher to execute the constructed `ground_station.py` argument list instead of PowerShell's unrelated automatic `$Args` variable.
- Preserve forwarded manual options such as `--port COM5` while retaining automatic Uno discovery and Wi-Fi-only fallback.
- Added a packaging regression that fails if the launcher stops splatting `$LaunchArgs`.
- Windows package identity is now `16.0.0-rc.2`; user data and firmware are unchanged.

---

# V16 Phase 8 — Hardware-validation evidence framework — November 11, 2026

- Added a local hardware-evidence ledger covering Windows reconnects, SD/network-loss recording, sensors, outdoor GPS, receiver-side LoRa, shared SPI, power, OTA/rollback, mobile workflow, mechanical/RF installation and real flight.
- Every physical item defaults to NOT_TESTED; software diagnostics never auto-pass physical tests.
- Manual PASS now requires a measured note or retained evidence reference.
- Added exportable validation JSON and bounded local diagnostic snapshots.
- Even all required PASS entries only produce READY_FOR_HUMAN_REVIEW_NOT_FLIGHT_AUTHORIZATION.
- Documented exact physical evidence still required. No unperformed bench, RF, power, OTA or flight test is claimed as complete.

---

# V16 Phase 7 — Advanced visualization — November 11, 2026

- Added offline postflight 3D trajectory reconstruction from immutable RKT4/GIS1 originals.
- Uses verified current GPS for local East/North track and pressure-derived relative altitude for vertical position.
- Falls back to pressure-only vertical replay when GIS1/GPS is unavailable instead of inventing a track.
- Added measured-gyro relative orientation replay using normalized quaternion integration.
- Explicitly labels gyro drift, arbitrary initial reference, generic/non-scale rocket rendering and non-simulation limits.
- Added orbit, zoom, timeline replay and lightweight Canvas rendering; no visualization work runs on the Feather.
- Added synthetic trajectory/orientation regression coverage and offline caching.

---

# V16 Phase 6 — Windows packaging and updates — November 11, 2026

- Added one-click Windows startup with automatic Uno discovery and no normal COM-port prompt.
- Added pinned pyserial setup with graceful Wi-Fi-only fallback.
- Kept GREEN_ION_Data outside the replaceable application folder and added runtime health diagnostics.
- Added deterministic V16 Windows ZIP, SHA-256 and manifest artifacts to CI.
- Added an official-release updater that verifies SHA-256, installs side-by-side and never deletes user flight data.
- Kept Feather/Uno firmware updates separate from the Windows application updater.
- Expanded CI path coverage so Windows-only changes are tested.
- Added package/updater regression coverage.

---

# V16 Phase 5 — Data and cloud — November 11, 2026

- Grouped immutable BIN/SEN/TXT originals into flight packages with completeness and collision status.
- Added local SHA-256 reread verification and an exportable package manifest.
- Added explicit optional-cloud states: signed out, unavailable, offline, idle, pending, retrying, synced and error.
- Kept local browser/disk originals authoritative; cloud never gates recording, recovery or analysis.
- Preserved checksum deduplication and checksum-verified cloud restore.
- Added data/cloud regression coverage and refreshed offline assets.

---

# V16 Phase 4 — Five-tab interface — November 11, 2026

- Exposed exactly five normal top-level tabs: Dashboard, Flight, Flights & Analysis, Testing, and Settings & Advanced.
- Added a concise Dashboard mission summary and moved detailed live graphs/Sensor Lab behind an expandable section.
- Kept the V16 Mission Controller first in Flight and integrated Recovery into the same operational workflow.
- Combined Storage and History under Flights & Analysis while preserving offline RKT4/GIS1 analysis, reports, comparisons and local archives.
- Kept the full preflight/hardware/storage verification workflow in Testing.
- Grouped profiles, utilities, radio diagnostics, cloud, firmware updates and help into expandable Settings & Advanced sections.
- Preserved hidden legacy tab aliases for older internal links while mapping them to the correct visible tab.
- Updated Alt+1 through Alt+5 keyboard navigation and offline PWA caching for the new layout/navigation modules.
- Added strict tests for the exact five visible tabs and preservation of existing feature cards.

---

# V16 Phase 3 — Mission workflow — November 11, 2026

- Added a persistent mission state machine from preflight through recovery, download, analysis and completion.
- Integrated the existing full preflight as structured evidence instead of duplicating sensor tests.
- Kept measured failures and missing receiver evidence non-overridable; physical checks require separate explicit confirmation.
- Made fresh unified Feather/LoRa flight-state evidence authoritative for ascent, descent, landed and recovery transitions.
- Added GPS-missing recovery warnings without interfering with autonomous SD recording.
- Bound postflight transfer/save/report events to the current mission filename when known and ignore unrelated archived flights.
- Restore active browser sessions as INTERRUPTED rather than pretending continuity; fresh telemetry can restore the real phase.
- Added exportable, bounded mission evidence/transition history and offline help.
- **No ignition/deployment controls were added. Browser mission state is advisory; Feather flight detection and recording remain autonomous.**

---

# V16 Phase 2 — Unified telemetry — November 11, 2026

- Added one source-aware `/api/v16/telemetry` contract spanning independent Feather Wi-Fi and receive-only LoRa links.
- Added per-reading source, age and freshness plus packet/session identity, protocol compatibility and receiver integrity counters.
- Deduplicated duplicate/out-of-order radio frames from the live display while preserving them in bounded receiver history.
- Added sender reboot/session rollover, receiver reconnect tracking, RSSI/SNR and reception timestamps.
- Added safe automatic Arduino Uno discovery for the one-click Windows launcher, with explicit manual and Wi-Fi-only overrides.
- Changed Windows link diagnostics to consume one fused endpoint instead of parallel telemetry/link polling.
- Added Wi-Fi-only, LoRa-only, dual-link, stale, protocol mismatch, duplicate, out-of-order, reboot and reconnect regression coverage.
- **LoRa remains receive-only. RKT4 v2, GIS1 and the 70-byte radio packet remain unchanged. Physical hardware validation is still pending.**

---

# V16 Phase 1 — Windows reliability — November 11, 2026

- Added explicit Connected, Delayed, Reconnecting and Disconnected Feather Wi-Fi states with last-success age and diagnostics.
- Coalesced simultaneous browser status polling so duplicate dashboard callers share one request.
- Changed background Feather polling to defer during active transfers instead of treating local request contention as a Wi-Fi failure.
- Preserved last-known Feather status as visibly stale while keeping commands blocked until fresh verification returns.
- Added V16 Windows reliability regressions, including simulated one-hour operation, short interruptions, prolonged loss, reconnect and stale-state safety.
- Bumped the offline cache so installed clients receive the reliability changes.
- **No Feather/Uno firmware, RKT4 v2, GIS1 or autonomous recording format changes. Physical hardware validation remains pending.**

---

# V15 local-first software candidate — November 11, 2026

- Implemented one-button automatic real-sensor bench preflight, with no fabricated physical confirmations.
- Added blue Recovery tab with observed GPS fixes, source/freshness, offline schematic and map navigation.
- Added local Web Worker advanced RKT4/GIS1 flight analysis and automatic integrity-aware health reports.
- Served the current blue PWA on the Windows loopback ground station; mirrored immutable finished flight files into a SHA-256-verified computer archive; preserved existing Uno/Feather parallel links.
- Added battery endurance planning (requires tested battery capacity and measured current), receiver-side LoRa link metrics and offline searchable Help Assistant.
- Retained automatic opt-in Supabase backup retry when internet is available and account is signed in.
- **No changes to Feather/Uno firmware or RKT4/GIS1 formats.** Actual launch, RF range, battery, OTA and physical hardware tests pending. 3D flight and digital twin remain planned only.

---

# V14.0.0-beta.4 — Responsive sensors, private sync and organized documentation

- Retained original blue tabs and classic flight workflow, with device-width adaptive CSS, optional compact and imperial modes.
- Added read-only Sensor Lab for real BMP388, IMU, GPS, battery, LoRa, recorder and health metrics, including new firmware status fields.
- Added offline CRC32-verified GIS1 SEN analysis with X/Y/Z acceleration, gyro, GPS, CSV, track GeoJSON and companion RKT4 header validation.
- Improved Supabase cloud backup pagination, progress, exponential backoff, Data Saver behavior and verified restore.
- Simplified README with direct links to soldering, sensor, device, cloud, improvements and development subsections. Archived prior README.
- Still software beta: physical hardware, Safari and OTA testing required.

---

# V14.0.0-beta.3 — Dashboard layout and offline usability

- Removed blank card-grid rows and doubled margins while preserving the full blue design, mobile/desktop tab structure and all existing telemetry.
- Improved tabs with horizontal scrolling, keyboard navigation and accessible selection state.
- Fixed offline hardware-control guard so saved graphs, CSV/PDF reports, local history and appearance controls remain usable without the Feather.
- Added layout/interaction regression tests and offline caching of the new stylesheet and UI module.

---

# V14.0.0-beta.2 — Offline analysis improvements

- Added original RKT4 BIN analysis, complete offline altitude/velocity/acceleration/G/battery graphs, replay timeline, flight comparisons, CSV/JSON reports and Print / Save PDF to the History tab. All computed on device; original flight files unchanged.
- Added regression tests for invalid RKT4 headers, original file preservation and offline analysis integration.

---

# V14.0.0-beta.1 — Unified blue website and private cloud backups

- Preserved complete V13 blue dashboard layout/tabs, added browser Web Serial receive-only Uno LoRa, offline original file archive, private Supabase backups, GitHub update notifications, Feather V14 fallback firmware, PWA caching, Vercel deployment and CI artifacts.
- All original V12D### BIN/SEN/TXT formats, previous firmware sources and release tags remain supported.
- Software beta; real hardware and browser local-network validation required.

---

## GREEN ION V13 beta — Unified dashboard and easy downloads (2026-11-11)

- Added Feather Wi-Fi + Uno LoRa integrated Windows dashboard based on the existing blue Feather UI.
- Added connection-aware Wi-Fi status and LoRa fallback, receiver packet freshness, GPS last-known location, and explicit offline state.
- Added V13 Feather sketch while retaining V12D###, RKT4 v2, GIS1, profile/preflight and flight recorder behavior.
- Added isolated computer logs, one ZIP download workflow with compiled firmware, libraries and replacement instructions.
- Pending: successful V13 CI confirmation, real hardware bench/reconnect, antenna/range and flight verification.

# Green Ion changelog

Every known or referenced version from **V1 through V10** is listed here. See [VERSIONS.md](VERSIONS.md) for the complete archive index and exact preserved sketch links.

**Evidence:** Actual firmware snapshots exist for V8–V10. V1, V1.2, V3 and V3.1 have documented development or serial-output evidence, but original source has not been preserved. V2 and V4–V7 are undocumented placeholders, not confirmed releases. No missing source is fabricated.

## V10 — Field-Resilient Recorder (October 26, 2026)

- Added dual alternating power-loss checkpoints, emergency RAM spill recovery, persistent brownout evidence, health snapshots, watchdog feeding, and sensor plausibility scoring.
- Added phase-sensitive filtering, post-spike filter rebuilding, adaptive landing detection, environmental warnings, SD profiling, storage estimates, and prior-ground-test comparison.
- Added post-flight reports, a flight manifest, integrity scans, package downloads, configuration backup/restore, maintenance reminders, Field mode, and opt-in spoken phone cues.
- Added ESP image validation and guarded rollback to a valid alternate OTA partition.

## V9.2 — Resilient Two-Tap Flight Workflow (October 26, 2026)

- Reduced launch preparation to **Prepare for Launch** followed by **Arm & Watch for Launch**, while retaining compatibility API aliases.
- Added calibration-age, temperature-change, and ground-pressure freshness checks before arming.
- Added a downloadable per-mission preflight report with sensor, battery, SD, calibration, and ground-test evidence.
- Added ejection-pressure/transient anomaly markers that preserve raw RKT4 samples but exclude detected spikes from phase and landing decisions.
- Added debounced low-battery sample-rate reduction and a critical-battery checkpoint while keeping timestamped RKT4 records compatible.
- Added sensor-outage tracking, automatic BMP388 reinitialization attempts, recovery markers, and continued invalid records instead of an automatic stop.
- Added a stored confidence timeline containing phase, anomaly, battery-rate, sensor, and SD events.

## V9.1 — Accurate Altitude Calibration and Filtering (October 26, 2026)

- Replaced the short simple-average calibration with 15 warm-up readings, up to 96 valid stationary samples, trimmed-mean outlier rejection, pressure-noise measurement, and drift rejection.
- Added a separate zero-verification stage and explicit calibration quality feedback in meters and pascals per second.
- Preserved unmodified pressure values in RKT4 records while adding a three-reading pressure median for calculated altitude.
- Split altitude processing into a smooth dashboard path and a faster flight-detection path so display jitter can be reduced without delaying launch, phase, or landing estimates.
- Added calibration quality and filter-method fields to flight metadata and dashboard diagnostics.
- Added host-side outlier and filter-response tests while retaining the RKT4 32-byte header and 16-byte record format.

## V9 — Automatic Flight Workflow (October 24, 2026)

- Reorganized the dashboard into Status, Flight, Testing, Utilities, Storage, and System; moved all live graphs to Status and removed the visible Launch Assistant and flight-comparison interface.
- Added automatic control when one Wi-Fi station is connected, with the existing lease retained for multiple viewers.
- Added one-button preparation covering settings, time synchronization, sensor/SD/battery checks, calibration, and the complete 10-second ground test.
- Added a three-second launch prebuffer, adaptive launch confirmation, automatic Wi-Fi disconnect after a committed checkpoint, and automatic Wi-Fi restart after landing save.
- Added pressure-estimated powered ascent, coast, apogee, descent, probable parachute, and landing phases with confidence metadata.
- Added permanent mission numbers plus per-card sequence numbers persisted in `/GREENION.CFG`, independent of visible flight files.
- Simplified Storage and cleared all cached summaries/graphs after deletion before refreshing the card.
- Preserved the RKT4 header and 16-byte sample format.
- Published the verified `.ino.bin` beside the V9 source with a direct phone-download link and installation guide.

## V8.8 — Field Confidence Edition (October 24, 2026)

- Created the standalone `Green_Ion_V8_8` firmware snapshot and phone-update artifact.
- Added a 10-second end-to-end ground test covering BMP388 validity, sensor freshness, altitude noise, battery stability, sampling completeness, SD writes, and flush timing, with explicit PASS or REVIEW feedback.
- Added frozen-pressure detection, live successful-flush heartbeat, and maximum SD write/flush timing.
- Added configurable 2, 5, 10, or 20 minute automatic recording limits.
- Added phone-supplied date/time, firmware version, and Git commit build ID to flight metadata.
- Added minimum battery voltage, maximum voltage drop, postflight confidence score, and FNV-1a checksum metadata while retaining RKT4 binary compatibility.
- Added flight protection against dashboard deletion and post-recovery notes stored in the TXT companion file.
- Added a downloadable diagnostic report containing firmware/build, reset, sensor, SD, battery, timing, integrity, and latest error information.

**Dates:** October 24, 2026 repository commits are archive/commit dates, not necessarily original prototype creation dates.

This repository starts with complete source snapshots at V8. The V1–V7 history below is reconstructed from available conversation text and serial-output captures. Entries marked **documented by output** describe behavior visible in those captures but do not have preserved source in this repository. Entries marked **undocumented** are retained as version placeholders and do not claim features that could not be verified.

## V8.8 recording reliability (development, October 24, 2026)

- Added a five-second flight checkpoint journal alongside the existing 500 ms binary flushes.
- Added startup recovery of interrupted recordings, RKT4 header validation, complete-sample counting and trailing-partial-byte reporting.
- Preserved original BIN files during recovery; marked recovered metadata as interrupted, not complete.
- Prevented false interruption reports if a reset occurs between successful metadata finalization and journal cleanup.
- Added recording integrity, recovered flight count, last recovered sample count and numeric ESP32 reset reason to the dashboard.
- Strengthened CSV header validation and added synthetic RKT4 tests to the GitHub Actions workflow.
- Hardware power-loss and SD-card interruption testing remains outstanding; successful compilation is not a guarantee of crash consistency.

## V8.7 integrated launch workflow and report update (October 24, 2026)

- Extracted the embedded dashboard into `Past_Versions/Green_Ion_V8_7/Dashboard.h` to avoid Arduino's HTML/JS sketch preprocessing failure; restored successful firmware builds.
- Added CI validation of dashboard JavaScript syntax, duplicate IDs, and references to missing UI elements; the GitHub Actions artifact includes both the .ino and header source files.
- Added a contextual Launch Assistant, progress stages, physical preflight checklist, recovery review shortcut, and connection/troubleshooting help. Confirmed the system remains a **logger only** with no ignition or deployment controls.
- Added Sunlight high-contrast display mode, more responsive tablet/desktop cards, and improved button, status and file list layout.
- Kept unsaved motor and recording-type inputs from being reset by status polling. The Arm Logger action saves any changed mission and sampling-rate settings before arming.
- Reduced live polling from 500 ms to 1.2 seconds and only draws live graphs on visible tabs to reduce ESP32 and mobile-browser work.
- Improved Storage navigation so opening recent flight stats does not scroll away from SD diagnostics; corrected motor comparison selection; guarded asynchronous graph loads and long-flight calculations.
- Expanded downloadable/printable flight reports with altitude, velocity, estimated acceleration, vertical apparent G, battery voltage, pressure and temperature graphs, plus metadata, phases, and quality metrics.
- Added graphs to motor comparison printable PDF and included estimated acceleration and apparent vertical G columns in CSV exports. The raw RKT4 binary format is unchanged, and those estimates are not IMU readings.
- Hardware testing is still needed after firmware compilation; archived versions and their evidence remain preserved.

## V8.7 dashboard and SD reliability updates (October 24, 2026)

- Fixed the apparently unresponsive **Detect & Test SD Card** button: it now remains clickable in view-only mode, requests the controller lease automatically if available, and displays actionable progress and errors. If another phone holds control, it explains why the test cannot run.
- SD verification checks temporary-file write, readback, cleanup and at least 1 MB free. Added mounted, verified, free-space, and saved-recording diagnostics.
- Explicitly initialized ESP32 SPI for the documented breakout wiring: SCK GPIO5, MISO GPIO21, MOSI GPIO19, CS GPIO33. Confirm this wiring matches the physical board before using the updated firmware.
- Improved narrow-phone spacing, file action buttons, cards and status rows.
- Added an Overview recorder snapshot and a shortcut to saved flight stats. Storage now automatically opens the latest saved flight summary, with direct View stats buttons for others.
- Added saved-flight valid-sample percentage and imperial peak-altitude conversion (when Imperial is selected). Stats are read from saved TXT metadata; unavailable measurements are not invented.
- Corrected the status API version to 8.7.
- GitHub Actions compiles the sketch to the main `.ino.bin`; a successful build and real hardware verification are separate checks.

## V8.7 — Dashboard Reliability Edition

- Fixed sample-rate selection being overwritten by the 500 ms status refresh.
- Persisted 10/25/50 Hz sampling in ESP32 Preferences across restarts.
- Added automatic sequential recording labels, with Flight/Test selection.
- Added searchable motor suggestions while supporting custom designations.
- Fixed overlapping Take/Release Control buttons on narrow screens and improved padding.
- Preserved earlier V8.6 code under its original folder.

## V8.6 — Flight Control Edition

- Added a renewable controller lease so many phones can view while only one can change settings or recorder state.
- Added clear view-only/controller status and a Take Control action to the dashboard.
- Required the controller token for flight commands, SD deletion, LED changes, shutdown, and firmware installation.
- Kept read-only status, file listing, and downloads available to observers.

## V8.5 — Phone Update Edition

- Added safe upload of GitHub-built `.ino.bin` firmware from the phone dashboard.
- Blocked updates while armed, recording, calibrating, simulating, shutting down, or at critical battery voltage.
- Added automatic reboot only after the ESP32 Update library verifies the complete image.
- Updated GitHub Actions packaging to include only the correct main OTA image and the matching `.ino` source.

## V8.4 — Adaptive Flight Edition

- Added selectable 10, 25, and 50 Hz logging rates in the dashboard.
- Stored the selected rate in the RKT4 header and flight metadata.
- Added an adaptive altitude filter with a stationary deadband and faster response to large changes.
- Added velocity spike rejection and motion-sensitive speed filtering.

## V8.3 — Flight Data Edition

- Removed the four-flight concept in favor of sequential `FLT0001`–`FLT9999` recordings.
- Added flight labels and motor designations.
- Added companion postflight summaries with duration, peak altitude, minimum pressure, temperature range, sample counts, gaps, and completion reason.
- Added on-demand CSV conversion and phone downloads while preserving original RKT4 binary files.
- Added automated ESP32 compilation and downloadable GitHub Actions artifacts.

## V8.2

- Added a dashboard deep-sleep shutdown control with physical RESET wake instructions.
- Preserved explicit recorder-only safety scope.

## V8.1

- Added non-blocking BMP388 calibration and progress feedback.
- Completed dashboard command/API matching and request error handling.
- Included live altitude, battery, unit conversion, simulation, graph, LED, and storage controls.

## V8

- First complete source version preserved in this repository.
- Added the embedded mobile dashboard, BMP388 calibration, live altitude, battery estimate, microSD diagnostics, flight controls, RKT4 logging, file downloads, graphs, simulation, and LED controls.

## V7

- **Undocumented.** No V7 source, serial output, or reliable feature list was available.
- The existence of a distinct V7 release could not be independently verified from the preserved materials.

## V6

- **Undocumented.** No V6 source, serial output, or reliable feature list was available.
- The existence of a distinct V6 release could not be independently verified from the preserved materials.

## V5

- **Undocumented.** No V5 source, serial output, or reliable feature list was available.
- The existence of a distinct V5 release could not be independently verified from the preserved materials.

## V4

- **Undocumented.** No V4 source, serial output, or reliable feature list was available.
- The existence of a distinct V4 release could not be independently verified from the preserved materials.

## V3.1 — microSD recorder prototype

- **Documented by output; source not preserved.**
- Added microSD initialization and allowed the recorder to continue when no card was detected.
- Retained startup calibration, the armed state, relative altitude, vertical velocity, peak altitude, and battery-voltage reporting.
- Serial captures show substantially steadier velocity values than the V3 test output, but the exact filtering algorithm is not available for verification.

## V3 — flight-state recorder prototype

- **Documented by output; source not preserved.**
- Expanded the project from a basic altimeter into a recorder-style state machine.
- Added `ARMED`, `FLIGHT`, and `LANDED` states with automatic launch detection.
- Reported relative altitude, vertical velocity, maximum altitude, battery voltage, and recorder state over Serial.
- Bench-test captures show a detected launch and transition to the landed state.

## V2

- **Undocumented.** No V2 source, serial output, or reliable feature list was available.
- The preserved history jumps from the V1.2 altimeter to the V3 recorder test, so no V2 behavior is claimed here.

## V1.2 — calibrated relative-altitude prototype

- **Documented by output; source not preserved.**
- Detected the BMP388 and added a sensor warm-up period.
- Established a starting pressure while the sensor was held still.
- Converted pressure changes into altitude relative to the calibrated starting point.
- Printed relative altitude, temperature, and pressure over Serial; stationary samples clustered near zero instead of the large offset seen in the earlier prototype.

## V1 — initial BMP388 altimeter prototype

- **Partially documented by output; source not preserved.**
- Established the first known BMP388 pressure, temperature, and altitude readout.
- Early serial captures show a large incorrect altitude offset of roughly `-1839 m`, indicating that the initial reference-pressure handling was not yet suitable for relative flight altitude.
- Exact V1 source and its original release notes were not preserved.

## Original-source and archive links

- [V8 source](Past_Versions/Green_Ion_V8/Green_Ion_V8.ino)
- [V8.1 source](Past_Versions/Green_Ion_V8_1/Green_Ion_V8_1.ino)
- [V8.2 source](Past_Versions/Green_Ion_V8_2/Green_Ion_V8_2.ino)
- [V8.3 source](Past_Versions/Green_Ion_V8_3/Green_Ion_V8_3.ino)
- [V8.4 source](Past_Versions/Green_Ion_V8_4/Green_Ion_V8_4.ino)
- [V8.5 source](Past_Versions/Green_Ion_V8_5/Green_Ion_V8_5.ino)
- [V8.6 source](Past_Versions/Green_Ion_V8_6/Green_Ion_V8_6.ino)
- [V8.7 source](Past_Versions/Green_Ion_V8_7/Green_Ion_V8_7.ino)

Earlier historical records (not compilable firmware): [V1](Past_Versions/V1/README.md) · [V1.2](Past_Versions/V1_2/README.md) · [V2](Past_Versions/V2/README.md) · [V3](Past_Versions/V3/README.md) · [V3.1](Past_Versions/V3_1/README.md) · [V4](Past_Versions/V4/README.md) · [V5](Past_Versions/V5/README.md) · [V6](Past_Versions/V6/README.md) · [V7](Past_Versions/V7/README.md).
# V11 Release Candidate — Hardware Validation Pending

- Preserves RKT4 v2, 10/25/50 Hz recording, calibration, autonomous recording, recovery checkpoints, battery monitoring, OTA update and rollback behavior from V10.
- Adds byte-count/version evidence to raw BIN downloads.
- Adds an offline browser analyzer with worker-based parsing, pressure-derived altitude/motion estimates, quality warnings, CSV/JSON exports, graphs and local flight history.
- Adds installable PWA caching. Browser security is not weakened; files downloaded from the Feather are imported into the HTTPS/offline analyzer.
- Software tests and CI compilation are required; physical Feather, BMP388, battery and microSD validation remains pending.

# V12 Development Firmware — Current on Main

- Promoted the complete `Past_Versions/V12/Green_Ion_V12/`, `Past_Versions/V12/Ground_Receiver/`, `Ground_Station/`, shared protocol/recorder libraries, tests, and documentation to `main`.
- Added phone/laptop and USB hardware tests for the LSM6DSO32, PA1010D, and BMP388 status while preserving recorder priority and RKT4 v2.
- Preserved the full V11 release candidate under `Past_Versions/Green_Ion_V11/`.
- V12 has passed host tests and target compilation, but physical sensor, SD, radio, recovery, and flight validation remain required.

