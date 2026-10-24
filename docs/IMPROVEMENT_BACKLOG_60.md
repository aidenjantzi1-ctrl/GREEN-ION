# 60 focused improvement recommendations

Reviewed against the V8.7 firmware, dashboard, recovery tests and README on 2026-10-24. These are *incremental follow-ups* to existing features, not a request to add 60 new screens. **Only ship a few related changes per release.** `[done in this PR]` means a proposed change is already implemented on this review branch; all other items are recommendations, not claims of completed work.

**Safety boundary:** GREEN ION remains an experimental **data recorder only**. It must not operate ignition, staging, parachute deployment or recovery hardware. Sensor-derived acceleration/G and phase markers are estimates, not flight-critical measurements.

## P0 — Data integrity and reliability (1–15)

1. **Preserve blank CSV fields as missing values** instead of silently turning them into zero measurements. **[done in this PR]**
2. **Track the first CSV timestamp explicitly**, including when its value is exactly zero. **[done in this PR]**
3. **Break velocity/acceleration calculations at invalid sensor samples** and reject unreasonable time gaps rather than bridging them. **[done in this PR]**
4. **Run the firmware build and regression tests for pull requests**, not just pushes to main. **[done in this PR]**
5. Distinguish **SD card absent, mount failed, read failed, write failed and insufficient space** with separate recovery instructions.
6. Recheck SD readiness after a suspected removal or I/O failure instead of trusting a stale successful test.
7. Show a visible **partial-record warning** when an interrupted BIN has trailing bytes ignored by CSV export.
8. Compare BIN record counts with companion TXT metadata and label mismatches as **needs review**.
9. Add an on-demand **read-only integrity scan** for a selected saved BIN (header, record count and tail).
10. Require a second confirmation before deleting an **interrupted or incomplete** flight log.
11. Show the exact flight name and recovery result for each recovered journal, not only the total count.
12. Make repeated reboot/recovery passes idempotent and regression-test duplicate metadata avoidance.
13. Record worst-case SD flush duration and expose it in diagnostics to identify slow cards.
14. Flag a burst of missed samples in the status page, with a simple “try a lower rate” suggestion.
15. Stop and mark the log incomplete after repeated confirmed SD write failures, while retaining recoverable data.

## P1 — Simpler setup and dashboard (16–30)

16. Consolidate recording state into one prominent **Ready / Recording / Saved / Needs attention** banner.
17. Highlight **one primary next action** at a time in the Launch Assistant, leaving advanced controls secondary.
18. Explain **why Arm is unavailable** beside the button, using the first failed preflight condition.
19. Show when the SD read/write test last passed and whether it needs to be repeated.
20. Display an approximate remaining recording duration based on free space and the selected sample rate.
21. Indicate the age of the most recent valid pressure reading, not only “sensor connected.”
22. Display an explicit battery warning when voltage drops rapidly during a ground test.
23. Offer a compact, printable **ground-test checklist** that fits on one page.
24. Provide a short reconnect hint and retry button when the dashboard loses Wi-Fi.
25. Keep advanced settings collapsed by default on narrow phone screens.
26. Add explicit accessible labels and live status announcements for screen-reader users.
27. Show **Generating report…** feedback while graph images are prepared for printing.
28. Provide a clear **Cancel loading** action for a very large saved-flight analysis.
29. Show the specific download failure message instead of a generic browser error.
30. Add a three-step “SD not ready?” troubleshooting note beside the test control.

## P1 — Trustworthy postflight analysis (31–45)

31. Add a **data quality** line to each report: valid, invalid, missed and recovered samples.
32. Report sample completeness with its denominator explained in a tooltip or note.
33. Distinguish **no battery reading** from a measured zero voltage throughout exports.
34. Test the microsecond timestamp rollover boundary with synthetic flight data.
35. Show an estimated altitude noise range from stationary prelaunch samples when available.
36. Label inferred launch/apogee/landing markers with **confidence caveats** rather than implying exact events.
37. Warn when ground pressure drifts significantly during a long preflight wait.
38. Document whether each displayed graph uses raw altitude or smoothed altitude.
39. Warn before comparing flights that use different sample rates or significantly different data completeness.
40. Add a short CSV **column and units reference** beside the export link.
41. Carry the same phase-estimation caveat into the printed PDF next to its phase table.
42. Include a simple file checksum in exported metadata to detect accidental transfer corruption.
43. Use consistent mission labels and timestamps across BIN companion TXT, CSV downloads and PDFs.
44. Mark implausible derived acceleration spikes as **low confidence** without modifying raw sensor data.
45. Draw breaks in plotted lines where valid samples are separated by missing or invalid data.

## P2 — Maintenance and verification (46–60)

46. Add a one-line health summary for sensor, battery, storage and current recording status.
47. Consolidate repeated preflight readiness checks into a single tested helper in firmware.
48. Keep sample-rate choices and their display labels in one documented configuration location.
49. Test filename validation against path separators, unexpected extensions and overlong names.
50. Test controller-lease expiry and re-acquisition without requiring two physical browsers.
51. Test that SD diagnostics are blocked during recording and never overwrite a flight.
52. Add malformed-journal and missing-companion-file cases to recovery regression tests.
53. Verify synthetic 10, 25 and 50 Hz logs produce correct relative times and expected sample counts.
54. Add a reproducible test fixture for intermittent pressure-sensor failure.
55. Add a reproducible test fixture for truncated or partially flushed SD writes.
56. Attach a concise build manifest (board target, firmware version, commit ID) to each firmware artifact.
57. Keep a short physical ground-test release checklist separate from automated compile success.
58. Document each common error message with one concrete next step.
59. Keep user-facing release notes brief: **what changed, why, how to verify**.
60. Release improvements in small groups (ideally 3–5 related fixes), with a rollback path and no feature bloat.

## Suggested implementation order

**First batch (this PR):** #1–4, plus tests and a quick-start guide. These correct data interpretation and make review safer without changing the user workflow.

**Next batch:** #5, #6, #18, #19 and #30 (clearer SD troubleshooting and preflight readiness).

**Later:** #7–15 and #31–45 after synthetic and physical ground tests. P2 items are maintenance tasks to schedule when they reduce risk; none should become a mandatory extra dashboard screen.

## Acceptance checks for every firmware release

- GitHub Actions firmware compile and unit tests pass.
- Dashboard script syntax and referenced HTML IDs validate.
- Ground test on the actual Feather and a disposable FAT/FAT32 SD card passes.
- Normal stop, interrupted recording recovery, BIN/CSV/TXT download and PDF printing are checked.
- No new ignition, staging or recovery-control code is introduced.
