@echo off
setlocal
cd /d "%~dp0"
rem Backward-compatible launcher. V16 no longer asks for a COM port.
call START_GREEN_ION.cmd
endlocal
