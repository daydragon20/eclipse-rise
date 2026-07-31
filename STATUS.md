# ECLIPSE — STATUS

*De compacte statuskaart. Vervangt het inlezen van `archief/HANDOFF.md` (154 KB → dit).*
*Bijgewerkt 2026-07-31 laat · **houd dit onder 5 KB**: details horen in het document dat
ze bezit, hier staat alleen wat een verse sessie moet weten om te beginnen.*

---

## Waar we staan

**Fase 2 — Vertical Slice "Thirteen Bullets"** is de actieve milestone.

Bar (gemeten 31-07 ~20:00): build groen `-NoUba` · **193 tests, 0 gefaald, 0 niet
gedraaid** · `EclipseValidateData` 7 validators / 9 assets / **0 fouten** · EventCatalog
**37/37**.

Spoor-B-prioriteit 1 is de **schermlaag**. Nathan speelt pas als die op niveau is.

> **De HUD is niet één scherm.** Pijler 2 (`01_game_vision.md`): *One War, Three
> Altitudes* — **boots** (third-person, wisselbaar naar first-person met C/R3), **base**
> en **map**, met **Command Mode** als overlay binnen boots. Alles rond het vizier moet in
> **beide** perspectieven leesbaar zijn. Scope: `.claude/agents/hud-builder.md`.
> Nulmeting boots: `phase0/EXECUTION_PLAN.md` §1b · base/map: §1c +
> `phase0/REFERENTIE_BASE_MAP.md`.

**De HUD-poort is geland (31-07)** en op frames geverifieerd: de spelerlaag staat nu in
beide perspectieven op een opname (`Saved/Screenshots/HUD_spelerlaag/`), de debuglaag
alleen in normaal spel. Daarmee is de blokkade uit §1b weg.

## Twee sporen, parallel

| Spoor | Wat | Plan |
|---|---|---|
| **A — Schrijven & stem** | Hele campagne uitschrijven; Act 1 ingesproken vóór de credits vervallen (**21-08**, werkdeadline **19-08**) | `phase0/SCRIPT_PRODUCTION_PLAN.md` |
| **B — Systemen & feel** | HUD eerst, dan de open dossiers, dan de backlog | `phase0/EXECUTION_PLAN.md` §2 |

Spoor A raakt de build niet aan en loopt dus altijd door. Act 1 heeft beats: 9
beat-sheets + 71 scène-stubs in `phase0/beats/` en `Eclipse/Content/Script/act1/`.
Casting fase 1 klaar: **104 kandidaten, 0 credits**.

## Open dossiers — alle drie met een gemeten stand

*Volledige metingen staan in `phase0/DEBUG_DISCIPLINE.md`. Lees die vóór je iets aanraakt;
alle drie zijn eerder de verkeerde kant op gestuurd door een plausibele redenering.*

1. **Inslagspoor — OPGELOST. Het rendert prima; het is 14 pixels groot.** Geen
   transform-bug (`ac09980`) én geen renderbug: het spoor is **9×9 cm**, plat, onder een
   scherende hoek vanaf ~10 m. **Gemeten**: van bovenaf 5.459 veranderde pixels, fel amber,
   GEZIEN — vanaf de spelercamera **14 pixels** (0,02 promille). De 44 grondvlakken samen
   halen daar 6. De laag rendert en is op ooghoogte niets waard.
   **De meting scheidt de twee verklaringen door constructie**: twee opnames van hetzelfde
   frame met exact één verschil (object verborgen), dus elke bewegende pixel komt van dat
   object. Controles eerst en alle drie groen: ruisvloer 0, wereld-kanarie 894.610,
   negatieve controle 0.
   **Volgende stap is dus geen RenderDoc maar een maat**: hoe groot moet een kogelspoor zijn
   om op 8–15 m te lezen. Falsificatie ligt klaar — hetzelfde harnas telt de pixels.
   **Eerst de dubbele spawner weg**: elke treffer zet er **twee** exact samenvallend neer
   (`SpawnImpactMark` + `OnWorldImpact`), dus elk differentieel leest 0 en de diagnose
   saboteert zichzelf. Dat is bovendien een duplicaat in verscheept gedrag.


2. **Trillen bij het schieten** — mechanisme **vast** (`bc881f4`, §4.2): de envelope
   herstart per schot. Géén AnimBP, dus geen vechtende blend-nodes. Fix is een aparte
   iteratie; falsificatie ligt klaar.
