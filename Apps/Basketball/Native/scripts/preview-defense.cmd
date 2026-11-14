@echo off
setlocal
if not exist "%~dp0..\build" mkdir "%~dp0..\build"
cd /d "%~dp0..\build"
if "%~1"=="" exit /b 2
call "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64 >nul
if errorlevel 1 exit /b 1
cl /nologo /I"..\src" /I"..\resources" /std:c++17 /EHsc /O2 /utf-8 "..\diagnostics\defense_visual_preview.cpp" /Fe:defense-visual-preview.exe /link user32.lib gdi32.lib
if errorlevel 1 exit /b 1
defense-visual-preview.exe "%~1" "%~2"
exit /b %errorlevel%

