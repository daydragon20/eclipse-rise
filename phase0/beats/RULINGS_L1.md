# L1 RULINGS — beslissingen van de story-architect
*Bindend voor `dialogue-writer`, `dialogue-critic`, `voice-director` | eigenaar: story-architect*
*Log. Nieuwe rulings komen onderaan. Een ruling wordt nooit stilzwijgend herroepen.*

---

## Ronde 1 — 2026-07-31, uit de acht escalaties van M1.1 (ijkmissie)

**Lees dit als je nu aan M1.2 t/m M1.6 werkt.** Vier van deze rulings veranderen wat je schrijft; drie veranderen alleen wat de validator doet. R1 en R2 zijn de belangrijkste.

| # | Onderwerp | Uitkomst | Raakt jou als schrijver? |
|---|---|---|---|
| **L1-R1** | Lengtebanden vs. variantieregel | **Banden zijn plafonds.** Ondergrens wordt een scène-check, geen regel-check | **ja** |
| **L1-R2** | Spelerskeuzes hebben geen veld | **Nieuw `choice:`-veld** | **ja** |
| **L1-R3** | Drie uitkomsten passen niet in één boolean | **Meerwaardige vlaggen via tag-bladeren** | **ja** |
| **L1-R4** | Run-kwaliteit is geen story-flag | **Nieuwe `run.`-namespace** | **ja** |
| **L1-R5** | Drie voice-key-namespaces | **Castingbestand wint. Hernoemen is gratis (gemeten)** | licht |
| **L1-R6** | Speaker-canoncontrole faalt op naamlozen | **Rolsprekers toegestaan via patroon** | nee |
| **L1-R7** | Dex' grappenteller sprak zichzelf tegen | **Schrijver had gelijk; beat-sheet gecorrigeerd** | alleen M1.1 |
| **L1-R8** | `{name}` buiten barks | **Toegestaan overal, met twee voorwaarden** | licht |

Alle acht zijn doorgevoerd in `phase0/SCRIPT_FORMAT.md` (§3, §4, §5, §6). Dat document is de bron; hieronder staat waarom.

---

### L1-R1 — Banden zijn plafonds, niet bereiken

**De escalatie was juist en het was een echte fout in de standaard.** §18.3 schrijft banden voor *en* eist ≥3× variantie binnen een scène. Die twee kunnen niet allebei waar zijn:

| `type` | Band | Max. haalbare variantie |
|---|---|---|
| `in-mission-radio` / `walk-and-talk` | 10–25 | **2,5×** |
| `hub` | 12–35 | **2,9×** |
| `ambient` | 6–16 | 2,7× |
| `bark` | 3–8 | 2,7× |
| `cutscene` | 20–60 | 3,0× — en dan alleen in een scène die uitsluitend uit 20- en 60-woordsregels bestaat |

De schrijver merkte bovendien op dat het **canonieke voorbeeld in `SCRIPT_FORMAT` §3 zelf zakt** voor een ondergrens-controle: *"Thirteen."* is één woord in een band van 10–25.

**Ruling: het voorbeeld heeft gelijk en de controle had ongelijk.**

Lees §18.3's eigen motiveringen. *"Anything longer is not heard."* *"Must land inside the reaction window."* *"Player may walk away mid-line."* *"Keep one idea per line."* *"Camera is holding."* — **elke motivering betoogt een bovengrens. Geen enkele legt uit waarom een regel niet korter mag.** En `21_quality_mandate.md` §21.4 zegt het met zoveel woorden: *"Kort per regel. Gul in aantal."*

Er was dus nooit een leveringsreden voor een ondergrens. Er was een *kwaliteits*reden — een cutscene waarin niemand ooit een volle zin zegt, gebruikt zijn register niet — en die verdient een eigen controle in plaats van een verkleed minimum.

**Wat er nu geldt:**

