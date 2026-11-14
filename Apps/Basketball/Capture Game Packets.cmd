@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Native\scripts\capture-game-packets.ps1" %*
if errorlevel 1 pause
