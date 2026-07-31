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

---

## Ronde 2 — 2026-07-31, Q-3 opgelost + budgetcorrectie

### L1-R9 — Petra Voss krijgt een vingerafdruk

**Q-3's personage-helft is hiermee dicht.** Wat openblijft is de *stem*, en dat is terecht een owner-keuze — maar die volgorde klopt nu: `voice-director` kan pas casten als er een vingerafdruk ligt om tegen te casten, want §19.3 laat elke kandidaat dezelfde signature-regel spreken en die regel volgt uit de vingerafdruk.

Toegevoegd aan `18_writing_standard.md` §18.4 als nieuwe sectie *Recurring non-companion voices* — zij is geen squadlid, dus zij hoort niet in de companion-tabel:

| | |
|---|---|
| **Syntax** | Imperatieven met weggelaten voornaamwoord. Huishoudelijke en keukenzelfstandignaamwoorden waar iedereen om haar heen operationele gebruikt. Op één na de kortste regels van het spel. |
| **Tic** | Beantwoordt een vraag die ze niet wil beantwoorden door een taak uit te delen. |
| **Nooit** | Zegt nooit "je" of "jouw". Laat zich nooit bedanken. Noemt nooit wat haar is aangedaan. |

**Signature-regel voor §19.3, castklaar:**

> **Petra:** *"Sit. Eat. Then tell me who died."*

Acht woorden, drie imperatieven, geen voornaamwoord, en de derde draait. Hij toont in één regel dat ze instrueert, dat ze eten als gereedschap gebruikt, en dat ze **niet fragiel is** — ze vraagt zelf om de verliezenlijst. Als casting-instrument test hij precies het moeilijke: kan deze stem kortaf zijn zonder koud te klinken. Dat is de hele rol.

**Twee botsingen die ik bewust heb uitgezet, want beide waren echt:**

**1. Petra vs. Brick.** Twee laconieke personages in dezelfde cast is een strip-test-risico (§18.9 C1). Het mechanische verschil: **Brick geeft je een zelfstandig naamwoord, Petra geeft je een werkwoord.** Hij antwoordt met een naam, zij met een klus. Twee korte regels naast elkaar moeten daarop alleen al toe te wijzen zijn.

**2. Petra vs. Mara — en dit was de gevaarlijke.** De waarschuwing was juist. Mara's hele arc hangt aan één voornaamwoord: ze zegt het spel lang "wij" en spendeert "jij" precies één keer, in M1.8.S08, en dát woord ís de commando-overdracht. Petra spreekt vier scènes eerder in diezelfde climax. Als zij ook in de tweede persoon zou werken, stort die ladder in.

Dat doet ze niet, en het is geen toeval: **haar imperatieven laten het voornaamwoord weg.** *"Sit down."* bevat geen "you". Het gevolg is dat het woord schaars is over de hele climax, waardoor Mara's ene gebruik in een vacuüm landt. Het is met een regex te controleren, en dat is het punt — een ontworpen scheiding, geen gelukkige.

**Eén gereserveerde breuk:** precies één keer in de campagne **vraagt Petra in plaats van instrueert**. Niet vóór act 3. Ongebruikt in act 1. (Bewust een ándere soort breuk dan die van Mara — twee personages met hetzelfde apparaat is één te veel.)

---

### L1-R10 — De gedeelde achternaam

**Wat de canon al vastlegt, en dat is meer dan de vraag veronderstelde.** `02_story_bible.md` §2.4: Voss werd grootgebracht door tante **Petra Voss** nadat beide ouders stierven bij de **Foundry Collapse van 484 AE** — een "bedrijfsongeval" dat in AEGIS-dossiers staat als *optimal loss allocation*, ontdekbaar in act 3.

Rekenwerk: heden is 503 AE, Voss is 26 → hij was **zeven**. Petra is ±55 → zij was **36 toen ze een kind van zeven overnam**.

**Wat ik NIET heb gedaan.** De vraag liet drie deuren open: familie, naamgenoot, of iets dat de speler pas laat begrijpt. Twee daarvan zijn dicht en niet door mij: de bijbel zegt *tante*. Van Petra stiekem de moeder maken, of de naam een dekmantel, is een wijziging aan §2.4 en dus een **owner-beslissing**. Ik heb op "tante" doorgebouwd in plaats van eromheen.

**Wat wel van mij was, en wat ik vastleg:**

