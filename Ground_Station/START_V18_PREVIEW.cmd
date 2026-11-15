@echo off
rem GREEN ION V18 preview; this does NOT flash or update firmware.
call "%~dp0START_GREEN_ION.cmd" --v18-offline
exit /b %ERRORLEVEL%
