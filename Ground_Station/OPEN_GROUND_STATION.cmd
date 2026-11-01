@echo off
cd /d "%~dp0"

where py >nul 2>nul
if %errorlevel%==0 goto use_py

where python >nul 2>nul
if %errorlevel%==0 goto use_python

echo Python was not found. Install Python 3, then open this file again.
pause
exit /b 1

:use_py
py -3 ground_station.py
exit /b %errorlevel%

:use_python
python ground_station.py
exit /b %errorlevel%
