# ECLIPSE — EXECUTION PLAN (NU → RELEASE)
*Werkdocument game-planner | aangemaakt 2026-07-23 | **laatst bijgewerkt 2026-07-31 ~19:30**: §2 volledig geijkt tegen de werkboom (de vijf "in flight"-sporen van 24-07 waren alle vijf geland), §1b/§1c HUD-nulmetingen erbij, T-9 rechtgezet op `19_voice_production.md` §19.1 | risk-first per 13.1*
*Bronnen: 13_roadmap.md, SPEC-P2-00, 14_ai_dev_instructions.md (14.4/14.5), 12_technical_design.md, HANDOFF.md "LAATSTE STAND".*

**Status:** Phase 2 — Vertical Slice "Thirteen Bullets" is per expliciete owner-instructie (2026-07-23) de **actieve milestone**. De 13.2-gatevraag van Phase 1 ("speelt een tester vrijwillig een tweede loop?") blijft open als **staande owner-actie** (§4, T-1) — het antwoord kan de fidelity-investering nog bijsturen, maar blokkeert de Phase-2-systemenbouw niet.

**Leesregels:** bouwvolgorde per spec volgt altijd 14.5 (dataschema → pure-logic core + unit tests → subsystem-wrapper + events → debug-UI → echte UI/content laatst). Elke taak eindigt build-groen. Bouwer: **main** = hoofdagent (architectuur/systemen/specs, seams per 14.6), **eb** = element-builder(s), parallel spawnbaar, één element per agent.

---

## 1. Genummerde backlog — NU tot release

### Fase 2 — VERTICAL SLICE "Thirteen Bullets" (actief; volgorde = SPEC-P2-00 §Spec set)

| # | Item | Doel (één zin) | GDD-ref | Afhankelijk van | Bouwer |
|---|---|---|---|---|---|
| 1 | Milestone-administratie | ACTIVE_MILESTONE-blok → Phase 2 flippen en HANDOFF §4/§5 + `progress_data.js`-takenlijst op de Phase-2-backlog zetten, met de 13.2-playtest expliciet als open owner-actie. | 13 (ACTIVE_MILESTONE), dashboard-workflow | owner-instructie 2026-07-23 (binnen) | main |
| 2 | SPEC-P2-01 Squad of 4 & Classes — **GELAND 2026-07-24** | Geland in `1a85b78` (review GO, 0 blockers): squad van 4, Assault/Medic/Sniper als púre data (`DT_ClassDefs`), save v2→v3 + fixtures per 14.3.6, derde validator; 38/38 ✓, validatie 0 ✓, catalog 21/21 ✓. Follow-ups m4/m6/m7 ✓ afgehandeld in `3c2f3a5`. | 4.2.2/4.2.3, 8.3 | — | main |
| 3 | Body-swap echte meshes — **materieel vervuld** | Vervuld via de DT_BodyDefs-body-pipeline (sessie 23-07): speler/squad/vijanden dragen echte meshes (Belica, RAISOR-soldiers), 5 Dominion-archetypes; Quaternius blijft reserve/bewoners. Rest-QC lift mee in de 15.8-rondes. | 12.3 (character), 15.5 | — | eb |
| 4 | SPEC-P2-02 Command Mode final feel | Command Mode van debug-orderpicker naar shipping feel: hold-to-enter, 30% dilatatie, 8.4-ordertabel, verbale refusals ("orders zijn beloftes"). *Spec gebankt `29cd549`; **Stage A GELAND** `b5aa157` (review GO) — de feel-gauntlet staat als owner-actie (§4 T-11, R3-verdict open); **Stage B pas ná verdict "true"**.* | 8.4, 4.1.2, 9.5 | #2 ✓ | main |
| 5 | SPEC-P2-03 Hollow Point walkable base | Menu-basis vervangen door de beloopbare Act-1-vault met 4 faciliteiten op de strategische klok en zichtbare groei (besluit: Intelligence Center vóór Medbay). *Spec gebankt `7194cf4`; **stap 1–2 GELAND** `deabc9f` (review GO; 5 bevindingen verplicht naar stap 3); **stap 3 GELAND** `69b5f4b` (transactie-API + `Event.Base.*` ×4 + `setup_base_data.py`) — zie §2c. **Resteert: vault-shells**, §2d.* | 5.1–5.4, 12.3 | P1-08 ✓, P1-03 ✓ | main (systeem) + eb (vault-shells) |
| 6 | SPEC-P2-04 Missies M1.1–M1.4 | De vier openingsmissies autoren op de gedeelde quest-runtime met de volle 11.1-loop en stealth-viable routes. *Spec geschreven + main-review verwerkt (**ACCEPTED**; 4 besluiten in de spec: geen prologue/recap-opening, R7-Gauntlet als bouwstap 1, nul nieuwe primitieven, Brick = Assault); **bouwstap 1 (R7-skelet) GELAND** `60014e6`, **bouwstap 2 (StoryFlags + `DT_StoryMissions`, save v4→v5 mét migratie) GELAND** `fd38933` — zie §2c.* | 2.9, 11.1, 11.4, 12.3 | #2, #4, #5, P1-05 | main (runtime/graphs) + eb (per missie-site) |
| 7 | SPEC-P2-05 Liberation-instance | Eén campagne-missietemplate instantiëren op Kessara's districtsgraaf zodat M1.3 zichtbaar regiostaat flipt en de strategische laag antwoordt. *Spec **gebankt** `cdad8f5` (open vraag 4 = schaal, besloten op Foothold/3 regio's) + **pure core GELAND** `9be3786`, bedraad via `Event.Strategy.LiberationResolved` — zie §2c.* | 11.2, 11.3-inputs | #6, P1-04 | main |
| 8 | SPEC-P2-06 Save v1 | Save v0 promoveren naar de versioned plugin met per-subsystem contracten, v0-migratie, autosaves en missie-checkpoints — vóór content-freeze zodat elke nieuwe struct serialiseerbaar landt. | 12.3 (save), 14.3.6 | #2, #5, #7 | main |
| 9 | SPEC-P2-07 UI Stack v1 | Debug-UI vervangen door de CommonUI-stack (HUD, command, basis, map, prep/debrief) met controller+M/KB-pariteit; UI is pure consumer van de bus. | 12.1, 8.8 | #4, #5, #6 | main (C++ bases) + eb (widgets/schermen) |
| 10 | SPEC-P2-08 Fidelity-district Kessara Underworks | Het slice-district aankleden tot de gelockte 15.5-revisie (Nanite-dense kits, software-Lumen, cel+ink) door de 15.8-loop tot 15.12 "AAA-ready" binnen 12.4-budgetten. | 15.5/15.8/15.12, 3.3 | #6 (spaces locked), Fab-kliks (owner, §4) | eb ×N parallel (kit-pass, verticaliteit, particles, interieurs) |
| 11 | SPEC-P2-09 Audio-infrastructuur | 16.7 adaptieve muziek (verticale lagen, Quartz, never-silent) + combat-audio-basics + squad-barks als primaire game-state-audio, TTS-placeholder. | 16.7, 16.9, 16.14, 16.4 | #4, #6; live-runs achter ElevenLabs-scopes (§4) | main (systeem) + eb (stems/barks) |
| 12 | Phase-2-testlagen compleet | Alle zes 14.4-lagen draaiend voor de slice: Gauntlet per missie, scenario-suite op 4+klassen, save-roundtrip, perf per merge, nightly soak M1.1→liberation. | 14.4, SPEC-P2-00 §Tests | groeit mee met #2–#11; CI-runner (§4) | main |
| 13 | Gate-review Phase 2 | Externe reviewers spelen de 3 uur koud; verdict "I want the rest of this game" ja/nee — bij "nice systems, but…" eerst de feel/fidelity-kloof dichten. | 13.2, SPEC-P2-00 §DoD | #1–#12 | owner + main |

