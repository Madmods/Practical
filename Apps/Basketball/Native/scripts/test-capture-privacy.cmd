@echo off
setlocal
cd /d "%~dp0.."
call "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64 -no_logo >nul
if errorlevel 1 exit /b 1
if not exist build mkdir build
cl /nologo /std:c++17 /EHsc /O2 /utf-8 tests\capture_privacy_tests.cpp /Fo:build\capture-privacy-tests.obj /Fe:build\capture-privacy-tests.exe /link user32.lib
if errorlevel 1 exit /b 1
build\capture-privacy-tests.exe
exit /b %errorlevel%
