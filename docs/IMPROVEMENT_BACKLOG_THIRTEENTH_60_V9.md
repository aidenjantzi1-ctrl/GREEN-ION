# GREEN ION V9 — thirteenth set of 60 prioritized recommendations

Reviewed 2026-10-25 against the V9 source and draft PR #12. These are 60 additional concrete proposals beyond the previous 720, for 780 documented recommendations across thirteen batches. They are **not** 780 implemented features. The recommendations are intentionally scoped to protecting evidence, understandable recovery, honest reporting, and repeatable tests. Most should remain deferred unless a real problem is observed. The small changes implemented in this branch are listed separately below and are not counted among these 60.

## P0 — Preserve raw evidence and make failures recoverable

1. Reject Add note when the TXT companion is missing instead of letting append mode create a misleading partial summary.
2. Check whether a TXT note append actually wrote the complete expected line before reporting success.
3. Keep the original TXT readable if power is lost midway through adding an optional user note.
4. Treat duplicate numeric keys in a TXT summary as ambiguous and show a warning instead of silently using the last value.
5. Set a reasonable maximum TXT summary download size for browser analysis; retain a separate raw TXT download path.
6. Distinguish a zero-byte TXT from a TXT containing only whitespace before calling it readable metadata.
7. Report an orphan TXT in optional diagnostics when its corresponding BIN is absent, without deleting it automatically.
8. Record whether BIN removal succeeded separately from TXT cleanup when an SD deletion returns a partial failure.
9. After an SD write error, require a fresh successful card test before allowing the next recording, without erasing old files.
10. Check the return value of the final metadata flush so a saved BIN is not labeled as having a complete summary prematurely.
11. Ensure that failed note writes do not change a protected file's protection marker or existing recording data.
12. Detect a readable BIN whose header baseline is NaN and allow raw pressure/temperature viewing without a false altitude plot.
13. Reject a BIN with a correct RKT4 magic but an impossible sample-rate header before starting a browser graph.
14. Make an interrupted TXT summary explicitly identifiable as incomplete rather than quietly accepting its final partial line.
15. Never automatically rename or repair BIN files during browsing; preserve exact source filenames for forensic recovery.

## P1 — Simplify recovery and operation

16. Show a short 'BIN readable, TXT missing' notice in the existing summary panel without opening another page.
17. Use 'View data' instead of 'View stats' on a row known to lack its TXT companion.
18. Disable Add note for BIN-only recordings until the user understands it would create new metadata.
19. Offer a one-sentence recovery hint when the CSV endpoint rejects a BIN-only recording.
20. Keep the selected BIN filename in the graph loading indicator after a missing-summary fallback.
21. Use the same plain-language error for a missing TXT in Storage and the printable report.
22. After an unsuccessful Delete, leave the file row in place until a successful refresh confirms its state.
23. After a partially successful Delete, refresh Storage before suggesting a second attempt.
24. Show 'BIN deleted, TXT cleanup failed' as a distinct warning, not a generic request failure.
25. Prevent repeated Delete taps while the first deletion request is still pending.
26. Require the operator to re-confirm a Delete if the file's protected status changed since the list loaded.
27. Display a lightweight 'Reading original recording' progress message before parsing a long CSV.
28. Keep the PDF button disabled while raw BIN-to-CSV conversion is still loading.
29. Show a specific retry instruction when a BIN-only graph fails because Wi-Fi disconnected.
30. Preserve the user's chosen graph measurement when switching between recordings that have no TXT.
31. Keep the Storage screen useful when every saved flight lacks a TXT companion.
32. Do not automatically select the first flight when a user has explicitly cleared their selection.
33. Label a successful raw-data fallback as a partial report, not a complete recovered flight summary.
34. Make the graph canvas keyboard-accessible through its nearby measurement selector and text status.
35. Add a short help example distinguishing a failed summary read from an unreadable original BIN.

## P2 — Keep calculations and PDF output honest

36. On a BIN-only PDF, identify exactly which fields were derived from CSV rather than copied from a saved TXT.
37. Keep the motor name blank in a BIN-only report instead of borrowing the current dashboard motor selection.
38. Label the finish reason 'unknown' when the TXT is missing, not 'landed' based on a barometric estimate.
39. Do not infer the configured flight type from filename numbering or file creation time.
40. Keep an explicit raw-sample count beside a BIN-only chart, separate from any estimated valid-sample count.
41. Distinguish pressure validity flags from physically plausible pressure values in the PDF data-quality note.
42. Show an unavailable launch time when no sustained launch phase is found, even if a peak altitude exists.
43. Keep the earliest and latest timestamp displayed in the report even when middle samples are invalid.
44. Use a visible 'derived after download' caption for calculated acceleration and apparent G on BIN-only reports.
45. Do not generate a motor comparison from a BIN-only recording until its required saved comparison metadata exists.
46. Print the raw file's name on each report page when the browser prints multiple pages.
47. Explain that a missing TXT prevents confirming the saved stop reason and any operator notes.
48. Use the same rounding for on-screen and printed BIN-only performance numbers to avoid apparent disagreement.
49. Make a PDF created from BIN-only data distinguishable from one created with a complete saved summary.
50. Keep the report printable if one optional graph has no usable data; show a text explanation in its place.

## P3 — Test and document before adding features

51. Simulate a missing TXT with a valid BIN and verify View data still loads graphs and estimates.
52. Simulate a 409 SD-busy summary response and verify it does not trigger a misleading BIN-only fallback.
53. Simulate a stale missing-TXT response arriving after a new flight selection and verify the newer flight wins.
54. Simulate failed BIN deletion with an existing TXT and verify the TXT remains byte-for-byte unchanged.
55. Simulate successful BIN deletion followed by TXT removal failure and verify the UI reports partial cleanup.
56. Test the BIN-only printable report with zero, one, and hundreds of valid sensor records.
57. Test a summary containing repeated keys, very long lines, and incomplete trailing data on a disposable card.
58. Test a card that becomes unavailable between BIN selection and CSV loading; show a retryable error.
59. Verify the entire Storage → BIN-only graph → PDF workflow on phone, iPad, and desktop browsers.
60. Do not add more controls until the existing stationary recording, interruption recovery, and export checklist passes on real hardware.

## Implemented in this branch (not part of the 60 proposals)

- The dashboard now attempts to load and graph original BIN measurements if the TXT companion returns HTTP 404. Busy or other errors do not trigger this fallback, and stale responses cannot replace a newer flight selection.
- A printable report based on BIN-only data explicitly states that saved metadata, motor, and finish details are unknown; estimates are identified as derived from BIN-to-CSV conversion.
- Firmware now checks BIN existence and deletion success before touching its TXT companion. If TXT cleanup fails after BIN deletion, it reports the partial failure.
- Added source and Node-based regression checks for missing TXT, busy SD, stale responses, BIN-only PDF wording, and failed deletion ordering.

## Verification and release gate

Run `python3 -m unittest discover -s tests -p 'test_*.py' -v`, dashboard syntax validation, and ESP32 firmware compilation in CI. Before installing or merging, use a stationary ESP32 with a BMP388 and disposable FAT32 microSD card. Check a complete BIN/TXT recording, a copy of a BIN with its TXT removed, a deliberately invalid BIN copy, a failed/blocked deletion, downloads, and PDF printing. Never perform destructive tests on the only copy of real flight data. This draft PR is stacked on PR #12; review/merge in dependency order and leave `main` unchanged until verification.