### Fase 3 — EARLY BUILD "One Planet War" (na gate #13)

| # | Item | Doel | GDD-ref | Afhankelijk van | Bouwer |
|---|---|---|---|---|---|
| 14 | Kessara + Tarsis compleet | Twee volledige planeten met alle districten, occupation-states en missie-sites. | 3.3/3.4, 13.2 | #13 | eb ×N + main (World Partition/Data Layers) |
| 15 | Act 1 volledig + Act 2-skelet | M1.5–M1.8 plus de Act-2-ruggegraat op dezelfde runtime; companions voorbij Mara+Brick. | 2.9, 11 | #6, #14 | main + eb |
| 16 | Alle 9 klassen + Training Academy | Klassenroster compleet met Academy-toewijzing (het Phase-2-besluit "pre-classed" vervalt hier). | 4.2.3, 5.3 | #2, #5 | main |
| 17 | Basis-tiers 1–2 + officer/company-laag v1 | Basisgroei voorbij de 4-facility-vault en de eerste officerslaag. | 5.4, 7.x | #5, #8 | main |
| 18 | Dominion Response Tiers 0–3 + economie-vol-loop | Strategische tegenspeler (utility-planner op strategische tijd) en de volledige economieband per Part 6. | 9.x, 6.x, 12.3 | #7, #8 | main |
| 19 | Mission Generator v1 | Generator componeert dezelfde objective-primitieven als de authored missies, op beide planeten. | 11.3, 12.3 | #6, #7, #14 | main |
| 20 | Hybrid battle v1 — "Siege of Hollow Point" | Derde existentiële risico bewijzen: grond+simulator in één gevecht, front-swap ≤10 s. | 8.6, 12.4, 13.1 | #17, #18 | main |
| 21 | MetaHuman-evaluatie hero-companions | Beslissen (en zo ja pipelinen) of Mara/Brick/companions MetaHuman-gezichten krijgen. | 12.1, HANDOFF it.4 | owner-sessie (§4) | main + owner |
| 22 | Gate: 10-uurs externe playtests | Retentie meten op een ~25 u-campagne. | 13.2 Phase 3 | #14–#20 | owner |

### Fase 4 — ALPHA "The Whole War, Ugly"

| # | Item | Doel | GDD-ref | Afhankelijk van | Bouwer |
|---|---|---|---|---|---|
| 23 | Alle 10 planeten graybox→dressed | Volledige wereldruimte speelbaar (scope-ladder: 10→8 bij nood, besluit vooraf gedocumenteerd). | 3.x, 13.1 | #22 | eb ×N + main |
| 24 | Alle acts + twists/branches + tech tree compleet | Verhaal start-tot-eind speelbaar met volledige onderzoeksboom (graaf zonder orphans/cycles, unit-getest). | 2.x, 10.x, 14.4 | #15, #19 | main + eb |
| 25 | Fleet-laag | Vlootlaag erin (cut line vooraf: 2D-tactische kaart, simulator ongewijzigd; schepen nooit player-piloted). | 7.x, 12.3, 13.1 | #20 | main |
| 26 | Content-complete + maandelijkse bot-playthrough | Elke missie/template/systeem bestaat; maandelijkse volledige bot-campagne + save-integriteits-soaks draaien. | 13.2 Phase 4, 14.4 | #23–#25 | main |
| 27 | Gate: campagne uitspeelbaar zonder dev-ingreep | Volledige campagne haalbaar door de bot én een mens. | 13.2 | #26 | owner |

### Fase 5 — BETA · Fase 6 — RELEASE+POST

