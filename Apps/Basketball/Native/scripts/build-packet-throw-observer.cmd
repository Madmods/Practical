@echo off
setlocal
cd /d "%~dp0.."
call "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64 -no_logo >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /O2 /utf-8 diagnostics\packet_throw_observer.cpp /Fo:build\packet_throw_observer.obj /Fe:build\packet-throw-observer.exe /link user32.lib ole32.lib windowsapp.lib
exit /b %errorlevel%
