# ECLIPSE — STATUS
*De compacte statuskaart. Vervangt het inlezen van HANDOFF.md (154 KB → dit).*
*Laatst bijgewerkt: 2026-07-31 · werk dit bij aan het eind van elke sessie, houd het onder 5 KB.*

---

## Waar we staan

**Fase 2 — Vertical Slice "Thirteen Bullets"** is de actieve milestone.
Bar: build groen · **185/185 tests** · validatie 0 fouten · EventCatalog in sync · werkboom schoon (31-07 18:08).

> Let op: `progress_auto.js` is van 26-07 en meldt nog 110 tests. Dat cijfer is verouderd — het dashboard toont er eerlijk "5 dagen geleden" bij. Draai `Tools/update_progress.ps1` om het te verversen.

De owner-stop op nieuwe features is **opgeheven** (31-07). Er geldt nog één voorwaarde: Nathan speelt pas als de **schermlaag (HUD)** op niveau is — dat is nu spoor-B-prioriteit 1.

## Twee sporen lopen nu parallel

| Spoor | Wat | Waar staat het |
|---|---|---|
| **A — Schrijven & stem** | Hele campagne uitschrijven, Act 1 inspreken vóór de credits vervallen | `phase0/SCRIPT_PRODUCTION_PLAN.md` |
| **B — Systemen & feel** | HUD eerst, dan de open dossiers, dan de backlog | `phase0/EXECUTION_PLAN.md` |

Spoor A raakt de build niet aan en kan dus altijd doorlopen.

## Open dossiers (spoor B)

1. **Inslagspoor rendert niet.** **Diagnose staat en is nu ook gemeten bevestigd (31-07):** het gespawnde object verschijnt *bij het personage*, niet op de inslagplek — dat is een **transform-bug, geen rendering-bug**. De twaalf eerdere "uitsluitingen" zaten in de verkeerde helft van de zoekruimte. Volgende stap per `DEBUG_DISCIPLINE.md` §4.3: hit-locatie uit de trace naast de uiteindelijke spawn-transform loggen, 20 schoten. Geen dertiende hypothese.
2. **Trillen bij het schieten — NIET opgelost.** Stand 31-07: de **additieve terugslag-take is geland** (`b19929e`, ligt nu op het bovenlijf), maar de hand-omklappen staan nog op **28**. Dat was oorzaak 4 van de vier in `DEBUG_DISCIPLINE.md` §4.2. **De hoofdverdachte is oorzaak 1: een oscillerend blendgewicht** — bovenlichaamslaag en aim-offset die om dezelfde bones vechten. Begin daar, en begin met *kijken*: open de **Rewind Debugger** op de AnimBP en lees het gewicht per frame af. Niet opnieuw repareren vóór die meting er is — twee eerdere fixes zijn juist daarom teruggedraaid.
3. **Zwevend wapen.** Ook bekend UE-gedrag: socket-lag van 0,5–1,5 frame door tick-volgorde. Oplossing in `DEBUG_DISCIPLINE.md` §4.1.

## Wacht op Nathan

| # | Actie | Blokkeert |
|---|---|---|
| O-1 | **Verloopdatum van de 310k ElevenLabs-credits bevestigen** | de hele spoor-A-kalender |
| O-2 | Commerciële gebruiksrechten bevestigen op het abonnement | bulk-generatie |
| O-3 | Stemmen kiezen uit de kandidaten (smaak, niet techniek) | alles wat gesproken wordt |
| O-4 | IJkmissie M1.1 beluisteren en goed-/afkeuren | de massaproductie |
| O-5 | **Wapenbron** — er bestaat nergens een los wapenmesh | wapenwerk spoor B |
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
