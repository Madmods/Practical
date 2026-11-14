@echo off
setlocal
call "%~dp0build-webui.cmd"
if errorlevel 1 exit /b 1
call "%~dp0test-network-inspector.cmd"
if errorlevel 1 exit /b 1
call "%~dp0test-webui.cmd"
if errorlevel 1 exit /b 1
call "%~dp0test-game-modules.cmd"
if errorlevel 1 exit /b 1
call "%~dp0test-ff3-qb.cmd"
if errorlevel 1 exit /b 1
call "%~dp0test-ff3-qb-native.cmd"
if errorlevel 1 exit /b 1
copy /y "%~dp0..\build\Vanta-Basketball.exe" "%~dp0..\..\Vanta.exe" >nul
if errorlevel 1 (
 echo Close Basketball before installing the new executable. The candidate is in Native\build.
 exit /b 1
)
copy /y "%~dp0..\build\vanta-release.json" "%~dp0..\..\vanta-release.json" >nul
if errorlevel 1 exit /b 1
echo Installed Vanta with Basketball and FF3 modules and the local release catalog. Start it with Start Basketball.cmd.
exit /b 0


