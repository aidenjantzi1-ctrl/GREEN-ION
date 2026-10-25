# GREEN ION V9 — seventh set of 60 focused recommendations

Reviewed 2026-10-24 against the current V9 firmware and the earlier six 60-item improvement batches. These are **60 further proposals**, not 60 installed features. Avoid extra screens, hardware, or controls; ship only fixes supported by tests or a clear user need.

**Scope:** flight-data recording, review, and reporting only. GREEN ION must not operate ignition, staging, or recovery equipment. Barometric velocity, acceleration, G-force, and flight phases are estimates, not flight-critical readings.

## P0 — Clock provenance and recording metadata

1. Record whether each completed file used a phone-provided clock or an uptime-only timestamp, without changing the BIN layout.
2. Include the age of the last successful phone clock synchronization in diagnostics rather than implying the clock is always current.
3. Display the phone clock synchronization result in existing System status text only when the request fails.
4. Clarify that the JavaScript timezone offset is minutes west of UTC, avoiding inverted local-time displays.
5. Use one shared conversion routine for displaying local recording times in Storage and PDF headers.
6. Mark a report timestamp as approximate when the recorder has rebooted since its last phone synchronization.
7. Never infer a calendar date from a relative recording timestamp when no absolute reference exists.
8. Preserve original UTC metadata while allowing a report to display local time for convenience.
9. Label the timezone offset used for each exported report so comparisons across travel are understandable.
10. Show a clear 'clock unavailable' placeholder rather than a fabricated 1970 date.
11. Verify the reported date remains consistent when the recording crosses midnight.
12. Add a regression case for a timezone offset that changes after the recording has ended.
13. Add a regression case for a leap-day recording and for a daylight-saving transition.
14. Ensure an invalid time-sync request does not update the displayed last-sync age.
15. Document that phone time is informational metadata and is not used to trigger flight events.

## P1 — SD-test isolation and storage clarity

16. Keep temporary test-file naming within 8.3-compatible filenames for common FAT cards.
17. List the exact temporary filename in diagnostic logs only when a test fails, not on the main dashboard.
18. Use a bounded number of candidate test filenames so a crowded card cannot hang the interface.
19. Leave every pre-existing temporary-looking file untouched when the card is inserted.
20. Report a specific non-destructive error when no unused temporary test filename exists.
21. Ensure a failed SD test does not change the selected recording in Storage.
22. Show whether the last SD test verified write, readback, and cleanup, not merely card detection.
23. Distinguish a failed test-file cleanup from a failed sensor recording in diagnostics.
24. Allow a user to inspect the test-file cleanup warning without being prompted to format the card.
25. Document how to identify and manually back up leftover temporary test files after an interruption.
26. Test the SD-check workflow with filenames that differ only by capitalization on FAT.
27. Test an SD card that becomes unavailable between test-file creation and readback.
28. Test that the SD checker never removes a completed FLT binary or its companion TXT file.
29. Test that the SD checker leaves user-created GITEST.TMP content unchanged.
30. Keep the SD test's success message short enough to fit on a narrow phone without horizontal scrolling.

## P1 — Phase confidence and truthful reports

31. Label launch, apogee, and landing markers explicitly as postprocessed estimates in printable summaries.
32. Count how many valid samples support each inferred launch or landing marker.
33. Show a marker as unavailable when the recording has no contiguous threshold crossing.
34. Avoid reporting a landing timestamp earlier than the estimated apogee.
35. Explain when an automatic recorder trigger differs from the browser's postprocessed launch estimate.
36. Include the time resolution of phase estimates based on the actual local sample spacing.
37. Mark apogee as tentative when it occurs at the final valid sample in the file.
38. Mark landing as tentative when the recording ends immediately after the near-ground threshold.
39. Do not infer an entire descent phase from a single post-apogee observation.
40. Preserve the measured peak altitude even when no launch or landing phase can be inferred.
41. Test phase inference on recordings that start after launch, without manufacturing a launch marker.
42. Test phase inference on a stationary sensor that drifts gradually by a few meters.
43. Test a false near-ground landing cluster followed by renewed ascent in a synthetic trace.
44. Test two separate altitude bursts in one file without interpreting them as two launches.
45. Explain in Help why barometric phase markers cannot confirm actual recovery-system deployment.

## P2 — Accessible review and release discipline

46. Keep the primary preflight action visible on a small tablet without hiding error messages below the fold.
47. Ensure disabled controls provide a text reason that works with screen readers.
48. Make SD-test results readable without relying on green or red color alone.
49. Use consistent button verbs for Prepare, Test, Stop, Save, and Export across all tabs.
50. Keep long filenames selectable and copyable even when visually truncated.
51. Preserve keyboard focus after a Storage list refresh rather than jumping to the top.
52. Announce only meaningful state changes to assistive technology, not every status poll.
53. Add a simple offline message explaining how to reconnect to the device's Wi-Fi.
54. Explain the difference between downloading BIN and exporting CSV in one Help sentence.
55. Make printable reports legible in grayscale on an ordinary school printer.
56. Keep PDF charts and tables inside page margins at common A4 and US Letter sizes.
57. Add a compact printed footer with firmware version, source filename, and report-generation date when available.
58. Run a browser smoke test with a narrow viewport and increased text size.
59. Add a regression test that checks the main Help instructions match the actual API route names.
60. Use a release checklist that distinguishes automated compilation, simulated tests, and unperformed physical checks.

## What this seventh branch actually changes

The branch implements a **small, separate set of existing reliability needs**, not all 60 proposals:

- Validate phone epoch and UTC-offset strings completely before changing either value, and reject clock changes during active preparation or recording.
- Use an unused, FAT-friendly temporary SD-test filename rather than deleting an existing `GITEST.TMP` or another user file. If all bounded candidates exist, fail safely.
- Infer launch and landing only from three consecutive valid altitude samples in the same uninterrupted segment, rather than connecting readings across gaps.
- Add focused regression checks for the clock, SD-test file ownership, and phase inference.

These changes address specific items identified in earlier backlogs; **the numbered 60 above remain a future roadmap**. Defer most of them until hardware testing demonstrates a need.

## Verification and release discipline

1. Review draft V9 PR #4, then #5, then #6, then this stacked PR; keep `main` unchanged.
2. Require a successful GitHub Actions V9 firmware compile, dashboard JavaScript/HTML check, and Python/Node regression suite.
3. Bench-test on a stationary ESP32 without ignition/recovery connections. Verify the clock updates only while idle and invalid values leave the previous time untouched.
4. On a disposable FAT/FAT32 SD card, create a user file named `GITEST.TMP`, run the SD test, and confirm that file remains byte-for-byte unchanged. Repeat with occupied `GIT0000.TMP` slots.
5. Use synthetic CSV with invalid sensor rows to confirm launch/landing markers do not span gaps, then verify PDF graph exports.
6. Verify normal BIN, TXT, CSV, and PDF output, interrupted recording recovery, and usability on phone, tablet, and computer before installing or merging.

**No physical hardware tests are claimed.** A passing firmware build does not establish flight readiness.
