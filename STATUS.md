# ECLIPSE — STATUS
*De compacte statuskaart. Vervangt het inlezen van HANDOFF.md (154 KB → dit).*
*Laatst bijgewerkt: 2026-07-31 · werk dit bij aan het eind van elke sessie, houd het onder 5 KB.*

---

## Waar we staan

**Fase 2 — Vertical Slice "Thirteen Bullets"** is de actieve milestone.
Bar: build groen · **184/185 tests — één rood, zie hieronder** · validatie 0 fouten · EventCatalog in sync.

> Let op: `progress_auto.js` is van 26-07 en meldt nog 110 tests. Dat cijfer is verouderd — het dashboard toont er eerlijk "5 dagen geleden" bij. Draai `Tools/update_progress.ps1` om het te verversen.

De owner-stop op nieuwe features is **opgeheven** (31-07). Er geldt nog één voorwaarde: Nathan speelt pas als de **schermlaag (HUD)** op niveau is — dat is nu spoor-B-prioriteit 1.

> **De HUD is niet één scherm.** Pijler 2 (`01_game_vision.md`): *One War, Three Altitudes* — **boots** (third-person, **wisselbaar naar first-person** met C/R3), **base** (management) en **map** (strategie), met **Command Mode** als overlay binnen boots. Elke hoogte heeft zijn eigen schermlaag, en alles rond het vizier moet in **beide** perspectieven leesbaar zijn. Volledige scope in `.claude/agents/hud-builder.md`.

## Twee sporen lopen nu parallel

| Spoor | Wat | Waar staat het |
|---|---|---|
| **A — Schrijven & stem** | Hele campagne uitschrijven, Act 1 inspreken vóór de credits vervallen | `phase0/SCRIPT_PRODUCTION_PLAN.md` |
| **B — Systemen & feel** | HUD eerst, dan de open dossiers, dan de backlog | `phase0/EXECUTION_PLAN.md` |

Spoor A raakt de build niet aan en kan dus altijd doorlopen.

## ⚠️ ÉÉN TEST STAAT ROOD — en hij is NIET van de wijzigingen van 31-07 avond

`Eclipse.Feel.Input.DocumentedConsoleCommandsExist` faalt. **Drie keer gedraaid, drie
keer rood, óók nadat de enige codewijziging van die avond was teruggedraaid** — dus
hij is niet veroorzaakt door dat werk. Bar verder: 185 tests, 1 gefaald, ValidateData
0 fouten.

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

1. **Inslagspoor rendert niet.** **Diagnose staat en is nu ook gemeten bevestigd (31-07):** het gespawnde object verschijnt *bij het personage*, niet op de inslagplek — dat is een **transform-bug, geen rendering-bug**. De twaalf eerdere "uitsluitingen" zaten in de verkeerde helft van de zoekruimte. Volgende stap per `DEBUG_DISCIPLINE.md` §4.3: hit-locatie uit de trace naast de uiteindelijke spawn-transform loggen, 20 schoten. Geen dertiende hypothese.
2. **Trillen bij het schieten — NIET opgelost.** Stand 31-07: de **additieve terugslag-take is geland** (`b19929e`, ligt nu op het bovenlijf), maar de hand-omklappen staan nog op **28**. Dat was oorzaak 4 van de vier in `DEBUG_DISCIPLINE.md` §4.2. **De hoofdverdachte is oorzaak 1: een oscillerend blendgewicht** — bovenlichaamslaag en aim-offset die om dezelfde bones vechten. Begin daar, en begin met *kijken*: open de **Rewind Debugger** op de AnimBP en lees het gewicht per frame af. Niet opnieuw repareren vóór die meting er is — twee eerdere fixes zijn juist daarom teruggedraaid.
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

**Wat NIET is gelukt:** de spelergerichte HUD vóór `IsDebugHudAllowed()` zetten zodat
de opnameronde hem kan zien. Teruggedraaid; zie §1b voor waarom die poort de eerste
bouwstap is.

## Wacht op Nathan

| # | Actie | Blokkeert |
|---|---|---|
| ~~O-1~~ | ~~Verloopdatum credits~~ — **✓ beantwoord 31-07: 21 augustus 2026.** Werkdeadline generatie = **19 augustus** (2 dagen buffer). Kalender staat in `SCRIPT_PRODUCTION_PLAN.md` §4. | — |
| ~~O-2~~ | ~~Commerciële rechten~~ — **✓ BEANTWOORD door Nathan zelf, 31-07 in de chat.** Verbatim: *"ja in mijn abonnement zitten auteursrechten en je kunt dat ook zien in de voice library of die voice rechten heeft."* Dus: plan ✓ én per stem zichtbaar. Beleid blijft: `voice-director` weigert elke Voice-Library-stem zonder commerciële licentie.<br>**Let op voor agents:** dit is één keer ten onrechte teruggedraaid met "Nathan heeft alleen O-1 beantwoord". Dat klopte niet — het antwoord viel in een chatsessie die niet in de repo staat. Draai dit niet nog eens terug; het citaat hierboven ís de bron. | — |
| O-3 | Stemmen kiezen uit de kandidaten (smaak, niet techniek) | alles wat gesproken wordt |
| O-4 | IJkmissie M1.1 beluisteren en goed-/afkeuren | de massaproductie |
| O-5 | **Wapen** — er hángt een wapen; het is alleen geen los object. Herweging, geen inkoop. Zie `phase0/REFERENTIE_TPS.md` §WIJ NU | wapenwerk spoor B |
| O-6 | **Stijlvraag**: blijft de Borderlands-lock, of wil je fotorealisme? | `20_world_dressing_standard.md` §20.8 |

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
