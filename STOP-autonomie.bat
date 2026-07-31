@echo off
title ECLIPSE - autonome loop STOPPEN
cd /d "%~dp0"

echo ============================================================
echo   AUTONOME LOOP STOPPEN
echo ============================================================
echo.

> ".claude\STOP" echo Autonome loop uitgezet door Nathan op %date% %time%

if exist ".claude\STOP" (
    echo   [OK] De rem staat erop.
    echo.
    echo   Claude mag nu gewoon stoppen als hij klaar is met een taak.
    echo   Een sessie die NU bezig is, stopt zodra hij zijn huidige
    echo   stap heeft afgerond - niet middenin.
    echo.
    echo   Weer aanzetten? Dubbelklik HERVAT-autonomie.bat
) else (
    echo   [FOUT] Kon .claude\STOP niet aanmaken.
    echo   Bestaat de map .claude wel?
)

echo.
pause
