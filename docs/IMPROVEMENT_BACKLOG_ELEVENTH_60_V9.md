# GREEN ION V9 — eleventh set of 60 prioritized recommendations

Reviewed 2026-10-25 against the current V9 dashboard and firmware, building on draft PR #10. These are **60 additional scoped proposals**, bringing the documented planning backlog to **660 recommendations** across eleven batches. They are not 660 implemented features. Avoid new screens, new sensors, or flight-control hardware. Favor reliability and comprehensible existing controls.

The fixes implemented in this branch are listed separately below and **are not counted among these 60 new proposals**. Most of the new proposals should remain deferred until hardware testing validates the core recorder.

## P0 — Trustworthy comparison and file selection (1–15)

1. Add visible First / Second column headings above comparison values so the slash separator is unambiguous.
2. Show a neutral 'same motor label' note when both selected flights use the same motor; do not imply two motors were tested.
3. Identify a ground-test recording beside its comparison filename rather than treating it as a powered launch.
4. Show which selected recording has a missing TXT companion instead of displaying a generic comparison failure.
5. Keep the selected filenames visible while a comparison request is loading on a slow Wi-Fi connection.
6. Explain in the comparison card that two saved recordings are required before comparing.
7. Repeat both source BIN filenames immediately above the comparison table, not only in a PDF.
8. Keep the selected pair in place after one summary request fails, allowing a simple retry.
9. Invalidate a comparison result if a file-list refresh discovers that one of its source BINs disappeared.
10. Display 'summary unavailable' separately from a legitimately recorded numeric zero in comparison tables.
11. Distinguish an empty but readable SD directory from a card whose directory cannot be opened.
12. On a transient file-list refresh failure, label the last list stale rather than silently implying it is current.
13. Mark a BIN with a non-record-aligned trailing byte count as 'check integrity' in the file list.
14. Mark a BIN without a matching TXT summary before the operator attempts a PDF.
15. Keep the current flight report visible when an unrelated protected file is deleted; clear only affected selections.

## P1 — Explain measurements without inventing certainty (16–35)

16. Report when the saved sample-rate field is missing rather than silently treating 25 Hz as the measured rate.
17. Separate total recording time from launch-to-landing flight time in the saved summary.
18. Keep descent speed's signed value in raw analysis while labeling its positive magnitude in the summary.
19. Use the phrase 'estimated vertical apparent G' consistently wherever the derived G trace appears.
20. Add a report note when the original pressure baseline is invalid and altitude cannot be trusted.
21. Count discontinuous valid-data segments and include the count in optional quality details.
22. Flag a timestamp rewind in the report's data-quality note without inventing missing time.
23. Label the maximum measured relative altitude separately from the estimated post-launch apogee.
24. Use 'motor not specified' for an absent motor field, not an assumed current motor selection.
25. Flag a single extreme altitude sample as a possible outlier rather than presenting it as verified apogee.
26. Show the time of the first valid pressure reading relative to the beginning of the BIN.
27. Estimate effective sample spacing from recorded timestamps and label it as an observation, not a configured setting.
28. Do not use a stationary ground test as a powered-flight performance baseline without an explicit warning.
29. Explain that a negative relative altitude is below the calibrated ground reference, not below sea level.
30. When landing is not detected, say why the descent duration is unavailable instead of leaving an empty row.
31. Put the source BIN filename beside each printed chart caption when reports contain multiple graphs.
32. Label the graph's zero-time origin as the first recorded sample, not ignition.
33. Distinguish the report generation date from any optional saved flight-clock timestamp.
34. Add a small report format/version identifier so printed reports can be matched to firmware analysis changes.
35. Put a two-line data-quality summary ahead of the PDF's derived performance values.

## P2 — Make routine use simpler, not larger (36–50)

36. Keep advanced LED patterns outside the first-use Prepare/Arm/Watch workflow.
37. Preserve the Storage tab's scroll position when a manual file refresh completes.
38. Explain in Help that BIN is the original recording while CSV is generated from it on request.
39. Keep each saved-file action row readable on a narrow phone without horizontal scrolling.
40. Show a plain 'Protected' status next to a protected file rather than relying on button wording alone.
41. After a file protection change, confirm the result in the same row instead of a generic popup.
42. Explain next to Stop & Save that it is for a recording already in progress.
43. Make the simulation-disabled reason visible when preparation or recording blocks simulation.
44. Explain beside Start Launch Watch that actual flight recording begins on automatic launch detection.
45. Label saved ground-test recordings in the file list when their mission-kind metadata is available.
46. Show the current firmware build identifier in the firmware-update confirmation dialog.
47. Keep the diagnostics download optional and explain when it helps troubleshoot an error.
48. Consolidate duplicated success/error notices so the same action does not produce conflicting messages.
49. Give all file-action buttons an obvious keyboard focus outline.
50. Keep the user on the Storage tab after a failed download and show a one-sentence retry instruction.

## P3 — Verification and maintenance (51–60; defer until useful)

51. Exercise comparison tables with empty, whitespace-only, NaN, zero and nonzero metadata values.
52. Check that a missing numeric comparison input never creates a zero-height bar in a printed graph.
53. Exercise the Storage view with total bytes less than used bytes and confirm it does not show zero free space.
54. Test deleting the last recording while an old comparison is still loading.
55. Test deleting one selected comparison file before its asynchronous summary response arrives.
56. Test a two-recording comparison when only one companion TXT file is present.
57. Test a saved flight whose every pressure sample is invalid; derived motion must remain unavailable.
58. Test the display of total/used/free space with a synthetic 8 GiB card and non-round byte counts.
59. Check narrow-tablet layouts with long but valid filenames and the printable comparison heading.
60. Check the blocked-popup path for PDF export and verify it gives a specific browser instruction.

## Actually implemented in this branch (not counted above)

- Reveal the already-existing saved-flight comparison card and label its two recording selectors.
- Disable Compare when fewer than two files are present and clear stale comparison selectors when storage is empty.
- Preserve true zeroes and show missing/blank/NaN metadata as unavailable in comparisons and printable bar graphs.
- Use KiB/MiB/GiB consistently for binary storage byte counts; flag impossible or zero-capacity SD totals as unavailable instead of showing fabricated free space.
- Preserve the widest available file size type when serializing SD file listings.
- Invalidate pending comparison/summary requests after file deletion.
- Add automated tests for these behaviors.

## Priority and acceptance gate

**Now:** Confirm automated dashboard tests and ESP32 compilation; then perform stationary hardware checks with a disposable microSD card. **Next:** Validate a ground recording, zero/blank comparison values, 0/1/2 saved files, SD capacity, BIN/CSV/TXT downloads, and print-to-PDF on a phone and computer. **Later:** Select at most a few high-value P0/P1 proposals from the list above; do not implement the whole backlog indiscriminately. Never merge to `main` or install before hardware verification. This branch is stacked on draft PR #10.
