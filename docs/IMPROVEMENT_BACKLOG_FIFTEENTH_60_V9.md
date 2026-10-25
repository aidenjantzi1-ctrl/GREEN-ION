# GREEN ION V9 — fifteenth set of 60 prioritized recommendations

Reviewed 2026-10-25 against the firmware, embedded dashboard, regression tests, and earlier fourteen backlogs. These are **60 additional focused proposals beyond the previous 840**, bringing the cumulative documented backlog to **900 proposals across fifteen batches**. They are **not 900 implemented features**. Keep the interface small: implement only reproducible reliability and usability improvements, and defer the rest until a ground test demonstrates a need.

Priority key: **P0** protects data and prevents misleading UI; **P1** improves the existing Storage workflow; **P2** improves explanation and provenance; **P3** supplies release tests. No ignition, recovery deployment, new hardware, new accounts, or new dashboard pages.

## P0 — Protect the active Storage view and recorded evidence

1. Tag each Storage refresh with a monotonically increasing request number; ignore an older successful response once a newer refresh starts.
2. Ignore an older refresh error if a later refresh has already displayed valid recordings.
3. After a newer refresh fails, prevent a delayed older success from falsely restoring an apparently healthy SD view.
4. Validate that the file-list response contains an array before clearing the current rows; show a recoverable protocol error otherwise.
5. Reject non-object entries in the returned file list rather than attempting to build action buttons from undefined fields.
6. Detect duplicate BIN names in a malformed response and flag the listing as ambiguous before enabling Delete.
7. Require the browser-side recording name to match FLT plus four digits plus .BIN before constructing download or mutation links.
8. Display an unknown-size warning instead of sorting a file with a negative, fractional, or non-finite byte count as if it were valid.
9. Invalidate a pending file-list refresh as soon as a destructive operation begins, not only after its POST completes.
10. Temporarily disable a row's Delete control while its request is in flight to prevent repeated submissions.
11. Temporarily disable a row's Protect/Unprotect control while its request is in flight so two taps cannot reverse each other unexpectedly.
12. Require the first line of an existing TXT to identify the expected GREEN ION V9 metadata format before appending a note.
13. Require a TXT's saved file= value to match the requested BIN exactly before accepting a note.
14. Limit how much of a malformed TXT header the firmware scans before rejecting a note; never read an unlimited corrupted line into RAM.
15. On a failed note-identity check, report that the BIN and TXT were not modified and direct the operator to download them for inspection.

## P1 — Reduce surprises in the existing Storage screen

16. Show a subtle 'Checking card…' state on the Refresh Files control, without opening a new dialog.
17. Show the last completed Storage refresh time as browser-local observation time, explicitly not the recorder's flight timestamp.
18. Keep the last confirmed file list visible while a manual refresh is pending, but label it 'previous results' until the request completes.
19. Use a short, consistent recovery message when the file-list JSON is malformed: reconnect to GREEN-ION Wi-Fi and retry.
20. Keep the selected flight unchanged when an unrelated row's Add note request succeeds.
21. Refresh the selected flight's TXT statistics after a note save without scrolling away from the Storage actions.
22. Restore keyboard focus to the triggering row control after Storage rebuilds the list, when that row still exists.
23. Announce changed file counts through an accessible status element without reading every file row aloud.
24. Allow long recording warnings to wrap inside their own row rather than pushing the action buttons off a narrow phone.
25. Keep the file action order identical for complete, BIN-only, and protected recordings so controls do not jump.
26. Distinguish 'Storage list could not load' from 'card contains zero flights' using separate plain-language empty states.
27. Show that a view-only browser can download existing files but cannot modify them, without requiring the operator to open System.
28. Keep the active sort label and comparison choices visible during an SD retry, even if comparison actions are temporarily disabled.
29. Use a consistent minimum touch target for Refresh, View data, BIN, CSV, TXT, Protect, Note, and Delete at phone widths.
30. Provide a brief non-modal 'Note saved' acknowledgement tied to the relevant filename only after the firmware confirms the write.
31. Keep the selected report and graphs on screen when a note write fails, so the operator can retry without reselecting a flight.
32. Prevent accidental horizontal scrolling in Storage when a long SD error or filename is displayed at 320 CSS pixels.
33. Use a single 'Download original BIN' label in the help text whenever the summary cannot be trusted.
34. Keep file-size and companion-TXT warnings as copyable text, not only icon colors or hover-only tooltips.
35. Retain the same Storage layout on phone, tablet, and desktop; defer any separate file-manager page.

