# ECLIPSE — STATUS
*De compacte statuskaart. Vervangt het inlezen van HANDOFF.md (154 KB → dit).*
*Laatst bijgewerkt: 2026-07-31 · werk dit bij aan het eind van elke sessie, houd het onder 5 KB.*

---

## Waar we staan

**Fase 2 — Vertical Slice "Thirteen Bullets"** is de actieve milestone.
Bar: build groen (`-NoUba`) · **188/188 tests, 0 gefaald, 0 niet gedraaid** · `EclipseValidateData` 7 validators / 9 assets / **0 fouten** · EventCatalog 37/37 in sync. *(Gemeten 31-07 ~19:40; de suite groeide van 185 naar 188 door het parallelle agentwerk van die avond.)*

> **De testteller op het dashboard was stuk, niet verouderd — gerepareerd 31-07.**
> Hier stond "draai `Tools/update_progress.ps1` om het te verversen". Dat kón niet
> werken: dat script las `Eclipse/Saved/Automation/index.json`, terwijl elke echte
> testronde via `verify.ps1` naar `Eclipse/Saved/TestReport/` schrijft. Gemeten stond
> het eerste pad vijf dagen bevroren op 110 tests naast 185 in het tweede. Het script
> kiest nu het **nieuwste** van de bekende rapportpaden (bewust een vaste lijst, zodat
> een falsificatieronde van één test nooit het dashboardcijfer wordt) en meldt 185/185.

De owner-stop op nieuwe features is **opgeheven** (31-07). Er geldt nog één voorwaarde: Nathan speelt pas als de **schermlaag (HUD)** op niveau is — dat is nu spoor-B-prioriteit 1.

> **De HUD is niet één scherm.** Pijler 2 (`01_game_vision.md`): *One War, Three Altitudes* — **boots** (third-person, **wisselbaar naar first-person** met C/R3), **base** (management) en **map** (strategie), met **Command Mode** als overlay binnen boots. Elke hoogte heeft zijn eigen schermlaag, en alles rond het vizier moet in **beide** perspectieven leesbaar zijn. Volledige scope in `.claude/agents/hud-builder.md`.

## Twee sporen lopen nu parallel

| Spoor | Wat | Waar staat het |
|---|---|---|
| **A — Schrijven & stem** | Hele campagne uitschrijven, Act 1 inspreken vóór de credits vervallen | `phase0/SCRIPT_PRODUCTION_PLAN.md` |
| **B — Systemen & feel** | HUD eerst, dan de open dossiers, dan de backlog | `phase0/EXECUTION_PLAN.md` |

Spoor A raakt de build niet aan en kan dus altijd doorlopen.

## ✅ DE RODE TEST IS OPGELOST (31-07, `8e9bf53`)

`Eclipse.Feel.Input.DocumentedConsoleCommandsExist` stond drie rondes rood. Hij is
groen; de bar staat weer vol.

**OORZAAK GEVONDEN 31-07 (gemeten uit het testrapport, niet geredeneerd).** De
melding staat gewoon in `Eclipse/Saved/TestReport/index.json`:

    Error | Expected 'stuurtekens: HANDOFF.md is leesbaar' to be true.

De test controleert de owner-bestanden op stuurtekens en leest `HANDOFF.md` **uit de
repo-root**. Dat bestand is op 31-07 naar `archief/HANDOFF.md` verhuisd toen
STATUS.md het verving. De test bleef in de root zoeken, vond niets, en viel om.

**De eerdere diagnose hierboven was fout en is weerlegd.** Er stond dat het harnas
niet startte. Het harnas start prima — dat staat letterlijk in hetzelfde rapport:
`harnas: lokaal=1 · staat=Playing · op-de-grond=1 · wereld-begonnen=1`. De conclusie
"vroege uitgang" was afgeleid uit een *afwezige* melding terwijl de melding er wel
was; niemand had het rapport geopend. Precies de vorm die `DEBUG_DISCIPLINE.md`
beschrijft: observeren met de tools, niet redeneren over code.

**Fix:** de bewaker dekt nu ook `STATUS.md` en `NIEUWE_CHAT_PROMPT.txt` — de twee
bestanden die HANDOFF.md hebben overgenomen als eerste wat een verse sessie leest, en
die stonden onbewaakt. En de faalmelding noemt voortaan het **pad** dat geprobeerd
is, zodat een verhuisd bestand zichzelf diagnosticeert in plaats van als harnasfout
gelezen te worden.

## Open dossiers (spoor B)

