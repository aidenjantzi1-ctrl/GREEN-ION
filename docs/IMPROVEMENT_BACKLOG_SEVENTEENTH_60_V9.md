# GREEN ION V9 — seventeenth set of 60 prioritized recommendations

Reviewed 2026-10-25 against the ESP32 firmware, embedded dashboard, prior Storage/report changes, and the sixteenth backlog. **60 additional, narrowly scoped recommendations** are recorded here after the previous 960 (cumulative **1,020 documented proposals in seventeen batches**). These are not 1,020 implemented features. Most are intentionally deferred; prioritize real-hardware acceptance over new functionality.

## P0 — Data correctness and file protection

1. Reset the previous flight's cached TXT sample count immediately when a different BIN is selected.
2. Reset the previous flight's missed-sample count before a new TXT request completes.
3. Reset the previous flight's invalid-sample count before a new TXT request completes.
4. Reset the previous flight's stored sampling rate when the selected recording changes.
5. Require all three TXT sample counters before computing recording completeness.
6. Reject negative TXT counters instead of clipping an impossible completeness percentage.
7. Reject fractional TXT counters rather than treating them as physical sample counts.
8. Reject a saved invalid-sample count larger than the saved total sample count.
9. Treat whitespace-only TXT counters as unknown rather than JavaScript zero.
10. Treat a zero denominator for recorded-plus-missed samples as unknown completeness.
11. Preserve the current report when deleting a different recording.
12. Reconcile comparison sources after deletion instead of clearing unrelated comparisons unconditionally.
13. Reject the reserved FLT0000.BIN name consistently in both firmware and dashboard.
14. Make repeated Protect requests idempotent so an existing .LCK marker is not appended to.
15. Prevent a note submission from overlapping Delete or Protect on the same visible row.

## P1 — Low-friction everyday operation

16. Keep the current selected filename visible after deleting an unrelated BIN.
17. Leave the currently displayed altitude graph intact when another flight is removed.
18. Leave the current phase markers intact when another flight is removed.
19. Do not silently navigate to a newly first-listed recording after unrelated deletion.
20. Refresh only the currently viewed TXT report after a successful note on that same recording.
21. Do not replace the viewed report when saving a note on a different recording.
22. Disable the Add note button as soon as its POST starts, not only after the response.
23. Restore Add note after a failed request when the TXT still exists.
24. Block a disabled Delete control before showing its confirmation dialog.
25. Block a disabled Protect control before sending any HTTP request.
26. Keep a row's other mutation controls disabled while a note is pending.
27. Show the selected flight's filename alongside a completeness-unavailable explanation.
28. Keep the original BIN and CSV download buttons available when TXT counters are missing.
29. Retain the user's Storage sort mode after a note operation completes.
30. Use the existing Storage page for these safeguards rather than adding a new settings panel.

## P2 — Explain measurements without overclaiming

31. State that missing TXT counts make recording completeness unknowable from BIN data alone.
32. Separate a valid sensor-row percentage from missed-sample completeness in future reporting.
33. Show an actual zero missed-sample count only when TXT explicitly records zero.
34. Do not silently turn a missing TXT invalid count into zero invalid samples.
35. Retain an unavailable completeness result in printable BIN-only reports.
36. Mark a calculated speed as barometric even when the firmware CSV contains a speed column.
37. Keep the time origin labeled first recorded sample, never ignition time.
38. Distinguish a stationary recording's altitude peak from a detected launch apogee.
39. Show a short count of finite speed estimates before labeling a peak speed reliable.
40. Keep G estimates labeled vertical apparent G, not total rocket loading.
41. Warn when the BIN has fewer parseable rows than the TXT claims were saved.
42. Keep a recorded battery zero distinct from a missing battery reading.
43. Explain why a landing estimate can be absent from an otherwise complete capture.
44. Make a comparison's saved duration explicitly recording duration rather than detected flight time.
45. Preserve source filenames in both flight and comparison printouts even when metadata is absent.

## P3 — Focused regression and release checks

46. Test BIN-only selection after a prior TXT with 100 samples and zero missed samples.
47. Test a saved TXT with all counters present and a known 75% completeness result.
48. Test a TXT invalid-sample count larger than its total sample count.
49. Test a TXT whose missed-sample counter is an empty string.
50. Test a TXT with a negative counter and ensure completeness is not printed.
51. Test a TXT with a fractional sample count and ensure completeness is not printed.
52. Test deleting FLT0002 while FLT0001's report and graph remain selected.
53. Test deleting the currently selected BIN and confirm stale graph state is cleared.
54. Test repeated Add note clicks while the first network request is pending.
55. Test Add note failure and verify mutation buttons are restored.
56. Test a disabled Delete handler never invokes confirm or a mutation endpoint.
57. Test repeated Protect on an existing lock file does not grow its contents.
58. Test that FLT0000.BIN is refused by the firmware name validator.
59. Test a delayed TXT response after switching to BIN-only data; older counts must not return.
60. Run a stationary disposable-card capture, export, PDF print, and power-cycle retention check before merge.

## Implemented in this branch (limited subset)

- Reset saved TXT sample-count caches on every new flight selection. Show unknown completeness when counts are missing, blank, negative, fractional, inconsistent, or zero-denominator.
- Keep the selected report and comparison context when deleting an unrelated recording. Refresh the list to reconcile actual remaining sources.
- Prevent duplicate note submissions and conflicting row mutations while a note is pending; only refresh the viewed report when the note targets it.
- Align firmware filename validation with the dashboard by rejecting FLT0000.BIN; make repeated Protect idempotent without appending to an existing .LCK.
- Add browser behavior and source-contract regression tests for these cases.

## Deliberately deferred

Do not add sensors, pages, cloud accounts, file formats, or flight-control functionality. Avoid expanding the report until a stationary ESP32/BMP388/microSD acceptance run confirms real recordings and exports. Keep this as a **draft stacked on PR #16**, not a change to `main`.

## Release verification

Run dashboard syntax and ID checks, Python/Node tests, and ESP32 Feather V2 compilation. Hardware verification still requires an actual stationary recorder, disposable microSD, BIN/CSV/TXT exports, note/protect/delete checks on disposable files, and phone/desktop PDF printing. Do not interpret automated tests as physical validation.
