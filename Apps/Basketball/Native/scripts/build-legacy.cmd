@echo off
if not exist "%~dp0..\build" mkdir "%~dp0..\build"
cd /d "%~dp0..\build"
call "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64 >nul
cl /nologo /I"..\src" /I"..\resources" /std:c++17 /EHsc /O2 /utf-8 /DUNICODE /D_UNICODE "..\src\main.cpp" /Fe:Vanta-legacy.exe /link /SUBSYSTEM:WINDOWS d3d11.lib d2d1.lib dwrite.lib dcomp.lib dxgi.lib dwmapi.lib user32.lib ole32.lib

