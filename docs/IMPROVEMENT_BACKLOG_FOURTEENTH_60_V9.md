# GREEN ION V9 — fourteenth set of 60 prioritized recommendations

Reviewed 2026-10-25 against the V9 firmware, embedded dashboard, regression suite, and earlier thirteen backlogs. These are **60 additional proposals beyond the prior 780**, for **840 documented proposals across fourteen batches**, not 840 implemented features. The goal is reliable notes, straightforward Storage actions, and release readiness. Prefer fixes for observed failures; defer everything else.

## Prioritization rule
**P0** = prevent data loss or misleading success; **P1** = simplify the existing interface; **P2** = improve report provenance and maintenance without new controls; **P3** = targeted validation. Do not implement the entire backlog automatically.

## P0 — Data preservation and failure truthfulness

1. Return a distinct HTTP conflict status when a note targets a BIN whose TXT companion vanished between listing and submission.
2. Reject appending to a zero-byte TXT unless it contains the expected GREEN ION summary preamble; do not silently treat it as a complete flight summary.
3. Reject appending to a TXT that lacks a recognizable recording filename field, leaving the BIN untouched.
4. Preserve a clear error if a note append opens successfully but reports fewer bytes written than requested.
5. Keep an SD write-error latch visible after an incomplete note append until the operator performs a fresh card test.
6. Detect when a note append begins after an SD card swap and require reloading the file list first.
7. Do not append a note while a firmware update is being received, even if flight recording is idle.
8. Check for insufficient free space before starting a note append and report a storage-specific error.
9. Bound the total note text stored in a single TXT so repeated notes cannot consume the card unexpectedly.
10. Ensure a failed note append never changes the associated BIN file's modification state or bytes.
11. Define whether protected recordings allow non-destructive note additions and document that choice consistently.
12. Do not turn a recovered-interrupted summary into a 'complete' summary merely by adding a user note.
13. Validate the full generated note line length in bytes, not only the visible character count.
14. Normalize embedded carriage returns and tabs in note input so each note remains a single parseable TXT line.
15. Prevent duplicate note submission if the browser retries a timed-out POST without knowing whether the first write succeeded.

## P1 — One-screen usability and accessible feedback

16. Display a brief in-row 'note saved' result without moving the operator away from the selected recording.
17. Explain beside a disabled TXT button that the original BIN remains downloadable.
18. Keep the View data label for BIN-only files after sorting by size or filename.
19. Show a short action hint when a recording has a valid BIN but no saved TXT statistics.
20. Keep the Add note button visually grouped with TXT rather than with destructive Delete.
21. Use distinct action names in screen-reader labels, including the exact BIN filename.
22. Provide a keyboard-visible focus indicator on every enabled Storage action.
23. Give disabled note actions an accessible explanation rather than relying solely on a hover tooltip.
24. Keep buttons readable when the device's browser text size is increased to 200 percent.
25. Ensure a note prompt is never opened for a BIN-only row, including from keyboard activation.
26. After a failed note save, leave the Storage list and current graph intact.
27. After a successful note save, update only the selected flight's statistics instead of reloading unrelated graphs.
28. Show a specific retry instruction if the note failed because the card was removed while idle.
29. Preserve the currently selected Storage sort order after note creation.
30. Show the latest note's saved text in a read-only row only when the TXT readback confirms it.
31. Do not show the 'TXT missing' warning as a fatal flight-data error when BIN and CSV remain usable.
32. Give the Storage refresh button a brief loading state while checking companion-file presence.
33. Distinguish an intentionally disabled Add note action from a temporary SD-busy action in accessible status text.
34. Keep the file row's filename visible next to the note action at 320-pixel phone widths.
35. Document the two-step BIN-only workflow: download BIN, then view CSV-derived data without editing the original.

## P2 — Export provenance and maintainability

36. Include a concise 'operator notes are stored in TXT' explanation in the existing export help, without adding a new export format.
37. Retain literal equals signs inside user notes when parsing TXT key-value lines for reports.
38. Keep multiple user_note lines in their original order instead of silently replacing earlier notes.
39. Show an explicit count of saved operator notes only when the TXT was successfully read.
40. Do not infer note timestamps from browser time unless a timestamp was actually saved with the note.
41. Escape user note content when inserting it into the printable HTML report.
42. Allow a printable report to omit optional notes without changing the measured performance table.
43. Keep raw BIN, generated CSV, TXT summary, and browser-derived PDF clearly distinguished in download help.
44. Display note-save failures as storage errors rather than as sensor-quality failures.
45. Use a single firmware helper for TXT companion existence checks if future routes repeat this logic.
46. Add a small source comment explaining why FILE_APPEND is unsafe without an existence check.
47. Keep a one-page recovery procedure for a valid BIN paired with an unreadable TXT.
48. Document that the note-write byte-count check does not prove persistence after sudden power loss.
49. Keep the notes mechanism optional; avoid building a new note editor or cloud account system.
50. Before adding new report statistics, prioritize reconciling BIN sample counts with existing TXT totals on a disposable test copy.

## P3 — Specific regression and release checks

51. Test a valid BIN with no TXT and confirm Add note is disabled while BIN and CSV actions remain enabled.
52. Call the note endpoint directly for a missing TXT and confirm no new TXT is created.
53. Inject a short FILE_APPEND write in a firmware test double and confirm the endpoint reports failure.
54. Test a complete BIN/TXT pair and verify a successful note remains readable after reopening the TXT.
55. Test a note containing an equals sign and confirm it remains a single parseable user_note line.
56. Test a note containing newline, carriage return, and tab input to ensure the summary is not split into fake metadata keys.
57. Test rapid double submission of the same note on a slow Wi-Fi connection using a disposable TXT copy.
58. Test a card swap between Storage listing and note submission; ensure the original flight data is not modified.
59. Check BIN-only actions and disabled-button explanations with keyboard and screen reader on one desktop browser.
60. Before releasing, confirm all earlier automated tests, firmware compilation, and stationary SD/BMP388 recording and exports pass; record hardware checks separately.

## Implemented in this branch (not counted among the 60 proposals)

- Firmware refuses to create a new TXT when Add note targets a BIN-only recording.
- Firmware checks the number of bytes accepted for a note append and reports incomplete writes as failures requiring SD retesting.
- Storage labels BIN-only rows **View data**, and disables TXT download and Add note when their companion TXT is missing.
- Adds focused firmware source checks and browser regression tests for these behaviors.

## Release gate

Run the existing Python/Node suite, embedded JavaScript/HTML validation, and ESP32 Feather V2 firmware compile in GitHub Actions. **No physical testing is claimed.** Before installation or merging, use a stationary recorder and disposable microSD card to check normal recording, note success, missing-TXT behavior, short-write handling where practical, BIN/CSV/TXT exports, and PDF printing on a phone and computer. Keep the PR draft and stacked on #13 until reviewed; do not modify `main` directly.
