@echo off
setlocal
cd /d "%~dp0.."
call "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64 >nul
if errorlevel 1 exit /b 1
if not exist build mkdir build
cl /nologo /std:c++17 /EHsc /O2 /W4 tests\ff3_trajectory_replay_tests.cpp /Fo:build\ff3-trajectory-replay-tests.obj /Fe:build\ff3-trajectory-replay-tests.exe
if errorlevel 1 exit /b 1
build\ff3-trajectory-replay-tests.exe
exit /b %errorlevel%
