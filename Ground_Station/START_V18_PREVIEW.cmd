@echo off
rem GREEN ION V18 preview; this does NOT flash or update firmware.
rem Force this extracted development preview, never redirect to installed V17.\r\ncall "%~dp0START_GREEN_ION.cmd" -NoRedirect --v18-offline
exit /b %ERRORLEVEL%
