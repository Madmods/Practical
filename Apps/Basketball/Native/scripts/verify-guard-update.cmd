@echo off
setlocal
if not exist "%~dp0..\build" mkdir "%~dp0..\build"
cd /d "%~dp0..\build"
call "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64 >nul
if not "%errorlevel%"=="0" exit /b 1
cl /nologo /I"..\src" /I"..\resources" /std:c++17 /EHsc /O2 /utf-8 "..\tests\defense_tests.cpp" /Fe:defense-tests.exe
if not "%errorlevel%"=="0" exit /b 1
defense-tests.exe
if not "%errorlevel%"=="0" exit /b 1
cl /nologo /I"..\src" /I"..\resources" /std:c++17 /EHsc /O2 /utf-8 /DUNICODE /D_UNICODE "..\tests\defense_court_tests.cpp" /Fe:defense-court-tests.exe /link user32.lib
if not "%errorlevel%"=="0" exit /b 1
defense-court-tests.exe
if not "%errorlevel%"=="0" exit /b 1
cl /nologo /I"..\src" /I"..\resources" /std:c++17 /EHsc /O2 /utf-8 "..\tests\guard_tracking_tests.cpp" /Fe:guard-tracking-tests.exe
if not "%errorlevel%"=="0" exit /b 1
guard-tracking-tests.exe
if not "%errorlevel%"=="0" exit /b 1
cl /nologo /I"..\src" /std:c++17 /EHsc /O2 /utf-8 "..\tests\anti_post_tests.cpp" /Fe:anti-post-tests.exe
if not "%errorlevel%"=="0" exit /b 1
anti-post-tests.exe
if not "%errorlevel%"=="0" exit /b 1
cl /nologo /I"..\src" /std:c++17 /EHsc /O2 /utf-8 "..\tests\fadeaway_tests.cpp" /Fe:fadeaway-tests.exe
if not "%errorlevel%"=="0" exit /b 1
fadeaway-tests.exe
if not "%errorlevel%"=="0" exit /b 1
cl /nologo /I"..\src" /std:c++17 /EHsc /O2 /utf-8 "..\tests\ball_side_block_tests.cpp" /Fe:ball-side-block-tests.exe
if not "%errorlevel%"=="0" exit /b 1
ball-side-block-tests.exe
if not "%errorlevel%"=="0" exit /b 1
cl /nologo /I"..\src" /I"..\resources" /std:c++17 /EHsc /O2 /utf-8 /DUNICODE /D_UNICODE "..\src\main.cpp" /Fe:Vanta-guard-update.exe /link /SUBSYSTEM:WINDOWS d3d11.lib d2d1.lib dwrite.lib dcomp.lib dxgi.lib dwmapi.lib user32.lib ole32.lib
exit /b %errorlevel%