| # | Item | Doel | GDD-ref | Afhankelijk van | Bouwer |
|---|---|---|---|---|---|
| 28 | Content-lock + balanspassen per act | Telemetrie-gedreven balans (casualty-rates, economie-curves binnen Part-6-banden). | 13.2 Phase 5, 6.x | #27 | main |
| 29 | VO-opname + implementatie | TTS-placeholders vervangen door opgenomen VO (companions/story), barks-accentensets. | 16.4, 12.3 | #28, contract (§4) | owner (contract) + main |
| 30 | Performance min-spec + accessibility + lokalisatie | 60 fps-doelen op min-spec, accessibility compleet, EN-VO + FIGS+NL+PL+BR+RU+ZH-tekst. | 12.4, 13.2 | #28 | main + eb |
| 31 | Closed beta per act → gate crash-free ≥99,5% | Beta-programma draaien en de funnel gezond krijgen. | 13.2 Phase 5 | #28–#30 | owner + main |
| 32 | Gold + day-one patch + hotfix-discipline | Release en de launch-window bemand. | 13.2 Phase 6 | #31 | owner + main |
| 33 | Post-launch: NG+ → QoL → "Ashfall"-drop → expansie-evaluatie | Vaste post-launch-volgorde afwerken; NG+ nooit launch-blocking. | 13.1/13.2 | #32 | main + eb |

---

## 1b. HUD-NULMETING — wat er op 31-07 ECHT op het scherm staat

*Gemeten uit `EclipseMissionHudWidget`, niet uit herinnering. Spoor B prioriteit 1 is
de schermlaag; dit is het vertrekpunt zodat de bouwer met feiten begint.*

| Element uit de hud-builder-scope | Staat er nu | Opmerking |
|---|---|---|
| Vizier / crosshair | **ja** | `Crosshair`, een `+` in het midden. Geen spread, geen doelwit-info. |
| Hit-marker | **ja** | `HitMarker` + `HitMarkerSecondsLeft`; kop en romp worden onderscheiden. |
| Munitie | **deels** | `AmmoReadout` + `RefreshAmmoReadout()`: kogels in het magazijn. Geen magazijnen-over, geen herlaad-voortgang, geen leeg-staat. |
| Wapenstatus | **nee** | Geen actief wapen, vuurmodus of wisselindicatie. |
| Squad | **deels** | Rijen bestaan in de live-box (objectives + orders + Command Mode), maar niet als per-soldaat kaart met klasse/HP/status. |
| Gezondheid & stance | **nee** | Geen speler-HP, geen dekking/hurken/sprint op het scherm. |
| Minimap | **nee** | Bestaat niet. |
| Objective-marker | **deels** | Objectives staan als tekstregels, niet als richtingaanwijzer in de wereld. |

> ## ⚠️ DE HOOFDCONCLUSIE HIERONDER WAS FOUT — weerlegd 31-07, en dat is winst
>
> Hier stond: *"alles behalve het richtkruis zit achter `IsDebugHudAllowed()`, dus de
> schermlaag staat per constructie niet op een opname."* Daar is de hele eerste bouwstap
> op gebaseerd. **Het klopt niet, op twee niveaus, en allebei zijn gemeten.**
>
> **1. De poort stond helemaal niet dicht.** `FParse::Param` eist een woordgrens, dus
> `-EclipseShotPlay` — de vlag die de opnameronde gebruikt — matcht `EclipseShot` **niet**.
> `IsDebugHudAllowed()` gaf in de ronde gewoon `true`. De poort was nooit de blokkade.
>
> **2. De widgets werden nooit één keer getekend.** Gemeten in de engine-bron
> (`UserWidget.cpp:1190`): `UUserWidget::RebuildWidget()` bouwt de Slate-boom uit
> `WidgetTree->RootWidget` en draait **vóór** `NativeConstruct`. Alle drie de
> Eclipse-widgets zetten hun wortel *in* `NativeConstruct` — altijd één stap te laat, dus
> de Slate-kant bleef een lege `SSpacer`. De missie-HUD, de basis-hub én de strategiekaart
> zijn dus **nooit getekend**.
>
> Dat verklaart in één klap drie klachten die los van elkaar leken: "ik kan niet zien waar
> ik richt", "F3 doet niets", en "de HUD is niet te fotograferen".
>
> **Waarom niemand het zag, en dat is de les.** Het log meldde
> `zichtbaarheid=3 tekst='AR_Foundry 19 / 30' inViewport=1` terwijl er niets op het frame
> stond. `IsInViewport()` en `GetVisibility()` lezen de **UMG-administratie**, niet het
> scherm. Het instrument beantwoordde een andere vraag dan werd gesteld — en gaf daarbij
> het geruststellende antwoord. De tabel hierboven blijft dus geldig als inventaris van
> *wat er geconstrueerd wordt*, maar niet als antwoord op *wat je ziet*.
>
> Gerepareerd via `NativeOnInitialized` in alle drie de widgets. De poort op de
> montageplek (`EnterMissionMode` sloeg de hele widget over) is eruit: die hoort ín de
> widget, waar het verschil tussen een richtkruis en een debugregel bekend is.

## 1c. NULMETING VAN DE ANDERE TWEE HOOGTES — base en map

*Gemeten 31-07 uit `EclipseBaseHubWidget.cpp` (442 regels) en `EclipseStrategyMapWidget.cpp`
(129 regels). §1b hierboven meet **boots**; deze meet de twee hoogtes die nog nooit
gemeten waren. Pijler 2 is "One War, Three Altitudes" — een nulmeting van één hoogte
is dus per definitie incompleet.*

### Bevinding 1 — de debug-poort speelt hier GEEN rol

**Nul treffers op `IsDebugHudAllowed()` in beide bestanden.** Het probleem dat boots
heeft — de schermlaag staat per constructie niet op een opname, zie §1b — geldt hier
niet. Base en map zijn wél op een frame te controleren. De eerste bouwstap van §1b is
hier dus niet nodig, en dat scheelt.

### Bevinding 2 — het zijn allebei kale tekstlijsten

