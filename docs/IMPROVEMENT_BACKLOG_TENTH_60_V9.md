# GREEN ION V9 — tenth set of 60 prioritized recommendations

These are proposals, **not 60 new features**. The project should remain a simple flight recorder with a clear dashboard. Do not add new screens or hardware for this batch. Earlier backlogs already address SD scratch-file ownership, phase detection and phone-clock validation; avoid duplicating that work.

## P0 — Correctness and race safety (1–15)

1. Preserve full 64-bit SD total capacity in Storage JSON, including cards above 4 GiB.
2. Preserve full 64-bit SD used capacity instead of narrowing it to a 32-bit integer.
3. Flag a filesystem that reports used bytes greater than total bytes as unavailable.
4. Format storage sizes above 1 GiB in readable GB/GiB rather than thousands of MB.
5. Warn before long ground recordings when approximate free capacity is insufficient.
6. Reject a valid RKT4 header with zero complete 16-byte records before streaming CSV.
7. Reject a truncated first sample with an explicit 422 error, not a successful CSV.
8. Leave the original BIN byte-for-byte unchanged after any failed CSV validation.
9. Ignore a delayed summary response after the operator selects a different BIN.
10. Ignore a delayed summary error after a newer selected flight has loaded.
11. Clear the old phase and performance values as soon as a different BIN is selected.
12. Invalidate the old graph download immediately on flight selection changes.
13. Clear a previous comparison snapshot when a new comparison starts loading.
14. Ignore a late comparison result if either selected comparison filename changed.
15. Ignore a late comparison failure after a newer comparison has completed.

## P1 — Storage and CSV usability (16–30)

16. Display the selected BIN size beside its download control before a large transfer.
17. Show a brief stay-connected hint only when a file exceeds a defined size threshold.
18. Use consistent decimal/binary storage units in Storage, diagnostics, and Help.
19. Mark cached free-space values as stale after a failed refresh.
20. Label free-space-based remaining recording duration as approximate.
21. Include RKT4 header and record sizes in the optional diagnostics download.
22. Explain why a single-record BIN cannot provide a velocity estimate.
23. Preserve pressure, temperature and battery exports even if baseline altitude is unavailable.
24. Document that CSV seconds are rounded while the BIN retains microsecond timestamps.
25. Show a retry hint after a Wi-Fi download interruption without marking the BIN corrupt.
26. Reuse one SD directory scan for a single manual Storage refresh.
27. Show the last successful Storage refresh time in the existing status row.
28. Preserve exact filename capitalization in downloads and printed reports.
29. Only say the card is safe to remove after both BIN and companion metadata close.
30. Bench-test concurrent Storage listing and diagnostic downloads for web responsiveness.

## P1 — Flight reports and comparisons (31–45)

31. Show the selected filename beside each loading indicator in the review section.
32. Enable PDF export only when the summary and graph correspond to the same BIN.
33. Place a data-source caption above each printable flight chart.
34. Label printed graph time axes as seconds since first recorded sample.
35. Distinguish raw sensor channels from browser estimates in the printed legend.
36. Use 'not determined' for all ascent metrics when no sustained launch is detected.
37. Explain a stationary ground-test graph without falsely implying a launch occurred.
38. Keep filename, integrity status and headline measurements together on report page one.
39. Prevent printed chart headings from being stranded on a preceding page.
40. Check that landscape printing does not crop graph axes or labels.
41. Place text descriptions of chart peak values next to graphical plots.
42. Do not substitute the current motor label for missing saved-flight motor metadata.
43. Keep report metadata key order deterministic across repeated exports.
44. Clarify that measured battery voltage is recorder supply, not motor thrust power.
45. Place both source BIN filenames alongside motor labels in comparison reports.

## P2 — Regression tests and release discipline (46–60)

46. Simulate two summary responses arriving in reverse order; newest selection must win.
47. Simulate an old summary failing after a new summary succeeds; ignore old error.
48. Simulate comparison pair changes while two metadata requests remain pending.
49. Simulate a stale comparison failure after a newer comparison succeeds.
50. Assert via regression test that SD total and used bytes are never cast to uint32.
51. Assert via regression test that an empty RKT4 BIN cannot yield HTTP 200 CSV.
52. Test Storage formatting at synthetic 2, 4, 8 and 32 GiB capacities.
53. Test a card reporting zero capacity despite a readable root directory.
54. Test CSV with exactly one complete record and no fabricated speed estimate.
55. Test a truncated CSV download followed by an identical successful retry.
56. Test switching recordings rapidly while a PDF print window remains open.
57. Test comparisons where two metadata files use different key ordering.
58. Run a slow-network browser smoke test with rapid saved-flight selections.
59. Log the exact ESP32 board target, Arduino core, and dependency versions in CI.
60. Keep automated compilation, stationary hardware tests, and manual merge approval separate.

## Actually implemented in this branch

- ESP32 firmware: preserve 64-bit SD capacity and used-space reporting; reject CSV when no complete RKT4 record exists.
- Dashboard: discard stale saved-flight and comparison responses/errors, clear old analysis on selection changes, and invalidate old graph requests.
- Tests: add source contracts and Node simulations for out-of-order asynchronous responses.
- All other ideas remain **unimplemented**. Do not assume a recommendation is a tested feature.

## Verification and merge order

Review V9 draft PRs #4 through #9 before this stacked PR. Require passing Arduino compilation, dashboard syntax/ID checks and regression tests. On a stationary ESP32 with a disposable 8 GB+ FAT/FAT32 card, confirm plausible capacity, normal ground recording, safe rejection of empty/partial-first-record CSV, and correct flight selection when switching quickly. Recheck BIN, TXT, CSV and printable PDF exports. Physical hardware verification is still required before merging or installation. Leave `main` unchanged.
