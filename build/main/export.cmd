@echo off
setlocal
chcp 65001 >nul
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0export.ps1" %*
exit /b %ERRORLEVEL%