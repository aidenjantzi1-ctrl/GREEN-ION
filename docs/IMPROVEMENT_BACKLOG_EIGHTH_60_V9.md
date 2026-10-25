# GREEN ION V9 — eighth set of 60 prioritized recommendations

Reviewed 2026-10-24 against V9 and the previous seven improvement backlogs. This is a **new list of 60 focused suggestions**, bringing the documented proposal count to 480. The recommendations are not 60 new installed features.

**Design rule:** Keep the device a simple flight-data recorder. No ignition, staging, parachute deployment, or recovery controls. Prefer safe recording, trustworthy exports, and straightforward setup over additional screens.

## P0 — Ground-test file ownership and cleanup

1. Use a fresh per-run filename for the 10-second ground-test scratch recording instead of relying on a fixed path.
2. Never delete a pre-existing GNDTEST.TMP file just to start a new test.
3. Keep scratch filenames compatible with FAT 8.3 naming rules on common microSD cards.
4. Bound the ground-test filename search and fail promptly when no unused slot exists.
5. Explain a ground-test filename collision without suggesting that the user format the card.
6. Store the selected scratch filename for the duration of one ground test, rather than recomputing it during cleanup.
7. Treat failure to remove the ground-test scratch file as a REVIEW result, not PASS.
8. Require SD re-verification after a ground-test cleanup failure.
9. Preserve any scratch file left by an interrupted test for manual inspection rather than deleting it on the next boot.
10. Add a regression contract proving the ground-test routine never calls SD.remove on a fixed, unowned filename.

## P0 — Prepare command input consistency

11. Apply the same strict 13-digit epoch parser to Prepare and the standalone phone-clock route.
12. Apply the same UTC-offset range check to Prepare and standalone clock synchronization.
13. Reject Prepare when only an epoch or only an offset is supplied, instead of assuming the missing field is zero.
14. Reject trailing characters in Prepare timestamps before changing mission metadata.
15. Reject signed, scientific-notation, and whitespace-padded epoch text rather than silently coercing it.
16. Check malformed UTC offsets before changing the motor label, rate, or recording limit.
17. Keep previously saved mission settings unchanged when a Prepare clock field is invalid.
18. Test the earliest and latest supported epoch values in both clock-setting routes.
19. Test the accepted UTC-offset boundaries in both routes, including negative offsets.
20. Document that clock validation is about report timestamps, not the flight-triggering mechanism.

## P1 — Recording integrity and exports

21. Expose the number of records decoded from a selected BIN alongside the number declared in its companion summary.
22. Show a short warning if a saved BIN's final record is truncated even when earlier samples remain readable.
23. Distinguish a missing TXT companion from an unreadable BIN in Storage's error message.
24. Test exporting a recording whose pressure is valid but temperature is unavailable.
25. Test exporting a recording whose pressure is unavailable but battery voltage remains present.
26. Keep original microsecond timestamps available in the binary even when CSV rounds seconds for display.
27. Test that a zero-length BIN produces an explicit error instead of an empty-looking successful report.
28. Test that a valid header with no sample records produces a useful 'no samples' summary.
29. Test a download interrupted after the HTTP response begins without changing the saved BIN.
30. Ensure repeated CSV exports produce identical rows for the same unchanged BIN.

## P1 — Honest flight summaries

31. Separate 'recording finished' from 'all companion metadata written' in the report status wording.
32. Display a summary warning when the sample count is too low to support a meaningful speed estimate.
33. Label the displayed maximum acceleration as an estimate derived from altitude, not an accelerometer measurement.
34. Show 'not calculated' rather than zero when a derived metric has no valid samples.
35. Use the same numeric precision for battery minimum in Storage and the printed report.
36. Keep the original motor label in the report even if the dashboard's current mission label has changed.
37. Distinguish report generation time from the recorded flight start time in the printable header.
38. Include a short note that pressure-based altitude can drift with weather during a long recording.
39. Test that an all-invalid recording still generates a basic integrity summary without invented flight phases.
40. Include a concise report footnote explaining that phase estimates are not evidence of parachute deployment.

## P2 — Simple, low-friction dashboard

41. Keep the existing Ground Test button and place any scratch-file error in its existing result area.
42. Keep successful ground-test feedback to one short sentence; reserve technical details for the diagnostics text.
43. Make the latest SD re-test requirement visible beside the existing test control after a cleanup failure.
44. Use one consistent 'Needs review' label for incomplete or failed ground-test results.
45. Do not add a separate screen for temporary-file management; explain manual cleanup in Help.
46. Retain the user's selected Storage sort order after a failed file refresh.
47. Disable only conflicting actions while a ground test runs, leaving read-only status available.
48. Keep the currently selected flight summary visible while a different file is still loading.
49. Provide a short, copyable error message when the recorder's Wi-Fi connection drops during a report load.
50. Check that the primary controls remain usable at 200% browser text zoom without horizontal scrolling.

## P2 — Test coverage and maintenance

51. Add a source-level test for the bounded ground-test filename allocation loop.
52. Add a source-level test that cleanup failure invalidates SD verification.
53. Add a source-level test that Prepare uses both shared clock parsers before mutating state.
54. Add a source-level test that Prepare rejects incomplete epoch/offset pairs.
55. Exercise a card containing GNDTEST.TMP and GND0000.TMP during a disposable-card bench test.
56. Exercise all 16 occupied ground-test scratch slots and confirm that existing files are unchanged.
57. Power-cycle during a ground test, then verify that the next run uses a different scratch filename.
58. Simulate a failed scratch-file deletion and confirm the user is told to retest SD.
59. Verify normal 10-second ground-test results at 10, 25, and 50 Hz on the physical board.
60. Keep the eighth release checklist short: automated build, disposable-card test, CSV/PDF check, and explicit manual approval.

## Changes actually implemented in this eighth branch

Only a small, reviewable subset is implemented:
- Ground tests allocate an unused `GND0000.TMP`-style filename instead of deleting `GNDTEST.TMP`. A full set of occupied names produces an error without overwriting user data.
- A failed ground-test scratch-file cleanup forces a REVIEW result and clears SD verification, prompting a new card test.
- The Prepare route validates phone epoch and UTC offset with the same strict helpers used by `/api/time`. Missing or malformed fields are rejected before modifying recorder settings.
- New source-contract regression tests check these safeguards.

All other numbered items remain proposals, to be selected only when a real issue or a clear benefit justifies them.

## Verification before merge or installation

1. Require a successful GitHub Actions ESP32 firmware compile and Python/Node regression suite.
2. On a disposable FAT/FAT32 card, create `GNDTEST.TMP` and `GND0000.TMP` containing recognizable data. Run the 10-second test and confirm both files remain unchanged.
3. Test a full set of 16 occupied `GND0000.TMP` through `GND0015.TMP` slots; expect a clear blocked result, not deletion.
4. Submit invalid Prepare epoch/offset combinations and verify the mission, motor, rate, and limit remain unchanged.
5. Confirm normal BIN, TXT, CSV and PDF exports and interrupted-recording recovery on actual hardware.
6. Review stacked draft PRs in order: #4 → #5 → #6 → #7 → this PR. Do not merge to `main` before physical testing.

**Automated compilation is not physical flight qualification.** No physical ESP32, sensor, or SD-card testing is claimed.
