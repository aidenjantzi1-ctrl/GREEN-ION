@echo off
setlocal
cd /d "%~dp0"
echo.
echo ==============================================
echo  GREEN ION V15 - LOCAL-FIRST BLUE DASHBOARD
echo ==============================================
echo Join GREEN-ION-V14 Feather Wi-Fi when you need live rocket data.
echo The blue dashboard runs at http://127.0.0.1:8765 without internet.
echo Verified flights save locally under GREEN_ION_Data\flights.
echo Connect Uno by USB for LoRa backup; close Arduino Serial Monitor.
echo.
set /p "UNO_PORT=Uno COM port (example COM5), Enter for Wi-Fi only, or DEMO: "
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
if /i "%UNO_PORT%"=="DEMO" (
  %PYTHON_EXE% ground_station.py --sample
) else if defined UNO_PORT (
  %PYTHON_EXE% ground_station.py --port "%UNO_PORT%"
) else (
  %PYTHON_EXE% ground_station.py
)
if errorlevel 1 (
 echo.
 echo [ERROR] Check Python, COM port and pyserial.
 echo Uno mode: py -3 -m pip install pyserial
 pause
)
endlocal
