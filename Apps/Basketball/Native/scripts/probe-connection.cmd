@echo off
setlocal
cd /d "%~dp0.."
call "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64 -no_logo >nul
if errorlevel 1 exit /b 1
if not exist build\webui mkdir build\webui
cl /nologo /std:c++17 /permissive- /EHsc /O2 /utf-8 /DUNICODE /D_UNICODE diagnostics\basketball_connection_probe.cpp /Fo:build\webui\ /Fe:build\basketball-connection-probe.exe /link user32.lib ole32.lib windowsapp.lib
if errorlevel 1 exit /b 1
build\basketball-connection-probe.exe
exit /b %errorlevel%
