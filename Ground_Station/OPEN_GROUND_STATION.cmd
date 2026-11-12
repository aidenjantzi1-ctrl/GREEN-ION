@echo off
setlocal
cd /d "%~dp0"
echo.
echo ==============================================
echo  GREEN ION V16 - WINDOWS GROUND STATION
echo ==============================================
echo The dashboard runs at http://127.0.0.1:8765 without internet.
echo Join GREEN-ION-V14 Feather Wi-Fi when you need live rocket data.
echo Finished flights save under GREEN_ION_Data\flights.
echo The app auto-detects a recognizable Arduino Uno USB receiver when possible.
echo Close Arduino Serial Monitor before starting.
echo.
where py >nul 2>nul
if %errorlevel%==0 goto use_py
where python >nul 2>nul
if %errorlevel%==0 goto use_python
echo [ERROR] Python 3 not found. Install Python 3.
pause
exit /b 1

:use_py
set "PYTHON_EXE=py -3"
goto launch

:use_python
set "PYTHON_EXE=python"

:launch
if /i "%~1"=="DEMO" (
  %PYTHON_EXE% ground_station.py --sample
) else (
  %PYTHON_EXE% ground_station.py %*
)
if errorlevel 1 (
 echo.
 echo [ERROR] Check Python and optional Uno USB support.
 echo Uno mode: py -3 -m pip install pyserial
 echo Manual Uno selection: OPEN_GROUND_STATION.cmd --port COM5
 echo Wi-Fi only: OPEN_GROUND_STATION.cmd --wifi-only
 pause
)
endlocal
