# GREEN ION — 60 further ideas (second review)

These are **60 new proposals beyond** `IMPROVEMENT_BACKLOG_60.md`, reviewed against the V8.7 code and the first reliability PR. They are **not 60 new features to install**. Prioritize reliability and a clear, compact interface. Only implement a few items at a time after tests; most ideas are deferred. GREEN ION is a **data recorder only** and must not control ignition, staging or recovery hardware.

## P0 — Binary format and sensor-data correctness

1. Publish a byte-by-byte RKT4 layout with field types, endianness and offsets.
2. Document every RKT4 validity flag bit and reserve unused bits explicitly.
3. Keep one tiny, known-good BIN fixture with a hand-checked expected decode.
4. Reject a BIN shorter than the required header before attempting analysis.
5. Show an explicit unsupported-format message for unknown RKT header versions.
6. Check that the sample-rate field is plausible before using it in estimates.
7. Detect backwards or repeated timestamps that are not explained by rollover.
8. Exclude out-of-range pressure values from derived altitude, retaining raw records.
9. Keep temperature sensor plausibility checks separate from pressure validity.
10. Label a sensor timeout during recording without fabricating a replacement reading.
11. Test that JSON status never emits bare NaN or Infinity tokens.
12. Define a stable CSV quoting rule for any future text columns.
13. Keep BIN files read-only during statistics, preview and comparison operations.
14. Test sequential FLT filename allocation after gaps in an SD directory.
15. Ground-test startup when the card is nearly full, preserving existing files.

## P1 — Efficient, understandable dashboard

16. Place a short explanation of Test versus Flight beside the mission selector.
17. Clarify that the motor label is descriptive metadata, not a motor-control setting.
18. Put the selected logging frequency beside the Start Recording control.
19. Use a single consistent term for the control lease on every dashboard page.
20. Show the time of the last successful dashboard refresh in a subtle footer.
21. Keep reconnect errors in one persistent notice rather than stacking alerts.
22. Prevent accidental double taps from submitting duplicate start/stop commands.
23. Preserve the chosen graph metric when changing between metric and imperial units.
24. Make keyboard focus order match the visual order of controls.
25. Honor reduced-motion preferences for nonessential transitions.
26. Keep the Stop & Save label legible at 320-pixel screen widths.
27. Give icon-only controls accessible text when icons are introduced later.
28. Make the active tab visibly distinct in high-contrast and sunlight modes.
29. Keep the flight-file sort choice when revisiting Storage in the same session.
30. Offer a copyable diagnostic summary instead of requiring screenshots of errors.

## P1 — Clear, conservative flight analysis

31. Print the source BIN filename on every report for traceability.
32. Distinguish report creation time from the recording's relative flight time.
33. Use one rounding policy for repeated statistics in screen and PDF views.
34. Explain why calibrated relative altitude can legitimately be negative.
35. Identify whether an altitude series was smoothed in each graph legend.
36. Include negative values when choosing acceleration graph axis limits.
37. Limit canvas resolution on memory-constrained mobile browsers.
38. Ignore invalid rows when selecting candidate phase events.
39. Render an explicit 'no valid points' message for all-invalid graphs.
40. Separate recording duration from estimated flight-event duration.
41. Identify the number of valid samples used in each derived maximum.
42. Ask users to confirm similar rocket configuration before interpreting motor comparisons.
43. Keep a one-line 'estimates only' statement next to summary performance numbers.
44. Test PDF graph rendering when a series has exactly one valid sample.
45. Apply unit switching consistently to motor-comparison table labels.

## P2 — Maintainability, documentation and safeguards

46. Pin the ESP32 board package version used by automated builds.
47. Record the Arduino CLI version in CI logs for reproducibility.
48. Publish a SHA-256 digest for the downloadable firmware image.
49. Warn in CI if the compiled image approaches the configured partition limit.
50. Add a source check that disallows ignition or deployment-control APIs.
51. Measure static dashboard asset size to catch accidental growth.
52. Record the minimum free ESP32 heap during an optional ground diagnostic.
53. Move dashboard validation from inline YAML into a small standalone test script.
54. Maintain one board-specific pin-assignment table in the documentation.
55. Document recovery steps if an over-the-air update fails before reboot.
56. Explain the difference between resetting device preferences and formatting SD.
57. Maintain a short list of known limitations and unsupported hardware.
58. Identify measured, calculated and inferred values in a single glossary.
59. Add a deterministic test for booting with the BMP388 disconnected.
60. Link each release entry to its exact passing CI run and review PR.

## Implemented in the companion follow-up PR

This PR implements **three items from the original backlog** rather than indiscriminately adding features from this new list: SD re-mount on an idle retest (original #6), a plain-language reason why arming is unavailable (original #18), and SD troubleshooting beside the test button (original #30). It also adds regression tests. The 60 proposals above are intentionally left as a review queue.

## Next release rule

Wait for automated firmware compilation and physical SD-card ground testing before merging. Prefer one coherent 2–4-item batch per release. Do not add controls or graphs merely to increase feature count.
