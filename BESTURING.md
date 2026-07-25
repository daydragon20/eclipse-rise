# ECLIPSE — besturing, per actie geverifieerd

*Bijgewerkt 2026-07-25. Deze lijst is uit de code gelezen, niet uit het geheugen:
elke rij is een `MapKey`-regel plus een `BindAction`-regel in
`Eclipse/Source/Eclipse/Characters/EclipsePlayerController.cpp`. Staat er "ja" in
de handler-kolom, dan is er aantoonbaar een functie aan gebonden.*

## Veld (buiten Command Mode)

| Actie | Muis/toetsenbord | Controller | Handler |
|---|---|---|---|
| Lopen | W A S D | Linkerstick | ja — `HandleMove` |
| Rondkijken | Muis | Rechterstick | ja — `HandleLook` |
| Vuren | Linkermuisknop | RT | ja — `HandleFire` |
| Mikken (ADS) | Rechtermuisknop | LT | ja — `HandleAimStart` / `HandleAimStop` |
| Sprint | Shift | L3 (linkerstick indrukken) | ja — `HandleSprint` |
| Hurken | Ctrl | B | ja — `HandleCrouch` |
| Springen | Spatie | A | ja — `HandleJump` |
| 1e/3e persoon | C | R3 (rechterstick indrukken) | ja — `HandleToggleView` |

## Command Mode (wereld naar 30%)

| Actie | Muis/toetsenbord | Controller | Handler |
|---|---|---|---|
| Activeren | Q vasthouden | LB vasthouden | ja — `CommandMode->OnHoldPressed/Released` |
| Volgende soldaat | Tab / scroll omhoog | RB | ja — `CycleSoldierSelection(+1)` |
| Vorige soldaat | scroll omlaag | LT **tijdens de hold** | ja — `CycleSoldierSelection(-1)` |
| Soldaat onder richtkruis | E | X | ja — `PickSoldierUnderReticle` |
| Orders 1–4 | 1 2 3 4 | D-pad ↑ → ↓ ← | ja — `IssueSquadOrder` |
| Stance | Alt (bij het geven) | Y (togglen) | ja — `ToggleHeldStance` |

**LT doet twee dingen en dat is bewust.** Buiten Command Mode is LT mikken (de
genre-conventie); tijdens de Q/LB-hold is LT "vorige soldaat". Beide handlers
splitsen op `CommandMode->IsHeld()`. Dat is géén tweede modus-systeem — SPEC-P2-07
bezit de Enhanced Input context stacks — maar één tak in elke handler die de
toestand leest die er toch al is.

## Testgids en debug-overlay

| Actie | Muis/toetsenbord | Controller | Handler |
|---|---|---|---|
| Testgids openen/sluiten | F3 | View-knop | ja — `ToggleGuidePanel` |
| Gehaald / goed / ja | J | Menu-knop | ja — `ConfirmGuideStep` |
| Sla over / niet goed / nee | N | — | ja — `SkipGuideStep` |
| Controls-overzicht | F2 | — | ja — `ToggleControlsPanel` |
| 13.2-vragenpaneel | H | — | ja — `TogglePlaytestPanel` |
| Gauntlet-metingen | F4–F8, 6–0 | — | ja — per functie |

De gauntlet-meettoetsen blijven bewust toetsenbord-only: dat is instrumentatie voor
de beoordelaar, geen besturing die de speler uitvoert. De gids zelf moest wél op de
controller, anders is een controller-playtest er niet mee te doen — View en Menu
waren de enige onbezette pad-knoppen.

## Bestaat niet (en dat is geen defect maar een gat)

Melee, wapenwissel en herladen bestaan nergens in het project — geen actie, geen
handler, geen mapping. Ze staan hier zodat de lijst compleet is en niemand ze zoekt.

## Startopties

| Optie | Wat het doet |
|---|---|
| `-EclipseStartMission=TransitCheckpoint` | slaat de basis-hub over en landt direct in de missie |
| `-ExecCmds="Eclipse.Guide.Overlay 1"` | opent de testgids meteen (werkt in elke volgorde sinds de CVar-sink) |
| `-EclipseShot` | vaste-camera reviewronde; onderdrukt bewust ALLE debug-UI |
| `Eclipse.Look.InvertY 0/1` | Y-as van het kijken forceren (-1 = volg de tuning) |
| `Eclipse.Command.Dump` | Command Mode-metingen naar de console |
