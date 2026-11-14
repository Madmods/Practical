@echo off
setlocal
cd /d "%~dp0.."
if not exist build mkdir build
call "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64 >nul
cl /nologo /std:c++17 /EHsc /O2 /I src tests\game_module_tests.cpp /Fo:build\game_module_tests.obj /Fe:build\game-module-tests.exe
if errorlevel 1 exit /b 1
build\game-module-tests.exe
exit /b %errorlevel%
