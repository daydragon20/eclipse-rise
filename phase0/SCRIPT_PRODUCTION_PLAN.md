# SCRIPT PRODUCTION PLAN — het lagenplan
*Werkdocument | aangemaakt 2026-07-31 | owner-instructie Nathan | eigenaar: story-architect*
*Bronnen: 18_writing_standard.md, 19_voice_production.md, SCRIPT_FORMAT.md, 02_story_bible.md, EXECUTION_PLAN.md*

---

## 0. Waarom dit document bestaat

De eigenaar heeft een productiemodel opgelegd: **werk in lagen, en leg elke laag volledig voordat je de volgende begint.** Zijn argument, verbatim:

> *"Als je een hele missie hebt gemaakt van tekst tot graphics en je wilt dan iets veranderen, dan kan het niks structureels zijn — omdat dan al de dingen daarboven ook in elkaar vallen."*

**Dat argument klopt en het is de kern van dit plan.** Een dialoogregel veranderen nadat hij is ingesproken, geanimeerd en van camera's voorzien, kost het tienvoudige van diezelfde verandering op papier. Door de bovenste laag compleet en gelockt te maken, wordt élke latere verandering cosmetisch in plaats van structureel.

Dit document maakt dat model uitvoerbaar, met **één correctie** (§3) die het model redt in plaats van het te ondermijnen.

---

## 1. Twee sporen, niet één

De grootste fout die dit plan kan maken is watervalwerk: eerst alles schrijven, dan pas bouwen, en drie maanden lang geen speelbare game. Dat gebeurt hier niet, om een structurele reden:

> **Het schrijfspoor raakt de build niet aan.** Het produceert `.yaml`-bestanden in `Content/Script/`. Het compileert niets, het breekt niets, het heeft geen groene bar nodig, en het kan niet stuk. Daarom kan het op volle snelheid parallel lopen aan het bouwspoor, permanent.

| | **Spoor A — Schrijven & Stem** | **Spoor B — Systemen & Feel** |
|---|---|---|
| Produceert | scriptbestanden, audio-assets | C++, data, UI, art |
| Agents | story-architect, dialogue-writer, dialogue-critic, voice-director | main, element-builder, hud-builder, code-reviewer, art-reviewer |
| Groene bar nodig? | nee | ja, elke iteratie |
| Geblokkeerd door | niets | de openstaande dossiers |
| Deadline | **credits vervallen einde maand** | geen |

De sporen raken elkaar op precies twee plekken: de **ijkmissie** (§3) en **laag 5** (§4), waar tekst code wordt.

**Wat spoor B als eerste doet** — dit is expliciete owner-prioriteit, want het is Nathans poort om de game ooit zelf te spelen:

1. **De schermlaag (HUD).** Alles rond het vizier in 1e én 3e persoon: munitie, herlaad-indicatie, magazijnen, minimap, squad-status, gezondheid, objective-marker, stance. Nathan speelt pas als dit op niveau is. `hud-builder` bezit dit. Het is ook infrastructuur: zodra het staat, kan Claude de game zelf zinvol testen — daarna gaat elke andere iteratie sneller.
2. De twee open dossiers: het inslagspoor dat niet rendert, en het trillen bij het schieten.
3. Het wapen (owner-keuze, zie `JOUW_ACTIES.md` O-5). Er *hángt* een wapen; het zit alleen in de karaktermesh in plaats van als los object. Dat verklaart de ontbrekende attachment én de wapenwissel die visueel niets doet. Goedkope tint-stap of volledige isolatie — dat is Nathans weging, geen inkoop.
4. Daarna de reguliere `EXECUTION_PLAN.md`-backlog.

---

## 2. De lagen

Elke laag is **compleet en gelockt** voordat de volgende begint. Een laag is gelockt wanneer zijn gate groen is.

