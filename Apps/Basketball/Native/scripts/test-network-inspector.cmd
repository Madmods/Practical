@echo off
setlocal
cd /d "%~dp0.."
call "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64 -no_logo >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /O2 /utf-8 /DUNICODE /D_UNICODE tests\network_inspector_tests.cpp /Fo:build\network_inspector_tests.obj /Fe:build\network-inspector-tests.exe /link ole32.lib windowsapp.lib
if errorlevel 1 exit /b 1
build\network-inspector-tests.exe
exit /b %errorlevel%
