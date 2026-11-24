@echo off
setlocal
cd /d "%~dp0.."
call "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64 -no_logo >nul
if errorlevel 1 exit /b 1
set "WV2=third_party\webview2\build\native"
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\generate-assist-catalog.ps1
if errorlevel 1 exit /b 1
if not exist "%WV2%\include\WebView2.h" exit /b 2
if not exist build\webui mkdir build\webui
pushd resources
rc /nologo /fo ..\build\webui\basketball.res basketball-webui.rc
if errorlevel 1 (popd & exit /b 1)
popd
cl /nologo /std:c++17 /permissive- /EHsc /O2 /utf-8 /W3 /DUNICODE /D_UNICODE /I "%WV2%\include" src\basketball_main.cpp build\webui\basketball.res /Fo:build\webui\ /Fe:build\Vanta-Basketball.exe /link /SUBSYSTEM:WINDOWS /MANIFEST:EMBED /MANIFESTUAC:"level='requireAdministrator' uiAccess='false'" "%WV2%\x64\WebView2LoaderStatic.lib" user32.lib gdi32.lib ole32.lib oleaut32.lib shell32.lib dwmapi.lib advapi32.lib ws2_32.lib windowsapp.lib CoreMessaging.lib
if errorlevel 1 exit /b 1
rem The separate test host forces synthetic mode at compile time and never asks for elevation.
if not exist build\webui-test mkdir build\webui-test
cl /nologo /std:c++17 /permissive- /EHsc /O2 /utf-8 /W3 /DUNICODE /D_UNICODE /DVANTA_TEST_HOST /I "%WV2%\include" src\basketball_main.cpp build\webui\basketball.res /Fo:build\webui-test\ /Fe:build\Vanta-Basketball-UiTest.exe /link /SUBSYSTEM:WINDOWS /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" "%WV2%\x64\WebView2LoaderStatic.lib" user32.lib gdi32.lib ole32.lib oleaut32.lib shell32.lib dwmapi.lib advapi32.lib ws2_32.lib windowsapp.lib CoreMessaging.lib
if errorlevel 1 exit /b 1
mt -nologo -inputresource:build\Vanta-Basketball.exe;#1 -out:build\webui\production.manifest
if errorlevel 1 exit /b 1
mt -nologo -inputresource:build\Vanta-Basketball-UiTest.exe;#1 -out:build\webui-test\test.manifest
if errorlevel 1 exit /b 1
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\check-build-manifests.ps1
if errorlevel 1 exit /b 1
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\write-release-catalog.ps1
exit /b %errorlevel%
