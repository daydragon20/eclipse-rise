@echo off
title ECLIPSE - live dashboard
cd /d "%~dp0"

echo ============================================================
echo   ECLIPSE - live dashboard
echo   http://127.0.0.1:8377/
echo ============================================================
echo.
echo   Dit venster moet OPEN blijven staan.
echo   Sluiten = dashboard uit.
echo.

start "" http://127.0.0.1:8377/

where py >nul 2>nul
if %errorlevel%==0 (
    py -3 Tools\eclipse_dashboard.py
) else (
    python Tools\eclipse_dashboard.py
)

echo.
echo Dashboard gestopt. Druk op een toets om te sluiten.
pause >nul
