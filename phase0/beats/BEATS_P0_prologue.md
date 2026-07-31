# BEAT-SHEET — PROLOOG (P0) · de recap-opening
*L1 | story-architect | 2026-07-31 | hoort bij `ACT1_OVERVIEW.md`*
*Canon: `02_story_bible.md` §2.9 prologue · `01_game_vision.md` §1.7 moment 1 · `phase0/specs/SPEC-P2-04` decision 1 (ACCEPTED) · `phase0/RECAP_CARDS_M1.md` (bestaande copy)*

---

## 1. Dramatische functie

De proloog moet in negentig seconden vier dingen doen die de rest van de act nodig heeft, en verder niets:

1. **Wie ben ik** — een gewone Kessaraan die zijn hoofd omlaag houdt. Geen soldaat, geen uitverkorene.
2. **Wat kost dit systeem** — de Tithe, de rantsoenrij, het curfew. Niet uitgelegd: gezien.
3. **Waarom vecht ik** — Petra. Persoonlijk, niet ideologisch. De ideologie komt later en via Sela.
4. **Wie is Mara** — de vreemde die je eruit haalde. Zij is de reden dat je nog leeft, en dat is de schuld die de hele act draagt.

**Wat de proloog uitdrukkelijk NIET doet:** de speler vertellen dat er een rebellie is die gaat winnen. De cel is elf mensen in een dode geothermische vault. Dat moet klein voelen, want de hele game is de klim.

---

## 2. Wat er al ligt

`phase0/RECAP_CARDS_M1.md` bevat **zes gekeurde kaarten** (concept-draft main-agent, 2026-07-25), met cold-reader-protocol en implementatienotities. SPEC-P2-04 decision 1 staat **5–7** kaarten toe. De copy is goed: hij is kort, hij legt niets uit, en hij haalt de vier cold-reader-vragen.

**Ik wijzig dat document niet.** Het draagt een implementatiecontract (`FEclipseRecapCard`, `DA_CampaignSetup.RecapCards`) en een lopende owner-actie (cold reader). Twee schrijvers op één document is de divergentiebug. Wat hieronder staat is een **voorstel** dat pas na owner-akkoord (Q-2) in dat bestand landt.

---

## 3. Het gat — en de reparatie

**Bevinding C-2.** `01_game_vision.md` §1.7 moment 1 belooft:

> *"...of memorize the Enforcer's patrol badge (intel seed used in Act 1). No prompt tells them the third option exists."*

En `02_story_bible.md` §2.11 maakt daar een campagnedraad van:

> *"**The Enforcer** — the badge from the Prologue ration line can be traced across the campaign to a final personal choice."*

Toen de speelbare proloog geschrapt werd (SPEC-P2-04 decision 1 — terecht, om de juiste redenen), verdween die badge mee. De zes kaarten noemen hem niet. Daarmee heeft één van de vijf §2.11-draden **geen wortel**, en M1.2's intel-beat geen aanleiding.

Dit is precies het soort fout dat L1 hoort te vangen: niemand deed iets fout — een spec haalde twee uur inhoud weg, en één zin daarin was dragend voor twintig uur spel.

### Voorstel: een zevende kaart

Geplaatst **tussen de huidige kaart 2 (de Tithe) en kaart 3 (Petra)**, want de badge moet vóór Petra's arrestatie bestaan: het is het laatste moment waarop Voss nog *kijkt* in plaats van vlucht.

> ## Kaart 3 — De rij
> **Still:** rantsoenrij van bovenaf; een Enforcer trekt een man uit de rij; niemand draait zijn hoofd. Eén schouderplaat vangt licht.
> > A man ahead of you lost his work permit, and then his place in the line.
> > You did not look away, and you did not step forward.
> > You read the number on the Enforcer's plate, and you kept it.

Drie regels, geen uitleg, geen prompt. De speler krijgt precies de informatie die M1.2.S01 later uitgeeft. De laatste regel doet het werk van 01.7's "third option" zonder ooit te zeggen dat het een optie wás.

**Effect op de cold-reader-test:** geen. De vier vragen (wie ben ik / waar ben ik / waarom vecht ik / wie is Mara) worden door de bestaande kaarten gedekt; kaart 7 voegt geen vraag toe en beantwoordt er geen. Bij goedkeuring hernummeren de huidige kaarten 3–6 naar 4–7.

---

## 4. Scèneregister

