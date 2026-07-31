@echo off
title ECLIPSE - autonome loop HERVATTEN
cd /d "%~dp0"

echo ============================================================
echo   AUTONOME LOOP WEER AANZETTEN
echo ============================================================
echo.
echo   Let op: hierna stopt Claude NIET meer uit zichzelf.
echo   Bij elke poging om te stoppen wordt hij teruggestuurd
echo   naar de volgende taak uit de backlog.
echo.

set /p bevestig="Weet je het zeker? (j/n): "
if /i not "%bevestig%"=="j" (
    echo.
    echo   Geannuleerd - de rem blijft erop.
    echo.
    pause
    exit /b 0
)

if exist ".claude\STOP" del /q ".claude\STOP"

if exist ".claude\STOP" (
    echo.
    echo   [FOUT] Kon .claude\STOP niet verwijderen.
) else (
    echo.
    echo   [OK] Autonome loop staat weer aan.
    echo   Stoppen? Dubbelklik STOP-autonomie.bat
)

echo.
pause