| Hoogte | Wat er staat | Wat de scope vraagt en er niet is |
|---|---|---|
| **MAP** | Eén `UVerticalBox` met `UTextBlock`-regels. Kop `DISTRICT BOARD — Day %d`, daaronder per regio `regionId — PLAYER\|CONTESTED\|DOMINION \| garrison %d \| unrest %d`. Enige visuele codering is regelkleur (groen/geel/rood per eigenaar) — het commentaar noemt dat zelf eerlijk *"debug-grade, still readable"*. | Er is **geen kaart**: geen regio-vormen, geen posities, geen jump-lanes, geen Dominion Response Tier, geen missie-aanbod als aanbod. Een lijst regels is geen strategische laag. |
| **BASE** | Meer structuur: een `UWidgetSwitcher` met tabs Command / Workshop / Barracks / Memorial. Maar de inhoud is dezelfde vorm — tekstregels in een verticale doos, met de kop als één lange `printf` inclusief `walking is disabled here`. | Geen faciliteitenpanelen, geen bouw-ETA's als voortgang, geen crew-kaarten, geen voorraadmeters, geen strategische klok. |

### Wat daaruit volgt voor de volgorde

**Base en map zijn functioneel bedraad en visueel nog niet begonnen.** Dat is een
*ander* probleem dan bij boots, en het vraagt dus een andere eerste stap:

- **Boots** had een poort nodig — de data was er, hij was alleen onzichtbaar.
- **Base en map** hebben een **maatstaf** nodig vóór er code komt (owner-regel 27-07,
  `phase0/REFERENTIE_TPS.md`). Er ligt nu geen referentie voor hoe de strategische
  laag en de basis eruit horen te zien. Zonder die maatstaf wordt dit twintig props
  in een lege hoek, precies wat `20_world_dressing_standard.md` verbiedt.

**Volgende stap is dus niet bouwen maar de referentie vaststellen** — en dat raakt
owner-punt O-6 (de stijlvraag), want een gestileerde strategische kaart en een
fotorealistische zijn twee verschillende opdrachten.

## 2. Sprintbord — GEIJKT 2026-07-31 ~19:30 (elk punt draagt een hash of een pad)

> **Waarom dit bord opnieuw geijkt is.** De vorige versie was van 24-07 ~17:35 en beschreef
> vijf sporen als *in flight* die **alle vijf geland zijn**; op 31-07 stuurde hij de sessie
> naar werk dat al af was. Elk punt hieronder is tegen de **werkboom** gecontroleerd, niet
> tegen het vorige bord. **Regel van dit bord: een regel zonder commit-hash of bestandspad
> hoort er niet in.** Een plan dat verkeerd stuurt is duurder dan geen plan — het kost de
> tijd van iedereen die het gelooft.

DoD-basis (14.4): *spec gerefereerd + code + data + tests + EventCatalog/docs bijgewerkt + CI/lokale groene bar (build -NoUba ✓, tests ✓, validatie ✓, catalog ✓) + door een mens/reviewende agent gezien.*

**Bar bij het ijken (31-07 avond):** **185/185 tests, 0 gefaald** · `EclipseValidateData` 7 validators / 9 assets / **0 fouten** · EventCatalog in sync. De rode test van eerder die avond (`Eclipse.Feel.Input.DocumentedConsoleCommandsExist`) is opgelost. En let op bij het lezen van het dashboard: de testteller dáár was **stuk, niet verouderd** — `Tools/update_progress.ps1` las `Saved/Automation/` terwijl elke ronde naar `Saved/TestReport/` schrijft. Ook dat is gerepareerd; de getallen zijn nu te vertrouwen.

### 2a. Nu in flight — zes sporen, bewust file-disjunct

| # | Spoor | Eigenaar | Bestanden (= ownership zolang het loopt) | De falsificatie die hem afsluit |
|---|---|---|---|---|
| **A1** | **Schermlaag bouwstap 1 (boots).** De spelerlaag vóór `IsDebugHudAllowed()` halen, debuglaag erachter — zonder die poort staat de schermlaag *per constructie* op geen enkele opname (§1b). | hud-builder | `Eclipse/Source/Eclipse/UI/` — **de hele map, exclusief** | De opnameronde legt de spelerlaag vast op een frame **én** toont in dezelfde ronde dat de debuglaag afwezig blijft. Twee beweringen, één ronde. |
| **A2** | **Vitals-feed onder de schermlaag** (`Event.Player.VitalsChanged`). Gemeten gat: er bestaat **geen enkele `Event.Player.*`-tag** (`Core/EclipseGameplayTags.h`); HP/downed lopen via native delegates (`Characters/EclipseCharacter.cpp:1487/1524`) en de munitieregel **polt** `Weapon->GetAmmoInMagazine()` (`UI/EclipseMissionHudWidget.cpp:186`). Zonder feed heeft A1 een poort en niets om te tonen — en zou de bouwer gameplay-logica in een widget moeten schrijven, wat zijn eigen regel verbiedt. | element-builder 1 | `Core/EclipseEventPayloads.h`, `Core/EclipseGameplayTags.{h,cpp}`, **nieuw** `Characters/EclipseVitalsFeed.{h,cpp}`, `Characters/EclipseCharacter.cpp`, `Eclipse/Docs/EventCatalog.md`, **nieuw** `Tests/EclipseVitalsFeedTests.cpp` | 60 ticks zonder verandering = **0** broadcasts · 100→60 HP = **precies 1** met de juiste velden · hurken→staan→sprint = **3** in volgorde · `Eclipse/Tools/check_event_catalog.py` groen. |
| **A3** | **Inslagspoor: de ene meting die de twee verklaringen scheidt** (dossier 1). `phase0/DEBUG_DISCIPLINE.md` §4.3 concludeert *transform-bug* op grond van een kubus bij het personage — maar `Combat/EclipseHitscanWeaponComponent.cpp:181-195` zegt zélf dat dat blok de controleproef was: **vastgemaakt aan het personage**, neergezet bij BeginPlay. Dat hij daar stond is dus per constructie zo. De logregel op `:220-229` logt `Spot`, maar **nooit `Mark->GetActorLocation()`** — de post-spawn transform is nooit gemeten. | element-builder 2 | `Combat/EclipseHitscanWeaponComponent.cpp` (logregel), **nieuw** `Tests/EclipseImpactMarkTests.cpp` | `Eclipse.Combat.ImpactMarkLandsOnTheHitAndNotOnTheShooter`: 20 treffers, elk **≤1 cm** van de inslagplek én **≥100 cm** van de schutter. Rood = transform-bug bevestigd, vindplaats aangewezen. Groen = §4.3 herschrijven en het dossier gaat terug naar de renderkant. |
| **A4** | **Trillen: het gewicht per frame, maar headless** (dossier 2). De voorgeschreven Rewind-Debugger-meting kán niet zoals beschreven: de speler draait **geen AnimBP** maar `UEclipseAnimInstance` (`Characters/EclipseCharacter.cpp:1296`), een C++-proxy die gewogen poses optelt — §4.2 oorzaak 1 wijst voor de speler naar een graaf die niet bestaat. Wat er wél staat: `OneShotTime = 0.0f` bij **elk** schot (`Characters/EclipseAnimInstance.cpp:287`) op een envelope `Peak × sin(alpha × PI)` (`:389`), dus per schot 0→piek→0. | element-builder 3 | `Characters/EclipseAnimInstance.{h,cpp}` (gedragsneutrale extractie), **nieuw** `Tests/EclipseAnimOneShotWeightTests.cpp` | Omkeringen lopen **1:1 met het aantal schoten** (10/20/27 → 10/20/27) en veranderen **niet** als de bemonstering van 120 naar 60 Hz gaat. **Niet repareren in dezelfde iteratie** — twee eerdere fixes zijn juist daarom teruggedraaid. |
| **A5** | **L1 beat-sheets, Act 1 eerst** (spoor A). | story-architect | `phase0/SCRIPT_PRODUCTION_PLAN.md` §4 + de scriptmappen | Raakt de build niet en kan dus altijd doorlopen; sluit op de kalender: werkdeadline generatie **19-08** (credits vervallen 21-08). |
| **A6** | **Tier 0 casting-kandidaten, 5k credits.** | voice-director | `phase0/VOICE_LEDGER.md`, `19_voice_production.md` §19.2 | Per rol 2-3 kandidaten op **dezelfde signature-regel**, zodat de owner eerlijk vergelijkt. Casting is permanent (de cache-sleutel bevat de stem-ID); de keuze zelf staat in §2e. |

