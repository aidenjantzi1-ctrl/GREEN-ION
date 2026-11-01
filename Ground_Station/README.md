# GREEN ION V12 Ground Station

The app uses only the Python standard library in simulation mode. It serves a local dashboard, plots an offline relative GPS trajectory, and saves every received telemetry row to `logs/*.csv`.

```powershell
cd Ground_Station
py -3 ground_station.py --sample
```

Open `http://127.0.0.1:8765`. The dashboard labels simulated input and distinguishes live, stale, and missing link/GPS data.

For an Arduino Uno receiver, install `pyserial` and provide its port:

```powershell
py -3 -m pip install pyserial
py -3 ground_station.py --port COM5
```

The relative map does not need an internet connection or paid map service. It intentionally omits street/satellite tiles; coordinates remain available for copying to a separate navigation app.

