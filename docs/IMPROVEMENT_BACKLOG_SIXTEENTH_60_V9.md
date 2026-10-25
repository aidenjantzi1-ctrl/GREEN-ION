# GREEN ION V9 — sixteenth set of 60 prioritized recommendations

Reviewed 2026-10-25 against the V9 firmware, embedded dashboard, automated tests, and earlier improvement batches. This set contains **60 additional concrete proposals** after the previous 900 (cumulative **960 documented proposals across sixteen batches**). These are not 960 implemented features. Favor protecting recorded data and keeping the existing interface understandable. P0 is highest priority; P3 items are tests rather than extra product features.

## P0 — Trust boundaries and data preservation

1. Reject an SD file-list payload that is not a JSON object before reading capacity fields.
2. Reject a `files` field that is not an array; show a retryable error instead of rendering partial actions.
3. Reject file-list entries that are null, strings, or arrays rather than recording objects.
4. Accept only `FLT0001.BIN`–`FLT9999.BIN`-shaped names before building download or mutation requests.
5. Treat duplicate BIN names in one listing as an ambiguous response; do not offer Delete until refreshed.
6. Show `Size unavailable` for negative, fractional, nonnumeric, or unsafe-integer file sizes.
7. Sort unknown-size recordings last when sorting by size, without inventing a byte count.
8. Bound every TXT line read during note identity checks to protect ESP32 memory on a damaged card.
9. Reject a note append if a required TXT header or `file=` line exceeds the bounded line length.
10. Retain support for CRLF metadata lines while enforcing the new TXT line bound.
11. Invalidate an in-flight Storage refresh immediately after confirming a Delete action.
12. Disable Delete, Protect, and Add note controls on the affected row while deletion is pending.
13. Disable conflicting row actions while a Protect or Unprotect request is pending.
14. Restore controls after a failed mutation without silently changing the displayed protection state.
15. Keep the original BIN unchanged when a TXT identity check fails or the metadata line is too long.

## P1 — A simpler everyday Storage workflow

16. Show the exact recording filename in a short status line while a file mutation is pending.
17. After a failed Delete, offer one explicit `Refresh Files` recovery instruction instead of reopening dialogs.
18. Show `Unknown size` consistently in the recording list and any confirmation message.
19. Use `Read only` beside file actions when another device currently holds control.
20. Keep file actions in a fixed order on narrow screens even when the filename wraps.
21. Label the Refresh control `Checking…` only while its latest request is pending.
22. Show a one-line `Card not verified` explanation beside file-list failures when the device reports SD trouble.
23. Explain that Protect prevents deletion but does not lock operator notes, unless firmware behavior changes.
24. After changing protection, preserve the same sort mode and selected comparison files.
25. Keep the currently selected report visible when a different file's Protect action fails.
26. Display `0 saved recordings` only after a valid empty listing, never after a network error.
27. Use a short accessible announcement when a Storage refresh changes the number of files.
28. Show a single `Try Refresh Files` recovery action rather than automatic repeated SD queries.
29. Keep long SD errors within the existing card instead of creating horizontal scrolling.
30. Keep the existing Storage page as the only file-management surface; defer folders and batch operations.

## P2 — Understandable calculations and printable reports

31. Wrap long TXT metadata values within PDF table cells so filenames and notes remain readable.
32. Keep report metadata labels on the same page as their values where the print engine permits.
33. Print `Not measured` instead of `0` for an absent sensor value in a saved-flight report.
34. Mark estimated acceleration as derived from pressure-based altitude, not an accelerometer.
35. Show whether the first recorded timestamp precedes detected launch when both are available.
36. Distinguish `recording duration` from `detected flight duration` in the printable summary.
37. Identify the number of usable vertical-speed estimates before displaying a peak-speed figure.
38. Identify the number of usable acceleration estimates before displaying a peak-G figure.
39. Show the sensor-gap rule beside the printed speed and acceleration graphs.
40. Show a concise reason when landing time is unavailable for a stationary ground test.
41. Put the source BIN filename in a small footer on each printed report page where supported.
42. Display missing TXT provenance prominently when a BIN-only flight is printed.
43. Preserve battery-voltage units on comparison graphs and in their text alternatives.
44. Make report graphs fit both US Letter and A4 without clipping axis labels.
45. Add a short explanation that a pressure-derived vertical G estimate is not a structural load rating.

## P3 — Regression tests and physical release checks

46. Test a file-list payload with `files: null` and confirm that no action buttons appear.
47. Test a file-list payload with `files: {}` and confirm the error is not presented as an empty card.
48. Test an array containing a null recording entry and confirm it is rejected before rendering.
49. Test a filename containing `../` and confirm no download or delete action is created.
50. Test two identical valid BIN names and confirm that no ambiguous Delete control is created.
51. Test negative and fractional file sizes and confirm both display as unknown.
52. Test a size-sort list containing a valid zero-byte file and an unknown-size file.
53. Test rapid repeated Delete taps against a delayed response; only one request should be sent.
54. Test Protect and Delete overlap; conflicting row controls should remain disabled until completion.
55. Test that a failed mutation restores row controls and leaves the original report selected.
56. Test a TXT header longer than 160 characters and confirm note append is rejected.
57. Test a `file=` line longer than 160 characters and confirm the BIN and TXT remain intact.
58. Test a valid CRLF TXT with a matching BIN and verify the bounded reader still permits notes.
59. Print a report containing a 120-character operator note on phone and desktop; verify table wrapping.
60. Complete one stationary disposable-card acceptance run with removal/reinsertion, recording, exports, PDF printing, and power-cycle retention.

## Implemented in this branch (a small subset)

- Validate the Storage API response before creating any recording actions; reject malformed/duplicate filenames and show unknown sizes without inventing measurements.
- Guard Delete and Protect buttons against repeated taps while their requests are in flight, and invalidate pending file-list refreshes before mutations.
- Bound firmware TXT metadata line reads before appending notes, while preserving ordinary CRLF summaries.
- Wrap long metadata values in printable report tables.
- Add focused automated regression tests for the above behavior.

## Deliberate deferrals

Do not add more pages, new sensors, batch deletion, new file formats, automatic SD retries, or firmware-driven flight controls in this batch. The next real priority is **stationary hardware acceptance testing and consolidation** of the existing draft PR chain. The above proposals are a menu, not an implementation mandate.

## Verification and release gate

Run dashboard JavaScript validation, Python/Node regression tests, and the ESP32 Feather V2 build in GitHub Actions. Physical ESP32/BMP388/microSD testing and phone/desktop PDF print checks are still required before use. Keep this PR draft and stacked on PR #15; do not modify `main` directly.