**Twee coördinatieregels — disjuncte bestanden zijn niet hetzelfde als disjuncte iteraties:**
1. **Eén build-slot.** Schrijven mag parallel, **landen is serieel** — en een open owner-editor blokkeert het slot volledig (Live Coding).
2. **A2 vóór de munitiefeed (N-a).** Beide raken `Eclipse/Docs/EventCatalog.md` én `Core/EclipseGameplayTags.cpp`; parallel is dat een merge-knoop, geen snelheid.

### 2b. Daarna klaarstaand (in deze volgorde)

**N-a — Munitie- en wapenstatus op de bus** *(element-builder; tweede helft van de datalaag onder boots)*
§1b meet munitie **deels** (alleen kogels in het magazijn) en wapenstatus **nee**. Feit-op-de-bus bij verandering: magazijn, magazijnen over, herlaad-voortgang, leeg-staat, actief wapen, vuurmodus — zodat `UI/EclipseMissionHudWidget.cpp` kan stoppen met pollen.
*Falsificatie:* 30 schoten = 30 events met aflopend magazijn · herladen = start + monotone voortgang + eind · wapenwissel = precies 1 event met de nieuwe naam · een frame zonder verandering = **niets**.

**N-b — Eerste kaartstap (map), en hij is kleiner dan hij lijkt** *(element-builder; §1c + `phase0/REFERENTIE_BASE_MAP.md`)*
Het gat is **geen ontbrekend schema**: `Strategy/EclipseRegionGraphAsset.h` draagt de kanten al (`FEclipseRegionDefinition::ConnectedRegionIds`) mét symmetriecontrole in `Strategy/EclipseStrategyLogic.cpp:84-102`. Het gat zit **tussen twee structuren**: `UI/EclipseStrategyMapWidget.cpp:103` itereert `Campaign->GetState().Regions`, en `FEclipseRegionState` heeft vier velden (`Strategy/EclipseCampaignTypes.h:38-50` — RegionId, Owner, Unrest, GarrisonStrength) en **geen kanten**. De eerste stap is die twee koppelen, niet "bouw een strategische laag".
*Falsificatie:* de kaartlaag noemt per regio zijn buren, en een asymmetrische kant in de data laat de validator roodlopen vóórdat hij het scherm haalt.
*Let op:* de **visuele** invulling van base en map wacht op de stijlvraag (O-6, §2e) — deze koppeling niet.

**N-c — Fix-iteratie op de winnaar van A3/A4** *(element-builder)*
Pas plannen als de meting er ligt. Geen fix zonder benoemde oorzaak plús meting (`phase0/DEBUG_DISCIPLINE.md` §3 regel 2).

### 2c. Ledger — geland, mét bewijs. **Niet opnieuw controleren.**

*Elke regel is op 31-07 tegen de werkboom geverifieerd. Staat een punt hier, dan is de vraag "is dit nou geland?" beantwoord — en hoort de volgende sessie er geen tijd meer in te steken.*

