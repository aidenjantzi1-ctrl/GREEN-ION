# GREEN ION V12 temporary airplane test

Use only when permitted by the airline and crew. Cabin-pressure-equivalent change is not aircraft altitude.

## Prepare and start

1. Upload the CI-built V12 firmware, insert a tested SD card, and fully charge the battery.
2. Join `GREEN-ION-V12`, open `http://192.168.4.1`, select **Airplane Test**, then **Prepare & Calibrate** while stationary.
3. Do not start unless the mode says `READY`, SD/BMP388/IMU pass, and battery is at least 3.50 V.
4. Use USB Serial at 115200 baud for reliable offline control: `airplane prepare`, `airplane status`, then—only when permitted—`airplane start`.

Start persists a radio-silence latch, creates `AIRP###.BIN/.SEN/.TXT`, turns the Wi-Fi AP off, holds RFM95W reset low, calls Bluetooth shutdown, and blocks telemetry/radio recovery. GPS is receive-only. Dashboard disconnection is expected; recording, 500 ms flushes, and five-second checkpoints continue offline. The automatic limit is eight hours.

## Stop and retrieve

1. Reconnect USB Serial and send `airplane stop`; wait for `stop=PASS`.
2. Radios remain off. Send `airplane unlock`, then reboot when radio use is permitted.
3. Reconnect to the dashboard and download the matching BIN, SEN, and TXT files under Storage.
4. Export pressure/temperature/battery CSV in the browser. Export IMU/GPS CSV plus integrity JSON with:

   `python Ground_Station/sensor_log.py AIRP000.SEN --rkt4 AIRP000.BIN --csv AIRP000-sensors.csv --json AIRP000-sensors.json`

The airplane browser view suppresses rocket launch/apogee/landing labels. Unexpected power loss retains checkpoint/recovery evidence.

## Limits

CI verifies software paths and compilation, not RF emissions. The code blocks firmware-controlled Wi-Fi, LoRa, Bluetooth, telemetry, and radio recovery after the persistent latch is set, including after reset. It cannot prove physical RF silence, damaged-hardware behavior, boot-ROM behavior, or airline acceptance. Follow crew instructions.