| Controle | Geldt voor | Zakt wanneer |
|---|---|---|
| **Plafond** | elke regel | woorden **boven** de bovengrens |
| **Register** | scènes van **≥6 regels** | géén enkele regel haalt de ondergrens — de scène gebruikt de ruimte niet die zijn context hem geeft |
| **Spreiding** | scènes van ≥6 regels | langste ÷ kortste **< 3×** |

Scènes onder zes regels zijn vrijgesteld van register en spreiding. Een scène van vier regels heeft geen zinvolle verdeling, en compressie is het punt van een korte scène. M1.1.S03 is precies dat geval.

**Wat dit doet met de variantieregel: hij wordt eindelijk haalbaar én scherper.** Met de ondergrens weg is 3× triviaal te halen, en de enige manier om er nog voor te zakken is elke beurt even lang schrijven — precies de tell waarvoor §18.9 D de regel bedacht. Gemeten op de ijkmissie: **M1.1.S03 haalt 9×, M1.1.S05 haalt 15×.** De regel doet nu werk in plaats van in de weg te zitten.

**Nieuw veld `band:`.** Een radioscène met een vuurgevecht erin heeft callout-regels nodig. Dat is de meeste gevechtsscènes in het spel — in act 1 alleen al M1.1.S04, M1.3.S05, M1.5.S05, M1.6.S02, M1.6.S05, M1.7.S06 en M1.8.S07. Dat verdient een veld, geen afspraak. Regels met `band:` worden tegen de override getoetst en tellen **niet** mee voor de registercontrole.

---

### L1-R2 — `choice:`, want 42 missies vol keuzes hebben geen notitieveld nodig

De schrijver markeerde de drie opties in M1.1.S05 met `note:`. Dat werkt precies één missie lang.

```yaml
  - id:      M1.1.S05.160
    speaker: VOSS
    voice:   voss
    choice:
      group: m11_conscript
      set:   'story.m11_conscript_choice = "bound"'
      label: "Dress it."
    text:    "Dressing. Above the hip."
```

`group` bindt opties samen (volgorde-onafhankelijk, regels hoeven niet aaneengesloten te zijn). `set` gebruikt dezelfde grammatica als `condition` — één grammatica, niet twee.

**`label` zit er vanaf dag één in, en dat is met opzet.** In elke game die een dialoogwiel heeft, zijn de wieltekst en de gesproken regel twee verschillende stukken schrijfwerk. Dat onderscheid er na twintig missies in bouwen is hoe een script duizend mismatches krijgt. Laat `label` weg en de UI kort `text` af; dat is een prima default en het is een keuze, geen ongeluk.

---

### L1-R3 — Meerwaardige keuzes krijgen tag-bladeren, geen booleans

De schrijver had gelijk: mijn stub vroeg één boolean voor drie uitkomsten, en "gebonden en levend achtergelaten" moet te scheiden zijn van "laten doodbloeden" — anders klapt de T5-telling twee morele posities samen.

`FEclipseCampaignState.StoryFlags` is een `TArray<FGameplayTag>` zonder waarden. Drie uitkomsten worden dus drie elkaar uitsluitende bladeren onder één ouder:

```
Story.Choice.M11_Conscript.Finished
Story.Choice.M11_Conscript.Left
Story.Choice.M11_Conscript.Bound
```

Geen schemawijziging, en de ouder blijft opvraagbaar als *"is deze keuze überhaupt gemaakt"*. Script-zijde blijft `story.m11_conscript_choice == "bound"`; `script_to_seed.py` doet de vertaling.

**Dit geldt voor elke keuze met meer dan twee uitkomsten**, en er lopen er nu drie in productie:

| Keuze | Bladeren |
|---|---|
| M1.1.S05 conscript | `.Finished` · `.Left` · `.Bound` |
| M1.5.S04 shift-baas | `.Killed` · `.Prevented` · `.Warned` |
| M1.6.S04 de trein | `.Run` · `.Emptied` · `.Split` |

