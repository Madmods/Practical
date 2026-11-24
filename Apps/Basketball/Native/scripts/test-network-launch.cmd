@echo off
setlocal
cd /d "%~dp0.."
call "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64 -no_logo >nul
if errorlevel 1 exit /b 1
if not exist build mkdir build
cl /nologo /std:c++17 /permissive- /EHsc /O2 /utf-8 tests\network_launch_tests.cpp /Fo:build\network_launch_tests.obj /Fe:build\network-launch-tests.exe /link user32.lib advapi32.lib
if errorlevel 1 exit /b 1
build\network-launch-tests.exe
exit /b %errorlevel%
