@echo off
setlocal
title Vanta - Setup
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0Apps\Basketball\Native\scripts\build-easy.ps1" -SetupOnly
set "VANTA_RESULT=%ERRORLEVEL%"
echo.
pause
exit /b %VANTA_RESULT%
