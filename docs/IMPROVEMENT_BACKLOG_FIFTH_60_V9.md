# GREEN ION V9 — fifth set of 60 prioritized recommendations

Reviewed 2026-10-24 against V9 and the previous four 60-item backlogs. These are **60 additional proposals**, not 60 installed features. The highest-value implementation in this branch addresses a separate, reproducible problem: API requests that were rejected as busy or invalid could still modify recording settings, and shutdown/OTA/SD maintenance could overlap active work.

**Keep it manageable:** do not add pages, new hardware, or a complicated setup wizard. Prefer clear errors, reliable recordings, and tiny regression tests. GREEN ION is a **data recorder only**; never add ignition, staging, parachute or recovery controls.

## P0 — Safe commands and state transitions

1. Reject an unknown Flight/Test value in a Prepare request before modifying any mission field.
2. Reject an invalid phone-clock value in Prepare without changing the motor label or logging preferences.
3. Invalidate a previously passed preparation when the user changes the sample rate or recording limit.
4. Block deep sleep during a firmware upload, so the device cannot power down halfway through updating.
5. Block deep sleep during the 10-second ground test, automatic preparation, or launch watch.
6. Prevent the simulation command from changing recorder state while automatic preparation is active.
7. Lock mission-label editing during every preparation stage, not only during calibration.
8. Refuse a separate SD-card test while the ground-test file is open.
9. Return HTTP 409 for busy-state command conflicts, retaining human-readable JSON errors.
10. Give a rejected Prepare request a stable error code in addition to its explanation.
11. Keep all API state transitions in a documented allowed-state table for future review.
12. Record which command last changed the logger's operating mode, without storing controller tokens.
13. Make a repeated Stop request return the previous save result without touching SD again.
14. Reject a second firmware-upload start while an earlier upload is still being received.
15. Snapshot motor label, mission kind, sample rate, and limit together when the BIN header is created.

## P1 — Sensor, time and SD diagnostics

16. Calculate and retain the calibration pressure standard deviation so unstable setup can be diagnosed.
17. Record the elapsed time spent in each of the three automatic preparation stages.
18. Count sensor reads that take longer than the selected sample interval.
19. Count detected 32-bit timestamp wraps while decoding a flight, separately from dropped samples.
20. Compare actual median BIN sample spacing against the requested 10/25/50 Hz setting.
21. Track the longest run of identical pressure readings during a ground test.
22. Report the time from the final valid pressure sample to the last saved record.
23. Separate temperature-only failures from pressure-only failures in future validity metadata.
24. Track the largest interval between successful SD flushes, not just the longest individual flush.
25. Record the first ground-test failure reason separately from later secondary warnings.
26. Record whether calibration completed before or after the last SD-card mount.
27. Detect when an inserted card has a different GREENION.CFG identity from the one cached in RAM.
28. Verify the newly written per-card sequence file can be reopened before treating it as persisted.
29. Keep a backup of the last readable card-sequence configuration during updates.
30. Distinguish card remount failure from a missing card without resetting a previously recorded flight.

## P1 — Clearer review and exports

31. Show the selected recording's first and last valid sample times beside its summary.
32. Show a small warning when the final five percent of a recording has no valid pressure samples.
33. Print the companion TXT completion status directly under the source BIN name in reports.
34. Show the time interval between the last logged sample and the reported save event, if both are known.
35. Mark report statistics that rely on fewer than five valid observations as low confidence.
36. Identify phase markers that fall inside a gap in sensor readings rather than drawing them as exact points.
37. Add a one-tap Copy filename action to the existing Storage row, without another page.
38. Keep the currently selected flight visibly highlighted after the file list refreshes.
39. Offer a compact table showing valid-pressure percentages before and after detected apogee.
40. Explain a negative recording-relative time when a prelaunch buffer is present instead of silently clipping it.
41. Display the approximate number of missing samples in each of the three longest timing gaps.
42. Label report charts with the time span actually represented after filtering invalid data.
43. Show the last successful metadata-read time in the existing Storage details area.
44. Keep the exact source BIN name visible while the user chooses two flights to compare.
45. Add a small field-origin key to PDF tables: sensor, firmware-derived, or browser-estimated.

## P2 — Testability and maintenance (defer until useful)

46. Add a host-side test that an invalid Prepare kind leaves all stored settings unchanged.
47. Add a host-side test that an invalid Prepare epoch leaves the motor label unchanged.
48. Add a host-side test that changing the recording limit invalidates prior preparation readiness.
49. Test that a simulated shutdown command is rejected during a firmware upload.
50. Test that simulation cannot start while a ground-test file is open.
51. Test that a second SD test cannot run during automatic preparation's ground-test stage.
52. Test that malformed sample-rate text such as 25junk is rejected rather than parsed as 25.
53. Test that negative or oversized recording limits cannot wrap into accepted 8-bit values.
54. Test that repeated Stop calls never append duplicate final-status metadata.
55. Test a card replacement with a different saved sequence number before allowing the next flight.
56. Test a reset during GREENION.CFG rewrite and confirm the previous flight files remain readable.
57. Test file listing performance with 1,000 completed recordings and a phone browser.
58. Test a firmware-upload request when a controller lease expires midway through transfer.
59. Test the manual recovery instructions with the phone disconnected and an almost-full SD card.
60. Add a release signoff recording the exact firmware commit, test card, and ground-test result.

## What this branch actually implements

- A shared busy-state guard for mission/settings edits, automatic preparation, simulation, OTA and shutdown.
- Full input validation before a Prepare request changes the motor, rate, limit or phone time.
- Atomic rate/limit validation in Settings, plus preparation invalidation when logging settings change.
- SD retest blocked during a 10-second ground test; the existing dashboard explains why.
- Regression contracts for these state guards. No BIN format changes, new sensors, new dashboard pages or new report calculations.

## Verification and merge order

1. Review and merge V9 PR #4 first (or retarget this stacked PR after #4 is merged).
2. Confirm this branch's GitHub Actions firmware compile and Python/Node tests pass.
3. On a stationary, non-flight bench setup, run Prepare; attempt a second Prepare, SD test, simulation and shutdown while the ground test is active. Confirm all conflicting actions are rejected without interrupting the test.
4. Try an invalid recording limit with a valid sample rate and confirm the rate remains unchanged.
5. Repeat a normal 15–30-second recording and verify BIN/TXT/CSV/PDF exports and an interrupted-recording recovery on a disposable SD card.
6. Do not merge or install until hardware tests pass. Physical tests have **not** been performed by this branch.

**Defer the remaining 60 ideas** until a real user problem or test failure justifies them.
