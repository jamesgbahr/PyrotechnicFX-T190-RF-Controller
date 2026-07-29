@echo off
setlocal
cd /d "%~dp0receiver"
call "FULL_FLASH_RECEIVER.bat"
exit /b %errorlevel%
