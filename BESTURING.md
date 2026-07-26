# ECLIPSE — besturing, per actie geverifieerd

*Bijgewerkt 2026-07-26. Deze lijst is uit de code gelezen, niet uit het geheugen:
op één uitzondering na is elke rij een `MapKey`-regel plus een `BindAction`-regel
in `Eclipse/Source/Eclipse/Characters/EclipsePlayerController.cpp`. Staat er "ja"
in de handler-kolom, dan is er aantoonbaar een functie aan gebonden.*

*Sinds 26-07 is dit **geen erewoord meer maar een test**. Drie bewakers draaien mee
in de suite: de F2-controletabel mag geen binding claimen die niet bestaat
(`Eclipse.Feel.Input.ControlTableClaimsOnlyBindingsThatExist`), de getallen in de
testgids moeten die van `DA_CharacterTuning` zijn
(`...GuideNumbersStillMatchTheTuning`), en elk console-commando uit de tabel
onderaan moet echt bestaan (`...DocumentedConsoleCommandsExist`). Alle drie zijn
gefalsifieerd voordat ze groen bleven staan. **Voeg je hier een rij toe, dan hoort
de bijbehorende claim in die tests mee te landen** — de eerste test telt de rijen,
dus hij valt vanzelf als je het vergeet.*

*De uitzondering is **stance op toetsenbord**: LeftAlt is géén Enhanced-Input-actie
en heeft dus geen mapping en geen handler — `IssueSquadOrder` vraagt op het moment
van geven of de toets neer is. Dat werkt, maar het staat buiten het
actiesysteem, en deze lijst hoort dat te zeggen in plaats van "ja" te claimen.
Alle overige rijen zijn deze ronde één voor één tegen de `MapKey`-regels
gecontroleerd; twee klopten niet en zijn gecorrigeerd (R3 en deze).*

## Veld (buiten Command Mode)

| Actie | Muis/toetsenbord | Controller | Handler |
|---|---|---|---|
| Lopen | W A S D | Linkerstick | ja — `HandleMove` |
| Rondkijken | Muis | Rechterstick | ja — `HandleLook` |
| Vuren | Linkermuisknop | RT | ja — `HandleFire` |
| Mikken (ADS) | Rechtermuisknop | LT | ja — `HandleAimStart` / `HandleAimStop` |
| Sprint | Shift (**vasthouden**) | L3 (**togglen**) | ja — `HandleSprintHold` / `HandleSprintToggle` |
| Hurken | Ctrl | B | 150 cm/s en je zakt 52 cm (van 176 naar 124 cm). Twee dingen zijn hier recent gerepareerd: de toets was dood (`bCanCrouch` stond uit) en je zakte daarna veel te diep — 80 cm is kruiphoogte, niet hurkhoogte |
| Springen | Spatie | A | ja — `HandleJump` |
| 1e/3e persoon | C | **geen** — R3 is er bewust af | ja — `HandleToggleView` (alleen op C) |

> **Wandelen bestaat alleen op de controller** (gemeten 26-07). `WalkSpeed` (180 cm/s)
> wordt alleen door de ANIMATIE gelezen — als ankerpunt voor de loop/ren-blend — en
> het bewegingscomponent mikt altijd op `RunSpeed`. Met een stick haal je elke
> tussensnelheid (de respons is lineair: 40% uitslag = 40% snelheid, dus 180 zit op
> ~43%), maar WASD zijn digitale toetsen en geven altijd 1,0. **Op toetsenbord loop
> je dus altijd 420.** Geen defect, wel een apparaatverschil; een wandeltoets zou
> een bindingskeuze zijn en Alt is bezet door stance.

## Command Mode (wereld naar 30%)

| Actie | Muis/toetsenbord | Controller | Handler |
|---|---|---|---|
| Activeren | Q vasthouden | LB vasthouden | ja — `CommandMode->OnHoldPressed/Released` |
| Volgende soldaat | Tab / scroll omhoog | RB **tijdens de hold** (erbuiten: WAPENWISSEL) | ja — `CycleSoldierSelection(+1)` |
| Vorige soldaat | scroll omlaag | *(pad: geen — RB wrapt rond)* | ja op muis; **de LT-tak is 26-07 verwijderd**, zie hieronder |
| Soldaat onder richtkruis | E | X | ja — `PickSoldierUnderReticle` |
| Orders 1–4 | 1 2 3 4 | D-pad ↑ → ↓ ← | ja — `IssueSquadOrder` |
| Stance | Alt **ingedrukt houden terwijl je de order geeft** | Y (togglen, **alleen tijdens Command Mode**) | pad: `ToggleHeldStance` · toetsenbord: **geen actie en geen binding** — `IssueSquadOrder` pollt `IsInputKeyDown(LeftAlt)` op het moment van geven. **LET OP (26-07): de stance wordt vandaag alleen ONTHOUDEN en getoond — de soldaat schrijft hem op (`CurrentStance`) en leest hem daarna nergens, dus je squad vecht er niet anders door.** Fase-1-placeholder, eerlijk zo gelabeld in de code; wanneer de gedragssplitsing komt is een owner-keuze |