1. **Ze draagt de naam door bloed, niet door huwelijk.** Wiens zus ze is laat ik open — dat invullen kost canon en levert niets op.
2. **Zij is de laatste volwassene die de naam draagt.** De Foundry Collapse nam de anderen.
3. **De achternaam is daarmee de draad tussen de Foundry Collapse en de AEGIS-onthulling.** Act 3 laat zien dat AEGIS die doden boekte als *optimale verliesallocatie*. Dat is niet abstract: het gaat om de mensen wier naam de speler de hele game draagt, en Petra is de enige die er nog is om dat te dragen. Die draad lag ongebruikt in de canon.
4. **De productie-beperking wordt karakterisering.** §2.4 zet de achternaam vast *"for voiced dialogue"* — de voornaam is speler-gekozen en kan dus nooit ingesproken worden. Iedereen zegt daarom "Voss", en vanaf M1.8 "Cinder". **Petra is de enige levende die hem kende voordat een van beide woorden iets betekende** — en als zij "Voss" zegt, zegt ze ook haar eigen naam en die van haar dode broer of zus.
5. **Daarom noemt Petra de hoofdpersoon nergens iets.** Geen naam, geen "je". Ze gebruikt taken. Daarmee wordt de hardste VO-beperking van het spel een eigenschap in plaats van een gat — en dat is het soort oplossing waar de andere elf castingregels niets voor hoeven te doen.
6. **De act-1-beat waar dit alles samenvalt: M1.8.S99.** Op de avond dat Petra terugkomt, ruilen de cellen de familienaam in voor een symbool. **Zij is de enige in de kamer die niet meedoet aan het benoemen** — geen afkeuring, ze gebruikt gewoon geen namen. De vingerafdruk en de naamruling zijn dezelfde beat, en hij kost één shot-noot.

---

### L1-R11 — Het budget van 131.000 en wat er met de hub gebeurt

De hub-laag (was tier 4, ~33k) valt weg. **Drie dingen nagelopen in plaats van aangenomen:**

**1. De dragende plant overleeft, en niet per ongeluk.** `HUB.A1.mara_letters` was de plant voor Mara's opnames in acts 2–4 (canon §2.5). Maar **M1.8.S91 is een missiescène**, dus tier 2, dus wél ingesproken — en ik heb hem expliciet zo gespecificeerd dat hij zonder de hubscène volledig werkt, met de hub als voorwaardelijke verrijking. **AR-9's plant-redundantieregel beschermde tegen een budgetsnede, en daar had ik hem niet voor geschreven.** Hij was bedoeld tegen spelers die optionals overslaan. Dat hij ook dit vangt, is de beste aanwijzing dat de regel klopt.

**2. Twee andere hub-beats hebben een ingesproken achtervang, gecontroleerd:**

| Wat de hub droeg | Ingesproken achtervang |
|---|---|
| Brick vraagt om de namen van de doden van vóór zijn komst (`brick_bunk`) → zet M1.8.S90 op | **M1.4.S03** (zijn eerste regel ís drie namen) + **M1.4.S99** (Reyes vraagt naar de namen) — beide tier 2 |
| Reyes' ex-Dominion-register (`reyes_triage`) | **M1.4.S99** — haar grootste scène staat al in een missie |

De lasbeat in M1.8.S90 houdt dus zijn opzet. Dunner, niet stuk.

**3. De verleiding is hubmateriaal in missiescènes trekken. Niet doen.** Dat zou tier 2 opblazen — die zat volgens bevinding C-6 al ~15% over zijn oude, ruimere plafond — en het zou downtime-schrijfwerk in scènes proppen die ergens heen moeten. **De hub blijft tekst en wacht.** Precies wat §19.2 met de rest van het spel doet.

**Wat wél verandert: de schrijfvolgorde.** De twaalf hubgesprekken concurreren deze maand niet meer om credits, dus ze gaan naar achteren — ná de beat-sheets van acts 2–4. **Met één uitzondering: `HUB.A1.mara_letters` wordt nu geschreven**, want act 2's Mara-opnames worden deze week tegen die scène aan geschreven en ze moeten in dezelfde stem staan.

**Openstaande vraag voor `voice-director`, met het rekenwerk erbij.** Bevinding C-6 stond al open: act 1's verhaaldialoog is ~17.200 te genereren woorden (12.800 script + Voss' gender-verdubbeling + as-varianten) ≈ **~103.000 credits**, tegen een tier 2 dat toen 90.000 was. Bij een totaal van 131.000 kan tier 2 die 103.000 vrijwel zeker niet meer dragen naast tier 1's barks. **De knop is Q-7, het Voss-variantbeleid — niet het aantal scènes.** Varianten zijn de enige post in act 1 die je kunt halveren zonder een beat te verliezen; scènes schrappen kost plant-dragers, en AR-9 zegt dat die verplicht zijn. Ik heb de nieuwe tierverdeling niet gezien, dus dit is een vraag en geen ruling.
