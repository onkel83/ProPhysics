@echo off
setlocal
chcp 65001 >nul
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0run_alpha_tests.ps1" %*
exit /b %ERRORLEVEL%