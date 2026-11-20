@echo off
setlocal
cd /d "%~dp0.."
set "DECODER_PYTHON=python"
if exist .venv\Scripts\python.exe set "DECODER_PYTHON=.venv\Scripts\python.exe"
"%DECODER_PYTHON%" -B -m unittest discover -s diagnostics\network_decoder -p test_*.py
exit /b %errorlevel%