| Punt (oude bordcode) | Wat | Bewijs |
|---|---|---|
| **T1–T8** | Fase-administratie en de eerste specs: milestone-flip · SPEC-P2-01 · SPEC-P2-03 gebankt · SPEC-P2-02 gebankt. T6 vervallen (geen los GAS-ability-systeem nodig), T7 materieel vervuld via de `DT_BodyDefs`-body-pipeline. | P2-01 `1a85b78` (save v2→v3 per 14.3.6; 38/38) · `7194cf4` · `29cd549` |
| **Cyclus 2 (24-07)** | P2-02 **Stage A** · P2-03 **stap 1–2** · m4/m6/m7 (spawn-fan + cover-scorer → `DA_SquadTuning`) · pack-slim (11 accepts → `/Game/Art/Imported`, ~5,9 GB vrij; **repo blijft privé**, vastgelegd in `SOURCES.md`) · `DA_CommandModeTuning` · SPEC-P2-04 geschreven + review ACCEPTED | `b5aa157` · `deabc9f` · `3c2f3a5` · `a0a39b9` · `6fe5081` |
| **F1 + N1** | R7-falsificatie-skelet (`M11SkeletonCarriesAuthoredAsset`) + debrief-dag-regel | `60014e6` (07-24 17:20) · `Eclipse/Source/Eclipse/Tests/EclipseMissionM1Tests.cpp` |
| **F2** | Builder-strings naar de Imported-paden | `7036849` (07-24 17:20), volgt op `a0a39b9` · 12× `/Game/Art/Imported` in `Core/EclipseGrayboxBuilder.cpp` |
| **F3 + N2 + N3a** | P2-03 stap 3: transactie-API + `Event.Base.*` ×4 + `setup_base_data.py` | `69b5f4b` (07-24 17:36) · tags in `Core/EclipseGameplayTags.h` · wrapper in `Base/EclipseBaseSubsystem.cpp` · `Eclipse/Tools/setup_base_data.py` |
| **F4** | MetaHuman-curatie ronde 1 (SentinelC geland, Kaya provisioneel gekoppeld) | `15dfe61` (07-24 17:21) · `Eclipse/Tools/setup_metahuman_data.py` — **rest = owner-kliks, §2e** |
| **F5 + N4** | SPEC-P2-05 gebankt + liberation pure core + bedrading | spec `cdad8f5` (07-24 17:37) · core `9be3786` (07-25 13:10) · `Strategy/EclipseLiberationLogic.cpp` · catalogrij `Event.Strategy.LiberationResolved` = *implemented* |
| **N3b** | Audio-import 16.12 naar `/Game/Audio` | `937f35f` (07-24 17:33) · `Eclipse/Tools/import_generated_audio.py` |
| **N5** | P2-04 stap 2: StoryFlags + `DT_StoryMissions` + save **v4→v5** mét migratie (derde R6-toets) | `fd38933` (07-24 17:52) · `Strategy/EclipseCampaignTypes.h:222` → `SchemaVersion = 5` · `Eclipse/Tools/setup_story_missions.py` (`ee07b16`) |
| **N3c (deels)** | 15.8-dressing rondes 1–2 | `phase0/DRESSING_ITERATIE_2.md`, `phase0/DRESSING_ITERATIE_3.md` — **watch-items open, zie 2d** |
| **31-07 avond** | Rode test opgelost; dashboard-testteller gerepareerd (`Saved/Automation/` → `Saved/TestReport/` in `Tools/update_progress.ps1`) | bar **185/185, 0 gefaald** · validatie 7 validators / 9 assets / 0 fouten |

### 2d. Nog echt open — gescopet, met de test die hem afsluit