3. **GPU-crash — GEPAUZEERD: hij reproduceert niet meer** (§4.5). Page fault, geen
   TDR-timeout; breadcrumbs wezen **SkyAtmosphere** aan (Lumen/TSR/SSR stonden op "Niet
   gestart"). **Maar de sky-hypothese is dood:** de bouwer draait één keer, op
   `renderframe=0` vóór BeginPlay, en sloopt daar `0/0/0/0` — GrayboxDistrict draagt zelf
   geen zon of sky, dus die sloopregels zijn op deze map dode code. Er ís geen mid-play
   herbouw. En **35 rondes met 123.572 geforceerde sky-herbouwen gaven 0 crashes**; bij de
   historische 1-op-9 is dat 1,6% waarschijnlijk. **Niet verder zoeken zonder signaal** —
   `r.SkyAtmosphere 0` zou nul met nul vergelijken. Twee leads liggen open: de crash viel
   *vroeg* (frame 259), wat elke "loopt vol"-verklaring tegenspreekt, en het crashende log
   heette `Eclipse_2`, dus er was **contentie**. De enige niet-nagebootste vorm is een
   **open editor** tijdens een ronde — en die kost het build-slot, dus dat is een
   owner-afweging, geen agent-besluit.

**Twee dingen die alleen een frame liet zien:** er staat een **STOP-verkeersbord** in het
district (`EclipseGrayboxBuilder.cpp:1432`) terwijl `20_world_dressing_standard.md` §20.2
dat verbiedt — doc geschreven, code nooit aangepast. En de squad-lijst toont `45434C53`
in plaats van namen: de ASCII-kop "ECLS" van de GUID uit `EclipseRosterLogic.cpp:15`.

## Fouten uit screenshots halen — autonoom

**Spawn `screenshot-inspector` aan het begin van elke werkcyclus.** Hij kijkt naar nieuwe beelden in `Eclipse/Saved/Screenshots` én in `C:\Users\natha\Pictures\Screenshots` (daar staan Nathans Win+PrtSc-opnames van foutmeldingen), en schrijft wat er mis is naar `phase0/SHOT_FINDINGS.md`.

Waarom: Nathan zat foutmeldingen door te sturen — een verminkt pad, een GPU-crash, een omgekeerd wapen, ontbrekende handen. Dat is werk dat een agent kan doen, want hij kijkt naar dezelfde beelden. Owner-instructie 31-07: *"laat een agent die fouten er zelf uit halen, ook autonoom."*

## Hoe je Nathan iets vraagt

**Niet in de chat — met een knop.** Vraag in `phase0/owner_questions.json` (id, vraag,
waarom, stappen, opties met label/waarde/gevolg, advies). Hij ziet een kaart op het
dashboard en klikt.

Zijn antwoord komt in **`phase0/OWNER_ANSWERS.md`** — **lees dat elke sessie**, het is
bindend. Haal een beantwoorde vraag daarna uit `owner_questions.json`, anders vraag je hem
opnieuw iets dat hij al besloot.

## Wacht op Nathan

| # | Actie | Blokkeert |
|---|---|---|
| **T-12** | Drie ElevenLabs-API-scopes (`user_read`, `speech_history_read`, `models_read`) | **alle generatie — staat nu stil** |
| O-3 | Stemmen kiezen uit de 104 kandidaten | alles wat gesproken wordt |
| O-4 | IJkmissie M1.1 beluisteren | massaproductie |
| O-9 | Wat wil je in eerste persoon van jezelf zien | first-person-inhoud |

*Beantwoord en verwerkt: O-1, O-2, O-5, O-6, O-7, **O-8**, **T-1**, T-2, **T-7**, T-10, T-11. Zie `OWNER_ANSWERS.md` — **lees dat elke sessie**, antwoorden komen via de dashboardknoppen binnen en niet via de chat.*

> **Twee antwoorden die de koers veranderen.** **O-8 = "vol"**: de HUD mag vól Borderlands worden — `phase0/REFERENTIE_HUD_BORDERLANDS.md` is de maatstaf, met §3 als tegeneis (vol in vórm, streng in hiërarchie; leesbaarheid gaat vóór stijl). En **T-1 = "nog niet"**, verbatim: *"Ik speel nog NIET. Eerst laat ik alle basisdingen bouwen waar je nu mee bezig bent. **Fase 0 en 1 zijn hiermee afgesloten — wacht niet meer op mijn playtest, die staat niet langer in de weg.**"* De staande 13.2-playtestpoort is dus **weg**; niets hoeft meer op zijn speelronde te wachten.*

## De staande kwaliteitsopdracht

**Graphics en uitgebreidheid zijn de twee hoogste prioriteiten. Nooit de kortste weg.**
Liever drie dialogen van twintig regels dan één van twee. Nooit één licht. Twaalf
bark-varianten, geen zes. Volledig in `21_quality_mandate.md` — kort, lees het één keer.

## Harde werkregels

- Bouw met **`-NoUba`**. Elke iteratie eindigt groen: build, tests, `EclipseValidateData`
  0 fouten, EventCatalog in sync — vóór elke commit.
- **Laat de werkboom nooit onbouwbaar achter, ook niet tussen twee stappen door.** Meerdere
  agents delen één checkout, en Nathan kan op elk moment willen spelen. Schrijf een aanroep
  dus pas als het ding dat je aanroept bestaat — header en cpp in **dezelfde** stap — en
  bouw na elke stap die een aanroep toevoegt. *Dit is geen theorie: op 31-07 kon Nathan de
  game niet starten omdat een aanroep en zijn declaratie in twee stappen waren gesplitst.
  De melding luidde "Missing Eclipse Modules" en wees dus naar een ontbrekend bestand,
  terwijl het probleem twee modules met verschillende BuildIds was.*
- Bouwvolgorde per spec = **14.5**: dataschema → pure-logic core + tests →
  subsystem-wrapper + events → debug-UI → echte UI/content laatst.
- Commits: `[System] Verb summary (GDD-ref)`. Cross-system alleen via de event-bus.
- **Start de game nooit via de Bash-tool.** Argumenten die met `/Game/`, `/Script/` of
  `-ExecCmds=` beginnen worden door Git Bash verminkt. PowerShell. (`DEBUG_DISCIPLINE.md`
  §4.4 — dit kostte een halve avond.)
- **Bugs:** volg `DEBUG_DISCIPLINE.md`. Drie iteraties zonder diagnose = escaleren.
- **Owner-consent:** geen installs/downloads/security-prompts zonder akkoord. Nathan heeft
  **geen adminrechten** — queue dat als owner-actie, bouw er niet stil omheen.
- **Geen `/loop`, geen ScheduleWakeup.**
- **Nooit `PROGRESS.html` of `progress_auto.js` bewerken.** Data gaat in `progress_data.js`.
- **Nooit een dicht markdown-document schrijven dat voor Nathan bedoeld is.** Hij leest één
  scherm: het dashboard. Owner-informatie hoort in `progress_data.js` (`ownerActies`, mét
  stappen) en in de owner-tabel hierboven. Kan hij het niet in 30 seconden scannen, dan is
  het niet voor hem geschreven.

## Het dashboard

`START_DASHBOARD.bat` → **http://127.0.0.1:8377/** — het enige scherm dat Nathan leest.
Live, ververst elke 5 s. **Werk na elke iteratie `progress_data.js` bij**: `bijgewerkt`,
`taken`-statussen, `ownerActies` (afgehandeld → `jijGedaan`).

Houd deze drie vers, het dashboard kleurt ze rood als ze verlopen: `STATUS.md` (elke
sessie), `JOUW_ACTIES.md` (bij elke owner-wijziging), `BESTURING.md` (7 d).

## Waar de rest staat

| Nodig? | Lees dit |
|---|---|
| Wat we bouwen | `00_INDEX.md` → doc 01–20 |
| Volgende taak spoor B / A | `phase0/EXECUTION_PLAN.md` §2 · `phase0/SCRIPT_PRODUCTION_PLAN.md` §4 |
| Een bug efficiënt aanpakken | `phase0/DEBUG_DISCIPLINE.md` |
| Hoe een dialoogregel eruitziet | `18_writing_standard.md` |
| Hoe credits besteed worden | `19_voice_production.md` §19.2 |
| Waarom de wereld nu niet goed oogt | `20_world_dressing_standard.md` |
| Maatstaf third-person / base+map | `phase0/REFERENTIE_TPS.md` · `phase0/REFERENTIE_BASE_MAP.md` |
| Historie vóór 31-07 | `archief/HANDOFF.md` (alleen gericht zoeken) |
