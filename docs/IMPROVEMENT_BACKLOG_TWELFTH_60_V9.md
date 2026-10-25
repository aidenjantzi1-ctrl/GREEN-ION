# GREEN ION V9 — twelfth set of 60 prioritized recommendations

Reviewed 2026-10-25 against V9 and draft PR #11. These are **60 additional concrete proposals** beyond the prior 660, for **720 total documented proposals across twelve batches**. They are not 720 implemented features. Most are deliberately deferred. No new sensors, ignition controls, pages, or mandatory setup steps are proposed. The small changes in this branch are separate from this numbered backlog.

## P0 — Make stored evidence trustworthy (1–15)

1. Offer an optional **deep-check** command that reads a BIN's first 32 bytes only when the operator requests it; do not slow routine Storage refreshes.
2. Differentiate an unrecognized four-byte BIN magic value from a supported RKT4 file with a damaged payload.
3. Report an unsupported RKT4 format version by number rather than calling it a generic CSV failure.
4. Check the header's declared record width against the expected 16 bytes before offering detailed analysis.
5. Flag a companion TXT that is present but empty; mere file existence is not sufficient for a usable summary.
6. Validate that the TXT's sample count is a nonnegative integer before using it to calculate completeness.
7. Compare the BIN-derived complete-record count with the TXT's reported count and label disagreements as 'check recording'.
8. Show the number of leftover trailing bytes on a malformed BIN only in optional file details, not on every file row.
9. Preserve the original BIN when any metadata or analysis check fails; repair actions must never rewrite raw measurements.
10. Distinguish an SD directory read error encountered mid-scan from reaching the directory's normal end.
11. Return a clear 'partial listing' indicator if memory limits force the firmware to stop enumerating files.
12. Bound firmware JSON response size for a card containing hundreds of recordings without silently claiming all were listed.
13. Return a machine-readable distinction between an unmounted SD filesystem and a temporarily busy recorder, while keeping the user-facing message short.
14. When a recovered BIN has no TXT, surface any available checkpoint recovery status without generating a guessed replacement summary.
15. Require a disposable-card bench test before changing the recovery or deletion paths based on new file warnings.

## P1 — Improve first-use and Storage flow (16–35)

16. Put a short 'Record → Save → Storage → View stats → PDF' path in Help using the existing controls and no new wizard.
17. Explain that a saved BIN is the primary evidence and that the TXT summary may be absent after an interrupted write.
18. State beside the file list that a correct byte count is not proof that sensor readings are accurate.
19. Use a single consistent label for the firmware's RKT4 record-size check across Storage, Help, and diagnostics.
20. Keep a file row's action buttons grouped under its filename at phone widths below 360 CSS pixels.
21. Add an explicit 'last reviewed flight' label when a saved graph remains on screen after a newer recording is created.
22. Rate-limit repeated manual file refresh clicks on a slow card to avoid queuing multiple overlapping directory scans.
23. Show an unobtrusive retry instruction after a 409 'SD busy' file-list response; do not prompt the user to format the card.
24. Do not automatically begin a large CSV download merely because the user opens the Storage tab.
25. Explain that the browser can lose the recorder's local Wi-Fi when the device joins another network; provide a one-step reconnect hint.
26. Use a visible filename in every download error so the operator knows which recording failed.
27. Keep a successful Protect/Unprotect result in the same row until the next refresh, rather than requiring a modal dialog.
28. On narrow screens, allow long flight labels to wrap without pushing the Delete button outside the viewport.
29. Preserve the operator's comparison pair when navigating away from and back to Storage within the same browser session.
30. Give a disabled Compare button a nearby text explanation when only one readable TXT summary exists.
31. Label a BIN-only recording 'raw file available' instead of 'no flight' when its summary is missing.
32. Do not silently auto-switch the reviewed flight if a refresh discovers a newer recording.
33. Make a failed TXT read retryable without reloading the whole dashboard or resetting the selected BIN.
34. Keep file warnings as selectable text so they can be copied into a troubleshooting report.
35. Use the same plain-language 'Check SD card and retry' recovery wording across Storage errors and Help.

