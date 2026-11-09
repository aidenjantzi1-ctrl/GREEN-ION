@echo off
setlocal
cd /d "%~dp0"
echo.
echo ============================================
echo  GREEN ION V12 - COMPUTER GROUND STATION
echo ============================================
echo Connect the Arduino Uno receiver to the computer by USB.
echo Find its COM port in Windows Device Manager ^> Ports.
echo.
set /p "UNO_PORT=UNO COM port (example COM5), or Enter for DEMO data: "

where py >nul 2>nul
if %errorlevel%==0 goto use_py
where python >nul 2>nul
if %errorlevel%==0 goto use_python

echo [ERROR] Python 3 not found. Install Python 3 and try again.
pause
exit /b 1

:use_py
if defined UNO_PORT (
  py -3 ground_station.py --port "%UNO_PORT%"
) else (
  py -3 ground_station.py --sample
)
goto done

:use_python
if defined UNO_PORT (
  python ground_station.py --port "%UNO_PORT%"
) else (
  python ground_station.py --sample
)
goto done

:done
if errorlevel 1 (
  echo.
  echo [ERROR] Ground station could not start. Check COM port and pyserial.
  echo Install pyserial with: py -3 -m pip install pyserial
  pause
)
endlocal
