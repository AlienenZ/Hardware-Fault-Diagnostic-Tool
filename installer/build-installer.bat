@echo off
chcp 65001 >nul 2>&1
title HW-Diag Installer Builder
cd /d "%~dp0"

:: -- Check admin rights --
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo  [!!] Requesting admin privileges...
    powershell -NoProfile -Command "Start-Process -FilePath '%~f0' -Verb RunAs"
    exit /b 0
)

:: -- Run build script --
cd /d "%~dp0"
powershell -ExecutionPolicy Bypass -NoProfile -File "build-installer.ps1"

echo.
echo  Press any key to close...
pause >nul
