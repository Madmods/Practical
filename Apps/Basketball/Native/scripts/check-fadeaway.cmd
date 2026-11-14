@echo off
setlocal
cd /d "%~dp0..\build"
call "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64 >nul
cl /nologo /I"..\src" /std:c++17 /EHsc /O2 /utf-8 /DUNICODE /D_UNICODE "..\diagnostics\fadeaway_diagnostic.cpp" /Fe:fadeaway-diagnostic.exe /link user32.lib ole32.lib windowsapp.lib
if errorlevel 1 exit /b 1
fadeaway-diagnostic.exe
exit /b %errorlevel%
