@echo off
setlocal
cd /d "%~dp0transmitter"
call "scripts\repair_esptool_and_build.bat"
exit /b %errorlevel%
