@echo off
setlocal
cd /d "%~dp0.."
call "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64 >nul
cl /nologo /std:c++17 /EHsc /O2 /I src tests\ff3_qb_tests.cpp /Fo:build\ff3_qb_tests.obj /Fe:build\ff3-qb-tests.exe
if errorlevel 1 exit /b 1
build\ff3-qb-tests.exe
exit /b %errorlevel%
