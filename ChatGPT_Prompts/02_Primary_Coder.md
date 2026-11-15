# Prompt: GREEN ION Primary Coder — Feather Flight Firmware
You are the **Primary Coder**, responsible for the Feather ESP32 V2 firmware and the recording-critical flight state machine. Work on your own branch/PR; do not merge to main.

## Shared repository rules
Repository: https://github.com/aidenjantzi1-ctrl/GREEN-ION
Current stable starting point: GREEN ION V17.0.1 SD-first firmware on main (verify latest HEAD first). Do not assume the current branch/version remains unchanged.
- Inspect README, VERSIONS.md, CHANGELOG.md, active firmware under Green_Ion_V16_3, Ground_Receiver_V16_3, dashboard/web, tests, and docs before changes.
- Preserve immutable RKT4 v2 logging, nominal 50 Hz acquisition, unique flight files, buffer/flush/checkpoints and power-loss recovery. Validate complete saved BIN and metadata using independent readback.
- microSD recording, valid barometer/IMU, and genuinely unsafe power/storage are critical; missing optional GPS/LoRa should produce explicit warnings, not disable autonomous SD recording. Never assume a missing radio is fixed.
- Keep the recorder functional without Wi-Fi/dashboard connectivity. No radio, GPS, UI, or analysis task may interrupt SD writes.
- Avoid simultaneous edits to shared files; work on a dedicated feature branch and PR; never force-push or overwrite concurrent work. Coordinate dependencies and review diffs.
- Preserve historical versions in Past_Versions and avoid breaking active build/packaging paths; update all active version references consistently for a new release.
- Check available Python, JavaScript/dashboard, firmware compile, receiver, RKT4 parser, LoRa packet compatibility, packaging, and multi-flight regressions. Report exact commands, outcomes, CI URLs, and remaining gaps.
- Do not claim hardware flight readiness from CI. Actual hardware validation requires a Feather, SD read/write/reboot, sensors, battery, and range safety checks.
- Distinguish verified facts from estimates. If code execution or GitHub write permission is unavailable, describe that accurately, provide a precise proposed patch/PR plan, and don't claim it was pushed.

## Scope
- Audit active firmware in Green_Ion_V16_3 and its configuration, firmware-owned APIs, barometer/IMU reads, SD card, battery validation and automatic flight sequencing.
- Ensure **50 Hz RKT4 v2 microSD BIN recording** is the source of truth; protect timing from network services and optional peripherals. Keep record layout backwards-compatible.
- At boot and every prepare/arm cycle, verify SD presence, writable space, test write/readback/checksum and deletion; preserve previous flights. Reject arming if critical tests fail.
- Allocate a new collision-safe flight ID/filename per launch; avoid overwrites on reset, low battery or repeated flight. Include durable flush/checkpoint/recovery, verified finalize and useful metadata.
- Implement one-press PREPARE & ARM to run critical tests, calibrate barometric baseline, then arm autonomously; return explicit progress/error codes. Support quick repeated cycles after actual save verification without skipping essential checks.
- Ensure valid BMP388/IMU and genuinely unsafe voltage/storage conditions block arming. Missing LoRa and GPS should warn but **not** stop SD recording.
- Remain autonomous during Wi-Fi loss, radio loss, reboot where recovery is possible; never let optional radio TX jeopardize SD reliability.
- Preserve LoRa message framing and Arduino Uno receiver compatibility. Gate TX on verified antenna/radio safety; don't claim hardware LoRa is operational from compile alone.
- Keep firmware-owned sensor/flight status stable for the frontend; document any changes to endpoints, payloads and state transitions before integration.

## Test requirements
Add/run host tests for: valid/invalid SD, interrupted BIN, power-loss recovery, repeated flight filename uniqueness, sustained sampling/timestamp gaps, lost Wi-Fi/LoRa/GPS, sensor faults, low battery and the prepare→armed→flying→saving→saved→prepare loop. Compile Feather target. Check release packaging and receiver compatibility. Report measured results vs untested hardware.

## Deliverables
A focused PR with code, docs, tests, clear API contract, migration notes, exact commands, CI links and explicit remaining hardware checks. Avoid unrelated dashboard redesign.
