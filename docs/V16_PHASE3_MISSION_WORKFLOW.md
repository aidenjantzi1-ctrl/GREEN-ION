# GREEN ION V16 Phase 3 — Mission workflow

## Goal

Turn the existing tests, live telemetry, recovery tools, file transfer and postflight analysis into one persistent mission workflow without moving flight authority into the browser.

The Feather remains authoritative for:

- launch detection;
- flight phase;
- autonomous SD recording;
- landing/recovery state reported by firmware.

The browser mission controller organizes operator steps and records evidence around those facts.

## Mission states

The V16 mission controller uses:

`IDLE → PREFLIGHT → READY → PRELAUNCH → ASCENT → DESCENT → LANDED → RECOVERY → DOWNLOADING → ANALYZING → COMPLETE`

It also has `ERROR` and `INTERRUPTED`.

Transitions are recorded with timestamp, event and reason. The transition journal is bounded so long-running use does not grow memory without limit.

## Preflight integration

Selecting **Start Mission** enters PREFLIGHT.

The existing full preflight remains the actual measurement routine. When it ends, it emits structured evidence to the mission controller:

- whether every step completed;
- measured failures;
- non-physical/receiver evidence still unverified;
- manual/physical checks still unverified;
- recent detailed evidence.

Measured failures or incomplete sessions enter ERROR. Missing non-physical evidence cannot be overridden by checking a manual box.

After measured checks pass, the user must separately confirm the physical rocket/recovery/power/antenna checks. Only then does the workflow enter READY. This is a workflow readiness state, not launch authorization.

## Flight transitions

After the user begins PRELAUNCH, the controller waits for real Feather/LoRa telemetry.

It does not infer launch from browser timers. Fresh unified telemetry drives:

- firmware phase 3 or 4 → ASCENT;
- firmware phase 5 → DESCENT;
- firmware phase 6 → LANDED;
- firmware phase 7 → RECOVERY.

If the Feather reports flight before the local workflow reached PRELAUNCH, the controller follows the real phase but records an evidence warning.

Missing/stale GPS does not stop autonomous recording. It adds a recovery warning and preserves the last confirmed recovery position.

## Recovery, file retrieval and analysis

After landing, the operator can begin recovery. The existing Recovery tab remains the GPS/LoRa recovery tool.

When automatic SD retrieval begins, the mission enters DOWNLOADING. It moves to ANALYZING only after the matching BIN has been checksum-preserved in local history.

When the matching integrity-aware postflight report completes, the mission becomes COMPLETE.

If the Feather exposed an active/completed log filename, the controller binds postflight events to that filename. Unrelated older archived flights are ignored rather than completing the wrong mission.

## Persistence and interruption

Mission state is persisted in browser local storage under the localhost/PWA origin. App updates do not intentionally clear it.

If the browser restarts during PRELAUNCH, ASCENT, DESCENT, DOWNLOADING or ANALYZING, the workflow restores as INTERRUPTED rather than pretending continuity. Fresh Feather/LoRa flight evidence can then restore the real airborne/landing phase.

Autonomous Feather recording is independent of browser persistence.

## User control

The mission card provides:

- Start Mission;
- Run Full Preflight;
- explicit physical-check confirmation;
- Begin Pre-Launch;
- Begin Recovery;
- shortcuts to Recovery and postflight reports;
- exportable mission evidence JSON;
- workflow reset that does not delete flight files, telemetry logs or cloud backups.

## Acceptance coverage

Automated tests cover:

- IDLE/PREFLIGHT start behavior;
- preflight failure;
- incomplete receiver evidence;
- physical confirmation and READY;
- PRELAUNCH;
- Feather-driven ascent/descent/landed/recovery;
- missing GPS warning;
- download/analyze/complete flow;
- unrelated archived flight rejection;
- restart → INTERRUPTED → telemetry recovery;
- bounded transition history;
- wiring to existing preflight, verified file-save and postflight-report events.

Real launch/landing detection, GPS recovery, RF range, file transfer timing and field workflow still require physical validation.
