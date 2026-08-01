# ECLIPSE — STATUS

*De compacte statuskaart. Vervangt het inlezen van `archief/HANDOFF.md` (154 KB → dit).*
*Bijgewerkt 2026-07-31 laat · **houd dit onder 5 KB**: details horen in het document dat
ze bezit, hier staat alleen wat een verse sessie moet weten om te beginnen.*

---

## Waar we staan

**Fase 2 — Vertical Slice "Thirteen Bullets"** is de actieve milestone.

Bar (zelf nagedraaid 02-08 ~03:25): build groen `-NoUba` · **250 tests, 0 gefaald, 0
niet gedraaid** · `EclipseValidateData` 7 validators / 9 assets / **0 fouten** ·
EventCatalog **40/40** · creditmeter- en kaartpoort groen.

> ⚠ **De BAR is ROOD op twee punten, en allebei liggen ze buiten de build.** Dat de
> suite groen staat is dus geen groene bar — de bar meet met opzet meer:
>
> 1. **`check_voice_resolves.py`** — `eclipse_fighter` heeft **vier slots** in de
>    scripts en **twee gekozen stemmen**, dus C en D hangen aan niets. Acht regels
>    die géén foutmelding geven maar **stilte**: een lege `ElevenLabsVoiceId` wordt
>    op Display gelogd en overgeslagen. **Owner-actie (O-3), bewust géén fallback** —
>    terugvallen op stem A maakt de bar groen terwijl die acht regels nog steeds geen
>    gekozen stem hebben.
> 2. **`validate_script.py`** — **4** bevindingen, allemaal `REGISTER`. Dat is voor het
>    eerst onder de vijf, en de twee die de tool zijn bestaan waard maakten zijn **weg**:
>    `M1.1.S99` handelde twee van drie uitkomsten af, `M1.6.S05` miste `split`. De laatste
>    vertakkingsfout — `M1.8.S99`, de slotscène — is op 02-08 gesloten met een **verklaarde
>    stilte** in plaats van een geschreven tak, want de speler die geen pact sloot hóórt
>    ook niets: dat is wat een geweigerde bondgenoot aan het eind van act 1 is. Er staan
>    nu **twee bewust stille takken** zichtbaar in het rapport — doorgelaten, niet verdwenen.
>
>    De vier die overblijven zijn cutscenes die de ondergrens van hun band met één tot drie
>    woorden missen. Drie ervan zijn onderzocht en **terecht rood**: hun verdedigende blokken
>    zijn nagerekend en elke toetsbare claim erin klopt. Er naar 20 woorden toe schrijven is
>    opvullen om een meter te halen.
>
>    Zijn **zelftest draait ervóór** in de bar: zakt het bewijs dat de controles rood kúnnen
>    worden, dan is het oordeel over het script waardeloos.
>
> De vier `Eclipse.Base.Vault*`-tests die hier eerder als derde punt stonden zijn **groen**:
> dat was in-flight werk en het is geland (`f3e5c0b`).

**Act 1 kost ~100.000 credits, niet 65.015.** Dat oude bedrag telde zes van de acht
missies: M1.7, M1.8 en de twaalf hub-gesprekken waren lege stubs en telden als nul, en
nul kosten leest als gratis. `Tools/count_generation_cost.py` maakt de telling
herhaalbaar en **drukt altijd zijn eigen dekking af**. Saldo 125.612 — act 1 alleen is
dus ~vier vijfde van alles wat er nog is, en O-14 is daarmee een andere vraag.

