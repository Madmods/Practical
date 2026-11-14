@echo off
setlocal
cd /d "%~dp0.."
call "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64 >nul
cl /nologo /std:c++17 /EHsc /O2 /utf-8 diagnostics\ff3_qb_probe.cpp /Fo:build\ff3_qb_probe.obj /Fe:build\ff3-qb-probe.exe /link user32.lib ole32.lib windowsapp.lib
if errorlevel 1 exit /b 1
build\ff3-qb-probe.exe
exit /b %errorlevel%