## P2 — Explain existing exports and measurements more precisely

36. Document the difference between 'record length aligned' and 'RKT4 header verified' wherever file-integrity wording appears.
37. Identify the actual source sample interval used for velocity calculations when sample-rate metadata is absent or inconsistent.
38. Explain in Help that acceleration is calculated from changes in estimated vertical velocity, not measured by an accelerometer.
39. Keep the units for estimated acceleration (m/s²) and apparent vertical G adjacent in the printable report.
40. Report how many valid pairs were available for derived velocity before displaying a peak-speed estimate.
41. Report how many valid triples were available for derived acceleration before displaying a peak-G estimate.
42. Mark a graph's first valid timestamp explicitly as its time origin rather than implying it marks motor ignition.
43. Add a concise 'gaps excluded' count to a printable chart caption only when invalid or missing samples were skipped.
44. Use the same chart ordering in the browser and printed report to make side-by-side review straightforward.
45. Prevent a single unreasonably long metadata value from overflowing the printable report's table width.
46. Keep an incomplete or identity-mismatched TXT out of authoritative report fields until its provenance is checked.
47. Separate operator-entered notes visually from firmware-generated measurements in the existing printable metadata table.
48. Include both source BIN filenames in a motor-comparison print footer so loose pages remain attributable.
49. Explain that an estimated landing marker can be unavailable for a short stationary test even when logging succeeded.
50. Keep optional export annotations separate from the original RKT4 BIN and avoid adding new mandatory file formats.

## P3 — Verification and release work before additional features

51. Test three overlapping file-list responses in success-success-error order; only the last request may control the UI.
52. Test three overlapping file-list responses in error-success-success order; an early failure must not erase the final list.
53. Inject a file-list response with a non-array files field and confirm the dashboard offers a retry rather than crashing.
54. Inject duplicate and malformed recording names and verify no destructive action is offered for ambiguous entries.
55. Use a zero-byte TXT beside a valid disposable BIN and verify that Add note cannot turn it into a plausible summary.
56. Use a TXT with the correct preamble but another BIN's file= field and verify the original file bytes remain unchanged.
57. Use a correctly matched TXT with CRLF line endings to verify the note identity check works across platforms.
58. Exercise a successful note append on an already protected flight to document whether protection covers deletion only or all edits.
59. Run a slow-Wi-Fi browser test with a note submission and simultaneous manual Storage refresh; confirm neither resets the active selection.
60. Complete and record one stationary ESP32/BMP388/microSD acceptance run: card check, capture, stop, power cycle, BIN/CSV/TXT downloads, and phone/desktop PDF printing.

## Implemented in this branch (separate from the 60 proposals)

- Firmware checks the existing TXT preamble and its `file=` identity before allowing a note append. A missing, incomplete, or mismatched summary is left untouched; original BIN is not changed.
- Storage refreshes use a request-generation guard. Out-of-order responses, including errors, cannot replace the newest requested view.
- Saving a note updates the selected flight without automatically scrolling to the report.
- Focused regression tests cover note identity and three Storage response-order cases.

## Deliberate deferrals

Do not implement every suggestion. In particular, hold off on extra file-manager screens, mandatory export formats, aggressive automatic SD retries, or complicated integrity scanning until a physical test exposes a specific failure.

## Release gate

Run JavaScript syntax/HTML checks, Python and Node regression tests, and the ESP32 Feather V2 firmware build in GitHub Actions. This does **not** substitute for physical testing. Before any merge or installation, use a stationary recorder and disposable microSD card; verify normal and interrupted recordings, note identity rejection, storage refreshes, BIN/CSV/TXT downloads, and print-to-PDF on a phone and computer. Keep this PR draft and stacked on #14. Do not change `main` directly.