> **Dit stond hier tot 26-07:** vier controls (stance, volgende, vorige,
> onder-kruis) deden buiten Command Mode stil niets. Drie ervan zijn opgelost.
> **RB** wisselt buiten de modus van WAPEN (26-07 avond). Dat was tot die avond
> het camerastandpunt; die ruil is bewust. Van wapen wisselen is een
> gevechtshandeling die je tientallen keren per missie doet, 1e/3e persoon is een
> voorkeur die je één keer zet — en een bumper is beter voor iets middenin een
> vuurgevecht, want je duim blijft op de rechterstick. Het camerastandpunt houdt
> **C** op het toetsenbord. Het genre legt de wapenwissel meestal op Y/Driehoek
> (Division, Call of Duty, Gears, Destiny), maar Y draagt hier de stance.
> **X** geeft
> HERLADEN (**R** of **X**; buiten Command Mode). Valt er niets te herladen, dan
> geeft dezelfde knop de snelle hergroepeer-order via het bestaande orderpad —
> geen dode knop, ook niet als het magazijn vol is. **LT** is geen
> moduskeuze meer (zie hieronder). **Y (stance) blijft dood**, met opzet: stance
> verandert vandaag alleen de HUD-regel, en er iets anders op zetten zou
> verbergen dat de stance zelf nog niet af is.

**LT is sinds 26-07 altijd mikken.** Hij was buiten de modus mikken en erbinnen
"vorige soldaat". Zo'n overlading op een TRIGGER is in dit genre ongebruikelijk,
en daar is een reden voor: een trigger heeft een analoge slag, dus je drukt hem
half per ongeluk. Division en Gears houden de triggers heilig (mikken en vuren)
en zetten moduskeuzes op de bumpers en het d-pad.

Selectie cycelt daarom nog maar één kant op, met **RB**, en wrapt rond. Bij vier
soldaten is dat geen verlies — drie keer RB is hetzelfde als één keer terug. Het
muiswiel blijft beide richtingen doen, want een wiel is niet dubbelzinnig. De
resterende contexttakken splitsen op `CommandMode->IsHeld()`; dat is géén tweede
modus-systeem — SPEC-P2-07 bezit de Enhanced Input context stacks — maar één tak
in elke handler die de toestand leest die er toch al is.

## Testgids en debug-overlay

| Actie | Muis/toetsenbord | Controller | Handler |
|---|---|---|---|
| Testgids openen/sluiten | F3 | View-knop | ja — `ToggleGuidePanel` |
| Gehaald / goed / ja | J | Menu-knop | ja — `ConfirmGuideStep` |
| Sla over / niet goed / nee | N | — | ja — `SkipGuideStep` |
| Controls-overzicht | F2 | — | ja — `ToggleControlsPanel` |
| Feel-meting (camera/snelheid) | F9 | — | ja — `DumpFeelState` (ook `Eclipse.Feel.Dump`) |
| 13.2-vragenpaneel | H | — | ja — `TogglePlaytestPanel` |
| Gauntlet-metingen | F4–F8, 6–0 | — | ja — per functie |

