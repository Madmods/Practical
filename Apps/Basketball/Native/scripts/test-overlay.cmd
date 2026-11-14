@echo off
if not exist "%~dp0..\build" mkdir "%~dp0..\build"
cd /d "%~dp0..\build"
call "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64 >nul
cl /nologo /I"..\src" /I"..\resources" /std:c++17 /EHsc /O2 /utf-8 "..\tests\pump_overlay_tests.cpp" /Fe:pump-overlay-tests.exe /link user32.lib gdi32.lib
if errorlevel 1 exit /b 1
pump-overlay-tests.exe
if errorlevel 1 exit /b 1
exit /b %errorlevel%
