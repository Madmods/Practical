@echo off
setlocal
cd /d "%~dp0.."
set "DECODER_PYTHON=python"
if exist .venv\Scripts\python.exe set "DECODER_PYTHON=.venv\Scripts\python.exe"
"%DECODER_PYTHON%" build\network_decoder\test_network_transport_lab.py
if errorlevel 1 exit /b 1
"%DECODER_PYTHON%" build\network_decoder\test_aes_schedule_scan.py
if errorlevel 1 exit /b 1
"%DECODER_PYTHON%" build\network_decoder\test_network_wire_lab.py
if errorlevel 1 exit /b 1
"%DECODER_PYTHON%" build\network_decoder\test_live_network_decoder.py
exit /b %errorlevel%
