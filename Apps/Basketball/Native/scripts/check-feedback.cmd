@echo off
setlocal
cd /d "%~dp0.."
call "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64 -no_logo >nul
if errorlevel 1 exit /b 1
cl /nologo /I"src" /I"resources" /std:c++17 /EHsc /O2 /utf-8 /DUNICODE /D_UNICODE diagnostics\feedback_diagnostic.cpp /Fo:build\ /Fe:build\feedback-diagnostic.exe /link user32.lib
if errorlevel 1 exit /b 1
build\feedback-diagnostic.exe %*
exit /b %errorlevel%
