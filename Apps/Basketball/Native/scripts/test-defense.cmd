@echo off
if not exist "%~dp0..\build" mkdir "%~dp0..\build"
cd /d "%~dp0..\build"
call "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64 >nul
cl /nologo /I"..\src" /I"..\resources" /std:c++17 /EHsc /O2 /utf-8 "..\tests\defense_tests.cpp" /Fe:defense-tests.exe
if not "%errorlevel%"=="0" exit /b 1
defense-tests.exe
if not "%errorlevel%"=="0" exit /b 1
cl /nologo /I"..\src" /I"..\resources" /std:c++17 /EHsc /O2 /utf-8 "..\tests\defense_input_tests.cpp" /Fe:defense-input-tests.exe /link user32.lib
if not "%errorlevel%"=="0" exit /b 1
defense-input-tests.exe
if not "%errorlevel%"=="0" exit /b 1
cl /nologo /I"..\src" /I"..\resources" /std:c++17 /EHsc /O2 /utf-8 "..\diagnostics\defense_diagnostic.cpp" /Fe:defense-diagnostic.exe /link user32.lib

