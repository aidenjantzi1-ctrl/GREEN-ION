# GREEN ION V12 build guide

V12 is an isolated development tree. `Green_Ion_V11/` remains the working flight baseline and must not be overwritten while V12 is incomplete.

## Arduino IDE setup

Install these board packages and libraries through Arduino IDE Library Manager:

- ESP32 board package; select **Adafruit Feather ESP32 V2** for `Green_Ion_V12`.
- Adafruit BMP3XX Library
- Adafruit LSM6DS
- Adafruit GPS Library
- RadioHead
- Adafruit BusIO and Adafruit Unified Sensor (normally installed as dependencies)

Add `libraries/GreenIonProtocol` to the Arduino library search path, or copy that complete folder into the Arduino sketchbook `libraries` directory. Compile:

- `Green_Ion_V12/Green_Ion_V12.ino` for `esp32:esp32:adafruit_feather_esp32_v2`.
- `Ground_Receiver/Ground_Receiver.ino` for `arduino:avr:uno`.

CI performs both compilations. A successful compile is not a hardware test.

## Ground station

Simulation mode needs only Python 3:

```powershell
cd Ground_Station
py -3 ground_station.py --sample
```

Open `http://127.0.0.1:8765`. For a receiver on `COM5`, install pyserial and run `py -3 ground_station.py --port COM5`.

## Automated tests

```powershell
py -3 -m unittest discover -s tests -p "test_*.py" -v
node tests/js/test_rkt4.mjs
```

The Python suite includes V12 telemetry corruption, version, fix-staleness, and project-layout checks while retaining all V11 regression tests.