| Laag | Inhoud | Wie | Gate om te locken |
|---|---|---|---|
| **L0** | **IJkmissie** — M1.1 door álle lagen heen (§3) | allen | De eigenaar heeft de missie gehoord en gezegd: "zo moet het klinken" |
| **L1** | **Verhaalfundering** — beat-sheets voor alle 42 authored missies, 4 acts | story-architect | Continuïteitsaudit: elke twist geplant én betaald, geen weesdraad, geen tegenstrijdigheid met `02_story_bible.md` |
| **L2** | **Dialoog** — elke regel van elke scène, `SCRIPT_FORMAT`-conform | dialogue-writer ×N → dialogue-critic | 100% van de scènes op `critic: GO`. Steekproef van 10 scènes doorstaat de strip-test (§18.9 C1) |
| **L3** | **Stem** — generatie in credit-tier-volgorde | voice-director | `VOICE_LEDGER.md` sluit; **131k besteed, 0 over** op 19/08; alles importeert en speelt af |
| **L4** | **Wereldbeschrijving** — per locatie: wat staat er, hoe ziet het eruit, wat moet bestaan | story-architect + art-reviewer | Elke missielocatie heeft een objectlijst die een element-builder zonder vragen kan bouwen |
| **L5** | **Assets & implementatie** — meshes, materialen, missies bedraad, scènes gespeeld | element-builder ×N + main | Groene bar + speelbaar + art-review GO |

**De regel die de lagen laat werken:** *binnen* een laag mag je zoveel itereren als je wilt. Je gaat nooit terug naar een laag erboven. Als L2 een gat in L1 blootlegt, is dat een **escalatie naar story-architect**, geen stille rewrite door de schrijver. Dat is precies het verschil tussen een fundering die houdt en een die verschuift.

---

## 2b. SCHRIJVEN WACHT NERGENS OP — correctie 31-07

**Er zat een valse afhankelijkheid in dit plan en die kostte dagen.**

De keten was: massaproductie wacht op de ijkmissie-gate → de gate wacht tot Nathan M1.1 *hoort* → horen wacht op stemgeneratie → generatie wacht op de ElevenLabs-scopes. Gevolg: één ontbrekend vinkje in een API-instelling hield **al het schrijfwerk** tegen, terwijl schrijven geen enkele credit en geen enkele API-aanroep kost.

**Zo hoort het:**

| Stap | Wacht op | Kan nu? |
|---|---|---|
| Beats (L1) | niets | ✅ act 1 klaar |
| **Dialoog schrijven (L2)** | **de beats** | ✅ **NU — voor heel act 1** |
| **Critic-gate** | de dialoog | ✅ **NU** |
| Stem genereren (L3) | de scopes | ⛔ geblokkeerd |
| Owner hoort de ijkmissie | de stem | ⛔ geblokkeerd |

**De regel:** de ijkmissie-gate bewaakt of de **schrijfstandaard** klopt. Dat oordeel kan Nathan grotendeels op **tekst** vellen — hij hoeft M1.1 niet te horen om te zien of de personages klinken als personages. Laat hem het script van M1.1 lezen zodra de critic GO geeft, en behandel de gesproken versie als bevestiging achteraf.

**Dus: schrijf door.** Act 1 mag volledig geschreven en gekeurd worden terwijl de scopes openstaan. Alleen de audio wacht. Als de scopes morgen aangaan, staat er dan een hele act klaar in plaats van niets.

---

## 3. De correctie: de ijkmissie (L0)

Dit is het enige punt waar ik van de opgegeven volgorde afwijk, en het is de reden dat de rest kan slagen.

**Het probleem met "schrijf eerst alles":** als je 132.000 woorden schrijft voordat je één regel in de game hebt gehoord, ontdek je een fout in de schrijfstandaard bij woord 132.000. Dan is de fundering compleet — en compleet verkeerd. Dat is precies het scenario dat het lagenmodel moest voorkomen.

**De oplossing:** vóór de massaproductie gaat **één missie door alle lagen tegelijk**. M1.1 *Thirteen Bullets* — hij is al gespecificeerd (SPEC-P2-04, ACCEPTED), hij is de openingsmissie, en zijn beats liggen vast.

```
M1.1  beats → dialoog → critic → stem → import → in PIE gehoord
      ───────────────── één week, ~8.000 credits ─────────────────
```

