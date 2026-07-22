@echo off
REM ============================================================
REM  ECLIPSE - start_fable.bat
REM  Dubbelklik = Claude Code start op het JUISTE (Max / Fable 5)
REM  account via de secrets-vault config-dir, los van een
REM  eventueel eigen Claude-account op deze PC.
REM
REM  Zet dit bestand op de nieuwe PC in C:\Dev\ECLIPSE_SECRETS\
REM  (naast de map fable-config\). Het reist ook mee in de repo
REM  als canonieke bron - kopieer het vandaar naar de vault.
REM ============================================================

title ECLIPSE - Fable 5 (Max)

REM --- De vault-config met de Max-login (bevat .credentials.json) ---
set "CLAUDE_CONFIG_DIR=C:\Dev\ECLIPSE_SECRETS\fable-config"

REM --- 1) Bestaat de vault-config-map? ---
if not exist "%CLAUDE_CONFIG_DIR%\" (
    echo [FOUT] Config-map niet gevonden:
    echo        %CLAUDE_CONFIG_DIR%
    echo.
    echo Kopieer eerst de map ECLIPSE_SECRETS van de USB naar C:\Dev\
    echo zodat het pad hierboven bestaat, en probeer opnieuw.
    echo.
    pause
    exit /b 1
)

REM --- 2) Staat de login (.credentials.json) in de vault? ---
if not exist "%CLAUDE_CONFIG_DIR%\.credentials.json" (
    echo [LET OP] .credentials.json ontbreekt in de config-map:
    echo          %CLAUDE_CONFIG_DIR%\.credentials.json
    echo.
    echo Kopieer .credentials.json van de USB hierheen
    echo    ECLIPSE_SECRETS\fable-config\.credentials.json
    echo en start dit bestand opnieuw. Zonder dit bestand log je
    echo NIET automatisch in op het Max/Fable-account.
    echo.
    pause
    exit /b 1
)

REM --- 3) Ga naar de repo als die er is, anders naar C:\ ---
if exist "C:\Dev\ECLIPSE_GDD\" (
    cd /d "C:\Dev\ECLIPSE_GDD"
) else (
    cd /d "C:\"
)

echo Config-dir : %CLAUDE_CONFIG_DIR%
echo Werkmap    : %CD%
echo.
echo Start Claude Code op Fable 5 (max, skip-permissions)...
echo Test na de start: stel een simpele vraag en typ  /model  -^> verwacht Fable 5.
echo.

REM --- 4) Start Claude Code op het juiste account ---
call claude --model fable --effort max --dangerously-skip-permissions

REM --- Venster openhouden bij afsluiten of fout ---
echo.
echo (Claude Code is afgesloten. Zie je hierboven "claude is not recognized"?
echo  Installeer dan Node.js LTS en:  npm i -g @anthropic-ai/claude-code )
pause
