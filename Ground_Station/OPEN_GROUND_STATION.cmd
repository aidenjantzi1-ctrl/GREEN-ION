@echo off
setlocal
cd /d "%~dp0"
rem Backward-compatible V16 launcher for ground_station.py.
rem Default: auto-detects a recognizable Arduino Uno with no COM prompt.
rem Optional legacy/manual override remains supported: --port COM5
rem Wi-Fi only remains supported: --wifi-only
call START_GREEN_ION.cmd %*
endlocal