| Open punt | Scope in één zin | Sluit af met |
|---|---|---|
| **Backlog #9 — UI-stack v1** | Dit **is** het HUD-spoor: boots (A1/A2/N-a), base en map (§1c, N-b). Drie hoogtes plus Command Mode als overlay, en alles rond het vizier leesbaar in 1e **én** 3e persoon. | De owner speelt een missie en leest van het scherm af hoeveel kogels hij heeft, hoe zijn squad ervoor staat en wat hij moet doen — zonder console, in beide perspectieven. |
| **Vault-shells** (rest van backlog #5) | De beloopbare vault-ruimtes op de al gelande base-systeemlaag (`69b5f4b`): het systeem staat, de ruimtes niet. | Een ronde loopt van faciliteit naar faciliteit zonder door geometrie te vallen, en de vier faciliteiten zijn in het frame te onderscheiden. |
| **15.8-watch-items + stubs** | Openstaande punten uit `phase0/DRESSING_ITERATIE_3.md` (o.a. pool-compositing over de ondergrond, barrièrewaarde 0,3203 = 1,8× de norm) + de **machine-lokale** opruiming van drie ~1,4 KB redirector-stubs (`phase0/ASSET_CLEANUP.md` §145-157). De **repo-kant is klaar** (`7036849`): de oude pack-paden leven daar alleen nog in commentaar. | Shotronde + art-review; de stubs pas weg als één ronde bewijst dat niets meer via de oude paden laadt. |
| **Backlog #12 — testlagen** | Alle zes 14.4-lagen voor de slice; het soak-logboek maakt "drie nachten groen" nog niet aantoonbaar (`phase0/SOAK_LOG.md`). | Drie opeenvolgende nachten groen in het logboek — inclusief de bewaarde rode regels die bewijzen dat het logboek ook rood schrijft. |
| **Backlog #13 — gate-review Phase 2** | Externe reviewers spelen 3 uur koud (werving = §4 T-7). | Verdict "I want the rest of this game": ja/nee (13.2). |
| **Backlog #11 — audio-infrastructuur 16.7** | Open en **bewust niet ingepland**: `Source/Eclipse/Audio/` bevat geen muzieklagen/Quartz, maar generatie kost credits die per 31-07 al aan stemwerk zijn toegewezen. Dat is een **owner-afweging over credits, geen agent-besluit**. | — (wacht op de owner, §2e) |

### 2e. Wacht op de owner — expliciet, nooit stil ingepland

| Punt | Blokkeert | Bron |
|---|---|---|
| **T-2** env-pack-kliks + 1 civilian/worker-pack | kit-pass #10 en de bevolking van Hollow Point | §4 T-2 |
| **T-8** MetaHuman-downloadkliks | rest van de MetaHuman-curatie (F4 is gedaan op wat binnen is) | §4 T-8 |
| **T-11** R3-verdict uit de feel-gauntlet | P2-02 **Stage B** blijft dicht tot het verdict | §4 T-11 · `phase0/FEEL_GAUNTLET_P2-02.md` |
| **O-3** stemkeuze (smaak, geen techniek) | alles wat gesproken wordt; casting is permanent | `STATUS.md` · `19_voice_production.md` §19.2 |
| **O-5** wapen: goedkope tint-stap of echt uit de mesh halen | wapenwerk spoor B — **geen inkoop**: er hángt een wapen, het zit alleen in de karaktermesh | `phase0/REFERENTIE_TPS.md` §WIJ NU |
| **O-6** stijlvraag: Borderlands-lock of fotorealisme | de **visuele** invulling van base en map (de koppeling in N-b niet) | `20_world_dressing_standard.md` §20.8 |
| **Credits voor muziek** | backlog #11 (zie 2d) | `progress_data.js` → `ownerActies` |

**Van deze lijst afgevoerd op 31-07 — beantwoord, en daarom hier weg:**
- **O-1 verloopdatum credits:** 21 augustus 2026; werkdeadline generatie **19 augustus** (2 dagen buffer). De kalender staat op echte datums in `phase0/SCRIPT_PRODUCTION_PLAN.md` §4. Spoor A is gedeblokkeerd.
- **O-2 commerciële gebruiksrechten:** bevestigd door de owner — vastgelegd in `19_voice_production.md` §19.1, dat die blocker daar zelf sluit.

---

## 3. Risicolijst — met vroegst mogelijke falsificatietest

| R | Risico | Vroegst mogelijke falsificatie |
|---|---|---|
| R1 | **Phase-1-loop is niet leuk** (13.2-gate nooit beantwoord) → alle slice-investering staat op een onbewezen loop. | **Vandaag, nul bouwwerk:** owner speelt de loop in PIE (controller + barks) en beantwoordt "tweede loop: ja/nee?". Bij "nee": eerst loop-pivot, fidelity-werk (#10) bevriezen. |
| R2 | **Squad-AI schaalt niet naar 4+klassen** — orders voelen niet langer "obeyed" (existentieel risico 2, 13.1). | Direct na T4: scenario-suite op 4 + PIE-ordertest op graybox, vóór er ook maar één klasse-ability of UI bestaat. Stille orderfout = stop. **Status 2026-07-24: falsificatie doorstaan** — scenario-suite op 4 mét klassen groen in de P2-01-landing (`1a85b78`, 38/38; medic-her-dispatch als review-fix erin). Bewaking blijft per merge via de suite. |
| R3 | **Command Mode-feel (30% dilatatie) overleeft echte gevechten niet** (leesbaarheid/chaos). | 14.5-stap-4-debug-versie van P2-02 in het bestaande graybox-district testen vóór UI/camera-polish — feel-verdict op de lelijkste versie. **Status 2026-07-24: falsificatiebuild GELAND** (`b5aa157`, review GO; 20×-exit-ladder + ack-scenario groen) — het verdict ligt nu bij de owner-feel-gauntlet (§4 T-11, draaiboek `phase0/FEEL_GAUNTLET_P2-02.md`; telemetry HeldSeconds/OrdersIssuedWhileHeld leest de agent uit). Stage B blijft dicht tot het verdict. |
| R4 | **15.12 "AAA-ready" onhaalbaar binnen 12.4-budgetten** op software-Lumen/1080 Ti. | Eén straatblok (niet het hele district) naar volle 15.5-revisie dressen en profileren tegen 12.4 op dev-preview-scalability — vóór de kit-pass districtbreed gaat. |
| R5 | **Fab-afhankelijkheid**: owner-kliks blijven uit → kit-pass (#10) stagneert. | Timebox: staan de shortlist-packs er over 7 dagen niet in, dan Quaternius/CC0+Blender-route als plan-of-record (al bewezen pipeline) en Fab als upgrade-pad. **Status: deels ontladen** — character-packs zijn binnen (11 Fab-packs, 23-07); de env-pack-kliks (§4 T-2) blijven de resterende afhankelijkheid voor de kit-pass. |
| R6 | **Save v1-migratie breekt v0-campagnes** (project-killer per 12.3). | Eerste schema-break in Phase 2: migratie-entry + v0-fixture-test in hetzélfde commit (14.3.6), per merge draaiend — niet wachten tot spec 06. **Status 2026-07-24: falsificatie bewezen werkend** — de eerste echte schema-break (v2→v3, roster-ClassId) landde mét migratie-entry + v0- én v2-fixture-tests in hetzélfde commit (`1a85b78`); pre-v3 saves landen deterministisch op classless. **Tweede bewijs geleverd 24-07:** de v3→v4-break (FEclipseBaseState) landde mét byte-getrouwe v3-fixture in hetzélfde commit (`deabc9f`, review "R6-discipline voorbeeldig"). **Derde bewijs geleverd 24-07:** de v4→v5-break (StoryFlags) landde mét migratie-entry en fixtures in hetzélfde commit (`fd38933`; `Strategy/EclipseCampaignTypes.h:222` → `SchemaVersion = 5`). De discipline herhaalt per break; eerstvolgende toets is de volgende schema-break, niet meer P2-04. |
| R7 | **Quest-runtime draagt geen authored missies** (StateTree-fases/consequence-commits te licht voor M1.x). | M1.1-skelet als Gauntlet spawn→complete-by-script→CampaignState-asserts bouwen vóór M1.2–M1.4 geautoreerd worden. **Status: FALSIFICATIE DOORSTAAN** (geijkt 31-07) — `Eclipse.Missions.M11SkeletonCarriesAuthoredAsset` groen geland in `60014e6` (§2c), bestand `Eclipse/Source/Eclipse/Tests/EclipseMissionM1Tests.cpp`: authored asset op het echte laadpad, mandatory-set, rewards, regio-onaangetast; de testrun is groen gedraaid. *Implicatie voor de economie:* de debrief-dag-regel in dezelfde changeset maakt "missie kost een dag" mechanisch waar — de P2-03-kalender (facility-ETA's, crew-dagen) klopt nu, en de econ-soak-asserts uit SPEC-P2-03 moeten met deze dag-kosten rekenen. |
| R8 | **Prologue-scope-explosie** (open vraag 5 maakt SPEC-P2-04 2 u groter). | Besluit bij spec-schrijven: slice opent bij M1.1 met briefing-recap; prologue alleen als de gate-review erom vraagt. Falsificatie = cold-reader begrijpt de recap zonder prologue. **Status 2026-07-24: BESLOTEN** in SPEC-P2-04 (ACCEPTED): geen prologue, recap-opening; de cold-reader-check staat als DoD-item in de spec. |
| R9 | **16.7-muziek blokkeert op ElevenLabs-scopes** (Music/SFX-endpoints dicht). | Eén live endpoint-call zodra owner scopes zet; tot dan verticale lagen bouwen op CC0-stems — systeem falsifieert zichzelf zonder API. |
| R10 | **12.4-budgetten stilletjes overschreden** naarmate Nanite-density groeit. | Per-merge budgetcheck op Underworks-referentiescène vanaf de éérste kit-pass-merge (niet pas bij spec 08-afronding); Insights-trace bij elke perf-verdachte PR (14.6 skill 7). |
| R11 | **Externe gate-reviewers niet geregeld** → Phase-2-gate wordt zelfbeoordeling. | Owner werft reviewers zodra M1.1+M1.2 speelbaar zijn (halverwege), niet aan het eind — eerste koude sessie is meteen een halfweg-falsificatie. |

---

## 4. Menselijke-touchpoints-wachtrij (owner-acties, gequeued — nooit blind omheen plannen)

*(Gesynchroniseerd met `ownerActies`/`jijGedaan` in `progress_data.js`, 2026-07-24 ~17:35 — de feel-gauntlet staat daar nu bovenaan, met T-10 (downloads annuleren) erachter.)*

| T | Actie | Waarvoor / blokkeert | Wanneer |
|---|---|---|---|
| T-1 | **Playtest 13.2**: loop in PIE spelen (Xbox-controller, squad-barks) en de gate-vraag beantwoorden. | Eerlijk Phase-1-verdict; stuurt R1 en de fidelity-prioriteit. | **Nu — staande actie**, ~30 min |
| T-2 | **Fab-kliks (rest)**: login ✓ en de character-packs zijn binnen; resterend zijn de env-pack-pulls (Factory Pack Vol.1, Industrial Building 49 PBR, UNIBLOCKS, Sci-Fi Hallway, Sci-Fi Light Pack, Auto Footsteps Utility, Niagara Footstep VFX, FPS Weapon Bundle, Free Muzzle Flash) + **1 gestileerd civilian/worker-pack** (4–6 bodies, Mannequin-rig — het enige character-gat: Hollow Point-crew/idlers en Kessara-burgers close-up). | Kit-pass backlog #10 + SPEC-P2-03-bevolking; monitor vuurt daarna autonoom. | Gauw, ~10 min (R5-timebox loopt op de env-packs) |
| T-3 | ~~ElevenLabs-scopes~~ — **✓ gedaan 2026-07-23** (Music/SFX aangezet, zie `jijGedaan`). | — | Afgerond |
| T-4 | ~~Blender-install~~ — **✓ gedaan 2026-07-23** (zie `jijGedaan`). | — | Afgerond |
| T-5 | **Mixamo-kliklijst** afwerken zodra de animatie-pass start (Quaternius-CC0 is het autonome alternatief). | Animatiekwaliteit body-pass (backlog #3-QC). | Bij animatie-pass |
| T-6 | **CI self-hosted runner** goedkeuren/aanwijzen (Phase-0-carryover). | 14.4-lagen per merge i.p.v. lokaal (backlog #12). | Deze fase, niet-blokkerend |
| T-7 | **Externe reviewers werven** voor de Phase-2-gate (koud, geen teamleden). | Backlog #13 + R11-halfwegtest. | Zodra M1.1+M1.2 speelbaar |
| T-8 | **MetaHuman-download-kliks**: basis is binnen (SentinelC + Common, 908 MB); resterend 1× Download-klik per overig MH-item in Window → Fab → My Library + sein "MetaHumans staan erin". De curatie op wat binnen is, is gedaan (`15dfe61`, §2c). De Creator-sessie voor hero-companions blijft de Phase 2→3-evaluatie (backlog #21). | Rest van de MetaHuman-curatie (named-character-slots). | Gauw, ~5 min |
| T-9 | Verder weg: **VO-kwaliteitspas op de AI-stemmen** (géén opnamecontracten — het VO-model ligt vast in `19_voice_production.md` §19.1, dat backlog #29 daar expliciet toe versmalt; verwijs daarheen, herhaal de inhoud hier niet), lokalisatievendors, closed-beta-programma (Phase 5); storefront/gold-beslissingen (Phase 6). | Backlog #29–#32. | Phase 5+ |
| T-10 | **Overbodige downloads annuleren** (Launcher → Downloads): alle Paragon-helden (Kwang, Narbash, Wukong, Riktor, Sevarog…), Old West VOL 4/6, Path of Adventure JRPG-muziek, NWIRO AI Pro; láát staan: Photogrammetry Snow, Plant Models Vol 60, Perfect Fire VFX. | Bespaart ~30 GB schijf. | Nu, ~2 min |
| T-11 | **P2-02 Stage A feel-gauntlet** (~20 min): de gelande Stage-A-build (`b5aa157`) spelen per draaiboek `phase0/FEEL_GAUNTLET_P2-02.md` (hold Q/LB → 30%, orders 1-4/D-pad, selectie Tab/RB of E/X), de 5 criteria scoren en "R3-verdict: true/false" + observaties doorgeven; telemetry (HeldSeconds/OrdersIssuedWhileHeld via de ModeExited-payload) leest de agent uit. | Beslist Stage B vs. spec-amendment (§2 "Wacht expliciet"). | **NU — staat bovenaan `ownerActies`** |

*Consent-protocol blijft gelden: installs/downloads/security-prompts alleen na uitleg + expliciet akkoord; bouwen met `-NoUba`; PROGRESS.html nooit direct bewerken.*
