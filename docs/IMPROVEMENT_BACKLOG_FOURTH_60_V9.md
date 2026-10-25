# GREEN ION V9 — fourth set of 60 prioritized recommendations

Reviewed on 2026-10-24 against **V9 on `main`**, following the three earlier 60-item lists from V8.7/V8.8 draft pull requests. These are **60 new, V9-specific proposals** about automatic flight recording, the three-second prelaunch buffer, automatic Wi-Fi shutdown/restart, and postflight confidence. **They are not 60 implemented features.**

**Keep it manageable:** do not add another dashboard page, complex setup wizard, or optional control panel to address every idea. Ship only a small, testable group when there is evidence it solves a real problem. GREEN ION is a **data recorder only**; no ignition, staging, parachute, or recovery control.

## P0 — Automatic recording and preservation (1–15)

1. Record the exact number of prelaunch RAM-buffer samples copied into each BIN in its TXT metadata.
2. Preserve chronological timestamp order when the three-second circular prelaunch buffer wraps at 50 Hz.
3. Record whether any prelaunch samples were dropped before the buffer was flushed to SD.
4. Identify automatic-flight versus manual-ground-test recording mode in each checkpoint journal.
5. Ensure a launch detector that stays above threshold cannot allocate a second file for the same flight.
6. Ignore repeated Prepare requests while a V9 preparation sequence is already active.
7. Clear transient launch-detector history when an operator cancels preparation before takeoff.
8. Require sustained stable altitude before declaring landing, rather than a single quiet sample.
9. Preserve the current flight and mark invalid readings if the BMP388 briefly fails near apogee.
10. Save with a distinct timeout reason when automatic landing detection never confirms touchdown.
11. Store the exact auto-stop cause: landing, time limit, operator stop, or interruption.
12. Check for enough free space for both the BIN and companion TXT before automatic preparation.
13. Include the number of prelaunch buffer overflows in the recording integrity summary.
14. Preserve existing FLT files when automatic filename creation or sequence persistence fails.
15. Label an interrupted automatic flight differently from an interrupted ground test after reboot.

## P1 — One clear workflow for phones and tablets (16–30)

16. Explain the difference between **Prepare automatic flight** and **Start ground test** next to the existing controls.
17. Express waiting-for-launch, recording, saving, and ready-for-review states in text as well as color.
18. After an automatic flight, explain that temporary Wi-Fi shutdown during flight is expected.
19. Offer a direct link to the most recently saved automatic-flight BIN in the existing review area.
20. Show the latest automatic-save reason in an existing status row instead of creating a new card.
21. Explain the launch detector in plain language without presenting its threshold as an exact measurement.
22. Explain the three-second prelaunch buffer in one sentence beside the automatic workflow.
23. After reconnection, prompt the user to confirm the saved file before printing a PDF.
24. Keep a clearly labeled manual Stop & Save available during safe bench testing of automatic mode.
25. Explain the automatic stop conditions and when a manual stop is still necessary.
26. When automatic preparation fails, show the first failed prerequisite in the same status message.
27. Distinguish an intentional in-flight Wi-Fi shutdown from a genuine unexpected disconnection.
28. Debounce repeated taps on Prepare while its first request is still in progress.
29. Remember the user's Test/Flight preference across page reloads without restoring a stale armed state.
30. Explain in Help that automatic launch detection can miss unusual or very low-altitude profiles.

## P1 — Reports that explain V9 automatic behavior (31–45)

31. Print **automatic** or **manual** start mode near the source BIN filename in the PDF.
32. Show the actual duration of prelaunch data included, not only the configured three-second target.
33. Distinguish the first logged timestamp from the estimated launch-detection timestamp.
34. Include the automatic end reason in the flight summary.
35. Count invalid pressure readings during the prelaunch buffer separately from later readings.
36. Include time between launch detection and intentional Wi-Fi shutdown in diagnostics.
37. Record when the first SD flush completed relative to the launch trigger.
38. Identify whether final metadata and journal cleanup completed before declaring a report complete.
39. State whether landing was automatically confirmed or a recording time limit ended the file.
40. Calculate the fraction of logged time before versus after detected launch, if the trigger is available.
41. Warn when a flight contains less prelaunch data than the configured buffer duration.
42. Record the first valid pressure reading after Wi-Fi restarts for postflight diagnosis.
43. Explain why a postprocessed launch marker can differ from the live automatic trigger time.
44. Flag a motor comparison where one file used automatic mode and the other manual recording.
45. Print the firmware's V9 workflow version separately from the RKT4 binary format version.

## P2 — Repeatable V9 verification and documentation (46–60)

46. Replay a stationary pressure trace and verify it never creates an automatic flight file.
47. Replay a single-sample pressure spike and verify it cannot trigger a false flight.
48. Replay a realistic rising-pressure-altitude profile and verify exactly one automatic trigger.
49. Replay a brief apogee plateau and verify it is not mistaken for landing.
50. Replay noisy touchdown samples and verify eventual single automatic stop.
51. Replay a short sensor dropout during ascent and verify the saved BIN remains readable.
52. Test a 32-bit microsecond counter rollover while the prelaunch circular buffer is active.
53. Test circular-buffer wraparound at each supported 10, 25, and 50 Hz logging rate.
54. Test simultaneous/repeated Prepare requests and verify one consistent recorder state.
55. Simulate power loss after automatic save but before journal deletion; recovery must be idempotent.
56. Simulate SD disappearance at launch detection without overwriting any existing FLT file.
57. Simulate failed Wi-Fi restart after landing and verify the SD recording remains safe.
58. Simulate low battery during automatic recording and verify incomplete data remains recoverable.
59. Publish a one-page V9 state diagram covering preparation, trigger, recording, landing, save, and reconnection.
60. Use a one-page operator signoff sheet for the V9 automatic-flight ground tests before field use.

## What this pull request actually implements

This branch intentionally implements **only a small set of existing correctness fixes**, rather than the 60 proposals above:

- Preserve blank CSV values as missing and reject malformed validity flags in dashboard analysis.
- Preserve a legitimate first RKT4 timestamp of zero; stop derived speed/acceleration across invalid pressure samples or implausibly large time gaps.
- Draw breaks in printed flight graphs when invalid measurements separate valid segments, including downsampled graphs.
- Add focused automated tests for the CSV parser, timestamp guards, and graph gap handling.

The third V8.8 draft PR includes related reliability fixes but targets an older source tree; this V9 branch independently ports the applicable corrections. Do not merge the old V8.8 PR blindly into V9.

## Suggested order

1. **Now:** review the small V9 data-integrity changes in this pull request. Require green CI and physical tests before installation.
2. **Next ground-test session:** focus on P0 #1–3, #5, #8 and #10 using disposable recordings and a known-good SD card.
3. **Only if the workflow is confusing:** select two or three P1 explanations that fit in existing labels and notices.
4. **Later:** add report fields only when firmware captures reliable source data. Avoid invented statistics.
5. **Defer** all other proposals unless they address a reproducible problem.

## Release gate

- GitHub Actions must compile the **current V9** and run Python/Node regression checks.
- On the physical Feather, perform a 15–30 second ground recording, confirm BIN/TXT/CSV and PDF outputs, and inspect invalid-sample handling.
- Test automatic preparation and flight-mode behavior only under safe stationary/bench conditions with no ignition or deployment connections.
- Test interruption recovery on a disposable FAT/FAT32 card. Never assume a green build proves physical reliability.
- Keep `main` unchanged until the user reviews and merges the pull request.
