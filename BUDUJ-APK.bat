@echo off
setlocal
cd /d "%~dp0"
title Akwarium LED NATIVE - budowanie APK
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0BUDUJ-APK.ps1" %*
echo.
echo Nacisnij dowolny klawisz, aby zamknac to okno.
pause >nul
