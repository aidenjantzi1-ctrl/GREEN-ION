# GREEN ION V12 build guide

V12 development is on `codex/green-ion-v12`; do not merge or flash from an unrelated branch by mistake. The complete V11 release candidate is preserved under `Past_Versions/Green_Ion_V11/` for rollback and format comparison. Neither version is flight-ready without measured hardware validation.

## Arduino IDE setup

Install Arduino IDE 2 from Arduino, then install these board packages and libraries through Boards Manager and Library Manager. The 2026-11-05 Pacific cloud compile used Arduino CLI 1.5.1, ESP32 core 2.0.17 and AVR core 1.8.7. Installed libraries were Adafruit BMP3XX 2.1.6, Adafruit BusIO 1.17.4, Adafruit GPS 1.9.0, Adafruit LSM6DS 4.7.4, Adafruit Unified Sensor 1.1.15 and RadioHead 1.121. Confirm installed library versions locally before reporting a hardware result.

- ESP32 board package; select **Adafruit Feather ESP32 V2** for `Green_Ion_V12`.
- Adafruit BMP3XX Library
- Adafruit LSM6DS
- Adafruit GPS Library
- RadioHead
- Adafruit BusIO and Adafruit Unified Sensor (normally installed as dependencies)

Add `libraries/GreenIonProtocol` and `libraries/GreenIonRecorder` to the Arduino library search path, or copy both complete folders into the Arduino sketchbook `libraries` directory. Restart Arduino IDE after adding local libraries. Compile:

- `Green_Ion_V12/Green_Ion_V12.ino` for `esp32:esp32:adafruit_feather_esp32_v2`.
- `Ground_Receiver/Ground_Receiver.ino` for `arduino:avr:uno`.

In Arduino IDE, connect the Feather with a data USB cable, select its COM port and **Adafruit Feather ESP32 V2**, open `Green_Ion_V12/Green_Ion_V12.ino`, then select **Sketch > Verify/Compile** and **Upload**. Open Serial Monitor at 115200 baud to inspect sensor, SD and radio startup status. For the receiver, select **Arduino Uno**, open `Ground_Receiver/Ground_Receiver.ino`, verify/upload, and inspect its 115200 baud JSON boot status. The Uno's RFM95W hardware is currently not connected, so expect radio hardware results to remain NOT TESTED. If a port is missing, try a known data cable and check Windows Device Manager; if a library is missing, install the named library and restart Arduino IDE. Never count a successful compile as a sensor or radio test. The changed Feather firmware compiled successfully in the 2026-11-06 Pacific cloud GIS1 quality follow-up using ESP32 core 2.0.17 (1,221,469 flash bytes, 69,928 RAM bytes); the unchanged Uno target passed in the earlier Phase 8 run. The ESP32 core printed discovery-tool warnings but compilation exited 0. CI is also configured to compile on matching changes. GIS1 reserved-byte altitude freshness semantics are documented in `V12_GIS1_ALTITUDE_QUALITY.md`. Compilation is not a hardware test.

## Ground station

Simulation mode needs only Python 3:

```powershell
cd Ground_Station
py -3 ground_station.py --sample
```

On Windows, install Python 3.10 or newer, run `Ground_Station/INSTALL_GROUND_STATION.cmd` once for pyserial, then run `Ground_Station/OPEN_GROUND_STATION.cmd`. Open `http://127.0.0.1:8765` if the browser does not open automatically. Join the Feather's `GREEN-ION-V12` Wi-Fi for Wi-Fi telemetry; `http://192.168.4.1/` is the Feather's own page, while the laptop Ground Station remains at its localhost address. For a receiver on `COM5`, use Settings or run `py -3 Ground_Station/ground_station.py --port COM5` from the repository root. If the Uno is absent, Wi-Fi-only telemetry and autonomous SD recording still work. The phone setup is in `V12_IPHONE_COMPANION.md` and requires a separate trusted HTTPS configuration.

The laptop writes live JSONL sessions under `Ground_Station/data/`, imported BIN/SEN originals under `Ground_Station/data/flights/raw/`, and library metadata under `Ground_Station/data/flights/metadata/`. In Analysis, verify and download a Feather SD BIN, attach a matching SEN if present, inspect graphs/replay, export CSV/JSON/report, then download a ZIP backup. Do not move the current live session to Trash. A laptop backup does not include recordings still only on Feather SD.

## Automated tests

```powershell
py -3 -m unittest discover -s tests -p "test_*.py" -v
node tests/js/test_rkt4.mjs
```

The Python suite includes V12 telemetry corruption, version, fix-staleness, RKT4 golden bytes/interrupted logs, and project-layout checks while retaining all V11 regression tests. CI also compiles and executes `tests/cpp/test_v12_rkt4.cpp` natively.