**Wat de ijkmissie bewijst, terwijl fouten nog goedkoop zijn:**

- Klopt de schrijfstandaard? Klinken de personages als personages?
- Werken de castingkeuzes? (Fout ontdekt hier: 8k credits. Fout ontdekt na laag 3: 90k credits.)
- Klopt de line-length-band voor radio tijdens gevecht, of praten ze over het schieten heen?
- Wat kost een missie écht in credits? Nu is de hele planning een schatting; na M1.1 is het een meting.
- Werkt `SCRIPT_FORMAT` → `script_to_seed.py` → generator → import → afspelen, end to end?
- Vindt de eigenaar het goed? **Dit is de belangrijkste vraag en hij moet vroeg worden gesteld.**

**Dit ondermijnt het lagenmodel niet — het kalibreert de mal voordat je er 42 giet.** Een gieterij maakt ook eerst één proefstuk. Nathan is derde-generatie gietertechnicus in de fictie van zijn eigen game; het beeld klopt.

**L0 is klaar wanneer** de eigenaar M1.1 in PIE heeft gehoord en akkoord geeft. Pas dan spawnt de massaproductie.

---

## 4. De sprintkalender

> ### STAND 02-08 VROEG — de kalender liep achter op de werkelijkheid
>
> Twee rijen hieronder waren onwaar geworden, en één ervan draagt een afhankelijkheid die
> **de owner zelf heeft geschrapt** (OBS-5, 31-07 21:01): *"er zat een valse afhankelijkheid
> in het plan. Massaproductie wachtte op de ijkmissie-gate, die op stemgeneratie, die op de
> scopes. Een ontbrekend vinkje hield daardoor AL het schrijfwerk tegen, terwijl schrijven
> geen credit en geen API-aanroep kost."*
>
> **Wat er in werkelijkheid staat:**
>
> | | |
> |---|---|
> | **L2 act 1** | **KLAAR.** 71 bestanden, 1.623 regels, alle acht missies + proloog + 12 hub-gesprekken. Niet "vanaf gate L0" — ruim ervóór, precies zoals OBS-5 voorschreef. |
> | **Ijkmissie M1.1** | **DOOR DE POORT.** 7 van 7 GO na drie critic-rondes. |
> | **Critic-gate** | **HET HELE CORPUS IS BEOORDEELD** (02-08, acht rondes). 49 GO · 9 NO-GO · 11 hub-scenes gehouden op de REEKS · 2 in behandeling. Die elf halen §18.9 apart maar de reeks zakte: negen van twaalf droegen dezelfde motor. Ze zijn herbouwd en staan opnieuw voor de poort. |
> | **Gate L0** | **niet gehaald, en niet om een schrijfreden.** De generatie is afgeblazen met **0 credits** omdat drie stem-ID's op twee rollen staan. Owner-kaart **O-16**. |
> | **Tier 0 casting** | fase 1 klaar (104 kandidaten, 0 credits) + een voorstel van tien stemmen met 19 previews. Wacht op één knop. |
> | **Begroting** | act 1 kost **97.659** gemeten (was 97.275 — elke scenereparatie schuift dit, en `check_owner_docs.py` bewaakt het sinds 01-08). Saldo 125.612. **Maar dat is niet het bedrag waarover besloten wordt:** `check_generation_ready.py` splitst het in **39 scenes / 55.729 credits KLAAR** (poort groen én stem gecast), 30.309 gehouden door de poort en 17.687 door de casting. |
>
> **Dagen tot de werkdeadline (19-08): 18.** Spoor A loopt daarmee **vóór** op deze kalender
> voor het schrijven en **achter** op de stem — en die achterstand is één owner-klik groot.
>
> *De rijen hieronder zijn de oorspronkelijke planning van 31-07. Ze blijven staan omdat de
> redenering eronder (waarom 19-08 en niet 21-08, waarom lagen per act locken) nog klopt.
> **Lees ze als plan, niet als stand.***



