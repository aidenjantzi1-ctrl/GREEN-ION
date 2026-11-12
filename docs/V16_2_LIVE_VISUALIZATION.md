# GREEN ION V16.2 — Dashboard flight visualization

The Dashboard side-view rocket is a lightweight Canvas2D visualization. It is **not** flight control and cannot affect recording.

Label: **Flight Visualization — Sensor-Based / Estimated**.

## What is real versus visual

Measured when valid/received:
- timestamps;
- pressure;
- battery voltage;
- GPS;
- recorded IMU measurements.

Calculated/estimated:
- barometric altitude;
- velocity;
- flight phase;
- relative gyro-integrated orientation.

Illustrative:
- rocket artwork;
- engine flame/exhaust;
- parachute;
- ground-contact drawing.

The flame is a phase/timing effect, not combustion proof. A parachute is shown only as an illustrative recovery-profile effect during descent; it is not proof of deployment.

When telemetry becomes stale, GREEN ION freezes the last observation and labels its age. It does not extrapolate indefinitely.

Saved BIN files can replay through the same visualization, but replay mode is prominently labeled **SAVED FLIGHT REPLAY — NOT LIVE**.
