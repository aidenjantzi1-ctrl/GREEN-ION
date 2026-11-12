# GREEN ION V16.2 — interactive 3D flight viewer

The V16.2 viewer is genuine WebGL with perspective, depth testing and interactive camera controls. Reconstruction still runs from immutable RKT4/GIS1 originals in a Web Worker.

## Evidence rules

- GPS coordinates: measured when valid/current.
- Pressure-derived altitude/velocity/acceleration/phases: estimates.
- IMU acceleration: measured when valid.
- Gyro-integrated orientation: relative estimate only; no magnetometer means drift/unknown absolute heading.
- Flame/parachute effects: illustrative.
- If GPS is unavailable, horizontal x/y remain zero. The viewer becomes an honest pressure-only vertical reconstruction.

## Controls

- drag: orbit;
- Shift-drag/middle drag: pan;
- wheel/pinch: zoom;
- Free / Follow rocket / Ground / Top-down / Side / Reset;
- Play/Pause;
- timeline slider;
- step backward/forward;
- 0.25× / 0.5× / 1× / 2× / 5×;
- jump to launch/apogee/descent/landing;
- click a trajectory point to move replay time.

The 3D timeline and analysis graph replay are synchronized.

Raw GPS and lightly smoothed display tracks are visually distinguishable. Display decimation never changes the original files.

If WebGL is unavailable, use the existing offline analysis graphs; the viewer failure does not affect data or recording.
