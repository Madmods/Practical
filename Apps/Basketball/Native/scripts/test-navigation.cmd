@echo off
if not exist "%~dp0..\build" mkdir "%~dp0..\build"
cd /d "%~dp0..\build"
call "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64 >nul
cl /nologo /I"..\src" /I"..\resources" /std:c++17 /EHsc /O2 "..\tests\navigation_motion_tests.cpp" /Fe:navigation-motion-tests.exe
if not "%errorlevel%"=="0" exit /b %errorlevel%
navigation-motion-tests.exe
exit /b %errorlevel%
