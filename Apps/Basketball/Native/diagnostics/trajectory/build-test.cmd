@echo off
rem Builds everything with MSVC, runs both test suites, writes build\trajectory.csv
rem and the scenario diagrams/reports to out\. "build-test.cmd bench" also benchmarks.
setlocal
cd /d "%~dp0"
where cl >nul 2>nul
if not errorlevel 1 goto build
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set "VSDEV=%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat"
if exist "%VSWHERE%" for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -find Common7\Tools\VsDevCmd.bat`) do set "VSDEV=%%i"
call "%VSDEV%" -arch=x64 >nul
where cl >nul 2>nul
if errorlevel 1 goto noenv
:build
if not exist build mkdir build
set "FLAGS=/nologo /std:c++17 /EHsc /O2 /W4 /permissive-"
for %%t in (tests planner_tests example simulate bench) do (
 cl %FLAGS% %%t.cpp /Fo:build\%%t.obj /Fe:build\%%t.exe || exit /b 1
)
build\tests.exe || exit /b 1
build\planner_tests.exe || exit /b 1
build\example.exe > build\trajectory.csv || exit /b 1
build\simulate.exe out || exit /b 1
if /i "%~1"=="bench" build\bench.exe
exit /b 0
:noenv
echo Could not find the MSVC compiler. Run from a "Developer Command Prompt for VS" or install the C++ build tools.
exit /b 1