**`Story.Choice.M11_ConscriptSpared` is hiermee ingetrokken.** Wie hem in een `condition` gebruikt: vervangen door het blad. Twee opslagplekken voor dezelfde waarheid is een divergentiebug, dus er komt geen afgeleide boolean bij.

---

### L1-R4 — `run.`: run-kwaliteit is geen campagnestaat

`story.m11_zero_casualty` bestond niet en hoort ook niet te bestaan. "Is er iemand neergegaan tijdens deze missie" is een **run-feit**, geen campagnefeit. Als elke missie zijn runkwaliteit als story-flag zou wegschrijven, groeit de save met transiënte data voor 42 missies.

Twee namespaces, en het verschil is niet cosmetisch:

| Namespace | Woont in | Overleeft de missie |
|---|---|---|
| `story.` | `FEclipseCampaignState.StoryFlags` | ja |
| `run.` | `FEclipseMissionOutcome` | **nee** |

`run.zero_casualty` leest de bestaande downed-latch uit het SPEC-P2-04-amendement — **er is geen nieuwe state voor nodig, alleen toegang.**

**Dit corrigeert ook mijn eigen stub met terugwerkende kracht:** M1.2.S05 schreef ik als `story.m12_alarm`. Dat is `run.alarm_raised`. Wie M1.2 schrijft: gebruik de `run.`-vorm.

---

### L1-R5 — Eén voice-namespace, en hernoemen kost niets

Drie namespaces liepen door elkaar: dit document (`mara_sovann`), het live castingbestand (`mara`), en wat schrijvers verzonnen toen geen van beide paste (`dex_callum`, `elin_reyes`).

**Het castingbestand wint** (`progress_media/casting/casting_stage1.json`), want zijn sleutels zijn al gebonden aan echte ElevenLabs-stem-ID's en aan de tier-planning. `SCRIPT_FORMAT` §3 is aangepast, niet andersom. `voss` blijft `voss`: dat is een *logische* sleutel die bij de build naar `voss_m`/`voss_f` oplost, precies zoals bedoeld.

**De escalatie noemde dit blokkerend omdat de cache-sleutel de naam zou bevatten. Dat is niet zo, en ik heb het nagekeken in plaats van het aan te nemen** — `EclipseGenerateVoicesCommandlet.cpp:325`:

```cpp
const FString Key = UEclipseDialogueVoiceSubsystem::MakeCacheKey(
    Voice->ElevenLabsVoiceId, Voice->ModelId, Line.Emotion, Line.Text);
```

De sleutel hasht de **opgeloste ElevenLabs-ID**, niet de scriptsleutel. De indirectie die `SCRIPT_FORMAT` §4 belooft, bestaat echt. Hernoemen is gratis, vóór én ná Tier 1.

**Het echte risico is een sleutel die nérgens op uitkomt** — `dex_callum` bestaat in geen enkele tabel, en dan faalt generatie of valt hij stil terug op een default. Daarom staat er nu een "voice resolves"-controle in §6. Dat is de controle die geld bespaart; de naamgeving is netheid.

Nog steeds ongecast en blokkerend voor Tier 2: `petra`, `iron_chorus_emissary`, `dominion_officer`, `civilian_kessara_a/b` (Q-3/Q-4/Q-5).

---

### L1-R6 — Rolsprekers

Een zuivere glossary-controle kan nooit slagen, want AR-1 en AR-10 houden Embers gelederen met opzet naamloos. Geldig is: een glossary-naam **of** `^(FIGHTER|CONSCRIPT|VEIL|CIVILIAN|OFFICER|PRISONER)_[A-Z]$`. Al het andere zakt — inclusief een bijna-goede canon-naam, en dat is precies waar de controle voor bestaat.

---

### L1-R7 — Dex' grappen: de schrijver had gelijk

Mijn beat-sheet sprak zichzelf tegen tussen §5 en §10.5 — samen één grap te veel. De voorgestelde oplossing is beter dan mijn oorspronkelijke, en om de reden die de schrijver zelf gaf: **S06 grenst aan de nasleep, en §18.6 wil daar nul komische beats.**