**De credits vervallen op 21 augustus 2026** (owner-bevestigd 31-07) en rollen niet door. Dat zijn 21 dagen. **Werkdeadline is 19 augustus** — twee dagen buffer, want een mislukte batch op de laatste avond is niet meer over te doen.

| Datum | Spoor A (schrijven & stem) | Spoor B (systemen & feel) |
|---|---|---|
| **31/07 – 01/08** | Casting: fase 1 = brede screening uit de Voice Library (**0 credits** — bladeren en previews zijn gratis), fase 2 = diepe test op 2 finalisten per rol. Eigenaar kiest, tabel gelockt (Tier 0, 12k) | HUD: de drie hoogtes, 1e/3e persoon |
| **02/08 – 06/08** | **IJkmissie M1.1** door alle lagen. Eigenaar hoort hem. | HUD verder; inslagspoor in de editor |
| **07/08** | **Gate L0.** Akkoord? → massaproductie. Nee? → standaard bijstellen, één iteratie, opnieuw. | trillen-dossier (Rewind Debugger eerst) |
| **31/07** | **L1 act 1 GELAND** — P0 + M1.1–M1.8 beat-sheets, 69 scène-stubs, plant/payoff-tabel. `phase0/beats/ACT1_OVERVIEW.md`. | |
| **01/08 – 11/08** | **L1: beat-sheets acts 2–4** — de 34 resterende van de 42 (26 verhaalmissies M2.1–M4.7 + 8 loyaliteitsmissies). story-architect, act voor act. Continuïteitsaudit aan het eind. | HUD-review + wapenbron zodra owner levert |
| **vanaf gate L0** | **L2: dialoog, act 1 eerst.** dialogue-writer ×4–6 parallel, één missie per agent. Critic-gate per scène. **Niet geblokkeerd door L1 acts 2–4** — act 1's beats liggen er al sinds 31/07. | reguliere backlog `EXECUTION_PLAN.md` |
| **10/08 – 19/08** | **L3: stem, in tier-volgorde.** Tier 1 barks → Tier 2 Act 1 → Tier 3 muziek → Tier 4 hub → Tier 5 SFX. Reserve niet toewijzen tot **17/08**, dan vrijgeven. Doel: **131k besteed, 0 over.** | audio-import, bark-bedrading |
| **19/08** | **HARDE STOP generatie.** Ledger sluiten. | |
| **20/08 – 21/08** | Acts 2–4 script-compleet afmaken (kost geen credits) | L4-wereldbeschrijving start |

**Waarom de generatie op 19/08 stopt en niet op de 21e:** als een batch op de laatste avond misgaat — verkeerde tag, verkeerde stem, afgebroken run — is er geen tijd meer om hem over te doen, en de credits zijn weg. Twee dagen buffer is het verschil tussen een deadline halen en hem missen.

**Parallelle overlap is bewust.** L2 start op dag 10, terwijl L1 nog aan act 3 werkt — omdat act 1's beats dan al gelockt zijn en schrijvers daar meteen op kunnen. Lagen locken *per act*, niet pas als alle vier klaar zijn. Dat is het verschil tussen een lagenmodel en een waterval.

---

## 5. Agent-orkestratie

Negen agents, waarvan vijf nieuw. Precieze systeemprompts staan in `.claude/agents/`.

| Agent | Spoor | Parallel? | Rol |
|---|---|---|---|
| `game-planner` | beide | 1 | Bestaand. Volgende taak + `progress_data.js`. |
| **`story-architect`** | A | 1 | **Nieuw.** Bezit L1 en L4. Beat-sheets, continuïteit, canon-bewaking. De enige die verhaalstructuur mag wijzigen. |
| **`dialogue-writer`** | A | **4–6** | **Nieuw.** Eén missie of trigger-set per agent. Schrijft tegen 18. Escaleert i.p.v. canon te wijzigen. |
| **`dialogue-critic`** | A | 2–3 | **Nieuw.** Alleen-lezen. Scoort §18.9. GO/NO-GO. Geen enkele regel gaat naar de generator zonder GO. |
| **`voice-director`** | A | 1 | **Nieuw.** Bezit casting, tags, generatie, de ledger. De enige agent die credits mag uitgeven. |
| **`hud-builder`** | B | 1–2 | **Nieuw.** Bezit de schermlaag: vizier, munitie, herladen, minimap, squad-status, 1e/3e persoon. |
| `element-builder` | B | **N** | Bestaand. Eén element per agent. |
| `code-reviewer` | B | 1 | Bestaand. GO/NO-GO vóór commit. |
| `art-reviewer` | B | 1 | Bestaand. §15.8-rondes. |