Eén scène, omdat de zeven kaarten één ononderbroken beweging zijn en de credit-cache per scène batcht.

| Scène | Titel | Type | Tier | Bestand |
|---|---|---|---|---|
| P0.S01 | *Before* | cutscene | 2 | `Eclipse/Content/Script/act1/prologue/P0.S01_recap.yaml` |

**ID-reservering** (`SCRIPT_FORMAT` §2: alles in tientallen, nooit hernummeren): per kaart een blok van drie.

| Kaart | ID-blok |
|---|---|
| 1 de stad | 010 · 020 · 030 |
| 2 de Tithe | 040 · 050 · 060 |
| 3 de rij *(voorstel, Q-2)* | 070 · 080 · 090 |
| 4 Petra | 100 · 110 · 120 |
| 5 Mara | 130 · 140 · 150 |
| 6 de container | 160 · 170 · 180 |
| 7 Ember Cell | 190 · 200 · 210 |

Wordt kaart 3 afgewezen, dan blijft blok 070–090 **leeg en gereserveerd**. Niet hernummeren — dat is hoe een script zijn audio verliest.

---

## 5. Vlaggen

| In | Uit |
|---|---|
| — | geen |

De recap is **state-derived**: zichtbaar zolang `Story.Beat.M11_ThirteenBullets` niet gezet is én de geselecteerde missie MT_M11 is (SPEC-P2-04 decision 1 / `TAAK4_STORY_SURFACE.md` §4). Geen enkele vlag wordt hier gezet, en dat is bewust: de proloog bevat geen keuze, en een vlag zonder keuze is ruis.

---

## 6. Wendingen

| Wending | Rol van de proloog |
|---|---|
| **T1** Blight | niets. De Blight is nog geen woord dat de speler kent — §18.8: genoemde gebeurtenissen vallen terloops, lang voor hun uitleg. |
| **T2** Whisper = Ilan Vex | niets. |
| **T3** De mol | niets. |
| **T4** AEGIS liet de opstand toe | **indirect maar dragend.** De Petra-kaart zegt *"Then AEGIS came 'pre-compliance'"*. De speler leert AEGIS kennen als een **arrestatiemachine**, niet als een voorspeller. In Act 4 kantelt dat: hetzelfde ding dat Petra kwam halen, is het ding dat de Veil te weinig middelen gaf. Die eerste indruk moet hard en plat zijn. **Hier niet nuanceren** — de nuance is de betaling. |
| **T5** Kaine's geweten | niets. Kaine bestaat nog niet voor de speler. |

---

## 7. Draden (§2.11)

| Draad | Handeling hier |
|---|---|
| **Petra Voss** | GEOPEND (kaart Petra) → betaald in M1.8 |
| **The Enforcer** | GEOPEND — **mits kaart 3/7 (Q-2)**. Zonder die kaart is de draad een wees vanaf regel één. |
| Letters from the Wall | — |
| Iron Chorus | — |
| The Conscript Letters | — |

---

## 8. Groei

Niemand groeit. Dat is het punt: de proloog toont een man die niets doet, zodat M1.1 de eerste keer is dat hij iets doet. De speler moet na de laatste kaart het gevoel hebben dat hij **te laat** is — niet dat hij begint.

---

## 9. Instructies voor de dialogue-writer

- **Tweede persoon, verleden tijd.** Dit is de enige plek in het spel waar dat mag. Nergens anders wordt de speler verteld wat hij deed.
- **Geen kaart benoemt een gevoel** (§18.9 A). De bestaande regel *"That was the day keeping your head down stopped working"* is de grens en blijft er netjes onder: hij benoemt een **gedrag**, geen emotie. Schrijf op dat niveau.
- **Maximaal drie regels per kaart**, elke regel in één adem (§18.2 wet 5).
- **De bestaande zes kaarten zijn gekeurd.** Je herschrijft ze niet. Je vult alleen kaart 3 in als Q-2 groen is, en je zet de bestaande copy over in de stub.
- **Als Q-1 "ja" is** (ingesproken): het is Voss' stem, in beide gender-varianten. Tag-advies: `[quietly]` op kaart 1 en de Petra-kaart, verder ongetagd. **Geen `[grieving]`** — dat plakt een label op wat de tekst zelf moet doen (§19.4 regel 2).
- **Als Q-1 "nee" is**: de scène blijft bestaan als tekstbron, `voice-director` slaat hem over, het bestand verandert niet. Schrijf hem dus hoe dan ook.
</content>
