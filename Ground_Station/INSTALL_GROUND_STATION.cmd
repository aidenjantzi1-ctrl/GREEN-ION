@echo off
cd /d "%~dp0"
echo GREEN ION Ground Station setup
echo Installing the only optional runtime dependency: pyserial.

py -3 --version >nul 2>nul
if %errorlevel%==0 goto use_py
python --version >nul 2>nul
if %errorlevel%==0 goto use_python

echo Python 3 was not found. Install Python 3 from python.org, then run this file again.
pause
exit /b 1

:use_py
py -3 -m pip install --user pyserial
goto finished

:use_python
python -m pip install --user pyserial

:finished
if not %errorlevel%==0 (
  echo Installation failed. Check the message above and your internet connection.
  pause
  exit /b 1
)
echo Setup complete. You can now open OPEN_GROUND_STATION.cmd.
pause
