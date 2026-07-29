@echo off
setlocal
cd /d "%~dp0transmitter"
call "FULL_FLASH_TRANSMITTER.bat"
exit /b %errorlevel%
