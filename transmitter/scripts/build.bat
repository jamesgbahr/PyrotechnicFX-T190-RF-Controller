@echo off
setlocal
cd /d "%~dp0.."
set "PIO=%USERPROFILE%\.platformio\penv\Scripts\platformio.exe"
if not exist "%PIO%" set "PIO=pio"
"%PIO%" run -e t190_sender
pause
