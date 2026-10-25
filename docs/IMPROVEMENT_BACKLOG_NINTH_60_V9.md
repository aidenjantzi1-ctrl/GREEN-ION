# GREEN ION V9 — ninth set of 60 prioritized improvements

Reviewed 2026-10-25 against the V9 firmware and the first eight backlogs. These are **60 new proposals**, bringing the documented total to **540 proposals**. They are not 540 installed features.

**Scope:** Flight-data logging and reporting only. Do not add ignition, staging, or recovery actuation. Prioritize reliable recording, understandable controls, and accurate summaries. Items are ordered by priority; lower-priority ideas are deliberately deferred.

## P0 — Card reconnection and idle retesting

1. Remount the microSD filesystem at the start of each explicit idle SD verification instead of trusting an old mounted-card flag.
2. Clear stale card-detected state before the remount so a removed card cannot inherit a previous PASS.
3. Return a plain 'SD not detected' result when remounting fails, rather than attempting the scratch-file write.
4. Ensure an active flight BIN handle is never closed by an SD retest request.
5. Keep the Prepare workflow's built-in SD verification using the same remount path as the standalone button.
6. Test a card removed after a successful check and confirm the next test fails cleanly.
7. Test insertion of a different formatted card after a successful check and confirm the next test checks the new card.
8. Test a temporary SPI/card communication failure followed by a successful retry without restarting the ESP32.
9. Verify that repeated idle SD tests leave all recorded FLT files and metadata byte-for-byte unchanged.
10. Display the remount failure in the existing SD feedback area rather than adding a troubleshooting screen.

## P0 — Consistent flight-phase calculations

11. Compute sustained launch/landing markers before calculating saved-flight durations.
12. Use the same three-valid-sample launch marker in the phase list and the performance summary.
13. Use the same three-valid-sample landing marker in the phase list and the performance summary.
14. Use the post-launch apogee marker for time-to-apogee instead of an unrelated earlier altitude spike.
15. Do not show ascent duration when no sustained launch is detected.
16. Do not show descent duration when no sustained landing is detected.
17. Keep the phase and performance panels synchronized when the selected recording changes.
18. Test that one isolated 5 m altitude spike does not create a flight-duration estimate.
19. Test a real sustained ascent followed by three valid near-ground readings for consistent durations.
20. Clarify in the report that three consecutive barometric samples, not an ignition signal, define detected launch.

## P1 — Saved flight summary consistency

21. Cross-check the selected flight filename against the loaded summary before enabling PDF export.
22. Show the BIN file size alongside the summary's declared sample count for quick sanity checking.
23. Label an absent companion summary separately from a zero-sample BIN.
24. Keep a visibly distinct status when metadata is recovered after a power interruption.
25. Compare the summary's sample rate with the RKT4 header rate and flag disagreements.
26. Show an explicit warning if the BIN's recorded duration disagrees materially with companion metadata.
27. Keep invalid-sample counts separate from missed-sample counts in all summaries.
28. Treat an empty metadata value as unavailable, never as a valid zero measurement.
29. Include the firmware version that recorded the flight when that information exists in its saved metadata.
30. Display the report's data-quality caveat beside the headline metrics, not only in fine print.

## P1 — Export and download reliability

31. Return an explicit CSV export error for a valid header with no complete data records.
32. Report a partial trailing RKT4 record as an integrity warning without altering the original BIN.
33. Test that CSV export preserves exact valid/invalid flags for alternating sensor samples.
34. Test that CSV row count matches the number of complete 16-byte records.
35. Check that repeat downloads of an unchanged BIN have matching sizes and checksums.
36. Show a short 'connection interrupted; retry download' message for incomplete browser transfers.
37. Keep downloaded filenames stable across phone, tablet, and desktop browsers.
38. Confirm TXT and BIN downloads remain possible even when CSV conversion is unavailable.
39. Provide one simple export help paragraph describing BIN, CSV, TXT, and browser-generated PDF.
40. Add a test that exporting a long recording does not exhaust available ESP32 heap.

## P2 — Dashboard and printable report polish

41. Prevent a slow previous flight-summary response from replacing a newly selected flight's statistics.
42. Preserve the last successfully viewed graph until the next graph has actually loaded.
43. Show 'not determined' rather than a fabricated zero for missing acceleration or speed.
44. Keep graph titles and units visible when printing from a narrow tablet viewport.
45. Repeat the recording filename in the PDF header so printed pages remain identifiable.
46. Include a concise report-generated timestamp distinct from the original recording time.
47. Keep phase labels readable at 200% text zoom without horizontal scrolling.
48. Add print-only page-break rules that avoid splitting the small phase table across pages.
49. Verify PDF report graphs remain legible in grayscale printing.
50. Show a simple warning when pop-up blocking prevents the PDF print window from opening.

## P2 — Regression tests, maintenance, and release

51. Add a source-contract test that an explicit SD retest always remounts before scratch-file writes.
52. Add a Node regression test that one altitude spike cannot generate a launch duration.
53. Add a Node regression test that sustained launch and landing produce matching phase and duration values.
54. Keep tests runnable locally with a single documented Python unittest command.
55. Validate dashboard JavaScript syntax on every pull request that changes the embedded page.
56. Compile the exact ESP32 board target in CI rather than only checking source text.
57. Review binary format changes against existing RKT4 recovery fixtures before merging.
58. Record automated build status and hardware test status separately in each pull request.
59. Require a disposable-card ground check before accepting SD remount behavior as hardware verified.
60. Keep this batch limited to card retesting, phase consistency, and tests; defer other proposals unless a failure is reproduced.

## Changes implemented in this branch

- Firmware: each idle SD verification ends any stale mount and reinitializes the card before read/write verification, allowing removed or swapped cards to be detected.
- Dashboard: saved-flight performance metrics use the same sustained launch, apogee, and landing markers shown in the flight-phase panel. A single altitude spike no longer produces a launch duration.
- Report wording: clarify that launch and landing markers require three consecutive valid barometric readings.
- Tests: source-contract checks for remount ordering and Node-based synthetic phase/duration checks.

All other recommendations remain **unimplemented** and should be selected only when they provide a clear benefit.

## Review and ground-test checklist

1. Confirm the GitHub Actions ESP32 build and full Python/Node test suite pass.
2. With a disposable microSD card, run the SD test, remove the card while idle, retest, reinsert or swap cards, and retest again. Confirm no existing FLT files change.
3. Run a stationary Prepare + 10-second ground test with the BMP388 connected. Confirm recording, recovery, and BIN/CSV/TXT/PDF export remain functional.
4. Open synthetic or known flight recordings on a phone, iPad, and desktop; check phase markers and duration consistency.
5. Keep this draft PR stacked on PR #8 and merge only after manual review and physical hardware validation.

Automated builds do **not** establish hardware or flight qualification.