1. **Inslagspoor rendert niet — de transform-diagnose is WEERLEGD (31-07, `ac09980`).** Hier stond "transform-bug, geen rendering-bug". Dat is gemeten en het klopt niet: `SpawnImpactMark` zet het spoor **precies** waar de `FHitResult` het wil hebben. Headless 20 schoten (250–3955 cm, 360° yaw, pitch −35…+35, schutter nooit op de oorsprong): **20/20** binnen 1 cm van de inslagplek, **0,000 cm** afwijking van de bedoelde plek, 20/20 minstens 249 cm van de schutter. In de draaiende game bij 11 treffers: 0,00 cm verschil, 627–848 cm van de schutter, 884 cm vóór de camera, `inbeeld=1`. **Controleproef inbegrepen** — een spoor met de hand óp de schutter wordt door beide eisen afgekeurd, dus de meting kán rood worden. De oude diagnose rustte op een blok dat *vastzat aan het personage* en er dus per constructie stond.<br>**Bonus-eliminatie:** `AEclipseGameMode::OnWorldImpact` zet er al een **tweede** neer, met een ander materiaal en zonder rotatie (22 levend bij 11 treffers). Beide onzichtbaar — dat sluit "wie spawnt" én "dit ene materiaal" in één klap uit.<br>**Volgende stap, een meting en geen dertiende hypothese:** zichtlijn-trace van de camera naar elk levend spoor, zodat `inbeeld=1` een *vrij zicht ja/nee* wordt. Vrij zicht en niets te zien → RenderDoc. Nooit vrij zicht → dit harnas meet de vraag niet. Zelfde vraag staat open voor de 38 grondvlakken van de bouwer: van die laag is nooit vastgesteld **dát** hij rendert — is die ook onzichtbaar, dan is dit geen spoor-defect maar een hele beeldlaag.
2. **Trillen bij het schieten — GEMETEN 31-07 (`bc881f4`), mechanisme vast, fix nog niet gedaan.** De diagnose "oscillerend blendgewicht" klopt: pieken lopen **exact 1:1 met het aantal schoten** (10/20/27 schoten → 10/20/27 pieken), identiek bij 120, 60 én 77 Hz — dat derde raster juist omdat 120 en 60 allebei een veelvoud van het vuurinterval zijn en het dus samen eens kunnen zijn over een artefact. **Maar het mechanisme uit §4.2 was fout:** de speler draait **geen AnimBP** maar een C++-proxy, dus er is geen blend-node die met een aim-offset om bones vecht en de Rewind Debugger heeft niets om terug te spoelen. De echte oorzaak is dat `PlayOneShot` de envelope **bij elk schot herstart** (`OneShotTime = 0.0f`). Afkappen is uitgesloten als verklaring: bij snelvuur zónder stille frames zijn het exact dezelfde 27 pieken. **Volgende stap (aparte iteratie):** een doorlopende envelope die vanaf het huidige gewicht verder loopt; de falsificatie ligt klaar in `Tests/EclipseAnimOneShotWeightTests.cpp` — pieken moeten dan ≪ N worden bij ongewijzigde bemonstering.
3. **Zwevend wapen.** Ook bekend UE-gedrag: socket-lag van 0,5–1,5 frame door tick-volgorde. Oplossing in `DEBUG_DISCIPLINE.md` §4.1.

## Wat er 31-07 's avonds landde

- **Zes ingesproken zinnen** ("Contact." / "Taking fire!" / "Reloading!" × twee
  stemmen): gegenereerd (9 gecached, 6 nieuw, 0 mislukt) én bedraad via
  `Event.Squad.SelfAction`, met 2 s rem per soldaat. Twee van de drie momenten vuren
  echt; **herladen niet, want squadmates herladen nergens in de code.**
  Daarvoor moest de seed van *create-only* naar *ontbrekende regels aanvullen* —
  nieuwe zinnen bereikten een bestaand stemasset anders nooit.
- **Terugslag is additief geworden.** `Primary_Fire_Med_MSA` lag al in het
  Belica-pack (mesh space additive); er hoefde niets ingekocht te worden. Eerste
  poging faalde omdat ik de *bron* maskeerde — een mesh-space-additief telt ná de
  blend op. Werkende vorm: additief op een kopie, per bot ingemengd. Voet-omklappen
  2–3, gelijk aan wat er stond. **Niet aangetoond dat het beter is**, wel de vorm die
  de referentie vraagt (`DEBUG_DISCIPLINE.md` §4.2 oorzaak 4).
- **HUD-nulmeting** staat in `phase0/EXECUTION_PLAN.md §1b`.
- Owner-lijst van **22 naar 4**; zijn vier beslissingen uitgevoerd.

~~**Wat NIET is gelukt:** de spelergerichte HUD vóór `IsDebugHudAllowed()` zetten.~~
**→ DAT IS ALSNOG GELUKT, later op 31-07. Zie hieronder.**

## Wat er 31-07 's LATE avond landde

