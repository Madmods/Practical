@echo off
setlocal
title Vanta - Check Setup
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0Apps\Basketball\Native\scripts\build-easy.ps1" -CheckOnly
set "VANTA_RESULT=%ERRORLEVEL%"
echo.
pause
exit /b %VANTA_RESULT%
