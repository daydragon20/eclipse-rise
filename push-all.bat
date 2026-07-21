@echo off
REM ECLIPSE — double-click this to commit + push all current work to GitHub.
REM Calls push-all.ps1 in this same folder (portable).
powershell -ExecutionPolicy Bypass -NoProfile -File "%~dp0push-all.ps1"
echo.
pause
