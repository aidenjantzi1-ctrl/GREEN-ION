# GREEN ION V18 — Guided Development Test

This document covers the Secondary Coder's **TEST / CHECK EVERYTHING** panel in PR #58. It is a development preview, not a V18 release and not flight qualification.

## Safe local check

Use **Flight → TEST / CHECK EVERYTHING** to open the existing Settings tab, then choose **RUN SAFE LOCAL CHECKS**. The app keeps exactly four top tabs (Flight / Recovery / Data / Settings) and the blue/navy GREEN ION layout.

The safe check is read-only. It evaluates:
- freshness of the local Windows bridge + Feather `/api/status`;
- current firmware-reported microSD, IMU, barometer and battery evidence;
- current calibration baseline/noise evidence;
- V18 service-worker/cache presence;
- a BIN previously imported through the V18 manual importer, reopened unchanged from IndexedDB and parsed/analyzed locally;
- a physically connected Uno with a fresh accepted protocol-v2 packet and current GPS evidence.

PASS applies only to the named evidence. For example, `sd_ok=true` is not a physical SD stress-test PASS.

## Explicit firmware diagnostic

**RUN FIRMWARE IMU + GPS DIAGNOSTIC** is separate from the safe checklist and requires a user click. It uses the existing firmware endpoint `POST /api/hardware-test` with `sensor=all`. Existing status fields `imu_test_result/message` and `gps_test_result/message` supply the result.

The button is disabled during unsafe/active flight states. It does not request PREPARE & ARM, recording, LoRa transmission, simulated flight, or file deletion.

## Always manual / real-hardware checks

These remain NOT TESTED until separately performed:
1. Actual offline reload after cache registration.
2. Physical SD write/read/verify/temp-delete with a bounded temporary file; never delete real flight BINs.
3. Sensor accuracy/noise under real stationary and motion conditions.
4. GPS/LoRa antenna, RF range, packet loss and recovery behavior.
5. Recovery/hibernation timing and physical wake.
6. Two distinct saved motor-free hardware flight/pressure cycles.

Manual BIN import preserves original bytes and uses the existing SHA-256 local history path. Browser/CI PASS never means hardware or flight readiness.