**De HUD-poort werkt — en dat is met eigen ogen op frames gecontroleerd, niet uit een
rapport overgenomen.** `Saved/Screenshots/HUD_spelerlaag/` toont in beide perspectieven
het richtkruis plus `AR_Foundry 19 / 30` (wapennaam én munitie, allebei ontbrekend bij de
nulmeting) en in 1e persoon een `HERLADEN`-indicator — **zonder één regel debugtekst**.
`HUD_volledig/` toont daarbovenop objectives, squad-orders en de commandoregel. De
spelerlaag staat in allebei. Daarmee is de blokkade uit §1b weg.

**Vier metingen die diagnoses omgooiden:**

1. **De rode test** was een verhuisd bestand, geen harnasfout (`8e9bf53`).
2. **Het inslagspoor is géén transform-bug** (`ac09980`) — 20/20 binnen 1 cm, 0,000 cm
   afwijking, mét controleproef. Terug naar de renderkant.
3. **Het trillen** komt van een envelope die per schot herstart, niet van vechtende
   blend-nodes; de speler draait geen AnimBP (`bc881f4`).
4. **De GPU-crash is een page fault, geen TDR-timeout**, en de breadcrumbs wijzen
   **SkyAtmosphere** aan — Lumen, TSR en SSR stonden alle drie op "Niet gestart"
   (`3d143e2`). Crashkans gemeten op 1-op-9, dus één schone proefrun bewijst niets.

**Ook geland:** de vitals-datalaag op de event-bus (`Event.Player.VitalsChanged`) — daarvóór
was het énige gezondheidsfeit dat de speler uitzond "dood", geraakt worden was volledig
stil. Act 1 volledig uitgetekend: 9 beat-sheets, 71 scène-stubs, 10 canon-conflicten.
Casting fase 1: 104 kandidaten voor **nul credits**. De testteller op het dashboard bleek
stuk in plaats van verouderd (verkeerd rapportpad).

**Twee dingen die alleen een frame liet zien:** er staat een **STOP-verkeersbord** in het
district (`EclipseGrayboxBuilder.cpp:1432`) terwijl `20_world_dressing_standard.md` §20.2
dat expliciet verbiedt — het document is 31-07 geschreven, de code nooit aangepast. En de
squad-lijst toont `45434C53` in plaats van namen: dat is de ASCII-kop "ECLS" van de GUID
uit `EclipseRosterLogic.cpp:15`, dus per constructie voor iedere soldaat gelijk.

## De drie documenten van Nathan — houd ze vers

Hij leest deze drie als **achtergrond** bij de keuzekaarten op het dashboard. Verouderde info kost hem een verkeerde beslissing, dus het dashboard kleurt ze **rood** zodra ze over hun houdbaarheid gaan.

| Document | Bijwerken wanneer | Max |
|---|---|---|
| `STATUS.md` | aan het eind van **elke sessie** | 24 u |
| `JOUW_ACTIES.md` | zodra een owner-actie verandert | 24 u |
| `BESTURING.md` | zodra de besturing verandert | 7 d |

Staat er een rood op het dashboard: eerst bijwerken, dan pas verder.

## Hoe je Nathan iets vraagt

**Niet in de chat — met een knop.** Zet je vraag in `phase0/owner_questions.json` (id, vraag, waarom, stappen, opties met label/waarde/gevolg, advies, vrijtekst). Hij ziet hem dan als kaart mét knoppen op het dashboard en beantwoordt hem met één klik.

Zijn antwoord komt in **`phase0/OWNER_ANSWERS.md`** — **lees dat elke sessie**, het is bindend. Verwerk het daarna in de tabel hieronder mét citaat.

## Wacht op Nathan

