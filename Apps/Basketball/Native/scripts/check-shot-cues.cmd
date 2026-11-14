@echo off
setlocal
if not exist "%~dp0..\build" mkdir "%~dp0..\build"
cd /d "%~dp0..\build"
call "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64 >nul
if not "%errorlevel%"=="0" exit /b 1
cl /nologo /I"..\src" /I"..\resources" /std:c++17 /EHsc /O2 /utf-8 /DUNICODE /D_UNICODE "..\diagnostics\shot_cue_diagnostic.cpp" /Fe:shot-cue-diagnostic.exe /link user32.lib
if not "%errorlevel%"=="0" exit /b 1
shot-cue-diagnostic.exe
exit /b %errorlevel%
