@echo off
setlocal
if not exist "%~dp0..\build" mkdir "%~dp0..\build"
cd /d "%~dp0..\build"
call "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64 >nul
if errorlevel 1 exit /b 1
cl /nologo /I"..\src" /std:c++17 /EHsc /O2 /utf-8 /DUNICODE /D_UNICODE "..\diagnostics\block_diagnostic.cpp" /Fe:block-diagnostic.exe /link user32.lib ole32.lib windowsapp.lib
if errorlevel 1 exit /b 1
block-diagnostic.exe
exit /b %errorlevel%
