@echo off
if not exist "%~dp0..\build" mkdir "%~dp0..\build"
cd /d "%~dp0..\build"
call "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64 >nul
cl /nologo /I"..\src" /I"..\resources" /std:c++17 /EHsc /O2 /utf-8 "..\tests\shooting_tests.cpp" /Fe:shooting-tests.exe
if not "%errorlevel%"=="0" exit /b %errorlevel%
shooting-tests.exe

if not "%errorlevel%"=="0" exit /b %errorlevel%
cl /nologo /I"..\src" /I"..\resources" /std:c++17 /EHsc /O2 /utf-8 "..\tests\pump_tests.cpp" /Fe:pump-tests.exe
if not "%errorlevel%"=="0" exit /b %errorlevel%
pump-tests.exe
if not "%errorlevel%"=="0" exit /b %errorlevel%
cl /nologo /I"..\src" /std:c++17 /EHsc /O2 /utf-8 "..\tests\timing_reviewer_tests.cpp" /Fe:timing-reviewer-tests.exe
if not "%errorlevel%"=="0" exit /b %errorlevel%
timing-reviewer-tests.exe
if not "%errorlevel%"=="0" exit /b %errorlevel%
cl /nologo /I"..\src" /std:c++17 /EHsc /O2 /utf-8 "..\tests\shot_feedback_tests.cpp" /Fe:shot-feedback-tests.exe
if not "%errorlevel%"=="0" exit /b %errorlevel%
shot-feedback-tests.exe
if not "%errorlevel%"=="0" exit /b %errorlevel%
cl /nologo /I"..\src" /std:c++17 /EHsc /O2 /utf-8 "..\tests\timing_latency_tests.cpp" /Fe:timing-latency-tests.exe
if not "%errorlevel%"=="0" exit /b %errorlevel%
timing-latency-tests.exe
if not "%errorlevel%"=="0" exit /b %errorlevel%
cl /nologo /I"..\src" /std:c++17 /EHsc /O2 /utf-8 "..\tests\auto_calibration_tests.cpp" /Fe:auto-calibration-tests.exe
if not "%errorlevel%"=="0" exit /b %errorlevel%
auto-calibration-tests.exe
exit /b %errorlevel%
