@echo off
setlocal EnableExtensions
cd /d "%~dp0"
title PyrotechnicFX T190 Transmitter Full Flash v1.6.3

set "PIO_EXE=%USERPROFILE%\.platformio\penv\Scripts\platformio.exe"
if not exist "%PIO_EXE%" set "PIO_EXE="
if not defined PIO_EXE (
  for /f "delims=" %%P in ('where platformio.exe 2^>nul') do if not defined PIO_EXE set "PIO_EXE=%%P"
)
if not defined PIO_EXE (
  echo ERROR: PlatformIO was not found.
  pause
  exit /b 1
)

if not exist "platformio.ini" goto :wrong_folder
findstr /I /L /C:"[env:t190_sender]" "platformio.ini" >nul
if errorlevel 1 goto :wrong_folder

set "PIO_ENV=t190_sender"

echo.
echo ============================================================
echo  PyrotechnicFX T190 TRANSMITTER Full Flash v1.6.3
echo ============================================================
echo Project:      %CD%
echo Environment:  %PIO_ENV%
echo.
"%PIO_EXE%" device list

echo.
set /p "UPLOAD_PORT=Enter COM port, for example COM6, or press ENTER for auto-detect: "
set "PORT_ARG="
if defined UPLOAD_PORT set "PORT_ARG=--upload-port %UPLOAD_PORT%"

echo.
echo STEP 1 OF 3: Building transmitter firmware...
"%PIO_EXE%" run -e %PIO_ENV%
if errorlevel 1 goto :failed

echo.
echo STEP 2 OF 3: Erasing the ESP32-S3 flash...
echo Close all serial monitors first.
"%PIO_EXE%" run -e %PIO_ENV% -t erase %PORT_ARG%
if errorlevel 1 goto :failed

echo.
echo STEP 3 OF 3: Uploading bootloader, partitions, and application...
"%PIO_EXE%" run -e %PIO_ENV% -t upload %PORT_ARG%
if errorlevel 1 goto :failed

echo.
echo ============================================================
echo  TRANSMITTER FULL FLASH COMPLETED SUCCESSFULLY
echo ============================================================
echo.
set /p "OPEN_MONITOR=Open serial monitor at 115200 baud? [Y/N]: "
if /I "%OPEN_MONITOR%"=="Y" (
  if defined UPLOAD_PORT (
    "%PIO_EXE%" device monitor --port "%UPLOAD_PORT%" --baud 115200
  ) else (
    "%PIO_EXE%" device monitor --baud 115200
  )
)
exit /b 0

:wrong_folder
echo.
echo ============================================================
echo  WRONG FOLDER FOR TRANSMITTER SCRIPT
echo ============================================================
echo Put this BAT directly in the PFX-TX folder that contains:
echo   platformio.ini
echo   src\main.cpp
echo.
echo Current folder: %CD%
pause
exit /b 1

:failed
echo.
echo ============================================================
echo  FLASH FAILED
echo ============================================================
echo Close the serial monitor. If connection fails, hold BOOT,
echo tap RESET, release RESET, then release BOOT as erase starts.
pause
exit /b 1
