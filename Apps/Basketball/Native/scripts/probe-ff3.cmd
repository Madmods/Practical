@echo off
setlocal
cd /d "%~dp0.."
call "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64 -no_logo >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /O2 /utf-8 diagnostics\ff3_probe.cpp /Fo:build\ff3_probe.obj /Fe:build\ff3-probe.exe /link user32.lib ole32.lib windowsapp.lib
if errorlevel 1 exit /b 1
build\ff3-probe.exe >build\ff3-probe.txt
exit /b %errorlevel%
