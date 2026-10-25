# GREEN ION V9 — sixth set of 60 additional recommendations

Reviewed 2026-10-24 against V9, five previous 60-item backlogs, and the stacked PR #5 branch. These are **60 further proposals, not 60 installed features**. The implementation in this branch is deliberately limited to two existing reliability needs: strict numeric rate/limit validation and gap-aware browser motion estimates/graphs. Avoid new pages, new sensors, and unnecessary options.

**Scope:** experimental flight-data recording and analysis only. No ignition, staging, parachute, or recovery-system control. Derived acceleration, apparent vertical G, and phase markers are estimates, not flight-critical measurements.

## P0 — Input validation and controller consistency

1. Reject requests that supply two different values for the same logging-setting key instead of silently choosing one.
2. Skip unnecessary flash preference writes when a saved recording limit is unchanged.
3. Check whether writing a recording limit to ESP32 Preferences succeeded and report a persistence failure.
4. Validate phone-supplied UTC offsets against the supported timezone range before narrowing the integer.
5. Reject phone timestamps containing trailing characters, signs, or nondecimal notation.
6. Reject implausibly distant future phone timestamps instead of stamping misleading metadata.
7. Do not replace the recording's established clock reference during active capture.
8. Make repeated requests to acquire the same controller lease idempotent and avoid resetting the UI.
9. Require an explicit control-holder decision before a second browser changes the recorder's wall clock.
10. Distinguish a missing HTTP parameter from a deliberately blank optional motor label.
11. Prevent a simulation-stop request from altering an unrelated active workflow.
12. Keep LED customization from overriding visible warnings during automatic ground-test preparation.
13. Return a stable machine-readable error category for invalid logging preferences without changing the existing message.
14. Use one firmware helper to validate all four allowed recording limits, reducing disagreement between routes.
15. Document how malformed HTTP form data is rejected before changing mission, motor, or recording preferences.

## P1 — SD storage and recording reliability

16. Track checkpoint-journal write errors separately from raw sample-file write errors.
17. Report a storage-list response as incomplete if its JSON buffer cannot contain every filename.
18. Avoid expensive directory scans while 50 Hz logging is close to its timing deadline.
19. Preserve the existing Storage list on a temporary refresh failure, with a visible stale-data notice.
20. Measure the elapsed time of an SD readback test so unusually slow cards can be recognized.
21. Use a uniquely named temporary file for each SD test to avoid colliding with an existing user file.
22. Never delete a pre-existing temporary test file until its ownership or expected contents are confirmed.
23. Limit SD remount retries within one user action to avoid an unresponsive dashboard.
24. Show storage totals as unavailable rather than zero when the card is not mounted.
25. Explain a write-protected card separately from a full or missing card when the error can be determined.
26. Make a protected-flight delete rejection visually different from a general SD failure.
27. Clear a stale selected comparison if one of its underlying recordings is deleted.
28. Retain full filenames in accessible labels when the narrow phone layout truncates visible names.
29. Add a short warning not to remove the card until the save-complete message appears.
30. Keep completed BIN files out of any temporary-file cleanup routine, even after interrupted tests.

## P1 — Browser analysis, charts and reports

31. Require three consecutive *contiguous* valid altitude samples before inferring a launch phase in the browser.
32. Require three consecutive *contiguous* valid near-ground samples before inferring a landing phase.
33. Ignore an isolated outlier when labeling a single peak speed as the flight's maximum.
34. Report the duration of the longest uninterrupted valid altitude segment as a quality indicator.
35. Count plotted graph discontinuities separately from the raw invalid-sample count.
36. Show the start and end times of each long sensor-data interruption in optional report diagnostics.
37. Preserve short peaks when downsampling long altitude plots by retaining per-bucket extrema.
38. Clip estimated phase-marker labels to the plotted time range instead of drawing them outside the chart.
39. Keep both the first and final measurement of every plotted segment when reducing graph points.
40. Show a plain-language reason when a particular PDF graph has fewer than two usable samples.
41. Use an explicit axis unit label on printed graphs, not only a unit in the graph title.
42. Mark comparisons as inconclusive when the two recordings have disjoint usable time ranges.
43. Do not draw a positive-only comparison bar for a signed statistic without displaying its sign.
44. Keep derived browser-only values visually distinct from measurements stored in the original BIN.
45. Prevent a pending flight-summary request from replacing the metadata for a newer file selection.

## P2 — Regression testing and release discipline

46. Add a regression case with repeated `rate` and `limit` HTTP parameters.
47. Test numeric settings with leading plus signs, spaces, hexadecimal prefixes, and exponent notation.
48. Test setting values just above 255 and 65,535 to catch narrowing and overflow regressions.
49. Inject a failed Preferences write and confirm the UI does not claim durable settings were saved.
50. Test clock synchronization while a controller lease is expiring.
51. Test simultaneous CSV download and an active high-rate ground recording on a disposable card.
52. Test file-list generation with hundreds of mixed-length names and verify no silent truncation.
53. Test the SD read/write check on a deliberately write-protected card or equivalent test fixture.
54. Test temporary SD-test-file collisions without deleting an unrelated file.
55. Test negative-valued comparison metrics and zero-height graph ranges.
56. Test printed graphs with repeated, backward, and very widely separated timestamps.
57. Measure dashboard responsiveness with a full-length 50 Hz recording on an older tablet.
58. Test changing dashboard tabs while an asynchronous report request is still pending.
59. Test report generation after Wi-Fi drops and reconnects, without mixing flight metadata.
60. Maintain a five-minute bench verification checklist with exact expected UI messages for each release.

## Changes actually implemented in this branch

- Firmware Settings and Prepare now reject nondecimal, signed, suffixed, and overflowed rate/limit text **before** converting to narrow integer types. The allowed 10/25/50 Hz and 2/5/10/20 minute choices do not change.
- Browser acceleration and peak-speed estimates use contiguous valid pressure/altitude segments. They no longer calculate slopes across invalid rows, timestamp reversals, or gaps over two seconds.
- Saved-flight on-screen graphs and printed PDF graphs break at invalid samples and large timestamp gaps instead of drawing misleading continuous lines.
- Added focused Python/Node regression tests for input contracts, segmenting, estimates, and graph breaks.

The 60 items above are **future ideas**. Only a small subset should be selected for a later release if ground tests show a real need. Some implemented fixes correspond to items proposed in earlier backlogs; they are not counted as new proposals here.

## Verification and merge order

1. Review stacked draft PR #4, then #5, then this PR; do not merge old V8.x branches into V9 without porting and retesting.
2. Require a passing V9 firmware compile, dashboard syntax check, and Python/Node regression suite in GitHub Actions.
3. On an isolated bench recorder with no ignition/recovery wiring, verify 10/25/50 Hz settings, invalid text rejection, a 15–30 second test recording, and normal BIN/TXT/CSV/PDF exports.
4. Test interrupted-recording recovery and SD retest with a **disposable** FAT/FAT32 card; verify saved files remain readable.
5. Confirm a report with synthetic invalid samples has discontinuous graphs and no false acceleration spikes.
6. Keep this PR draft and `main` unchanged until the user approves review and hardware verification.