## P2 — Explain analysis and printable reports (36–50)

36. Add the RKT4 file format version to an optional PDF provenance line only when it was actually read from the BIN.
37. Print 'file length checked' separately from 'sensor data checked' to avoid implying a full integrity scan.
38. Show a warning on comparison reports when the two saved sample-rate settings differ, without changing the underlying statistics.
39. Show a warning when comparing a short ground test with a long flight because raw sample totals are not normalized.
40. Label the report's recorded battery channel as ESP32 supply voltage, not propulsion-system voltage.
41. Display 'no estimate' when a report's velocity series has only one valid timestamp, rather than showing 0 m/s.
42. Describe a negative barometric altitude as relative to the saved baseline in the PDF footnote.
43. State the count of rows excluded from each derived chart when invalid readings were encountered.
44. Do not connect points on opposite sides of a timestamp rewind even if their altitude values look continuous.
45. Give printed charts a short text description of their metric for readers who cannot distinguish line styles.
46. Keep the original units next to every saved TXT statistic even when the live dashboard uses imperial units.
47. Print both recording filenames in a comparison chart's caption so screenshots retain their source identity.
48. Place the warning about uncertain phase detection before estimated flight duration in the PDF.
49. Allow an optional one-line 'test conditions' note in the existing TXT notes mechanism, rather than introducing a separate report editor.
50. Bench-test the PDF report's readable font size on both US Letter and A4 print settings, without adding a separate report-layout menu.

## P3 — Bench tests and maintenance; defer until core passes (51–60)

51. Use a synthetic BIN with a correct length but wrong RKT4 magic to verify that length-only checks never claim full validity.
52. Use a synthetic BIN with a correct header and one extra trailing byte to verify the warning and preserved download.
53. Use a BIN whose companion TXT is zero bytes to verify a clear warning and no fabricated performance numbers.
54. Test a 200-file SD directory to measure firmware heap use and Storage refresh time before optimizing enumeration.
55. Disconnect the browser halfway through a CSV download and verify the next complete download still matches the original BIN.
56. Test a card removed between a successful file listing and a TXT request; require a recoverable error, not a dashboard crash.
57. Exercise repeated manual refreshes during a long ground recording; none may interrupt the SD write path.
58. Test phone portrait, tablet landscape, and desktop printing with two long filenames and missing measurements.
59. Record exact firmware hash, board target, card capacity, and browser version in a small manual test log for every release candidate.
60. Freeze new dashboard features until one full stationary capture → BIN/TXT/CSV download → PDF print → power-cycle recovery checklist passes on hardware.

## Implemented in this branch (not counted among the 60)

- Firmware lists whether each BIN's byte length fits a 32-byte RKT4 header plus at least one complete 16-byte record; this is **not** a header/content validation.
- Firmware reports whether the companion TXT file exists, without modifying either file.
- Storage labels unusual BIN lengths and missing TXT summaries, while leaving raw BIN download available.
- File-list failures clear stale capacity and comparison state; removed comparison sources invalidate old results.
- Comparison displays both selected BIN filenames; missing TXT errors are explicit.
- Printed flight reports identify their source BIN and the time-axis/estimation caveat.
- Added automated source and Node regression tests for these behaviors.

## Practical release gate

**Now:** Run automated dashboard validation, Python/Node tests and ESP32 compilation. **Before any merge or installation:** Use a stationary ESP32 and a disposable FAT32 microSD card. Check normal ground recording, 0/1/2 files, missing TXT, truncated BIN copy, file-list refresh failure, download of untouched BIN, CSV/TXT exports, and browser print-to-PDF. Do not deliberately corrupt a card containing real flight data. Merge stacked draft PRs in order after independent review. `main` must remain unchanged until then.