**Vastgesteld: S01, S02, en S99 — en de derde alleen in de schone run.** Ligt er iemand gewond, dan is S99 nasleep en zijn het er twee. S06 is een vlakke redirect, geen grap.

`BEATS_M1.1.md` is gecorrigeerd. Dit is precies waar het escalatiepad voor bedoeld is: de schrijver zag de tegenspraak omdat hij hem moest uitvoeren, en heeft hem niet stil gerepareerd.

---

### L1-R8 — `{name}` mag overal

`SCRIPT_FORMAT` §5 definieerde de slot alleen voor barks, terwijl AR-10 hem in missiecontext nodig heeft: de vechter die Threx noemt in M1.7.S04 is de vechter die sterft in M1.8.S07, en die naam komt van het levende roster.

Toegestaan in elke `text:`, met twee voorwaarden die voor barks en scènes identiek zijn: **de omringende regel is voornaamwoordvrij** (geen "hij", geen "haar" — het roster bepaalt bij runtime het geslacht), en de slot verwijst naar een rosterlid dat de speler ontmoet heeft.

---

## De structurele keuze — "iedereen telt"

Dit is geen escalatie maar een ontwerpkeuze in de mal, en er werd terecht om een expliciet oordeel gevraagd.

**Oordeel: aangenomen, met één begrenzing.**

**Waarom hij goed is.** Het is niet één tic maar vier beroepen die dezelfde wereld tellen: Mara verdeelt dertien over vier mensen (leiderschap als verdeling), Reyes telt plasma twee keer (voorraadangst), Voss telt seconden van een lichtcircuit (technicus), de vijand telt een takel (arbeid). Dat is Kessara — een gieterijcultuur met ploegclans, scrip-lonen en rantsoenen. En het doet precies het werk dat §18.9 A verlangt: *"Four."* na een schot is een emotie leveren zonder er een te benoemen. Dat is de beste regel in de missie en er staat één woord.

Het overleeft ook de strip-test (§18.9 C1) **beter** dan neutraler geschreven dialoog, omdat elk personage in zijn eigen eenheid telt. Dat was de zorg die ik had toen ik de escalatie las, en het antwoord staat al in de tekst.

**De begrenzing: dit is het register van Kessara, niet van het spel.** Zonder die grens is het over 42 missies geen cultuur meer maar een gewoonte van de auteur — §18.9 D, iedereen deelt één tic. Dus:

1. **Act 1 telt.** Kessara telt. Blijf tellen.
2. **Act 2 verlaat Kessara en verandert van register.** Tarsis is een bergingscultuur met schuld-en-eer en "hull-right"-recht (`03_world_design.md` §3.3): dat register is **claimen en verschuldigd zijn**, niet tellen. Krad-9's lucht-broederschap is **delen**. Dat maakt van een sterke lokale keuze een planeet-identiteitssysteem, en het bedient `03_world_design.md` productieregel 4 rechtstreeks: *"culture through systems"*, niet alleen codex-tekst.
3. **Maximaal één telbeat per scène draagt de wending.** Tellen is hoe de wereld praat; het is niet de enige manier waarop een scène kan landen.

**Mara's `"His mark."` — aangenomen, en het is beter dan wat ik had.** Twee woorden tegen de squad in plaats van een toespraak tegen Voss. Het botst niet met AR-5: dat is derde persoon, en de gereserveerde "jij" blijft ongebruikt tot M1.8.S08. Sterker nog, het **versterkt** AR-5 door er een ladder van te maken:

> M1.1 — tactisch gezag, in de **derde persoon**, aan de squad: *"His mark."*
> M1.8 — commando, in de **tweede persoon**, aan hem.

Twee verschillende grammaticale bewegingen, oplopend, acht missies uit elkaar. Dat is architectuur die ik niet had bedacht en die ik overneem. **Vastgelegd als AR-5b.**
