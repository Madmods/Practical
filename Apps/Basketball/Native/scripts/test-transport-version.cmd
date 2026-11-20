@echo off
setlocal
cd /d "%~dp0.."
call "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64 -no_logo >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /O2 /utf-8 tests\transport_version_tests.cpp /Fo:build\transport_version_tests.obj /Fe:build\transport-version-tests.exe
if errorlevel 1 exit /b 1
build\transport-version-tests.exe
exit /b %errorlevel%
