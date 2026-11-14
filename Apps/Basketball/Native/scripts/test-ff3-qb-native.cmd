@echo off
setlocal
cd /d "%~dp0.."
call "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64 >nul
if errorlevel 1 exit /b 1
if not exist build mkdir build
cl /nologo /std:c++17 /permissive- /EHsc /O2 /utf-8 tests\ff3_qb_native_tests.cpp /Fo:build\ff3_qb_native_tests.obj /Fe:build\ff3-qb-native-tests.exe /link gdi32.lib user32.lib ole32.lib windowsapp.lib
if errorlevel 1 exit /b 1
build\ff3-qb-native-tests.exe
exit /b %errorlevel%

