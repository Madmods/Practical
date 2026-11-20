@echo off
setlocal
title Vanta - Start
if not exist "%~dp0Apps\Basketball\Vanta.exe" goto missing
if not exist "%~dp0Apps\Basketball\vanta-release.json" goto missing
start "" /D "%~dp0Apps\Basketball" "%~dp0Apps\Basketball\Vanta.exe"
if errorlevel 1 goto failed
exit /b 0
:missing
echo Vanta has not been built yet, or its release catalog is missing.
echo Close Vanta, then double-click "2 - Build Vanta.bat" in this folder.
echo.
pause
exit /b 1
:failed
echo Windows could not start Vanta. Check the message above.
echo.
pause
exit /b 1