> **Wat elke gauntlet-toets doet, want drie van de vijf R3-criteria vullen zichzelf
> NIET.** `F4` = deze pick was schoon · `F5` = dit was een mis-pick · `F6` =
> dilatatie-comfort goed/slecht · `F7` = vertrouwen goed/slecht · `F8` = markeer
> het einde van een gevechtsbeat. Zonder die toetsen blijft criterium 2
> (targeting) op *"nog niet gemeten"* staan, hoe lang je ook speelt — het telt
> alleen wat jij als schoon of mis boekt. Criterium 1 (antwoord binnen 1 s) en 5
> (gebruiks-trek) vullen zich wél vanzelf.
>
> **En het playtest-paneel (`H`) beantwoord je met `6` t/m `0`** — één toets per
> vraag, in de volgorde waarin ze op het scherm staan. Sinds 26-07 staat de toets
> vóór elke regel in het paneel zelf, zodat je het niet hoeft te onthouden.
>
> **En weet dit voordat je het verdict velt: criterium 1 (antwoord binnen 1 s) kan
> vandaag niet falen.** De slechtst gemeten order-round-trip is 0,000 s, want de
> order wordt gegeven en beantwoord binnen hetzelfde frame — beide tijdstempels
> staan in dezelfde functie-aanroep. Als bewaker is dat waardevol (hij gaat rood
> zodra iemand het orderpad asynchroon maakt), maar als bewijs dat het antwoord
> goed *voelt* zegt hij niets. Dat kun je pas beoordelen als je de squad hóórt
> antwoorden.

De gauntlet-meettoetsen blijven bewust toetsenbord-only: dat is instrumentatie voor
de beoordelaar, geen besturing die de speler uitvoert. De gids zelf moest wél op de
controller, anders is een controller-playtest er niet mee te doen — View en Menu
waren de enige onbezette pad-knoppen.

## Hold of toggle — de regel, en waarom hij per apparaat verschilt

*Besluit 2026-07-25, na de owner-eis "sprint wordt een toggle op L3" en de
bredere opdracht om alle bindings hierop na te lopen.*

De vraag is niet "hold of toggle" maar **hoe lang de toestand duurt**, en pas
daarna welke knop hem draagt:

| Soort toestand | Toetsenbord | Controller | Waarom |
|---|---|---|---|
| **Momentaan** op een SCHOUDERknop (seconden, tijdens een handeling) | hold | hold | Een schouderknop houd je comfortabel vast terwijl je stuurt en richt |
| **Momentaan** op een STICKknop | hold | **toggle** | Dezelfde stick ingedrukt houden waarmee je stuurt is onhandig; op een toetsenbord kost een pinktoets niets |
| **Aanhoudend** (minuten, een houding) | toggle | toggle | Een houding die je een hele infiltratie aanhoudt, hoor je niet vast te houden |

Elke actie langs die lat:

| Actie | Duur | Knop | Toetsenbord | Controller | Uitkomst |
|---|---|---|---|---|---|
| **Sprint** | momentaan | L3 = stick | **hold** (Shift) | **toggle** (L3) | **GEWIJZIGD** — was hold op beide |
| Mikken | momentaan | LT = schouder | hold | hold | Ongewijzigd, nu expliciet besloten |
| Command Mode | momentaan | LB = schouder | hold | hold | Ongewijzigd — de tijddilatatie hángt aan de hold |
| Stance | momentaan | Y = face | hold bij het geven | toggle | Ongewijzigd; die asymmetrie was al goed en is nu onderbouwd |
| Hurken | **aanhoudend** | B = face | toggle | toggle | Ongewijzigd, en bewust: hurken is de stealth-default (GDD 04) en die houd je minuten aan |
| 1e/3e persoon | aanhoudend | C | toggle | — | Ongewijzigd |

**Uitstappen van de sprint-toggle** — zoals Borderlands / Gears / The Division:
de sprint blijft aan tot je (a) ophoudt met vooruit duwen, (b) mikt, (c) vuurt,
of (d) nogmaals L3 drukt. Schuin vooruit sturen beëindigt hem **niet** — dat is
sturen, geen stoppen. Alle vier de uitstappen staan als losse assert in
`Eclipse.Feel.Input.SprintHoldsOnKeyboardAndTogglesOnPad`.

**Wat op de owner wacht:** hurken óók als hold aanbieden, als optie naast de
toggle. Dat vraagt een instellingenmenu, en dat is SPEC-P2-07.

## De mapping tegen de genre-conventie (owner-vraag 2026-07-25)

