@echo off
setlocal
title Vanta - Build
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0Apps\Basketball\Native\scripts\build-easy.ps1"
set "VANTA_RESULT=%ERRORLEVEL%"
echo.
if not "%VANTA_RESULT%"=="0" echo Build did not finish. Read the message above, fix it, then run this file again.
pause
exit /b %VANTA_RESULT%
