# GREEN ION — third set of 60 prioritized improvements

Reviewed against **V8.8 on main**, and compared with the two earlier 60-item backlogs in draft PRs #1 and #2 on 2026-10-24. These are **60 additional proposals**, not 60 installed features. They focus on security, clear setup, safe logging, trustworthy reports and test coverage. Keep only a few related changes per release.

**Scope:** GREEN ION records flight data. It does **not** control ignition, staging, parachutes or recovery. Estimated velocity, acceleration, apparent G and flight phases are analysis aids, not flight-critical measurements.

**Already changed in this PR (carried forward to current V8.8, not counted among the 60 new proposals):** preserve missing CSV values, handle zero first timestamp, avoid derivatives across invalid data, retest SD mounts, explain Arm blockers and enable PR CI.

## P0 — Device security and recording safety (1–15)

1. Offer a first-use checklist for changing the factory Wi-Fi password before field use, with a documented recovery method.
2. Show a nonblocking warning when the device still uses its published default Wi-Fi password.
3. Reject firmware uploads with an invalid binary header before starting an OTA update.
4. Check OTA image size against the available update partition and explain insufficient-space errors.
5. Test that an attempted OTA upload is rejected whenever a ground test or recording is active.
6. Show the current firmware version and incoming firmware version side by side before update confirmation.
7. Never include the controller lease token in downloadable diagnostics or error reports.
8. Never include the Wi-Fi password in downloaded diagnostics or metadata.
9. Expire controller access cleanly after a restart and show that another browser can take control.
10. Add a simple recovery procedure for an interrupted firmware update without implying automatic rollback exists.
11. Require a fresh recent BMP388 sample, not merely a past successful reading, before arming.
12. Treat implausible one-off battery ADC spikes as uncertain readings rather than immediately changing readiness.
13. Keep SD-fault status visible until a fresh successful read/write retest explicitly clears it.
14. Show an unambiguous error when no further FLT filename can be allocated without overwriting existing files.
15. Ensure a failed metadata write cannot be reported as a fully completed recording.

## P1 — Simpler setup and everyday dashboard (16–30)

16. Add a first-connection card explaining the two separate steps: join GREEN-ION Wi-Fi, then open the local dashboard.
17. Display Wi-Fi link state and control-lease ownership as two separate plain-language indicators.
18. Keep Stop & Save easy to reach on a long phone page without adding a second competing primary action.
19. After Stop & Save succeeds, show the exact saved BIN filename in the confirmation message.
20. Use a compact status timeline showing only the latest calibration, SD test and recording result.
21. Show estimated SD storage consumed per minute for the currently selected 10, 25 or 50 Hz rate.
22. Explain in one sentence how higher sample rates trade storage and battery use for finer time resolution.
23. Show how many seconds remain on the current controller lease, without requiring a new page.
24. Explain clearly when another connected browser has control and when a new lease can be requested.
25. Do not overwrite mission-label text while someone is typing and background status updates arrive.
26. Keep an unsaved mission-label draft locally when navigating between dashboard tabs.
27. Make the report action unavailable until a saved flight has been selected and loaded successfully.
28. Prevent pressing Enter in a settings field from accidentally invoking Start Recording.
29. Use one consistent term for 'test recording' throughout controls, files and report headings.
30. Announce a status change to screen readers only when its meaning changes, not on every polling cycle.

## P1 — More trustworthy summaries and exports (31–45)

31. Count CSV rows rejected for malformed validity flags and disclose the count beside loaded-sample totals.
32. Keep invalid sensor records in raw CSV exports even when analysis plots omit them.
33. Show 'baseline unavailable' explicitly in reports when calibrated ground pressure is not valid.
34. Separate sensor-validity quality from battery-measurement availability in the report summary.
35. List each detected frozen-pressure interval and its approximate duration when data permits.
36. Include the highest SD flush duration alongside the sample interval to explain possible gaps.
37. Show the time of the minimum battery voltage, not just the minimum value, when timestamps permit.
38. State whether a saved flight was protected from deletion when generating its report.
39. Put recovery status and recovered-sample count next to that flight's export buttons.
40. Allow a useful basic report when phase-event estimation is unavailable, instead of blocking all output.
41. Replace numerical acceleration estimates with 'insufficient valid samples' when a reliable derivative cannot be computed.
42. Show when maximum and minimum temperature occurred relative to the recording start.
43. Include the original sample-rate choice and recording limit in the PDF's recording-details section.
44. Identify the exact firmware build that produced the recording separately from the browser's report-generation version.
45. Clearly distinguish a checksum of the original BIN bytes from any checksum of a converted export.

## P2 — Focused tests, documentation and maintainability (46–60)

46. Test controller-token redaction against every downloadable diagnostic field.
47. Test OTA rejection with an invalid binary signature before any write is attempted.
48. Test OTA rejection when an active ground test is running.
49. Test repeated OTA upload requests without leaving update state stuck.
50. Test SD exhaustion while appending a checkpoint journal, preserving the last intact BIN.
51. Test power loss while appending companion TXT metadata and verify recovery remains readable.
52. Test file allocation when FLT9999 exists and no new filename is available.
53. Test status rendering when the battery ADC reports 0 mV, 2499 mV and 4401 mV.
54. Test a BMP388 that reports a long unchanged pressure series but still returns valid I2C responses.
55. Test two browser clients contending for control just before and after lease expiry.
56. Check that live polling does not replace a user's partially typed mission label.
57. Add an automated check that report printing still works when phase detection returns no events.
58. Track dashboard header size in CI and warn about unusually large growth.
59. Document the OTA partition layout used by the selected Feather ESP32 V2 board package.
60. Maintain a reversible release procedure: save BIN/TXT files, record firmware version, and document safe reinstallation.

## Practical implementation order

1. **This review branch:** the small V8.8 correctness and preflight fixes above, with tests; no new hardware, pages or complex setup.
2. **After hardware verification:** security/privacy items #1–4 and #7–8, only after confirming the board's OTA recovery and password-reset paths.
3. **Then:** selected usability items #16–19 and #21–23, preferably reusing existing status text rather than adding more cards.
4. **Later:** analysis improvements #31–45, only when sample fixtures and reports can demonstrate their accuracy.
5. **Defer:** the remaining ideas unless they solve a real observed issue. Avoid extra menus and advanced configuration.

## Verification before any merge or installation

- GitHub Actions compiles V8.8 and runs dashboard plus Python regression tests.
- On a disposable FAT/FAT32 SD card, retest after power-off removal/reinsertion and run a 15–30-second ground recording.
- Verify BIN/TXT/CSV exports, invalid-sample CSV rows, and PDF print on phone, iPad and desktop.
- Test interrupted-recording recovery without overwriting existing data.
- Keep both prior V8.7 draft PRs separate; do not merge them blindly into the V8.8 branch.
- No firmware build is flight-qualified solely because CI passes. Do not connect GREEN ION to ignition or deployment hardware.