> **Spoor A staat verder dan spoor B. ACT 1 IS COMPLEET GESCHREVEN** — acht missies, de
> proloog en de twaalf hub-gesprekken. **71 bestanden, 1.630 regels, 98.035 credits**
> (gemeten, `Tools/count_generation_cost.py`; deze drie getallen worden sinds 01-08 door
> `Tools/check_owner_docs.py` tegen die teller bewaakt). Saldo 125.612, dus **act 1 alleen
> is 78% van alles wat er nog is** — barks, muziek, SFX en de acts 2 t/m 4 komen daar nog
> bij. Dat is de vraag op **O-14**.
>
> **Maar dat is niet wat je vandaag zou uitgeven.** `Tools/check_generation_ready.py`
> splitst het: **42 scènes / 59.082 credits staan KLAAR** (poort groen én stem gecast),
> 18 scènes / 19.920 zijn door de kwaliteitspoort gehouden en 11 scènes / 18.961 wachten
> op casting. Het grote getal is wat act 1 *zou* kosten; dit is wat er *kan*.
>
> **DE IJKMISSIE IS DOOR DE POORT.** M1.1 staat op **7 van 7 GO** na drie critic-rondes.
> Corpus: **HET HELE CORPUS IS GEPOORT** — 71 van 71, in acht rondes. **53 GO, 7 NO-GO, 11 gehouden op de hub-reeks, nul onbeoordeeld**.
> De elf hub-scènes halen §18.9 apart maar de REEKS zakt — negen van twaalf delen
> dezelfde motor en elf van twaalf sluiten identiek af. Ze dragen daarom geen kaal
> `GO`: dat veld is een vergunning om te genereren.
>
> **De rem is nu de CASTING, niet de poort.** Alle acht stemmen van M1.1 resolven — en
> dat was waar én misleidend: **resolven is niet UNIEK resolven**. Drie stem-ID's staan op
> twee rollen, en twee daarvan zitten in de ijkmissie (Mara deelt een stem met een
> schutter, Dex met een Dominion-dienstplichtige in hetzelfde vuurgevecht). De generatie is
> daarop **afgeblazen met 0 credits uitgegeven**; `check_voice_resolves.py` vangt het nu.
> Zie **O-3/O-13**.

> ⚠ **Het `critic:`-veld werd tot 01-08 door niets gelezen** en stond 71 keer op `null`,
> ook voor scènes die als GO gemeld waren — die oordelen leefden alleen in agentrapporten.
> `EclipseGenerateVoicesCommandlet.cpp` noemt `critic`/`status`/`draft`/`GO` alle vier nul
> keer. **De poort is niet omzeild: de weg die hij bewaakt is nooit gebouwd** — die
> commandlet leest `DialogueSeed.json`, een bootstrap voor stem-*assets*. Wie de weg van
> script naar generatie bouwt, moet de poort erin zetten.

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

Spoor A raakt de build niet aan en loopt dus altijd door — dat is de reden dat hij
vooruit is gekomen terwijl het build-slot bezet was. De 71 scène-stubs uit
`phase0/beats/` zijn **geschreven**, inclusief de 12 hub-gesprekken. Casting fase 1 klaar:
**104 kandidaten over 19 rollen, 0 credits** — Petra ontbrak daar en wordt nu toegevoegd.

## Open dossiers — alle drie met een gemeten stand

*Volledige metingen staan in `phase0/DEBUG_DISCIPLINE.md`. Lees die vóór je iets aanraakt;
alle drie zijn eerder de verkeerde kant op gestuurd door een plausibele redenering.*

1. **Inslagspoor — GESLOTEN.** Het rendeerde altijd al; het was **14 pixels** groot vanaf
   de spelercamera. Nu 105 px op 15 m, 327 op 8 m. Twee conclusies zijn onderweg weerlegd
   (transform-bug, daarna renderbug) en de derde bleek geen bug maar een **maat**.
   Volledige meting, de controleproeven en de dubbele spawner: `DEBUG_DISCIPLINE.md` §4.3.
2. **Trillen bij het schieten** — mechanisme **vast** (`bc881f4`, §4.2): de envelope
   herstart per schot. Géén AnimBP, dus geen vechtende blend-nodes. Fix is een aparte
   iteratie; falsificatie ligt klaar.
