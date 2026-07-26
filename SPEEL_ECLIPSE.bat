@echo off
setlocal
chcp 65001 >nul 2>&1
title ECLIPSE - Kessara Underworks

set ENGINE="C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
set PROJECT="C:\Dev\ECLIPSE_GDD\Eclipse\Eclipse.uproject"
set MAP=/Game/Maps/GrayboxDistrict

cls
echo ===============================================================================
echo                  E C L I P S E   -   Kessara Underworks
echo ===============================================================================
echo.
echo   JE START DIRECT IN DE MISSIE - meteen bewegen, geen menu
echo.
echo   Deze snelkoppeling slaat de basis-hub over:
echo     -EclipseStartMission=TransitCheckpoint   ^<- direct de missie in
echo     Eclipse.Guide.Overlay 1                  ^<- testgids staat al open
echo.
echo   Je muis verdwijnt meteen en je personage werkt vanaf de eerste seconde.
echo   Zie je een CROSSHAIR? Goed - je zit in het veld.
echo   Zie je een HANDJE? Dan is de snelstart niet gelukt en zit je in de basis:
echo     COMMAND-tab -^> ADVANCE DAY -^> klik een MISSIE-OFFER aan.
echo.
echo   Testgids niet zichtbaar? Druk F3.
echo.
echo ===============================================================================
echo   BESTURING - MUIS + TOETSENBORD              ^|  CONTROLLER (Xbox)
echo ===============================================================================
echo   Lopen ................ W A S D              ^|  Linkerstick
echo   Rondkijken ........... Muis                 ^|  Rechterstick
echo   Vuren ................ Linkermuisknop       ^|  RT
echo   MIKKEN ............... Rechtermuisknop      ^|  LT
echo   SPRINGEN ............. Spatie               ^|  A
echo   Sprint ............... Shift VASTHOUDEN     ^|  L3 = AAN/UIT (toggle)
echo   Hurken ............... Ctrl (je zakt 52 cm) ^|  B
echo   1e/3e persoon wisselen  C                   ^|  (geen - RB draagt nu de wissel)
echo   WAPEN WISSELEN ....... (geen)               ^|  RB (buiten Command Mode)
echo   HERLADEN ............. R                    ^|  X  (buiten Command Mode)
echo   Squad hergroeperen ... E                    ^|  X  (als het magazijn vol is)
echo.
echo   Sprint verschilt per apparaat en dat is met opzet: op toetsenbord houd je
echo   Shift vast, op de pad zet L3 hem AAN en blijft hij aan. Hij gaat vanzelf uit
echo   bij loslaten van de stick, mikken, hurken of geen voorwaartse invoer meer.
echo.
echo   F9 ....... toont de bewegingswaarden op het scherm (en wat er sinds de
echo              vorige druk veranderd is). Hiermee controleer je de schaal-bug:
echo              blijf rennen en kijk of er iets meeschaalt met je snelheid.
echo.
echo   TESTGIDS:  F3 openen  ^|  J = gehaald/ja  ^|  N = overslaan/nee
echo   Op de pad: View-knop (links van Xbox-logo) opent de gids,
echo              Menu-knop (rechts ervan) = gehaald/ja
echo.
echo   ---------------------------------------------------------------------------
echo   IN-GAME TESTGIDS - laat de game je alles leren
echo   ---------------------------------------------------------------------------
echo   F3 ....... testgids openen/sluiten
echo   J ........ gelezen / goed / ja
echo   N ........ sla over / niet goed / nee
echo.
echo   DE GIDS IS SINDS 26-07 KORT. Hij bevat nog maar wat IK NIET KAN METEN:
echo   wat er veranderd is sinds je vorige sessie, twee oordelen, en de vijf
echo   13.2-vragen. Van de 23 stappen waren er 17 dingen die de dev zelf had
echo   moeten vaststellen - die zijn eruit, en de suite bewaakt ze nu.
echo   Niets vinkt zichzelf meer af: elke stap is een oordeel van jou.
echo   Aan het eind schrijft hij je verdict naar Saved/Logs.
echo.
echo   COMMAND MODE (wereld naar 30%%):
echo   Activeren ............ Q VASTHOUDEN         ^|  LB VASTHOUDEN
echo   Volgende soldaat ..... Tab / scroll omhoog  ^|  RB
echo   Vorige soldaat ....... scroll omlaag        ^|  (pad: RB wrapt rond)
echo   Soldaat onder kruis .. E                    ^|  X
echo   Orders geven ......... 1 2 3 4              ^|  D-pad
echo   DOCTRINE ............. Alt vasthouden       ^|  Y (cyclen)
echo.
echo   Die regels hierboven gelden TERWIJL je Q/LB vasthoudt. Buiten Command Mode
echo   doen RB en X iets anders (zie boven); LT is buiten en binnen gewoon MIKKEN.
echo   DOCTRINE VERANDERT SINDS 26-07 ECHT GEDRAG, en geldt meteen voor de hele
echo   squad - hij reist niet meer mee met een order. Y cycelt door vier kaders:
echo     recon ...... vuurt NIET tot er op hem geschoten wordt
echo     ready ...... de volle basis: vuurt, zoekt dekking, loopt mee
echo     overwatch .. blijft staan waar hij staat, vuurt vrij
echo     aggressive . zoekt GEEN dekking, sluit af op wie er schoot
echo   Welke actief is staat op de HUD, ook buiten Command Mode.
echo.
echo   Muis vrijgeven ....... Shift+F1
echo   Afsluiten ............ Esc
echo.
echo   Muis en controller werken tegelijk - gewoon oppakken, niets omzetten.
echo.
echo ===============================================================================
echo   CHECK 1 - FEEL-GAUNTLET (20 min) - het R3-verdict
echo ===============================================================================
echo   Wees streng. Een gefaald punt = verdict "false". Dat is geen mislukking,
echo   daar is deze test voor.
echo.
echo   [ ] 1. ORDER-ROUND-TRIP
echo          10 orders achter elkaar in Command Mode.
echo          Antwoord binnen 1 seconde ECHTE tijd?   Doel: 10/10
echo.
echo   [ ] 2. TARGETING
echo          10 orders onder vuur, op een BEPAALDE soldaat met een BEPAALD doel.
echo          Landt hij in een keer bij de juiste?    Doel: minstens 9/10
echo.
echo   [ ] 3. COMFORT
echo          Speel 3 gevechtsmomenten. Voelt instappen als SNELLER DENKEN
echo          of als een MENU OPENEN? Loslaten moet schoon hervatten.
echo.
echo   [ ] 4. VERTROUWEN
echo          Geen enkele STILLE orderfout tijdens de vertraging.
echo          Krijgt elk order hoorbaar of zichtbaar antwoord?
echo.
echo   [ ] 5. GEBRUIKS-TREK
echo          Speel daarna vrij. Stap je UIT JEZELF minstens 1x per gevecht
echo          de mode in? Een mode die je niet gebruikt, is een gefaalde mode.
echo.
echo   Console-commando voor je notities:  Eclipse.Command.Dump
echo.
echo   ZEG DAARNA:  "R3-verdict: true"  of  "R3-verdict: false"  + wat je zag.
echo.
echo ===============================================================================
echo   CHECK 2 - PLAYTEST 13.2 (30 min) - sluit Fase 1 af
echo ===============================================================================
echo   Speel de hele lus: missie - squad - gevecht - basis - volgende missie.
echo   Losse zinnen volstaan, geen analyse.
echo.
echo   [ ] Wat voelde GOED? Waar dacht je "dit is leuk"?
echo   [ ] Wat voelde SLECHT of traag? Waar verloor je aandacht?
echo   [ ] Wist je steeds wat je moest doen?
echo   [ ] Deed je squad wat je vroeg?
echo   [ ] DE GATE-VRAAG: wil je vrijwillig een tweede ronde spelen?
echo.
echo ===============================================================================
echo.
echo   Druk Enter - je landt meteen in de missie met de testgids open.
echo.
pause

echo.
echo   Starten (snelstart: direct de missie in, gids open)...
%ENGINE% %PROJECT% %MAP% -game -windowed -resx=1920 -resy=1080 -EclipseStartMission=TransitCheckpoint -ExecCmds="Eclipse.Guide.Overlay 1"

echo.
echo ===============================================================================
echo   Game afgesloten. Vergeet je verdict en notities niet door te geven.
echo ===============================================================================
pause
endlocal
