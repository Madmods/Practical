@echo off
setlocal
cd /d "%~dp0.."
call "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64 -no_logo >nul
if errorlevel 1 exit /b 1
if not exist build\network_decoder mkdir build\network_decoder
copy /y diagnostics\network_decoder\*.py build\network_decoder\ >nul
copy /y diagnostics\network_decoder\replication-type-maps.json build\network_decoder\ >nul
cl /nologo /std:c++17 /EHsc /O2 /LD diagnostics\network_decoder\scan_aes_schedules.cpp /Fo:build\network_decoder\scan_aes_schedules.obj /Fe:build\network_decoder\scan_aes_schedules.dll /link /IMPLIB:build\network_decoder\scan_aes_schedules.lib
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /O2 /utf-8 /I third_party\windivert diagnostics\network_decoder\capture_network_lab.cpp /Fo:build\network_decoder\capture_network_lab.obj /Fe:build\network_decoder\capture-network-lab.exe /link iphlpapi.lib ws2_32.lib
exit /b %errorlevel%