| # | Actie | Blokkeert |
|---|---|---|
| ~~O-1~~ | ~~Verloopdatum credits~~ — **✓ beantwoord 31-07: 21 augustus 2026.** Werkdeadline generatie = **19 augustus** (2 dagen buffer). Kalender staat in `SCRIPT_PRODUCTION_PLAN.md` §4. | — |
| ~~O-2~~ | ~~Commerciële rechten~~ — **✓ BEANTWOORD door Nathan zelf, 31-07 in de chat.** Verbatim: *"ja in mijn abonnement zitten auteursrechten en je kunt dat ook zien in de voice library of die voice rechten heeft."* Dus: plan ✓ én per stem zichtbaar. Beleid blijft: `voice-director` weigert elke Voice-Library-stem zonder commerciële licentie.<br>**Let op voor agents:** dit is één keer ten onrechte teruggedraaid met "Nathan heeft alleen O-1 beantwoord". Dat klopte niet — het antwoord viel in een chatsessie die niet in de repo staat. Draai dit niet nog eens terug; het citaat hierboven ís de bron. | — |
| O-3 | Stemmen kiezen uit de kandidaten (smaak, niet techniek) | alles wat gesproken wordt |
| O-4 | IJkmissie M1.1 beluisteren en goed-/afkeuren | de massaproductie |
| ~~O-5~~ | ~~Wapen~~ — **✓ BESLIST 31-07: "volledig".** Nathan kiest de grote variant: het wapen écht uit de karaktermesh, niet de goedkope tint-tussenstap. Vier falsificeerbare stappen (isoleren → los mesh per wapenfamilie → socket op `hand_r` → wisselogica eraan). Loopt bij een element-builder. Mijn advies was de goedkope stap eerst; hij koos anders en dat is zijn keuze. | — |
| ~~O-6~~ | ~~Stijlvraag~~ — **✓ BESLIST 31-07: "A".** De Borderlands-lock uit §15.5 **blijft**. Geen her-lock, geen materiaal opnieuw, toon-master blijft gelden. "Realistisch geloofwaardig" = geloofwaardig *binnen* de stijl; de klacht ging over compositie en dat bestaat in elke stijl. Deblokkeert de vormgeving van base en map — die worden gestileerd (`phase0/REFERENTIE_BASE_MAP.md` §3). | — |

## De staande kwaliteitsopdracht

**Graphics en uitgebreidheid zijn altijd de twee hoogste prioriteiten. Nooit de kortste weg naar het doel.**
Liever drie dialogen van twintig regels dan één van twee. Nooit één licht. Twaalf bark-varianten, geen zes.
Volledig in `21_quality_mandate.md` — lees dat één keer, het is kort.

## Harde werkregels

- Bouw UE altijd met **`-NoUba`**. Elke iteratie eindigt groen: build, tests, `EclipseValidateData` (0 fouten), EventCatalog in sync — vóór elke commit.
- Bouwvolgorde per spec = 14.5: dataschema → pure-logic core + tests → subsystem-wrapper + events → debug-UI → echte UI/content laatst.
- Commits: `[System] Verb summary (GDD-ref)`. Cross-system alleen via de event-bus.
- **Owner-consent:** geen installs/downloads/security-prompts zonder uitleg vooraf én akkoord. Nathans account heeft **geen adminrechten** — queue dat als owner-actie.
- **Bugs:** volg `phase0/DEBUG_DISCIPLINE.md`. Drie iteraties zonder diagnose = escaleren, niet doorgaan.
- **Geen `/loop`, geen ScheduleWakeup** — die vuren niet betrouwbaar bij onderbreking.
- **Nooit `PROGRESS.html` of `progress_auto.js` bewerken.** Data gaat in `progress_data.js`.
- **Nooit een dicht markdown-document schrijven dat voor Nathan bedoeld is.** Hij leest één scherm: het dashboard. Owner-informatie hoort in `progress_data.js` (`ownerActies`, met stappen) én in de owner-tabel van `STATUS.md` of `JOUW_ACTIES.md` — die leest de dashboardserver uit. Heeft het echt een eigen document nodig, dan als **HTML gekoppeld aan het dashboard**, niet als losse `.md`. Vuistregel: kan hij het niet in 30 seconden scannen, dan is het niet voor hem geschreven. (`EXECUTION_PLAN.md` is het voorbeeld van hoe het níét moet — dat is een agent-document.)

## Het dashboard

`START_DASHBOARD.bat` → **http://127.0.0.1:8377/**

Dit is het enige scherm dat Nathan leest. Het is live (ververst elke 5 s, ook zonder open chat), en het toont taken, owner-acties, de agents mét hun chats, alle documenten, screenshots, commits en het credit-ledger. Het ruimt ook zelf oude screenshots op (houdt de 50 nieuwste).

**Werk na elke iteratie `progress_data.js` bij** — `bijgewerkt`, de `taken`-statussen, en `ownerActies` (afgehandelde punten verhuizen naar `jijGedaan`).

## Waar de rest staat

| Nodig? | Lees dit |
|---|---|
| Wat we bouwen | `00_INDEX.md` → doc 01–20 |
| Volgende taak spoor B | `phase0/EXECUTION_PLAN.md` §2 |
| Volgende taak spoor A | `phase0/SCRIPT_PRODUCTION_PLAN.md` §4 |
| Hoe een dialoogregel eruitziet | `18_writing_standard.md` |
| Hoe credits besteed worden | `19_voice_production.md` §19.2 |
| Waarom de wereld er nu niet goed uitziet | `20_world_dressing_standard.md` |
| Een bug efficiënt aanpakken | `phase0/DEBUG_DISCIPLINE.md` |
| Historie vóór 31-07 | `archief/HANDOFF.md` (alleen als je echt iets zoekt) |