3. **GPU-crash — GEPAUZEERD: hij reproduceert niet meer.** Page fault, geen TDR-timeout.
   De sky-hypothese is dood (die code draait één keer, vóór BeginPlay, op een map zonder
   zon) en **35 rondes met 123.572 geforceerde herbouwen gaven 0 crashes** — bij de
   historische 1-op-9 is dat 1,6% waarschijnlijk. **Niet verder zoeken zonder signaal.**
   Twee leads liggen open, waarvan één een owner-afweging is (een open editor tijdens een
   ronde kost het build-slot): `DEBUG_DISCIPLINE.md` §4.5.

**Twee dingen die alleen een frame liet zien — allebei GEREPAREERD, hier bewaard omdat de
manier waarop ze binnenkwamen niet gerepareerd is.** Het STOP-verkeersbord, de
ISO-361-stralingstrefoil en het doodshoofd-boven-TOXIC zijn weg uit
`EclipseGrayboxBuilder.cpp` (`4102aa0`); wat er staat zijn nu commentaren die uitleggen wat
er wás. De squadlijst toont geen `45434C53` meer — die GUID (`Strategy/EclipseRosterLogic.cpp`)
is nog steeds de deterministische id en hoort dat te blijven, hij werd alleen als naam
getoond.

> **Waarom ze hier blijven staan.** Alle drie de borden kwamen door drie curatie-ingangen
> heen die alleen op **techniek** en **toon** oordeelden — een foto van een aards bord haalt
> een toonfilter moeiteloos. Daarom staat er sinds 31-07 een **criterium 0 (PAST DE FICTIE?)**
> vóór de vijf technische criteria in `phase0/CURATIE_ENVPACKS_2026-07-25.md`, met twee
> afgeleide regels: *beoordeel het asset, niet zijn naam* (juist de drie die onschuldig
> héétten — `labor`, `route`, `reactor` — waren letterlijke verkeersborden) en *een pack faalt
> per FAMILIE, niet per asset*.

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
- **Bewerk bestanden met de Edit-tool, niet met een script — tenzij de wijziging herhaald
  of berekend is.** Dit ging op 01-08 **twee keer** mis, in twee smaken:
  - `s.find()` geeft **-1** als het anker niet matcht, en `s[:-1] + nieuw + s[29:]` plakt
    het hele document er een tweede keer achter terwijl je `print("bijgewerkt")` gewoon
    afgaat. `STATUS.md` ging van 11 naar 24 KB met elk kopje dubbel, op één afwijkend
    aanhalingsteken.
  - Een regex met `re.S` en een niet-verankerde `.+?` matcht over het **hele bestand**.
    `check_owner_questions.py` klapte daarmee tot één regel samen.
  - **Lezen met `utf-8-sig` en schrijven met `utf-8` haalt de BOM eraf.** Zonder BOM leest
    PowerShell 5.1 een `.ps1` als ANSI, en dan breekt de eerste em-dash de parse — op een
    regel die je niet hebt aangeraakt. `verify.ps1` viel daarop om. Schrijf een `.ps1` of
    `.bat` terug met **`utf-8-sig`**, en parse hem daarna.

  Moet het toch met een script: **assert dat het anker precies één keer voorkomt vóór je
  schrijft**, laat het script **atomair falen** (bouw de hele nieuwe tekst op en schrijf pas
  aan het eind — dan laat een mislukte tweede stap niets half achter), en controleer daarna
  de bestandsgrootte tegen wat je toevoegde. Escaping in een heredoc vecht bovendien met
  Python-escapes; bij `
` of quotes in het patroon is de Edit-tool altijd sneller.
- **`.git` staat op 3,8 GB en dat is onderhoud, geen probleem.** Gemeten 01-08:
  `size-pack` is **326 MB** — de historie zelf is gezond. De rest zijn **2.873 losse
  objecten** (3,4 GB) die sinds de laatste opruiming niet gepakt zijn. `git gc` haalt dat
  weg zonder één commit te wijzigen. **Doe het niet terwijl er een agent draait die credits
  uitgeeft** — gc zet de repo kort op slot, en dat risico is klein maar een verwarde
  generatie is duur. De grootste enkele objecten zijn 4K-textures van 12–27 MB in
  `Art/Imported/Textures`; die horen er via de `SOURCES.md`-route thuis en blijven.
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