| Knop | Conventie (Borderlands/Gears/Division/Mass Effect) | ECLIPSE | Afwijking en reden |
|---|---|---|---|
| RT | vuren | vuren | — |
| LT | mikken | **altijd mikken** | 26-07: de overlading met "vorige soldaat" is weg. Een moduskeuze op een trigger is in dit genre ongebruikelijk — Division en Gears houden de triggers heilig, want een analoge slag druk je half per ongeluk. Selectie cycelt nu één kant op met RB en wrapt rond |
| A | springen | springen | — |
| B | hurken | hurken | — |
| X | herladen / interact | soldaat onder richtkruis | **Afwijking.** Herladen bestaat niet in dit project; X is de dichtstbijzijnde "interact met wat je aanwijst" |
| Y | wapen/gadget wisselen | stance togglen | **Afwijking.** Wapenwissel bestaat niet; stance is de gadget-achtige modifier die we wél hebben |
| LB | wapen wisselen | **Command Mode vasthouden** | **Bewuste afwijking, jouw vraag.** Zie hieronder |
| RB | wapen wisselen | volgende soldaat | Volgt LB: tijdens de hold is dit de natuurlijke buur |
| L3 | sprint | sprint | — |
| R3 | melee of camera | **niets** | Was 1e/3e persoon; eraf omdat je hem per ongeluk raakt met je richtstick |
| D-pad | snelacties | orders 1–4 | — |
| View | kaart / scorebord | testgids | Debug-tier; een kaart bestaat nog niet |
| Menu | pauze | gids bevestigen | Debug-tier; pauze bestaat nog niet |

**LB blijft Command Mode, en dat is een ontwerpkeuze, geen luiheid.** Drie redenen.
Command Mode is de kernmechaniek van ECLIPSE — het is niet één actie tussen andere,
het is de reden dat dit een action-*strategy* game is. Het is bovendien een HOLD, en
een hold van seconden hoort op een schouderknop die je comfortabel ingedrukt houdt;
op een face-button vecht hij met de stick waarmee je in diezelfde tijd moet richten.
En de conflicterende conventie is vandaag theoretisch: **wapenwissel bestaat niet in
dit project** — geen actie, geen handler, geen wapensysteem. Zodra dat er wel is, is
de eerlijke plek daarvoor RB/LB *buiten* de hold, met dezelfde contextsplitsing die
LT nu al doet.

**Wat de conventie zegt en wij niet hebben:** herladen, melee, wapenwissel, kaart,
pauze. Die staan hieronder als gat, niet als afwijking.

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
| `Eclipse.Feel.Dump` | **drie regels op F9**: (1) snelheid, mesh-schaal, boomlengte, camera-afstand, FOV en schijnbare grootte; (2) de bewegingswaarden zoals ze op het component staan — loopsnelheid, aanloop, rem, draaisnelheid, grondwrijving, zijwaarts/achteruit, stap-hoogte, sprong; (3) de kijkwaarden. Ook op het scherm. **Sinds 26-07 ook een VIERDE regel: het verschil met je vorige druk** — snelheid, schijnbare grootte in procenten, camera-afstand. Groen binnen de ruis, rood met "SCHAALT MEE — dit is S1" boven 2%. Dat is er omdat S1 een *verschil* is en geen absoluut getal: je drukt één keer stappend en één keer sprintend, en de dump rekent het zelf uit in plaats van jou getallen te laten onthouden terwijl je speelt. De drempel van 2% is dezelfde die de geautomatiseerde test gebruikt |
| `Eclipse.Input.ForceGamepad 0/1` | invoer als muis (0) of als stick (1) behandelen; -1 = autodetectie |

## Welke config-map een toetsbinding leest (feel-audit S3)

De owner bond F9 aan `Eclipse.Feel.Dump` via `+DebugExecBindings` in
`Saved/Config/WindowsEditor/Input.ini`, en er kwam nooit een regel in het log.

**`Saved/Config/` is de GEGENEREERDE configlaag.** Die is van de engine: hij
staat niet in de repo, hij reist niet mee naar een andere machine, en de engine
schrijft hem bij afsluiten terug. Een regel die je daar met de hand inzet,
overleeft dat niet betrouwbaar. De duurzame plek is
**`Eclipse/Config/DefaultInput.ini`**, en die staat wél in de repo.

Welke platformmap een run leest: de naam komt uit `FPlatformProperties`, en die
is **`WindowsEditor` voor elke run van `UnrealEditor.exe`** — óók met `-game`,
want de binary is de editor. Een gepackagede build leest `Windows`. Beide lezen
`Config/DefaultInput.ini` als onderliggende laag, dus daar werkt het in alle
gevallen.

Drie dingen zijn nu geregeld, van sterk naar zwak:
1. **F9 is een echte Enhanced-Input-binding in code** en heeft dus helemaal geen
   configlaag nodig. Dat is het antwoord; de rest is vangnet.
2. `+DebugExecBindings` voor F9/F10 staan in `Config/DefaultInput.ini`.
3. Bij missiestart logt de game **hoeveel debug-bindings er werkelijk geladen
   zijn en welke** — zodat "binding niet geladen" en "toets niet ingedrukt" niet
   langer op elkaar lijken.
