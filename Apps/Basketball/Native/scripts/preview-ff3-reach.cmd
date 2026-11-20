@echo off
setlocal
cd /d "%~dp0.."
call "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64 -no_logo >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /permissive- /EHsc /O2 /utf-8 diagnostics\ff3_reach_visual_preview.cpp /Fo:build\ff3_reach_visual_preview.obj /Fe:build\ff3-reach-visual-preview.exe /link user32.lib gdi32.lib ole32.lib windowsapp.lib
if errorlevel 1 exit /b 1
build\ff3-reach-visual-preview.exe
exit /b %errorlevel%