**Waarom er géén research-agent is.** Het onderzoek naar schrijftechniek is één keer gedaan en zit ingebakken in `18_writing_standard.md` §18.2–18.9, met bronnen in §18.11. Als elke schrijf-agent zelf research zou doen, betaalt de eigenaar veertig keer voor hetzelfde antwoord en drijven de standaarden uit elkaar. **Onderzoek doe je één keer en schrijf je op.** Dat is meteen de belangrijkste token-besparing in dit hele plan.

---

## 6. Wat dit plan bewust NIET doet

- **Niet de hele campagne inspreken.** Dat kost ~790k credits; er zijn er 131k. Alles wordt geschreven, ~40% wordt gesproken, de rest wacht als tekst op credits. Zie `19_voice_production.md` §19.2.
- **Niet de EXECUTION_PLAN vervangen.** De risk-first backlog, de falsificatietests (R1–R11) en de owner-wachtrij blijven leidend voor spoor B. Dit document is spoor A plus de koppeling.
- **Niet stoppen met bouwen tijdens het schrijven.** Zie §1.
- **Niet genereren zonder critic-GO.** Ook niet op D29. Zie `SCRIPT_FORMAT.md` §7.

---

## 7. Owner-acties (blokkerend)

| # | Actie | Blokkeert | Wanneer |
|---|---|---|---|
| ~~O-1~~ | ~~Verloopdatum credits bevestigen~~ — **✓ gedaan 31-07: 21 augustus 2026.** §4 staat nu op echte datums, met 19/08 als werkdeadline. | — | Afgerond |
| ~~**O-2**~~ | ~~Bevestig commerciële gebruiksrechten~~ — **✓ BEANTWOORD 31-07.** Het abonnement draagt ze, en per stem is in de Voice Library zichtbaar of die rechten heeft. Beleid dat eruit volgt staat in `19_voice_production.md` §19.3: `voice-director` weigert elke bibliotheekstem zonder commerciële licentie in fase 1, vóór hij een betaalde fase 2 kan bereiken. **Let op:** die licentie is *niet* uit de API te lezen (dat veld bestaat niet), dus het blijft één owner-klik per finalist. | — | gedaan |
| **O-3** | **Kies de stemmen** uit de kandidaten die `voice-director` voorlegt (§19.3). Smaak, niet techniek — dit is jouw beslissing. | L3, en dus alles | D2 |
| **O-4** | **Luister naar de ijkmissie** en geef akkoord of afkeuring. | L0-gate → massaproductie | D7–D8 |
| **O-5** | **Wapenbron** — er bestaat nergens een los wapenmesh (staande actie uit `JOUW_TAKEN.md`). | Spoor B wapenwerk | Staand |

---

## 8. Definition of Done — de maand is geslaagd als

- [ ] Alle 42 authored missies hebben een beat-sheet (L1 gelockt, alle 4 acts)
- [ ] Act 1 is volledig geschreven én gesproken en speelt af in PIE
- [ ] De bark-bibliotheek leeft: 16 triggers × 3 factievocabulaires
- [ ] Acts 2–4 zijn script-compleet als tekst, wachtend op credits alleen
- [ ] `VOICE_LEDGER.md` sluit op ≤290k, met ≥20k reserve onaangeroerd of bewust besteed
- [ ] Geen enkele gegenereerde regel zonder `critic: GO`
- [ ] De HUD-laag staat op een niveau waarop de eigenaar de game wíl spelen
- [ ] De groene bar is nooit rood geweest langer dan één iteratie
