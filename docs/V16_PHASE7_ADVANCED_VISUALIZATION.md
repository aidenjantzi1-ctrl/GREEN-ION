# GREEN ION V16 Phase 7 — Advanced visualization and postflight digital twin

## Goal

Add useful 3D postflight visualization **last**, without adding work to the Feather or changing any recorded flight bytes.

This phase is an offline recorded-data reconstruction. It is not a flight simulator, CFD model, guidance system, or flight-control system.

## Data sources

The visualization reads the same immutable originals already stored by GREEN ION:

- **RKT4 BIN** supplies pressure-derived relative altitude and timing.
- **GIS1 SEN** supplies current GPS fixes, measured IMU acceleration and measured gyroscope rates when available.

The originals are parsed locally and never modified.

## 3D trajectory

When current GPS fixes exist, V16 converts latitude/longitude into a local East/North coordinate system around the first confirmed position. Vertical position prefers the pressure-derived relative altitude at the matching time because that is GREEN ION's high-rate relative flight-height source. GPS altitude is only a fallback for visualization when pressure altitude is unavailable.

When GIS1/GPS is missing, V16 still provides a pressure-only vertical replay instead of fabricating a horizontal track.

The reconstruction reports:

- number of trajectory points;
- reconstructed maximum relative altitude;
- horizontal GPS-track distance;
- whether the mode is `gps+pressure` or `pressure-only`;
- source/limitations text.

## Relative orientation

Gyroscope X/Y/Z measurements are integrated into a normalized quaternion for a **relative orientation** replay.

This is intentionally labeled relative:

- there is no magnetometer/absolute heading source;
- gyro integration drifts over time;
- initial attitude is an arbitrary reference;
- the display must not be interpreted as precise absolute rocket attitude.

A generic rocket direction marker is drawn from that relative quaternion. The rocket drawing is not a scale CAD model.

## Interaction

The Data / Flights & Analysis area gets a 3D reconstruction card tied to the existing postflight BIN selection:

- Load selected flight
- Replay slider
- Play / pause replay
- Drag to orbit
- Mouse-wheel zoom
- Reset view

The local axes are labeled East, North and Up.

## Performance

No 3D processing happens on the rocket. The browser:

- decimates GPS fixes for display;
- caps work through the existing bounded RKT4/GIS1 parsers;
- uses a lightweight Canvas renderer rather than a large third-party 3D framework;
- retains full-resolution originals separately for analysis/export.

## Validation boundary

Software tests validate coordinate reconstruction, pressure-only fallback and quaternion normalization using synthetic recorded files. They do not validate real GPS accuracy, physical rocket orientation, aerodynamic motion, sensor mounting alignment or flight dynamics.
