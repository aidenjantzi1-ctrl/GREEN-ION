# V12 motor-free recorder and Wi-Fi disconnect check

This is a **physical hardware procedure**, not a completed test. The cloud build and browser simulation do not prove that the Feather records through a real Wi-Fi disconnect. Perform it with the rocket stationary and the motor and igniter removed. GREEN ION does not control ignition or deployment.

## Before starting

1. Use the V12 Feather firmware and Ground Station from the same development branch. Connect the Windows Ground Station to the Feather Wi-Fi. A LoRa receiver is optional.
2. Insert an SD card with free space and a charged LiPo. Let stationary barometer calibration finish. Run Preflight; resolve any SD, battery, or watcher failure. The bench Start button requires live Feather Wi-Fi, an idle calibrated watcher, ready SD, and at least 3.50 V.
3. Note the laptop time and the next `TESTnnn.BIN` file name. Keep previous `TEST`, `V12D`, and `AIRP` files; the firmware chooses an unused slot without overwriting an orphan companion file.

## Run and record evidence

1. In **Preflight**, select **Start bench recording** and complete both confirmations. Record the returned `TESTnnn.BIN` filename and start time. The Feather records RKT4 v2 data to SD independently of the laptop and automatically stops within two minutes. The test file has separate `bench_test` start and completion metadata.
2. Confirm that the live status reports `bench_recording=true`, the exact file, and increasing written records. Wait several seconds so the file contains samples.
3. Physically disconnect the Windows laptop from Feather Wi-Fi for 10–20 seconds. Note disconnect and reconnect times. The page must label the recorder state **unverified** during the loss; it cannot prove continued recording while disconnected.
4. Reconnect to Feather Wi-Fi. Confirm the file name and current recorder state. If the two-minute limit has not elapsed, select **Stop/Save test** with its confirmations. If it has elapsed, verify the recorder reports complete. Never use the normal flight Stop/Save control for this procedure.
5. Open **Analysis**, choose **Download and verify** for that exact `TESTnnn.BIN`, and retain the original BIN. Check the RKT4 validation result, record count, duration, warnings, and sequence gaps. Export a report if useful. Keep the matching `.SEN` and `.TXT` companions for deeper inspection. A continuous RKT4 sequence spanning the noted disconnect interval is evidence of SD recording independent of Wi-Fi; it does not prove power-loss survival.
6. Save the Preflight JSON report and write down the file name, times, firmware version, battery voltage, RKT4 result, and any warnings. Mark the physical check PASS only after reviewing actual hardware evidence. Mark it FAIL or INCONCLUSIVE if the file is missing, truncated, has unexplained gaps, or did not span the disconnect.

## If a step fails

- **Start disabled:** reconnect to Feather Wi-Fi, check V12 firmware identity, run stationary calibration, and inspect the SD and battery results in Preflight/Diagnostics.
- **Start rejected by Feather:** recheck the current watcher state, SD initialization, battery voltage and airplane-mode state. The laptop rechecks live Feather status immediately before forwarding the command.
- **File does not download:** reconnect to the Feather; use the matching SD card as the original source. Do not delete or reformat it to retry. Analysis imports a copy and preserves raw bytes.
- **RKT4 validation or sequence gaps fail:** retain the BIN and companion files, inspect power and SD wiring, and repeat with a new test file after resolving the cause. Do not treat a browser simulation as a passing replacement.
- **No GPS fix:** move outdoors with a clear sky for a separate GPS check. A GPS fix is not required to test autonomous SD recording.

Physical performance, radio behavior, SD power-loss survival, and launch readiness remain **NOT TESTED** in the cloud environment.
