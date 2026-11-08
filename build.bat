@echo off
title Build PracticalComplete
cd /d "%~dp0"
echo Joining src\engine then src\ui into dist\PracticalComplete.luau ...
echo.
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0build.ps1"
if errorlevel 1 (
  echo.
  echo BUILD PROBLEM - see message above.
) else (
  echo.
  echo Done. Run or copy:  %~dp0dist\PracticalComplete.luau
)
echo.
pause
