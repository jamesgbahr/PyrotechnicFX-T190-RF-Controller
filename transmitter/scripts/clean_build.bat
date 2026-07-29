@echo off
setlocal EnableExtensions
cd /d "%~dp0.."

set "PIO=%USERPROFILE%\.platformio\penv\Scripts\platformio.exe"
set "PIOPY=%USERPROFILE%\.platformio\penv\Scripts\python.exe"

if not exist "%PIO%" (
  where pio >nul 2>nul
  if errorlevel 1 (
    echo PlatformIO Core was not found.
    echo Enable or reinstall the PlatformIO IDE extension, reload VS Code, and retry.
    pause
    exit /b 1
  )
  set "PIO=pio"
)

if not exist "%PIOPY%" (
  echo PlatformIO's Python environment was not found:
  echo   %PIOPY%
  echo Reinstall the PlatformIO IDE extension, then retry.
  pause
  exit /b 1
)

set "PFX_WORKSPACE=%USERPROFILE%\.pfxpio\tx"

echo Removing the previous short workspace:
echo   %PFX_WORKSPACE%
if exist "%PFX_WORKSPACE%" rmdir /s /q "%PFX_WORKSPACE%"

echo Removing incomplete package downloads...
if exist "%USERPROFILE%\.platformio\.cache\tmp" rmdir /s /q "%USERPROFILE%\.platformio\.cache\tmp"

echo Clearing the PlatformIO download cache...
"%PIO%" system prune --cache --force
if errorlevel 1 goto :fail

echo Checking esptool Python dependencies...
"%PIOPY%" -c "import intelhex, rich_click" >nul 2>nul
if errorlevel 1 (
  echo Repairing missing intelhex/rich-click modules in PlatformIO's Python environment...
  "%PIOPY%" -m pip install --disable-pip-version-check --no-input "intelhex==2.3.0" "rich-click<2"
  if errorlevel 1 goto :fail
) else (
  echo esptool Python dependencies are already installed.
)

echo Installing dependencies and building sender firmware...
"%PIO%" run -e t190_sender
if errorlevel 1 goto :fail

echo.
echo BUILD COMPLETE.
echo Firmware:
echo   %PFX_WORKSPACE%\build\t190_sender\firmware.bin
pause
exit /b 0

:fail
echo.
echo BUILD FAILED. Copy the complete output and send it back.
pause
exit /b 1
