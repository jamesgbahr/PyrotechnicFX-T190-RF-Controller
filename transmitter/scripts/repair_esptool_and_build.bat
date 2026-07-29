@echo off
setlocal EnableExtensions
cd /d "%~dp0.."

set "PIO=%USERPROFILE%\.platformio\penv\Scripts\platformio.exe"
set "PIOPY=%USERPROFILE%\.platformio\penv\Scripts\python.exe"

if not exist "%PIO%" (
  echo PlatformIO executable was not found:
  echo   %PIO%
  pause
  exit /b 1
)
if not exist "%PIOPY%" (
  echo PlatformIO Python was not found:
  echo   %PIOPY%
  pause
  exit /b 1
)

echo Installing/repairing esptool Python requirements...
"%PIOPY%" -m pip install --disable-pip-version-check --no-input --upgrade "intelhex==2.3.0" "rich-click<2"
if errorlevel 1 goto :fail

echo Verifying imports...
"%PIOPY%" -c "import intelhex, rich_click; print('intelhex and rich_click OK')"
if errorlevel 1 goto :fail

echo Building sender firmware...
"%PIO%" run -e t190_sender
if errorlevel 1 goto :fail

echo.
echo BUILD COMPLETE.
echo Firmware:
echo   %USERPROFILE%\.pfxpio\tx\build\t190_sender\firmware.bin
pause
exit /b 0

:fail
echo.
echo REPAIR OR BUILD FAILED. Copy the complete output and send it back.
pause
exit /b 1
