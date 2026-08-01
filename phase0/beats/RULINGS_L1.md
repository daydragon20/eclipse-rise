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

**Openstaande vraag voor `voice-director`, met het rekenwerk erbij (zie ook L1-R15).** Bevinding C-6 stond al open: act 1's verhaaldialoog is ~17.200 te genereren woorden (12.800 script + Voss' gender-verdubbeling + as-varianten) ≈ **~103.000 credits**, tegen een tier 2 dat toen 90.000 was. Bij een totaal van 131.000 kan tier 2 die 103.000 vrijwel zeker niet meer dragen naast tier 1's barks. **De knop is Q-7, het Voss-variantbeleid — niet het aantal scènes.** Varianten zijn de enige post in act 1 die je kunt halveren zonder een beat te verliezen; scènes schrappen kost plant-dragers, en AR-9 zegt dat die verplicht zijn. Ik heb de nieuwe tierverdeling niet gezien, dus dit is een vraag en geen ruling.

---

## Ronde 3 — 2026-07-31, uit de critic-doorloop van M1.5 (GO) en M1.6 (4× NO-GO)

### L1-R12 — De vlagbreuk tussen M1.5.S04 en M1.6.S06 was van mij

**Dit is mijn fout en het is de duurste van de twee missies.** Ruling L1-R3 maakte van de shift-baas-keuze drie bladeren. Ik heb dat doorgevoerd naar de **zetter** (M1.5.S04) en naar het vlaggenregister — en niet naar de **lezer**, die in een andere missie staat, bij een andere schrijver. M1.6.S06 las nog `story.choice_m15_shiftbossspared`, een boolean die niets zet en die de derde uitkomst niet kán uitdrukken.

Wat de speler die **waarschuwde** zou hebben gekregen: hij zei tegen de man *"be gone by the second horn"*, hoorde *"He's running. Wonder who to."* — en kreeg in de scène die de draad **sluit** ofwel een gebeurtenis die zijn eigen keuze tegenspreekt, ofwel de gedode tak. Eén op de drie spelers, op het sluitmoment, en M1.8 mag het niet heropenen.

**De les, en die is generaliseerbaar:** als een vlag van vorm verandert, breekt het bij de **lezer**, en de lezer staat per definitie in een andere missie dan de zetter. Ik heb bij L1-R3 de setter-kant gepatcht omdat die zichtbaar was in het bestand dat ik aan het bewerken was. Vanaf nu: **een vlagvormwijziging is pas doorgevoerd als de lezers geteld en gepatcht zijn**, en het vlaggenregister in `ACT1_OVERVIEW.md` §6 krijgt daarom per vlag een *gelezen door*-kolom die niet leeg mag zijn.

En: **mijn eigen `SCRIPT_FORMAT` §6-controle "condition resolves" had dit gevangen.** Ik heb hem gespecificeerd en hij bestaat nog niet in code. Dat is het argument om `validate_script.py` vóór act 2 te bouwen en niet erna — hij vangt de fouten van de architect net zo goed als die van de schrijvers.

De drie takken en de eisen voor de nieuwe `.270` staan in de stub. De regel zelf schrijft de schrijver; ik heb er één eis aan toegevoegd die de criticus niet kon zien: **de warned-tak is waar Sela het minst zeker is**, want dat is de tak waarin Voss slim was in plaats van principieel, en haar arc gaat over wat slim kost.

**Continuïteit die de warned-tak opent, en dus meteen sluit:** een gewaarschuwde shift-baas die wegloopt, kent de gezichten van twee cellen. Dat is een levend los eind. Opgelost in M1.8.S99: in die tak wordt één van P3-d's drie verklaringen *"de man die we lieten lopen"* **in plaats van** een van de andere. Drie blijft drie. Nooit vier — een vierde maakt van P3-d een opsomming en dan is het geen wond meer.

---

### L1-R13 — `split` is pact-gated, en de limited-tak krijgt géén vervangende derde optie

De escalatie van de M1.5.S99-schrijver was juist. Drie redenen, op volgorde van gewicht:

1. **Fictie.** Je draagt geen wagon met mensen over aan een cel waarmee je geen werkafspraak hebt. Er ís geen mechanisme om ze aan over te dragen — het pact *is* dat mechanisme. In de limited-tak is de Iron Chorus een bondgenoot van gelegenheid in dezelfde straat, geen partij die custody overneemt. Dit is geen beperking om de netheid; het is de afwezigheid van iets dat nooit gebouwd is.
2. **Structuur.** §2.11 vraagt van act 2 fusie, schisma óf tragedie, en dat kan alleen als de twee act-1-takken onderscheidbaar blijven. Met `split` in beide takken convergeren ze op identiek act-2-bewijs. Precies het argument van de schrijver.
3. **Karakter.** In de limited-tak zei Voss: *"We fight beside them. We don't take names off a list."* Een spel dat hem één missie later de namen-van-een-lijst-optie aanbiedt, heeft van die zin decoratie gemaakt.

**En de limited-tak krijgt twee opties, wat compleet is — niet later "repareren" met een verzonnen derde.** Run/empty ís het §2.7-archetype *The Cost of Speed*, op zichzelf heel. Dat het pact een donkerdere derde optie ontgrendelt, is de juiste beloningsvorm: je alliantie koopt meer ruimte én meer touw.

---

### L1-R14 — Elf is het getal van Ember Cell en van niets anders

De melding noemde vier banen voor het getal elf. **Ik heb geteld in plaats van aangenomen, en het zijn er negentien, over minstens acht betekenissen:** mensen, kooien, minuten, meters, geweren, patrouilledagen, verhoordagen, transportdagen.

Dat is geen botsing meer, dat is **verzadiging** — en het is de §18.9 D-fout in zijn zuiverste vorm: vijf schrijvers grepen onafhankelijk naar hetzelfde getal omdat de cel elf is en het aanvoelde als het getal van de act, en niemand kon het totaal zien. Precies waarom L1 boven L2 zit.

**Regel: elf betekent de omvang van Ember Cell.** Bunks tellen mee — het aantal kooien ís het aantal mensen, en dat is een goed motief. De elf opnames in M1.8.S91 blijven; ze spiegelen de cel bewust en zijn het beste gebruik in de act.

**Elk gebruik waarin elf iets anders meet, wijkt.** Niet in paniek — bij de eerstvolgende keer dat die missie toch open is.

Twee uitzonderingen op het tempo, en allebei om kosten:

| Geval | Besluit |
|---|---|
| **Brick, M1.6.S03 — *"Krad-9. Eleven days."*** | **Wijkt nu.** Drie scènes eerder betekent "eleven days" hoe lang een man een verhoor doorstond; Bricks regel landt daardoor als verhoor in plaats van transport, precies op het moment dat AR-11 betaalt. AR-11 heeft de **beat** nodig, niet het getal — elke duur die te lang aanvoelt in een kist doet hetzelfde werk voor nul kosten. M1.6 gaat toch terug. |
| **De verhoorduur, M1.5.S03** | **Blijft staan.** M1.5 is GO en wordt niet heropend voor een getal. Op de lijst voor de act-1-continuïteitsdoorloop; als een latere missie elf nodig heeft voor de cel, wijkt deze dan. |

De incidentele gevallen (meters, geweren, minuten, de dag-loop) wijken bij aanraking. Dat is een veegbeurt, geen noodgeval.

---

### L1-R15 — Drie woordtellingen, want één telt het verkeerde

Overgenomen en uitgebreid. `words` alleen straft precies de scènes die `21_quality_mandate.md` vraagt: M1.5 meet 1.865 geschreven woorden tegen een norm van ~1.600 en lijkt opgeblazen; haal de ~50 variantregels eruit en een speelbeurt hoort ~1.250. **De scène was niet te lang, hij was goed vertakt, en de meter zei het omgekeerde van de waarheid.**

Maar geen van beide getallen is het getal dat geld kost. Daarom drie, en het derde is nieuw:

| Vraag | Veld |
|---|---|
| Heeft de schrijver genoeg werk geleverd? | `words` |
| Is de scène de juiste lengte om te spelen? | `words_heard` |
| **Wat kost dit om te genereren?** | **`words_generated`** |

Een Voss-beat met vier assen kost **vier** generaties, acht met gender, en levert één regel aan `words_heard`. Act 1 begroten op `words` onderschat de uitgave; begroten op `words_heard` onderschat hem ernstig. **`words_generated` is de invoer voor Q-7** en het enige van de drie dat in een budget thuishoort. Staat in `SCRIPT_FORMAT` §4.

---

### L1-R16 — §18.9 B's fingerprint-check verwijst naar C1

Juist gezien. B zakt letterlijk zodra één regel verplaatsbaar is, en dan zakt elke scène ooit geschreven — *"Move."* en *"Nine minutes."* horen bij niemand in het bijzonder en dat is goed. De fout is een **scène** waarvan de regels breed uitwisselbaar zijn, en de maat daarvoor is C1's striptest met 80%. §18.9 B is aangepast: scèneniveau, gescoord op C1's lat.

---

### L1-R17 — De wegverklaringen van twist 4 zijn vlak, niet gevat

De scherpste vondst van deze ronde, en er was een criticus over twee missies heen voor nodig. De twee bestaande T4-wegverklaringen zijn allebei citeerbaar geschreven — *"Police always come afterwards."* en *"Freight yards get freight guards."* Allebei goede regels, en dát is het probleem.

Een aforisme is precies wat een speler onthoudt en teruggaat bekijken. **Vier daarvan maakt de vórm de tell:** de speler leert dat een net spreekwoordje betekent dat het spel iets verbergt, en lost twist 4 op door proza te herkennen in plaats van doordat act 4 het verdient. Twee is toeval; vier is een systeem.

**P4-c en P4-d zijn dus vlak.** Saai en waar, ongepolijst, geen ritme, geen slotbeat. P4-d is bovendien de zwakste van de vier by design — Mara herhaalt zichzelf onder druk, en een gepolijst epigram is het tegendeel van een vrouw wier verklaringen opraken. **Zou het goed op een poster staan, dan herschrijf je het.** Doorgevoerd in de stubs van M1.7.S03 en M1.8.S03 en in de plant/payoff-tabel.

---

### L1-R6b — `EMISSARY` toegevoegd aan de sprekertokens

De M1.5-schrijver had gelijk: L1-R6's patroon rechtvaardigde zichzelf op AR-1/AR-10, en die gaan over Embers **gelederen** — dat dekte nooit de tweede hoofdrol van een dialoogmissie. Hem in `FIGHTER_C` begraven zou zestig regels onleesbaar maken voor de striptest, en dat is precies de test waar die scène voor bestaat.

**En zijn §18.4-rij is opgenomen zoals bij Petra**, verbatim van de schrijver, want hij is beter dan de meeste bestaande rijen:

| | |
|---|---|
| **Syntax** | Korte mededelende zinnen. **Telt waar een ander een bijvoeglijk naamwoord zou pakken.** |
| **Tic** | Herhaalt het laatste woord van de ander vlak terug |
| **Nooit** | **Gebruikt nooit iemands naam** — niet die van Mara, niet die van Voss, niet die van zichzelf. Dreigt nooit. |

Dat "nooit" is het personage, geen tic: hij hoort bij een cel wier veiligheid haar trots is, en een man die geen namen afgeeft ís die leer, hoorbaar gemaakt. Het is bovendien met een regex te toetsen, en dat is waar §18.4 voor bestaat. Hij telt omdat hij Kessaraan is (ronde 1), maar hij telt **andere dingen** dan Ember: zij tellen wat ze hebben, hij telt wat dingen kosten.

Dit maakt Q-4 minder dringend: een personage dat zelf geen namen gebruikt, heeft er zelf ook minder één nodig.

---

## Ronde 4 — 2026-07-31, de laatste blokkade op de ijkmissie

### L1-R18 — De approach-keuze in M1.1.S01 vervalt (optie b)

`.210 "The span. I want the height."` / `.220 "The drain. Let them walk over us."` **worden één Voss-regel.** Geen vlag, geen `choice:`-blok, geen conditionele varianten in S02/S04.

**Dit was mijn fout, en het is dezelfde familie als L1-R12.** Mijn stub vroeg om *"een keuze in de planning (de overbrugging of de onderdoorgang)"* terwijl hetzelfde beat-sheet S02 op de **Overpass** zet en S04 op de reling. Ik heb een vertakking en haar tegenspraak in één document gespecificeerd. De schrijver heeft de regels netjes geschreven en terecht geweigerd een tag te verzinnen om ze formaat-conform te maken — precies wat `SCRIPT_FORMAT` §6 verlangt.

**Waarom (b) en niet (a), en de doorslaggevende reden is niet het budget:**

1. **Het level heeft geen duiker.** SPEC-P2-04 geeft M1.1 één site: de overlook-lane, met de hinderlaag die armt wanneer de squad daar positie houdt. Optie (a) zou betekenen dat ik dialoog schrijf voor een route die het missie-asset niet kan instantiëren. Dat is het exacte tegendeel van waar L1 voor bestaat.
2. **M1.1's echte keuze is S04, en die is beter.** Voss herrekent de hinderlaag wanneer de patrouille in de verkeerde volgorde binnenloopt — genomen in het moment, onder druk, met de grond voor zich. Een kaarttafelkeuze vóór je de grond gezien hebt is een zwakkere versie van dezelfde beat, drie scènes te vroeg.
3. **De insertiekeuze hoort bij M1.3**, waar hij de ontworpen agency-beat is (plein vs. servicetunnel) met twee volledige takken. Hem eerst in M1.1 doen maakt van M1.3's versie een herhaling in plaats van een escalatie.
4. Budget is de vierde reden, niet de eerste. Maar hij telt: (a) koopt varianten in twee scènes in de duurste akte van het spel, voor een keuze die geen enkele latere scène leest.

**De overblijvende regel is geen restant maar een verbetering.** Beat 4 van die scène is al *"Voss leest de route van de kaart en zegt iets technisch dat niemand vroeg."* Maak die regel over **de hoogte**, en laat hem betalen in S04 wanneer de patrouilleorde niet klopt en juist die hoogte hem de oplossing geeft. Een decoratieve keuze wordt een plant met een betaling twee scènes later.

**§18.7 blijft voldaan.** Die vraagt om *"a choice, an interjection, or a movement beat"*. Dit wordt een **interjectie**: volledige Voss-as-varianten op de overblijvende regel, dus de speler kiest *hoe* hij het zegt, niet *wat*. Kost niets bovenop wat een Voss-regel toch al kost.

**En voor de mal:** M1.1 houdt precies **één** echte keuze (S05 — vlag, lezer, drie takken). Dat is het model. Een tweede, decoratieve keuze in de ijkmissie zou veertig schrijvers leren dat keuzes een sausje zijn dat je erover strooit. **De missie is een sterkere demonstratie met één echte keuze dan met één echte en één nepkeuze.**

*Opmerking bij de critic-beslissing: NO-GO in plaats van "GO met hold" was juist. Was S01 eerst gegenereerd, dan had (b) twee regels × twee geslachten weggegooid. De poort heeft hier letterlijk geld bespaard.*

---

### L1-R19 — Reyes krijgt een nullijn, en het is `M1.4.S99`

Scherpe vondst. §18.4 geeft haar *"drops contractions **when stressed** — training reasserts"*. Dat is een **differentiële** tell, en een differentiaal zonder nullijn vuurt nooit. Ze gebruikt in M1.1 nul samentrekkingen — daar verdedigbaar, want ze staat in al haar optredens onder druk — en laat ze opnieuw vallen in M1.6.S06 en hard in M1.8.S07. **Zonder een vastgelegde ontspannen Reyes gaan vier akten schrijvers ervan uit dat dat clipped register háár stem is**, en dan is de tell onzichtbaar precies wanneer hij het hardst zou moeten landen.

**M1.4.S99 is de nullijn**, en het is de goede scène: ze is er maximaal in haar element — een zorgvuldig onderzoek, in de basis, na een overwinning, op haar eigen voorwaarden. Daar contraheert ze, gewoon, zonder commentaar.

De act-1-ladder wordt: **M1.4.S99 nullijn → M1.6.S06 laat vallen → M1.8.S07 laat hard vallen.**

`HUB.A1.reyes_first_conversation` is een tweede nullijn, maar **daar kan niet op geleund worden**: de hub-tier viel uit het budget (L1-R11), dus M1.4.S99 is de enige *ingesproken* nullijn die ze krijgt. Dat is een gevolg van de budgetsnede dat niemand bij die snede kon voorzien, en het is precies het soort ding dat L1 moet opmerken.

**Veralgemeend en in §18.4 gezet:** een *"when X"*-vingerafdruk heeft een **ingesproken niet-X** nodig, eerder. Dat geldt ook voor Torrens stiltes en Kaya's ondermijning; beide vallen in act 2 en krijgen dezelfde behandeling als ik daar ben.

---

### L1-R20 — Voorbeeld-ID's in `SCRIPT_FORMAT` zijn `M0.0.S00.*`

Het `band:`-voorbeeld wees naar `M1.1.S04.070`, waarvan de werkelijke tekst iets heel anders is. ID's zijn permanent en dit document is de mal voor veertig schrijvers; een voorbeeld dat botst met een echte regel is hoe een scène zijn audio verliest. **Act 0 kan niet bestaan, dus `M0.0` botst nooit.** Doorgevoerd.

---

## Ronde 5 — 2026-08-01, negen escalaties uit drie critic-rondes en twee schrijversrondes

**Twee daarvan zijn fouten in mijn eigen rulings, en ze staan vooraan omdat ze doorgelekt zijn.** Beide zijn fouten in een *telling* die ik niet had nageteld — precies waar L1-R14 goed van werd, en de reden dat die discipline een regel is en geen anekdote.

| # | Onderwerp | Uitkomst | Raakt jou als schrijver? |
|---|---|---|---|
| **L1-R21** | L1-R17 schreef twee wegverklaringen aan de verkeerde scènes toe, en er zijn er **drie** | Erratum. **M1.6.S05.410 wordt vlak** | M1.6, M1.7, M1.8 |
| **L1-R22** | L1-R19's Reyes-nullijn bestaat niet — nul samentrekkingen in tien regels | Erratum. M1.4.S99 repareert, en de tell vuurt **binnen** de nullijnscène | M1.4 |
| **L1-R23** | `M1.8.S05`'s `turn:` belooft een voornaam die niet bestaat | Nieuw `turn:`. **AR-6 trede 4 krijgt twee bewegingen; S06 is de top** | M1.8 |
| **L1-R24** | Welke drie cellen telt `M1.8.S99.010`? | Ember + de veertig + de bevrijden. **De Chorus is geen versmolten cel** | M1.8 |
| **L1-R25** | `M1.6.S05` mist de `split`-tak | **Een regelblok, geen stilte** | M1.6 |
| **L1-R26** | `run.m17_record` wordt door niemand gezet | **De regel blijft, de vlag wordt gedeclareerd, de doelstelling is een datataak** | M1.7 |
| **L1-R27** | De hub kan geen `choice:` dragen | **Hub-takken zijn `run.`** Nul `story.`-vlaggen uit de hub | alle hub |
| **L1-R28** | Elke hubscène moet een `condition` hebben en er is geen kopveld | **Scalair kopveld `condition:`** — geen parserwijziging | alle hub, **nog niet gebruiken** |
| **L1-R29** | `HUB.A1.reyes_first_conversation` wees naar een ingetrokken vlag | **De schrijver had gelijk.** Beat-sheet gecorrigeerd | hub |

---

### L1-R21 — Erratum op L1-R17: het waren er drie, en twee stonden op de verkeerde naam

**Dit is mijn fout en hij is doorgelekt naar twee scènebestanden.** L1-R17 citeerde twee T4-wegverklaringen en zette ze allebei in de verkeerde missie. Nageteld, in speelvolgorde:

| # | Regel | Waar hij écht staat | Waar L1-R17 hem zette | Vorm |
|---|---|---|---|---|
| 1 | MARA *"They mark everything. They come back for what is worth the night."* | **`M1.2.S03.150`** | genoemd noch geteld | parallellisme + slotbeeld |
| 2 | DEX *"They're police, not an army. Police always come afterwards."* | **`M1.5.S05.200`** | "M1.2.S03" | antithese + maxime |
| 3 | MARA *"It's a freight yard. Freight yards get freight guards."* | **`M1.6.S05.410`** | "M1.5.S05" | anadiplose |

**Er staan er dus drie vóór M1.7, niet twee**, en de derde staat in geen enkele rij van de P4-tabel — de M1.6-schrijver redeneerde correct dat het geen nieuwe *plant* was, en had daarin gelijk, maar L1-R17 gaat niet over plants. Hij gaat over **vorm**, en vorm telt onafhankelijk van de tabel. Een aforisme dat niemand als plant boekt, is nog steeds een aforisme dat de speler screenshot.

De verkeerde toeschrijvingen zijn overgenomen in de koppen van `M1.7.S03` (r. 53–54) en `M1.8.S03` (r. 49–50). Twee schrijvers hebben tegen een onjuiste inventaris gewerkt, en dat is de duurste soort documentatiefout: hij ziet er geverifieerd uit.

**Wat dit doet met de ruling zelf: hij wordt dringender, niet zwakker.** L1-R17 zei *"twee is toeval, vier is een systeem"*. Bij drie is de drempel al gepasseerd voordat P4-c geschreven wordt. De criticus zette `M1.7.S03` destijds mede op NO-GO omdat het de vierde zou zijn — **die grond is juist en blijft staan.**  <!-- premisse-nagelopen: dit BESCHRIJFT een beoordeling uit juli, het beweert geen huidige poortstatus -->

**Ruling, in drie delen.**

**1. `M1.6.S05.410` wordt vlak.** De beat blijft; de politoer gaat eraf. Drie redenen, op volgorde van gewicht: hij staat in geen P4-rij, zijn scène is nog `draft` (M1.5 is GO en wordt hier niet voor heropend — zelfde afweging als L1-R14), en hij is de meest hoorbaar *geschreven* van de drie. `run`/`emptied` delen deze staart, dus één regel repareert beide takken.

**2. En dat levert iets beters op dan wat ik oorspronkelijk had.** Vlak-vanaf-nummer-drie maakt van de reeks een **afbraakcurve** in plaats van een stijlregel:

> `M1.2.S03.150` gepolijst, zeker, rekenkundig — *het is competentie*
> `M1.5.S05.200` gepolijst, zeker, uit een andere mond — *het is consensus*
> **`M1.6.S05.410` de eerste barst: vlak, en een kleinere claim dan de vorige**
> `M1.7.S06.160` haar eigen M1.2-zin, woordelijk herhaald, met het onderwerp dichtgeslagen
> `M1.8.S03.130` twee redenen, geen van beide af
> `M1.8.S09.130` vier vlakke woorden op een lege radio, en niemand antwoordt

Zes aanrakingen, en de vlakheid is nu een **karakterfeit over Mara** — een vrouw wier verklaringen opraken — in plaats van een auteursregel. Dat is precies wat L1-R17 van P4-d vroeg, en het blijkt de hele reeks te kunnen dragen. De M1.6-schrijver had gelijk dat dit *"dezelfde verklaring die steeds moeilijker te zeggen wordt"* is; wat ontbrak was dat de vorm dat dan ook moet doen.

**3. De inventaris komt in `ACT1_OVERVIEW` §4 te staan, in speelvolgorde, met vorm.** De fout was mogelijk omdat de P4-tabel *plants* telt en niemand de *aanrakingen* telde. Een tabel die de ene meet en waartegen de andere gecontroleerd wordt, is geen meting (L1-R14, tweede helft).

**Wat er nu moet gebeuren:** `dialogue-writer` M1.6 herschrijft `.410` vlak en verwijdert niets. `dialogue-writer` M1.7 en M1.8 corrigeren de citaten in hun eigen koppen — de instructie eronder verandert niet, alleen de bronvermelding. `dialogue-critic` hoeft M1.5 niet te heropenen.

---

### L1-R22 — Erratum op L1-R19: de nullijn bestaat niet, en de reparatie is beter dan de oorspronkelijke opzet

**Nageteld in plaats van aangenomen, en de melding telde zelf ook mis.** `M1.4.S99` bevat **tien** Reyes-regels, niet negen: `.070 .090 .100 .110 .130 .140 .150 .170 .190 .200`. Samentrekkingen daarin: **nul.** *"That is a place."* — *"Then it is old… and I cannot open it tonight."* — *"There is nothing in my kit for that."*

De differentiaal vuurt dus tegen een identieke achtergrond en is onhoorbaar. L1-R19 is daarmee een ruling die alleen op papier bestond.

**En de oorzaak staat in hetzelfde bestand, acht regels uit elkaar.** Mijn L1-blok op r. 40–52 zegt *"SHE CONTRACTS HERE — 'I've', 'he's', 'don't'"*. De L2-schrijversnotitie op r. 86 zegt *"18.4: complete clinical sentences, **zero contractions**, questions instead of accusations."* Dat is §18.4's statische lezing, en de schrijver heeft die gevolgd en netjes gedocumenteerd. **De instructie stond er en verloor van de vingerafdruktabel**, want die tabel is de plek waar een schrijver kijkt als hij een stem moet halen. Dat is geen schrijversfout; dat is een ruling die op de verkeerde plek woonde.

**Ruling — en de reparatie is niet "maak haar overal ontspannen".**

Een scène waarin ze tien regels lang gelijkmatig contraheert, is een nullijn en verder niets. Beter, en het kost geen woord extra:

| Regel | Vorm | Waarom |
|---|---|---|
| `.070` `.130` `.140` | **contraheren** | inventaris, een droge correctie op Brick, een diagnose. Ze werkt, in haar element, na een overwinning |
| `.190` `.200` | **contraheren niet, en dat blijft zo** | dit zijn de twee regels ná *"Tam. Oyelaran. Vic."* |

**De tell vuurt dus binnen zijn eigen nullijnscène, één keer, zacht** — precies op het moment waarop Brick drie namen op tafel legt en zij haar tas dichtdoet. Daarna is dezelfde instrument in `M1.6.S06` luider en in `M1.8.S07` hard. Een nullijn die alleen nullijn is, leert een speler niets; een nullijn waarin de naald één keer uitslaat, leert hem waar hij naar moet luisteren.

**Dit is ook de generalisatie die L1-R19 had moeten hebben:** een *"when X"*-vingerafdruk heeft een ingesproken niet-X nodig, **en het beste ingesproken niet-X bevat één X.** Anders is het geen nullijn maar een ander personage. Geldt bij act 2 ook voor Torren en Kaya.

**Kosten: nul, en de grond daarvoor is dat er niets gegenereerd is** — niet de poortstatus. Dat onderscheid is op 02-08 aangebracht nadat `check_ruling_premises.py` meldde dat de oude formulering (*"staat op `draft`, `critic: null`"*) verlopen was: M1.4 is inmiddels beoordeeld en `S99` draagt een NO-GO. De **conclusie** stond nooit op die status maar op de generatie, en `check_generation_ready.py` bevestigt dat M1.4.S99 nog steeds niets heeft gekost. Zo hoort een ruling te steunen: op het feit dat hij bedoelt, niet op een toestand die ernaast ligt en verder loopt.

**Wat er nu moet gebeuren, vóór tier 2:** `dialogue-writer` M1.4 contraheert `.070` `.130` `.140` en corrigeert zijn eigen L2-notitie op r. 86 — die notitie is de instructie die de volgende schrijver ook fout zou zetten. Ik heb het L1-blok in dat bestand uitgebreid met de `.190`-uitzondering, want die was nieuw en stond er niet. `voice-director`: dit is een tier-2-blokkade, niet een netheidskwestie. **Losstaand en niet hier beslist:** `M1.4.S99` mist de registerdrempel met één woord (langste 19 tegen 20) — dat staat al in `SCRIPT_FORMAT` §4 als een van de drie bekende gevallen, en het is de goedkoopste beurt om er meteen naar te kijken nu het bestand toch open is. Het is een scènevraag, geen ruling.

---

### L1-R23 — `M1.8.S05`'s `turn:`, en AR-6 trede 4 krijgt twee bewegingen

**Het `turn:`-veld belooft het onmogelijke en alleen ik mag het aanraken.** Er staat *"Threx uses Voss's first name, and Ember learns in one word how long he has been watching"*, terwijl L1-R10 punt 4 de voornaam permanent onspeelbaar maakt — hij is speler-gekozen en kan dus nooit ingesproken worden. De schrijver volgde de nieuwere en specifiekere ruling, schreef *"And Voss."*, en escaleerde in plaats van het stil op te lossen. **Dat is exact de juiste volgorde, en het bestand verscheepte ondertussen een driezinnencontract dat onwaar is over zijn eigen inhoud** — de duurste soort onwaarheid, want §18.7 zegt *"a scene without all three is not written"* en een criticus scoort tegen dat contract.

**Nieuw veld, doorgevoerd:**

> **turn:** *"Threx addresses them directly for the first time, and what he knows about Voss turns out to be older than Ember."*

Waar: `.070` is de aanspraak, `.080` (*"Circuits. There is a supervisor's note in that file that anybody would be pleased with."*) is een leerlingdossier van vóór er een cel bestond. De omkering is intact — ze kwamen inbreken en horen dat ze bekeken werden voordat er iets te bekijken was.

**En nu AR-6, want de criticus heeft gelijk en het gaat dieper dan één scène.**

Nagelopen, per trede, op het medium én op de aangesprokene:

| Trede | Waar | Medium | Tegen wie spreekt hij |
|---|---|---|---|
| 1 | M1.3.S02 | propaganda uit een mast | niemand |
| 2 | M1.3.S06 | afgeluisterd Veil-kanaal | iemand anders |
| 3 | M1.7.S04 | afgeluisterd live verkeer | iemand anders, **over een van ons** |
| **4a** | **M1.8.S05** | plafondluidspreker | **ons, in het openbaar** |
| **4b** | **M1.8.S06** | door glas, in persoon | **hem, alleen** |

**De as van AR-6 is niet de naamvorm maar de afstand, en dat moest wel.** De naamvorm loopt vast na trede 3: die trede is een Ember-vóórnaam, en de enige mogelijke escalatie daarop is Voss' voornaam, die het spel niet kan uitspreken. **AR-6's trede 4 was dus, zoals geschreven, onbouwbaar** — en dat is mijn fout, niet die van de schrijver.

**Ruling: trede 4 heeft twee bewegingen, en de top is `S06`.** De criticus heeft gelijk dat `M1.8.S06.040` — *"You have your aunt's hands."* — de trede draagt: dat is de **relatie** in plaats van een naam, in persoon, tegen hem alleen, en het is intiemer dan welke naamsvorm ook. Een voornaam bewijst dat hij een dossier gelezen heeft; *"your aunt's hands"* bewijst dat hij het gelezen heeft en het aardig vond.

Ik neem de tegenwerping over dat `S05` "qua medium gelijk is aan trede 3" **niet** over, en heb ernaar gekeken in plaats van het aan te nemen. Trede 3 is verkeer dat Voss *afluistert*; `S05` is de eerste keer in de hele act dat Threx **hen aanspreekt**. Dat is een echte trede-overgang en het is de reden dat de gang naar boven kijkt. `S05` is 4a en verdient het.

**Wat dit voor de schrijver verandert: niets.** Geen regel verandert, geen variant, geen ID. De reden dat ik het toch vastleg, is dat de volgende schrijver die AR-6 leest anders opnieuw een voornaam probeert te leveren en opnieuw op L1-R10 stukloopt.

**Doorgevoerd in:** `ACT1_OVERVIEW` §8 AR-6, `BEATS_M1.8` §6, en het L1-blok van `M1.8.S05`.

**Wat er nu moet gebeuren:** `dialogue-critic` herscoort `M1.8.S05`; ik heb alleen de reden bijgewerkt naar de waarheid. De architect-actie is uitgevoerd; er is geen schrijversactie.

> **Bijgewerkt 02-08: dit is gebeurd, en de oorspronkelijke zin hier is verlopen.** Er stond *"het `critic:`-veld staat nog op NO-GO en dat blijft zo tot hij dat doet"* — dat is sindsdien onwaar; de scène is herscoord. **Een ruling die een poortstatus als lopende toestand opschrijft, veroudert per definitie**, en dat is de fout die `check_ruling_premises.py` bestaat om te vangen. Dezelfde les als L1-R22: laat een ruling steunen op het feit dat hij bedoelt (hier: de architect-actie is af), niet op een toestand die ernaast ligt en verder loopt.  <!-- premisse-nagelopen: deze alinea MELDT de verlopen bewering en herhaalt hem niet als claim -->

---

### L1-R24 — De drie cellen zijn Ember, de veertig en de bevrijden. De Iron Chorus is er geen

De criticus vraagt terecht wat de `none`-tak telt, en de vraag is groter dan hij dacht. **Als de derde cel de Chorus is, klopt `.010` in géén enkele tak** — met pact zijn het er twee (Ember + Chorus), zonder pact één. Ik heb dus geteld welke groepen er in de kamer staan in plaats van te redeneren vanuit de vlag.

**Canon (§2.9): *"the loyalty of three fused cells."*** Dat is de muur en die verplaats ik niet.

**Mijn eigen beat-sheet gaf het antwoord al en niemand las het als een telling.** `BEATS_M1.8.md` §4 beat 1: *"De vault, vol. **Ember, de Iron Chorus (bij pact), de bevrijden, de veertig.**"* Dat zijn vier groepen, en de enige met een voorwaarde eraan is de Chorus. De drie onvoorwaardelijke zijn de drie cellen:

| # | Wie | Sinds | Voorwaardelijk |
|---|---|---|---|
| 1 | **Ember** — de elf min hun doden, plus Voss en Brick | P0 | nee |
| 2 | **De veertig** — Sela's instroom van de Tithe-trein | M1.6 | nee |
| 3 | **De bevrijden** — de gevangenen uit K-77, inclusief Petra | M1.8 | nee |

**En de Iron Chorus versmelt niet, en het spel zegt dat zelf, in zijn eigen regel.** `M1.8.S99.040` — *"Three cells and one door out. The Chorus sleeps somewhere else."* De emissaris telt de drie en zet zichzelf er buiten. `ACT1_OVERVIEW` §5 zet fusie met de Chorus expliciet in **acts 2–4** (*"fusie, schisma of tragedie"*), en L1-R13 hangt de hele act-2-vertakking op aan het onderscheid tussen full en limited. **Een Chorus die in act 1 al versmelt, sloopt L1-R13.**

**Ruling: `.010` is juist, onvoorwaardelijk, in alle drie de takken. De NO-GO staat op de verkeerde regel.**

**Wat wél stuk is, en het is kleiner en echt:** de `shot:` op `.010` zet *"a knot of Iron Chorus by the ramp"* onvoorwaardelijk in de kamer. In de `none`-tak zijn ze er niet — Voss zei *"We're not in. They can have the water and the street."* en dan staan ze niet in Hollow Point.

**Reparatie, en hij kost nul credits:** de Chorus-clausule verhuist van de `shot:` op `.010` naar de `shot:` op `.040`, die al `condition: 'story.m15_pact != "none"'` draagt. `shot:` wordt nooit ingesproken, dus dit is gratis en de cinematic pass leest het precies waar het waar is. Geen tweede spreekregel, geen `condition` op `.010`, geen variant.

**Waarom dit niet met `silence:` opgelost wordt:** `silence:` verklaart dat een *tak* niets speelt. Hier speelt de tak wel; alleen de camera stond verkeerd. Een stilte gebruiken voor een cameraprobleem is precies de verjaarde-stilte-fout die `SCRIPT_FORMAT` §4 beschrijft.

**Wat er nu moet gebeuren:** `dialogue-writer` M1.8 verhuist één clausule tussen twee `shot:`-velden. `dialogue-critic` herscoort. Ik heb het L1-blok van `M1.8.S99` en `BEATS_M1.8.md` §4 beat 1 uitgeschreven zodat de telling er letterlijk staat en niemand hem opnieuw hoeft af te leiden.

**Continuïteit die ik hier meteen sluit:** `M1.5.S99.030`'s idealist-variant telt Ember + Chorus als **twee**, en dat blijft juist — op dat moment bestaan de veertig en de bevrijden nog niet. Er is geen botsing. **Losstaand, gemeld en niet beslist:** `M1.5.S99.050`/`.060` (Dex, *"Two cells, one street, one net"*) staan onvoorwaardelijk en lezen in de `none`-tak als een pact-uitspraak. Verdedigbaar — twee cellen delen straat en band of je nu tekent of niet — maar het is de zwakste regel van de drie takken. Voor de act-1-continuïteitsdoorloop, niet blokkerend.

---

> ### ⚠ L1-R24 — NAGEMETEN OP 01-08, EN DE PREMISSE HOUDT NIET
>
> De ruling wijst `silence:` af met *"de branch plays, de camera was wrong"*. **De
> shot-clausule is verplaatst en de bevinding staat er nog**, woordelijk hetzelfde:
> `M1.8.S99` vertakt op `story.m15_pact` en handelt `full` en `limited` af van drie.
> Gemeten met `validate_script.py --no-voice` ná de verhuizing.
>
> Dat is te verwachten zodra je naleest wat de check doet: hij vergelijkt `positive`
> (`{full, limited}`) met `universe` en de uitsluiting op `.040` (`!= "none"`) dekt `none`
> niet af. Hij kijkt naar **regels**, niet naar `shot:`. De camera was inderdaad fout en het
> verplaatsen was juist — maar het was een ánder defect.
>
> **De schrijver heeft dit gezien en het veld NIET toegevoegd**, omdat hij niet stil over een
> benoemde ruling heen wilde gaan. Dat is het goede gedrag en het is de reden dat dit hier
> staat in plaats van dat het stilletjes opgelost is.
>
> **Waarom `silence:` waarschijnlijk tóch het juiste instrument is** — en dit is een
> voorstel, geen wijziging: de ruling zegt zelf in de `emissary:`-noot dat de emissaris niet
> in de kamer staat en dat **de afwezigheid ís de beat**. Dat is precies de vorm van een
> verklaarde stilte: de `none`-speler krijgt 20 van de 24 regels, en de pact-tak zwijgt omdat
> hij geen pact sloot. Een plakklare blok mét reden staat in het bestand.
>
> **Tot L1 dit herziet blijft de bar hier rood, en dat is de bedoeling.** Eén eerlijke rode
> regel is goedkoper dan een agent die een ruling overschrijft omdat een tool zeurt.

### L1-R25 — `M1.6.S05` krijgt een `split`-blok, geen verklaarde stilte

De scène vertakt op `story.m16_train_choice` en handelt `run` (`.200`–`.250`) en `emptied` (`.300`–`.340`) af. `split` krijgt niets. **De speler die de donkerste optie koos, krijgt in de scène die hem zijn prijs moet tonen ofwel de beats van een keuze die hij niet maakte, ofwel stilte.** Dat is de vorm van L1-R12, op de tak die het minst aantal spelers heeft en het meest te verliezen.

**`silence:` is hier het verkeerde gereedschap, en het is de moeite waard om te zeggen waarom.** Het veld bestaat voor *"hier hoort niets"*, en dat is hier niet waar. `S05` gaat over één ding: mensen tellen terwijl je ze over een rangeerterrein beweegt. In de `split`-tak **is het getal veranderd** — de helft is weg met mensen die niemand kan benoemen. Zwijgen in de enige scène over tellen, in de tak waarin het aantal is veranderd, is geen keuze maar een gat met een verklaring eromheen. **Een stilte die je niet kunt uitleggen is een verzuim in een keuzepak; een stilte die je wél kunt uitleggen maar die iets verbergt, is erger.**

**Ruling: een regelblok, `.350`–`.390`.** ID's lopen monotoon door na `.340` en vóór de gedeelde staart op `.400`, dus er hoeft niets hernummerd te worden. Drie tot vijf regels, en hij betaalt wat er al ligt:

1. **Sela telt, en het getal klopt.** Dat is de wreedheid: in `run` en `emptied` mist ze mensen, hier niet. Ze telt haar helft en ze komt uit.
2. **De vrouw in de gele werkjas.** De schrijver heeft haar in beide bestaande takken hetzelfde gezicht gegeven — neergeschoten op de spoordijk onder `run`, op haar knieën bij de weegbrug onder `emptied`. Haar derde afloop is de enige die dit blok nodig heeft en hij is gratis: **zij zat in de tweede wagon en niemand weet het.** Geen enkele regel mag daar een gevoel bij benoemen.
3. **Dit betaalt `M1.6.S04.190`** — Sela's *"Somebody should be able to say afterwards who got into that car."* Die zin is nu een belofte zonder afloop.
4. **Geen vierde wegverklaring.** De gedeelde staart `.400`/`.410` draagt T4 al voor alle takken, en L1-R21 telt. Niets in dit blok raakt de Dominion-reactie aan.
5. **Geen Iron Chorus-stem.** Q-4 laat de emissaris ongecast; de Chorus neemt de wagon in een `shot:`, zoals in `M1.6.S04.180`.

**Wat er nu moet gebeuren:** `dialogue-writer` M1.6 schrijft `.350`–`.390`. Dit is dezelfde beurt als L1-R21's `.410`, dus het is één keer het bestand open.

---

### L1-R26 — `run.m17_record` blijft, wordt gedeclareerd, en de doelstelling is een **datataak**

De criticus heeft de categorie goed (L1-R4) en heeft hem bewust níét gedeclareerd, met een reden die ik onderschrijf: **een declaratie die de validator laat zwijgen terwijl de regel nog steeds nooit speelt, is erger dan een rode balk.** Dat is de juiste instelling en het is precies waarom hij escaleerde in plaats van te patchen.

**Maar de premisse — "de doelstelling bestaat als systeemtaak nog niet" — houdt geen stand. Gemeten, niet aangenomen:**

| Wat nodig is | Bestaat het? | Bron |
|---|---|---|
| De objective-primitief (`CollectItem` / `InteractTarget`) | **ja** | `BEATS_M1.7.md` §3: *"Buiten SPEC-P2-04. Geen nieuwe primitieven nodig."* |
| De optionele doelstelling zélf, in de beat-sheet | **ja, al gespecificeerd** | idem: *"optionals: het requisitiedocument (P4-c) en het geredigeerde personeelsrecord (P2-d)"* |
| Runtime-opslag van "welke optionals zijn gehaald" | **ja, generiek en al aanwezig** | `FEclipseMissionOutcome::CompletedObjectiveIds` (`EclipseMissionTypes.h:165`) |

**Er is dus geen nieuwe state en geen nieuwe code nodig.** `run.m17_record` leest `CompletedObjectiveIds` op dezelfde manier waarop `run.zero_casualty` de downed-latch leest en `run.alarm_raised` `bAlarmRaised` — L1-R4's formulering *"er is geen nieuwe state voor nodig, alleen toegang"* geldt hier woordelijk. Wat ontbreekt is een **rij in M1.7's missiedefinitie** en een **mapping in `script_to_seed.py`**. Dat is een datataak van een uur, geen systeemfeature.

**Ruling: de regel blijft, de vlag wordt gedeclareerd in `SCRIPT_FORMAT` §4.**

`M1.7.S03.250` is P2-d, één regel, optioneel, en hij is de goedkoopste T2-textuur in de hele act: een personeelsdossier waarvan de naam én de functie zijn weggelakt. Whisper als **afwezigheid**, gelezen als bureaucratische ruis. 96 credits. AR-9 is voldaan zonder hem (P2-b is de verplichte drager in dezelfde scène), dus hij mag vallen — maar hij hoeft niet, en dat scheelt.

**En de generalisatie is meer waard dan deze ene regel.** `SCRIPT_FORMAT` §4 declareert er drie en dat leest als een gesloten lijst. Dat is het niet: **elke `run.`-vlag die "is deze optionele doelstelling gehaald" leest, is dezelfde vlag met een andere ID, en elke missie in het spel heeft optionals.** Een debriefregel over een optional kost daarmee nul nieuwe state, altijd. Die alinea gaat mee de tabel in, want zonder hem verzint schrijver zeventien een `story.`-vlag voor iets wat de run niet hoort te overleven.

**Wat er nu moet gebeuren:** ik declareer `run.m17_record` in `SCRIPT_FORMAT` §4 (`load_run_facts()` leest die sectie, dus dat is doorgevoerd zodra het er staat). `voice-director`: de 96 credits blijven in de begroting staan. **De datataak — één optionele objective-rij in M1.7 + de mapping — moet zichtbaar blijven tot hij gedaan is;** hij staat al in `VOICE_LEDGER.md` en dat is de goede plek. Als hij bij act-2-planning nog open staat, valt de regel alsnog, en dat is dan een goedkope beslissing in plaats van een verrassing.

---

### L1-R27 — Hub-takken zijn `run.` De hub zet nul `story.`-vlaggen

De vraag is gesteld als een schemaprobleem en het is er geen: `HUB.A1.reyes_triage` **verscheept al** een keuzeblok met vier opties op `run.hub_triage_order`, gezet en gelezen in hetzelfde bestand, en het valideert schoon. FLAGREG vuurt alleen op `story.`; choice-integriteit is voldaan omdat er lezers zijn. De schrijver leidde de vorm af uit `run.m18_threx_probe` (L1-R4) en dat is precies de juiste redenering.

**Ruling: hub-takken zijn `run.` De hub voegt geen rij toe aan `ACT1_OVERVIEW` §6, nu niet en later niet.**

**Drie redenen, en de derde is degene die het definitief maakt.**

1. **Mijn eigen beat-sheet zei het al.** `BEATS_HUB_A1.md` §5 heeft voor elf van de twaalf scènes een lege *Zet*-kolom, en §6 zegt met zoveel woorden: *"Geen wendingen planten… Een speler die de hub overslaat, mist gezelschap — nooit begrip."* Een hub-keuze die de campagne verandert, spreekt dat tegen.
2. **De enige uitzondering is geen keuze.** `Story.Thread.MaraLetters_Open` is een *aanwezigheids*draad die `M1.8.S91` verrijkt, niet vertakt — en die scène werkt volledig zonder (L1-R11). Dat is de vorm die een uitzondering moet hebben.
3. **De hub-tier is de eerste die uit het budget valt, en dat is één keer gebeurd (L1-R11).** Alles wat uit de hub *persisteert*, is daarmee een afhankelijkheid op een scène die misschien nooit klinkt. **Dat is precies de faalvorm waar AR-9 voor bestaat**, en de enige reden dat L1-R11 geen dragende plant heeft gesloopt, is dat AR-9 er toevallig al tegen beschermde. Twee keer geluk hebben is geen ontwerp.

**Wat een `run.`-hubkeuze wél is, en het is niet minder:** de speler kiest, het personage antwoordt op *zijn* keuze, en het gesprek is vier verschillende gesprekken. Wat niet gebeurt, is dat het de kamer uit loopt. Dat is wat een hubgesprek is.

**En het is goedkoper op de as die telt (L1-R15).** Vier opties in plaats van een vierassige variantenset op drie regels: vier korte Voss-generaties in plaats van basis-plus-vier-maal-drie, en de vier antwoorden zijn vier *verschillende* antwoorden — wat een variantenset per definitie niet kan, want die deelt één reply. `HUB.A1.reyes_triage` en `M1.8.S06` doen dit allebei en ze hebben er allebei gelijk in. **Aangenomen als de standaardvorm voor een keuze in een hubscène.**

**Wat er nu moet gebeuren:** niets aan `reyes_triage`; hij is goed zoals hij is. `dialogue-writer` hub: wie denkt een `story.`-rij nodig te hebben, heeft óf een beat die in een missie thuishoort, óf een owner-vraag. Escaleren, niet toevoegen. Ik zet de regel in `BEATS_HUB_A1.md` §2.

---

### L1-R28 — De scènepoort wordt een **scalair** kopveld `condition:` — geen parserwijziging nodig

`BEATS_HUB_A1.md` §2 en §7.3 eisen een `condition` op elke hubscène, en `SCRIPT_FORMAT` §4 heeft er geen kopveld voor. Twaalf schrijvers hebben dat op dezelfde manier opgelost — een **commentaarregel** `# condition: story.beat_m11_thirteenbullets == true` in het L1-blok. Dat valideert schoon en betekent niets: geen tool leest het, en de poort die *"ongepoorte hubdialoog is de snelste manier om een game dom te laten klinken"* moest voorkomen, bestaat nergens in machinevorm.

**De escalatie neemt aan dat dit een tweede `HEADER_MAPS`-naam nodig heeft. Nagekeken in de parser, en dat is niet zo.** `HEADER_MAPS` bestaat voor `silence:`, dat een **mapping** is (waarde → reden). Een scènepoort is één expressie. In `parse_scene_file` gaat een `key: value` op indentniveau nul al door de bestaande scalaire tak — dus:

**Ruling: `condition:` als scalair kopveld, met exact de grammatica van het regelveld.**

```yaml
type:         hub
credit_tier:  4
condition:    'story.beat_m14_quartermaster == true'   # de scène wordt pas aangeboden
```

**Waarom dezelfde naam, en niet `gate:` of `available:`.** Eén grammatica, niet twee — hetzelfde argument als L1-R2 voor `choice.set`. Het scopeverschil (wordt de scène *aangeboden* vs. speelt de regel *af*) is een niveauverschil, geen betekenisverschil, en de parser scheidt ze op inspringing. En twaalf schrijvers hebben onafhankelijk van elkaar dat woord opgeschreven; dat zijn twaalf stemmen voor nul heropvoedingskosten.

**Wat het moet doen om niet decoratief te zijn:** het kopveld telt als **lezer** in de feitentabel. Dan dekken "condition resolves" en de FLAGREG-kolom *gelezen door* ook de poorten, en een hubscène die op een dode vlag hangt, valt om. Gecontroleerd: alle twaalf poorten uit `BEATS_HUB_A1.md` §5 hebben een rij in `ACT1_OVERVIEW` §6 of staan in `EclipseGameplayTags.cpp` — op één na, en dat is L1-R29.

**Volgorde, en dit is de enige riskante kant.** `SCENE_OPTIONAL` in `validate_script.py` kent het veld nog niet, dus een scène die het vandaag gebruikt zakt op SCHEMA met *"unknown field"*. Daarom:

> **Schrijvers gebruiken `condition:` in de kop NOG NIET.** Het commentaar blijft staan tot de tool het veld accepteert. De wijziging is twee regels — `condition` toevoegen aan `SCENE_OPTIONAL`, en het door `COND_GRAMMAR` plus de feitentabel halen — en zij gaat eerst. **Ik heb de validator in deze sessie niet kunnen draaien; dat is de reden dat ik de veldnaam vastleg en de adoptie tegenhoud in plaats van beide tegelijk te doen.**

Vastgelegd in `SCRIPT_FORMAT` §4 als gespecificeerd-en-nog-niet-actief.

---

### L1-R29 — De hub-schrijver had gelijk: `M11_ConscriptSpared` is dood, de drie bladeren leven

`BEATS_HUB_A1.md` §4 scène 2 en §5 dragen `Story.Choice.M11_ConscriptSpared` — **door mijn eigen L1-R3 ingetrokken.** De schrijver van `HUB.A1.reyes_first_conversation` heeft dat gezien, het niet stil gerepareerd, en in plaats daarvan de drie bladeren gelezen die `M1.1.S05` werkelijk zet.

**Nageteld:** `M1.1.S05.170/.180/.190` zetten `story.m11_conscript_choice` = `finished` | `left` | `bound`. De hubscène leest alle drie, tweemaal (Voss' antwoord `.060`–`.080`, Reyes' aantekening `.090`–`.110`). **Drie van drie afgehandeld — niet de L1-R12-vorm.**

**Bevestigd. De schrijver had gelijk en het beat-sheet had ongelijk.**

Twee dingen die hij niet kon zien en die ik erbij zet:

1. **De dode naam stond op twee scènes.** §5 rij 1 zet hem op scène 1 *en* 2. Geteld: `HUB.A1.dex_first_conversation` leest de vlag **nul keer** — terecht, Dex heeft daar niets mee. De rij was gewoon fout. `BEATS_HUB_A1.md` §5 gecorrigeerd: alleen scène 2, alleen de bladvorm.
2. **Eén woord in zijn eigen L1-blok blijft verkeerd:** r. 25 noemt de takken *"killed / left / bound"*. Het blad heet `finished`. De `condition`-regels zijn goed; alleen het commentaar wijst naar een waarde die niet bestaat, en dat is hoe schrijver achttien hem overschrijft.

**En dit is de derde keer dat een lezer in een ander bestand staat dan de zetter** (L1-R12, L1-R29, en de vlagspellingveegbeurt van 01-08). De les uit L1-R12 was *"een vlagvormwijziging is pas doorgevoerd als de lezers geteld en gepatcht zijn"*. Ik heb bij L1-R3 de scènes gepatcht en **de beat-sheets niet**, en een beat-sheet is ook een lezer — hij wordt gelezen door een mens die daarna een `condition` typt. **Uitbreiding van de L1-R12-regel: het beat-sheet telt als lezer.**

**Wat er nu moet gebeuren:** `dialogue-writer` hub corrigeert één woord in zijn eigen L2-notitie (`killed` → `finished`). Verder niets — de scène is goed.

---

## Wat ronde 5 niet heeft opgelost, expliciet

- **Ik heb `validate_script.py` niet kunnen draaien** in die sessie (geen shell). Elke wijziging die ik in een scènebestand heb gemaakt is daarom beperkt tot commentaarblokken en één scalaire `turn:`-string, en ik heb de parser gelezen om te bevestigen dat beide categorieën door de bestaande tak lopen. **Wie hierna het eerst een shell heeft, draait `python Eclipse/Tools/validate_script.py --no-voice` en meldt het resultaat** — dat is een controle die ik verschuldigd ben, niet een formaliteit.
- **L1-R28 is gespecificeerd, niet actief.** De toolwijziging gaat vóór de adoptie.
- **`M1.5.S99.050`/`.060` in de `none`-tak** — gemeld in L1-R24, niet beslist, op de act-1-continuïteitsdoorloop.
- **`M1.4.S99`'s registerdrempel** (langste 19 tegen 20) — gemeld in L1-R22, niet beslist; scènevraag.

---

## Ronde 6 — 2026-08-01, een naamleugen in de stemsleutels

### L1-R30 — De Iron Chorus krijgt eigen stemsleutels. Een rolpool noemt een **factie**, geen beroep

**Gemeten, niet aangenomen, en de meting is de hele ruling.** `eclipse_fighter_c` en `eclipse_fighter_d` dragen in het hele corpus **acht regels, en alle acht staan in M1.5** — in de mond van mensen van de rivaal:

| Sleutel | Regels | Waar |
|---|---|---|
| `eclipse_fighter_c` | 4 | `M1.5.S02.220` · `S04.170` · `S05.050` · `S05.100` |
| `eclipse_fighter_d` | 4 | `M1.5.S03.110` · `S04.180` · `S05.070` · `S05.160` |

Geen enkele Ember-vechter gebruikt ze. **De bestanden zeggen het zelf**, en dat is wat dit van een smaakkwestie tot een fout maakt:

> `M1.5.S03` r. 40 — *"the Iron Chorus fighter carries `eclipse_fighter_d`"*
> `M1.5.S02` bij `.220` — *"**His own** fighter corrects the count upward"*, waarbij die *"his"* de emissaris is
> `M1.5.S05` L2-blok — *"IRON CHORUS numbers, never names. EMBER names. That is the audible difference between two cells fighting in one street."*

**Het onderscheid stond in de wóórden en werd door de sleutels ongedaan gemaakt.** S05 is de enige scène in act 1 die twee cellen in één straat laat vechten; hij is er letterlijk voor gebouwd. Met vier stemmen uit Embers pool zou `.050` (*"Two, screen. Three, door."*) tegenover `.060` (*"Brick, left side!"*) — vier woorden uit elkaar, de wending van de scène — in **dezelfde stem** hebben geklonken. §18.5 regel 5 hangt factie-identiteit expliciet aan de stem: *"Same trigger, three vocabularies."* Er zijn er hier vier, en de vierde had er geen.

**Dit is geen cosmetica en het is geen hernoeming om de netheid.** Het is het enige onderscheid dat de speler in S05 kan hóren.

**De beslissing zelf was niet van mij, en dat is de juiste volgorde geweest.** De M1.5-schrijver meldde het als escalatie 2 en noemde het een `voice-director`-beslissing; `BEATS_M1.5` §10 noemde precies deze afweging (*"de goedkope oplossing is hergebruik van de Eclipse-fighterstem; de betere is een eigen registerkeuze"*). De `voice-director` heeft de **betere** gekozen en het staat op owner-kaart **O-16**, met de reden erbij: slot C is *"een vrouw... en zij hoort bij de Iron Chorus, niet bij jou"*, slot D is *"hoorbaar iemand anders dan C en dan de emissaris"*. De voorstelbestanden heten al `02_Iron-Chorus-vechter_C_VOORSTEL_...`. **De casting was dus al eerlijk; alleen de sleutels logen nog.** Data hoort achter een genomen besluit aan te lopen, anders spreekt de kaart het script tegen.

**Ruling, in vier delen.**

**1. Twee nieuwe sleutels: `iron_chorus_fighter_a` en `_b`.** Doorgevoerd in `SCRIPT_FORMAT` §4 en in `VoiceKeyMap.json`. De acht regels zijn omgezet (C→A, D→B; de letter blijft gelijklopen met de sprekertoken, zoals hij nu ook doet).

**2. De sprekertokens gaan mee: `CHORUS_A` / `CHORUS_B`.** Dit was de vraag die aan mij gesteld werd, en het antwoord is ja, om twee redenen.

De eerste is dat de alternatie van L1-R6 **al een factievocabulaire was**, overal behalve op één plek. Nageteld: `VEIL` is een factie, `CONSCRIPT` is een rang binnen een factie, `CIVILIAN` en `PRISONER` zijn van niemand. **`FIGHTER` was het enige token dat één factie betekende zonder het te zeggen** — het betekende Ember — en de eerste missie met de geweerdragers van een tweede cel erin had daardoor geen plek om ze te zetten. Ze namen Embers token, en met het token namen ze Embers stempool. Dat is niet een schrijver die slordig was; dat is een vocabulaire met een gat erin.

De tweede reden is dwingender en hij gaat over wat er gebeurt als je het hálf doet. Alleen `voice:` omzetten laat dit staan:

```yaml
speaker: FIGHTER_C
voice:   iron_chorus_fighter_a
```

Acht keer, op aangrenzende regels, en het leest als een bug. Iemand repareert dat — en hij repareert het de verkeerde kant op, terug naar `eclipse_fighter_c`, omdat `FIGHTER_C` er gezaghebbender uitziet dan een sleutel die hij niet kent. **Een halve rename is geen halve verbetering maar een uitnodiging tot terugdraaien.**

*"De Chorus"* is geen verzonnen naam: `M1.8.S99.040` zegt het hardop — *"Three cells and one door out. The Chorus sleeps somewhere else."*

**Toolwijziging vóór adoptie, zoals L1-R28 het voorschrijft:** `CHORUS` is toegevoegd aan `ROLE_SPEAKER` in `validate_script.py` (die alternatie staat **hardcoded** in de tool, niet in het document — dat heb ik nagekeken, niet aangenomen) en aan de tekst in `SCRIPT_FORMAT` §4. Zonder die twee regels zakken de acht regels op SPEAKER.

**3. Géén aliaspaar, en de reden is machinaal.** De vraag was of `eclipse_fighter_c` als alias moest blijven staan zoals `dex` / `dex_callum`. Nee — en `alias_of` is daarbij een rode haring: **dat veld wordt door geen enkele tool gelezen.** Wat een alias een alias maakt, is dat twee sleutels dezelfde **rol** dragen. Voor `dex` / `dex_callum` is dat veilig: het is één man. Hier zouden twee sleutels dezelfde **rol + slot** dragen, en de botsingscontrole in `check_voice_resolves.py` groepeert op `role:slot` — twee facties op één stem zouden er dus **niet** uitkomen. Een alias die precies het defect verbergt waarvoor de controle herbouwd is, is geen vriendelijkheid. **De twee oude sleutels zijn ingetrokken, met de reden in het bestand.**

**4. De rol blijft `eclipse_fighter`, slot C en D — en dat is bewust half.** De sleutel is van mij; de rol staat in `CASTING_RESOLVED.json` en dat is de owner-knop. Belangrijker: **`resolve_casting_choice.py` leidt `slots_needed` af uit `VoiceKeyMap.json`**, en O-16's voorstel biedt Beth en Arric aan ónder `eclipse_fighter:C` en `:D`. Had ik hier een verse rol `iron_chorus_fighter` neergezet, dan wees de knop die de owner straks indrukt nergens meer heen en bleven de acht regels na O-16 nog steeds stil. **Een opruimactie die de openstaande owner-actie breekt, is duurder dan de rommel.** Het rolsplitsing-deel hoort bij O-16 en is daarna alsnog gratis (de cachesleutel hangt aan de stem-ID, L1-R5).

**Kosten: nul, en dat is gemeten en niet aangenomen.** Er is geen seconde audio; O-16 blokkeert alle generatie. Ná generatie zou het óók gratis zijn — `EclipseGenerateVoicesCommandlet.cpp:325` hasht de opgeloste ElevenLabs-ID, niet de scriptsleutel — maar dat argument heb ik niet nodig.

**Wat dit openlegt en wat ik niet zelf beslis: Embers barkpool is nu twee registers, geen vier.** Dat is geen gevolg van deze ruling maar van de casting die eronder ligt — O-16 geeft C en D aan de Chorus. §18.5 vraagt 6–12 varianten per trigger en regel 4 verbiedt redundantie; twee stemmen voor Embers hele gelederen is dun. **Of Ember een derde en vierde stem verdient, is een castingvraag (meer slots, meer owner-keuzes), geen schrijfvraag.** Gemeld, niet opgelost, en in `SCRIPT_FORMAT` §5 gezet waar een barkschrijver hem tegenkomt — met de uitdrukkelijke regel dat je hem **niet** oplost door de pool van een andere factie te lenen. Dat is namelijk precies hoe deze fout ontstond.

**Escalatie 1 in `M1.5.S02` heb ik in dezelfde beurt afgesloten.** Hij was op 31-07 beslist door L1-R6b en stond nog open in het bestand — inclusief een *"zero-cost fallback"* die de emissaris naar `FIGHTER_C` zou verhuizen. Die uitweg is nu verboden in plaats van ongebruikt: hij zou de rivaal opnieuw onder Embers token schuiven. **Een afgehandelde escalatie die open blijft staan, is een instructie aan de volgende schrijver.**

---

## Wat ronde 6 niet heeft opgelost, expliciet

- **Ik heb de twee controles opnieuw niet kunnen draaien** (geen shell, tweede keer op rij). Ik heb in plaats daarvan `validate_script.py`, `check_voice_resolves.py` en `resolve_casting_choice.py` gelézen en de uitkomst daaruit afgeleid; dat is een afleiding en geen meting, en zo staat het ook in mijn rapport. **De eerstvolgende met een shell draait beide en meldt het.** Wat ik verwacht: `validate_script.py --no-voice` blijft op **5 bevindingen** (4 REGISTER, 1 BRANCH — geen enkele check die ik heb aangeraakt telt mee), en `check_voice_resolves.py` blijft **rood** met de nieuwe sleutelnamen in de melding (`iron_chorus_fighter_a/_b → eclipse_fighter slot C/D, unbound`) plus de drie botsingen. **Wijkt dat af, dan is deze ruling het probleem en niet de tool.**
- **De rolsplitsing hoort bij O-16 en is niet gedaan.** `eclipse_fighter:C/D` heten nog Ember terwijl ze de Chorus zijn. Bewust: het is een owner-bestand en de knop staat op scherp. Daarna gratis.
- **Embers barkpool is twee registers.** Gemeld in deel 4 hierboven, castingvraag, niet beslist.
- **`M1.5` staat in dit log twee keer als GO** (L1-R14, L1-R21: *"M1.5 is GO en wordt niet heropend"*), maar gemeten dragen alle zes de scènebestanden `status: draft` / `critic: null`. Twee eerdere rulings hebben dus op een toestand geleund die de bestanden niet dragen. **Niet door mij op te lossen** — een verdict zetten is het veld van `dialogue-critic`. Gemeld, en het staat op de act-1-continuïteitsdoorloop.  <!-- premisse-nagelopen: deze regel MELDT de botsing, hij beweert hem niet -->
- **Q-4 (een eigen naam voor de emissaris) blijft open** en blijft optioneel; L1-R6b maakte hem al minder dringend en L1-R30 verandert daar niets aan.
- **Nog open uit ronde 5:** L1-R28 (gespecificeerd, niet actief), `M1.5.S99.050`/`.060` in de `none`-tak, en `M1.4.S99`'s registerdrempel.

> **En de rest is nagelopen, zodat niemand dat opnieuw doet.** Elke claim in dit document van de vorm *"M1.x is GO / draft / NO-GO"* is machinaal tegen de scènebestanden gehouden (01-08). **M1.5 is de enige die niet klopt** — drie voorkomens, alle drie tegenover zes bestanden op `critic: null`. De klas is dus begrensd en dit is de enige instantie, geen steekproef.

---

## Ronde 7 — 2026-08-01, twee dragende getallen, een kenteken, en een beat die twee keer bestaat

**Zes van deze tien zijn dezelfde fout in verschillende kleren:** een dragend feit dat op twee plekken een andere waarde heeft, waarbij geen van beide schrijvers het totaal kan zien. Elf, eenenveertig, het Enforcer-kenteken, het personeelsrecord, `CustodianKey` en `M12_Ghost`. Dat is geen toeval en het is ook geen slordigheid van schrijvers — **het is de faalvorm van parallel schrijven, en het is precies waarvoor L1 boven L2 zit.**

| # | Onderwerp | Uitkomst | Raakt jou als schrijver? |
|---|---|---|---|
| **L1-R31** | L1-R14's M1.5-uitzondering stond op een grond die onwaar is | **De uitzondering vervalt.** `M1.5.S03` wijkt naar **achttien**; M1.6's zeventien blijft | M1.5 |
| **L1-R32** | `forty-one` telt de eerste wagon én de instroom | **Eén getal, één betekenis: de instroom.** `S06.010` vertakt **niet** | M1.6, M1.7, M1.8, hub |
| **L1-R33** | `Nobody`/`Nothing` + presens is 1,6× de act-norm in M1.6 | **Geen plafond. Dit is thema**, en hier staat waarom | allen |
| **L1-R34** | O-4: de stub eist twee vragen van Sela, er is er één | **De stub wijkt.** De tekst had gelijk | M1.6 |
| **L1-R35** | O-7: Dex' grappen in `M1.6.S99` | **Ze blijven, onvoorwaardelijk.** En hier staat wat "nasleep" betekent | M1.6, en elke debrief hierna |
| **L1-R36** | Het Enforcer-kenteken is 261 én 917 | **Het is `two six one`.** `M1.3.S05.150` wijkt. Cijferconventie vastgelegd | M1.2, M1.3 |
| **L1-R37** | Het geredigeerde personeelsrecord bestaat twee keer | **P2-d woont in `M1.7.S03.250`.** `M1.2.S04.220`–`.250` vervalt | M1.2, M1.7 |
| **L1-R38** | `Story.Clue.CustodianKey` heeft twee gedeclareerde zetters | **`Story.Clue.*` wordt gezet waar hij gesproken wordt**, nooit in de debrief | alle clue-vlaggen |
| **L1-R39** | Het register noemt de verkeerde zetter voor `M12_Ghost` | **Het bestand had gelijk, het register niet** | M1.2 |
| **L1-R40** | Zes maximes uit één mond in één missie, en de tabel telt er één | **§18.9 B krijgt een maxime-rij**; `M1.2.S03.200` gaat de aanrakingstabel in | allen |

---

### L1-R31 — De M1.5-uitzondering vervalt. Elf blijft van Ember, ook in de scène die het duurst was

**De uitzondering had één gestelde grond en die was al onwaar toen ik hem schreef:** *"M1.5 is GO en wordt niet heropend voor een getal."* Alle zes M1.5-bestanden droegen `critic: null`. Ik heb dat zelf gemeten en opgeschreven in ronde 6 — en de ruling die erop leunde niet heropend. **Een uitzondering wiens enige grond is ingetrokken, is geen uitzondering meer maar een verzuim met een voetnoot eronder.**  <!-- premisse-nagelopen: L1-R31 CITEERT de oude grond om hem te verwerpen -->

**Maar de grond is niet het argument dat mij overtuigt, en dat is de moeite waard om te scheiden.** Er zijn er drie die zwaarder wegen, en de eerste twee zijn metingen:

1. **Het corpus is overal geveegd behalve daar.** **Vijftien** scènebestanden dragen inmiddels een expliciete `# eleven: L1-R14 checked`-notitie (geteld, niet aangenomen: negen hub-bestanden, zes M1.8-scènes). Dat is de slechtst denkbare verdeling van een regel: **hij wordt gehandhaafd waar hij niets kost en gepasseerd op de enige plek waar hij iets kost.** Een regel die zo verdeeld is, leert schrijvers dat handhaving over gemak gaat.
2. **De uitzondering was nooit gratis — hij is elders afgeschreven.** `M1.6.S03.220` heeft zijn getal ingeleverd (*"Krad-9. Eleven days."* → *"Seventeen days."*) met als opgeschreven reden: *"Three scenes earlier, in M1.5.S03, 'eleven days' means HOW LONG A MAN HELD OUT."* Een andere missie heeft de specificiteit van haar beat betaald voor een uitzondering die alleen stond omdat M1.5 GO heette.  <!-- premisse-nagelopen: citaat uit M1.6.S03 dat de oude toestand beschrijft -->
3. **En dit is de enige scène in het corpus met twee betekenissen van elf.** `.150` — *"We told her no. Eleven people is not a cell. It's a list somebody hasn't filed yet."* — is niet zomaar een gebruik: het is **L1-R14's eigen stelling, hardop gezegd, door de man die er het meeste recht op heeft.** Het is het beste gebruik van het getal in de act. Tachtig regels verderop meet hetzelfde woord verhoordagen, en `.240` is een vlakke herhaaltic die het getal isoleert precies op de botsing. **De tweede betekenis beschadigt de eerste, en de eerste is de reden dat de regel bestaat.**

**Ruling: de uitzondering vervalt. `.230` / `.240` / `.250` wijken.**

Er is geen kostenargument meer over: de scène staat op `draft` en op NO-GO, het bestand gaat toch open, en de timingvoorwaarde uit L1-R14 zelf (*"bij de eerstvolgende keer dat die missie toch open is"*) is voldaan.

**Wat er precies moet gebeuren, en de eisen zijn van mij, de regels zijn van de schrijver:**

| Regel | Eis |
|---|---|
| `.230` | **Exact acht woorden blijven** (L2-eis, nageteld). Vorm blijft *"Cell two. He gave you N days first."* |
| `.240` | **Eén woord blijven.** Dit is de tic van de emissaris (§18.4: *"repeats the other person's last word back at them, flat"*) en een tweewoordig getal maakt van een echo een mededeling |
| `.250` | **Eén dag ná `.230` blijven.** Dat is de beat: hij kwam eruit op de dag nadat de ander brak |

**Het getal is `eighteen`, en `.250` landt op de negentiende dag.** Ik noem het zelf in plaats van het aan de schrijver te laten, want vijf schrijvers hebben onafhankelijk naar hetzelfde getal gegrepen en dat is hoe L1-R14 ontstond. De uitsluitingslijst, gemeten:

| Uitgesloten | Waarom |
|---|---|
| **eleven** | de ruling |
| **seventeen** | staat sinds 01-08 in `M1.6.S03.220` als transportduur in een kist. Zeventien hier zou de reparatie **spiegelen** in plaats van hem af te maken: de speler leerde dan drie scènes eerder dat zeventien dagen verhoor betekent |
| **nine, six** | verzadigd — 30× resp. 29× gesproken in act 1, gemeten door de M1.6-schrijver |
| **thirteen** | M1.1's titel en zes gesproken gebruiken |
| **fourteen** | staat **in dezelfde missie**, `M1.5.S99.100`/`.110`, als burgertelling |
| **sixteen** | werkt alleen als `.250` op de **zeventiende** dag landt, en dat is `M1.6.S03` opnieuw |

Daarmee is achttien het enige vrije getal onder de twintig dat `.240` één woord laat blijven. **Restrisico, gemeld en aanvaard:** negentien is gesproken in `M1.8.S01` en `M1.8.S03`, beide over K-77's cellenblok. Dat is drie missies later, cardinaal tegenover ordinaal (*"nineteen doors"* tegenover *"the nineteenth day"*), en in een ander soort zin. Dat is een lean, geen botsing — zie L1-R33 voor waarom die twee klassen niet dezelfde behandeling verdienen.

**`M1.6.S03`'s zeventien blijft, en dat is geen coulance.** De vraag was terecht: is die betaling nu voor niets geweest? Nee — **L1-R14's hoofdregel veroordeelde die regel sowieso.** *"Elf betekent de omvang van Ember Cell"*, en *"eleven days in a car"* meet transportdagen. De botsing met M1.5 bepaalde alleen **wanneer** hij weg moest, niet **of**. Zeventien blijft staan, en ik heb dat in het L1-blok van `M1.6.S03` vastgelegd zodat niemand hem terugdraait met het argument dat de aanleiding verdwenen is.

**En `M1.5.S99.100` — *"Eleven. Maybe fourteen."* — blijft, geclaimd.** Dat is de vierde elf van de missie in een dérde betekenis (burgers in een portiek), en hij corrigeert zichzelf binnen vier woorden naar veertien, dat de scène daarna gebruikt. Geen poort. **Maar ongeclaimde gebruiken stapelen zich op, en L1-R14 bestaat omdat dat gebeurd is.** De goede lezing was al beschikbaar en is nu opgeschreven in een `note:`: **Voss telt vreemden in de eenheid van zijn eigen cel en corrigeert zichzelf.** Dat is geen tweede betekenis van elf; dat is elf gebruikt als maatstaf en meteen te klein bevonden — wat de regel eerder bevestigt dan ondermijnt.

---

### L1-R32 — Eenenveertig is de instroom en niets anders. `S06.010` vertakt niet

Buiten de opdracht gevonden, en het is exact de L1-R14-klasse. **Ik heb de arithmetiek van alle drie de takken uitgeschreven in plaats van de melding over te nemen, en de melding wees de verkeerde tak aan.**

| Tak | Wie er in Hollow Point aankomt | Wat het corpus daarover zegt |
|---|---|---|
| `run` | beide wagons, min wie op de spoordijk viel | niets — geen enkel getal |
| `emptied` | beide wagons, min de veertig die gemerkt op de weegbrug knielen (`S05.310` shot) | veertig achtergelaten |
| `split` | de **eerste wagon**, heel | *"Forty-one at the drawbar. Forty-one at this wall."* (`S05.370`) |

En `S06.010` zegt **onvoorwaardelijk** eenenveertig, in alle drie.

**Reken het na en de breuk zit niet waar de melding hem zette.** `emptied` en `split` zijn onderling consistent: ze verliezen allebei ruwweg één wagon, dus beide impliceren een trein van ~81 en leveren ~41. **`run` is de tak die breekt** — dat is de tak waarin iedereen meekomt, dus die zou ~81 moeten leveren en levert 41. **De tak waarvoor de speler een zwaarder gevecht kocht om niemand achter te laten, is de tak waarin het getal hem niets teruggeeft.**

**De criticus stelde één conditionele variant op `S06.010` voor. Ik neem dat niet over, en de reden is een telling die hij niet kon doen.**

Eenenveertig staat, gemeten, **op veertien plekken in vier missies plus de hub**: `M1.6.S06` (.010, .040, .110, .120, .190×2 varianten), `M1.6.S99` (.130, .140 varianten), `M1.6.S05.370`, `M1.7.S99` (3×), `HUB.A1.sela_intake` (1× gesproken), `M1.8.S99` (1×). **Eén conditionele variant op de eerste van die veertien maakt de andere dertien tot tegenspraken** — en het duwt het getal als vertakking act 2 in, waar het voor altijd een `condition` op elke regel over de instroom wordt. Dat is de duurste van de twee beschikbare reparaties en hij repareert de goedkoopste plek. **Dat is L1-R12 omgekeerd: de zetter vertakken en dertien lezers laten staan.**

**Ruling, in drie delen.**

**1. Eenenveertig is een campagneconstante. De takken verschillen in wíé, niet in hoevéél.**

Dat is geen boekhoudkundig gemak — het is wat de missie zelf zegt. `M1.6.S01.110`/`.120`: *"Nobody outside that yard has ever counted them."* / *"Then that isn't a number. That's a hope."* Reyes' *"sixty-six"* wordt twee regels later expliciet als schatting neergezet. **De trein is nooit geteld, en dat is geen omissie maar de these van de missie** (pijler 3, en de reden dat Brick in S01 als eerste spreekt). Binnen elke afzonderlijke speelbeurt klopt eenenveertig met alles wat die speler zelf gezien heeft; de tegenspraak bestaat uitsluitend **tussen** takken, in een totaal dat het spel weigert uit te spreken.

**En een vertakt getal zou iets beweren dat niet waar is:** dat iemand de trein geteld heeft. Legibel maken hoeveel mensen je gered hebt, is de gewoonte van het manifest dat *"counts the ore and counts the rest the same way"* (`S01.160`). Het getal dat de speler mag hebben, is het getal dat Ember zelf heeft: **één man die een vaultvloer twee keer telde en twee antwoorden kreeg.**

**2. De prijs van elke tak is kwalitatief en hij is al geschreven — beter dan een getal het kon.** Dezelfde vrouw in de gele werkjas: neergeschoten op de spoordijk onder `run`, geknield en gemerkt bij de weegbrug onder `emptied`, en onder `split` weet niemand in welke wagon ze stapte. **Drie afloopen, één jas, en de derde is de enige die niemand kan betreuren omdat niemand hem kan benoemen.** Dat is de kostprijs. Een ander getal in Dex' mond is een zwakkere versie van dezelfde mededeling.

**3. Wat er wél moet gebeuren is één notitie, en het is de enige regel die van eenenveertig een deeltelling maakt.**

`M1.6.S05.370` is juist zoals hij staat en er verandert geen letter — in `split` heeft Sela alleen nog de eerste wagon, ze telt hem bij het loskoppelen en bij de muur, en ze komt uit. **Het risico zit in het woord *"drawbar"**: als een latere schrijver dat leest als de telling uit `S03` (vóór de vertakking), dan is de eerste wagon in álle takken eenenveertig en klopt `run` niet meer. **De drawbar in `.370` is de koppeling waar de wagons gescheiden zijn, niet de ramp van S03.** Dat moet in het bestand staan.

`M1.6.S05` is deze ronde niet van mij — er zit een schrijver aan `.410`. **De notitie gaat mee in dezelfde beurt als L1-R21 (`.410` vlak) en L1-R25 (`.350`–`.390`), dus het bestand gaat één keer open, niet drie keer.** Kosten: nul credits, nul gesproken woorden.

**En dit gaat in het nummerregister** (`ACT1_OVERVIEW` §7). Elf had een ruling nodig omdat niemand het totaal kon zien; eenenveertig krijgt een rij vóórdat dat gebeurt.

---

### L1-R33 — `Nobody`/`Nothing` is thema. Geen plafond, en hier staat waarom, zodat niemand het opnieuw afleidt

De M1.6-criticus mat een tweede verzadiging met dezelfde vorm als elf: `Nobody`/`Nothing` + presens + locatief. **Nagemeten, en de meting klopt:** 30 regels in M1.6 tegen 159 corpusbreed (mijn telling; de zijne was 158 — het verschil is één regel en niet materieel). Genormaliseerd: **15,6% van M1.6 tegen 9,7% corpusbreed, 1,6× de act-norm.**

**Ruling: geen plafond. Dit is thema, en het verschil met elf is categorisch.**

| | **L1-R14 (elf)** | **Dit** |
|---|---|---|
| Wat drift | de **referent** — acht betekenissen op één token | niets. Eén constructie, één betekenis |
| Wat de speler oploopt | een feitelijke tegenspraak die hij kan horen | textuur die hij als stem ervaart |
| Klasse | **continuïteitsdefect** | **stijl-lean** |
| Instrument | corpusbrede ruling + veegbeurt | scèneoordeel van de criticus |

**Dat onderscheid is de eigenlijke opbrengst van deze ruling, en het geldt vooruit:** een corpusbrede telling is pas een ruling waard als de **betekenis** meeschuift. Een constructie die overal hetzelfde betekent, kan alleen te vaak zijn — en "te vaak" is een smaakoordeel op scèneniveau, waar de striptest en §18.9 B het al meten. Een percentageplafond zou schrijvers laten tellen in plaats van schrijven, en dat is de duurste soort regel: hij kost aandacht in elke scène om een schade te voorkomen die niemand kan aanwijzen.

**M1.6 mag de piek zijn en hij is het om de goede reden.** Het is de missie waarin niemand geteld, benoemd of opgeschreven is: *"Nobody outside that yard has ever counted them."* · *"Nobody wrote down where."* · *"Nobody in this room knows a single one of them."* · *"Somebody should be able to say afterwards who got into that car."* **Het onderwerp van de missie is grammatica geworden, en dat is precies wat §18.9 D's tegendeel is** — dat is geen auteur die naar dezelfde constructie grijpt, dat is een missie die één ding zegt.

**Twee begrenzingen, in dezelfde vorm als ronde 1's "iedereen telt":**

1. **Dit is het register van deze missie, niet van het spel.** Act 2 verlaat Kessara en verandert van register (L1-R30, ronde 1). Wie in act 2 op 15% zit, heeft geen thema maar een gewoonte.
2. **De constructie draagt hoogstens één wending per scène.** Ze is hoe de wereld praat; ze is niet de enige manier waarop een scène kan landen. Dat is dezelfde begrenzing als de telbeat uit ronde 1, en om dezelfde reden.

**Wat niet gebeurt: er komt geen §18.9-rij voor.** Een rij toevoegen zou de klasse gelijkstellen aan triaden en em-dashes, en dat is precies het onderscheid dat deze ruling maakt.

---

### L1-R34 — O-4: de stub wijkt. Sela wint met toestemming, niet met een tweede vraag

`BEATS_M1.6` §4 eist van Sela in `M1.6.S03` *"vier tot zes korte beurten, waarvan minstens twee vragen"*. Geteld: vijf beurten (`.140` `.160` `.170` `.190` `.200`), en **één** vraag (`.140`). `.190` — *"No. So say it out loud if you're staying, and the rest of us can stop counting on you."* — is een imperatief, geen vraag. De L2-notitie in het bestand claimt twee vragen en die claim is onwaar; ook dat moet weg.

**De criticus denkt dat de tekst gelijk heeft en de stub moet wijken. Aangenomen, en de reden is beter dan "het staat er nu eenmaal".**

**Mijn "minstens twee vragen" was een proxy en geen eis.** Wat de stub wilde, staat er twee regels boven: *"Sela mag geen toespraak houden (AR-7). Retoriek is structuur, geen volume."* De vraagvorm was de mechanische stok om dat af te dwingen. De scène levert waar het om ging — vijf beurten, langste negentien woorden, geen oration — via een ánder mechanisme, en dat mechanisme is sterker.

**En het is beter karakterisering dan wat ik vroeg.** Een tweede vraag maakt van haar een ondervrager. `.190` geeft de wagon **toestemming om te blijven**, en dat is precies wat ze in beweging brengt. Dat is de organisator: haar macht is dat ze mensen mag laten gaan en dat ze dan blijven. Het bedient §2.5 rechtstreeks (*"learns power's compromises"*) — ze begint als iemand die nog nooit heeft hoeven kiezen tussen twee goede dingen, en haar eerste wapen is dat ze niemand hoeft te dwingen.

**Doorgevoerd in `BEATS_M1.6.md` §4 S03.** De eis wordt wat hij altijd had moeten zijn: *geen oration, hoogstens zes beurten, en minstens één vraag* — en de vraag die er is (`.140`) draagt haar tic. **Wat er nu moet gebeuren:** `dialogue-writer` M1.6 corrigeert de L2-claim *"Two are questions (.140, .190)"* naar één. Geen regel verandert.

---

### L1-R35 — O-7: Dex' grappen blijven, in alle drie de takken. En dit is wat "nasleep" betekent

`M1.6.S99` geeft Dex twee komische beats (`.020`, en `.110` met `.170` als knop) onvoorwaardelijk — ook in `run`, waar de vrouw in de gele werkjas negentig seconden eerder op de spoordijk bleef liggen. §18.6 wil **nul** komische beats in nasleep, en L1-R7 heeft precies dit geval beslist voor `M1.1.S99`: *"Ligt er iemand gewond, dan is S99 nasleep."*

**Ik heb L1-R7's criterium tegen de act gehouden in plaats van het toe te passen, en het criterium is te ruim geformuleerd geweest.**

Als "nasleep" betekent *de missie heeft levens gekost*, dan is elke debrief vanaf M1.2 nasleep, en dan is het komische budget van zes opeenvolgende missies nul. Dat sloopt §18.6's eigen redenering (*"the game earns its darkness by having been funny earlier"*), het sloopt `M1.5.S99.070` — de bitterste grap van de act, expliciet zo gespecificeerd — en het sloopt Dex' halve functie in de cast.

**Ruling: een debrief is nasleep wanneer iemand in de kámer dood of gewond is.** Niet wanneer de missie doden heeft gekost. In `M1.1.S99` vuurt het criterium omdat er een squadlid op tafel ligt; dat is waarom die uitzondering aan `run.zero_casualty` hangt en niet aan een verliezenteller. In `M1.6.S99` is niemand in de kamer gewond en zijn de eenenveertig beneden in leven.

**En de takken vragen alle drie dezelfde terughoudendheid, wat betekent dat conditioneren niets oplost.** Onder `run` sterft een vrouw voor iedereens ogen; onder `emptied` knielen er veertig gemerkt op een weegbrug; onder `split` is een hele wagon weg. **Er is geen tak waarin de grappen "schoon" zijn**, dus een `condition` zou geen tonaal probleem oplossen maar er alleen één tak uit knippen.

**Drie redenen dat ze blijven, op volgorde van gewicht:**

1. **Ze voldoen aan §18.6 zoals hij bedoeld is.** *"The best jokes cost something. Dex jokes hardest when he's most frightened."* `.020` is een man die zijn werk onmogelijk ziet worden; `.110`→`.170` is een man die een last weigert en hem dan optilt. Geen van beide is opluchting en geen van beide gaat over de doden.
2. **De verhouding klopt.** Twee komische beats op zeventien regels is 1 op 8,5, ruim binnen §18.6's 1-op-6 voor downtime.
3. **En de productiereden, die noch de criticus noch een schrijver kan zien: een conditionele grap is een grap die de helft van de spelers nooit hoort, in de tak die daarmee de dunste wordt.** We hebben deze ronde net een ruling besteed (L1-R25) om de `split`-tak dikker te maken omdat hij de minste spelers en het meeste te verliezen had. Komedie uit een tak conditioneren doet het omgekeerde. `.170` is bovendien de laatste gesproken regel vóór het slotbeeld van Mara alleen bij de kaart; hem voorwaardelijk maken laat één op de drie spelers de scène zonder knop eindigen.

**Wat er nu moet gebeuren: niets.** De scène is goed zoals hij is. Ik leg het criterium vast in `BEATS_M1.6.md` §4 S99 en hierboven, want de volgende criticus zou het anders per debrief opnieuw afleiden — en dat is precies wat er nu gebeurd is.

---

### L1-R36 — Het kenteken is `two six one`. `M1.3.S05` wijkt, en cijfers worden los geschreven

`M1.2.S01` zegt *"Two-six-one"*; `M1.3.S05.150` zegt *"Nine one seven"*. Mijn eigen register (`ACT1_OVERVIEW` §5) wijst beide scènes aan als **dezelfde man**: *"M1.2.S01 (badge → intel), M1.3.S05 (hij antwoordt op de toren, levend gezien)"*. **Beide schrijvers hebben het gemeld; geen van beiden kon het oplossen, want er is nooit een registerrij voor het getal geweest.** §2.11 draagt deze draad tot het einde van de campagne, dus zonder rij verzint schrijver drie een derde nummer.

**Ruling: het kenteken is `two six one`. `M1.3.S05.150` verandert.**

Drie redenen, en de eerste is niet de goedkoopste maar de juiste:

1. **`M1.2.S01` is de scène waar het nummer intel wórdt** — het is letterlijk de `turn:` van die scène (*"Voss produces a badge number he has been carrying since the ration line, and the room's arithmetic changes"*). In `M1.3.S05` is het een callout van drie woorden bij een herkenning. **Het dragende gebruik wint van het bevestigende gebruik.**
2. **Zes voorkomens tegen één** (`.130`, `.145` plus vier as-varianten tegen één regel).
3. **De M1.3-schrijver heeft zich vooraf gewonnen gegeven** in zijn eigen bestand (r. 50–52: *"If M1.2's writer or the recap card gives this man a different number, L1 reconciles and I change .150"*). Dat is het goede gedrag en het maakt de wijziging kosteloos in overleg.

**En een eerlijkheid bij reden 2: die weegt minder dan hij lijkt**, want de zes M1.2-regels worden hoe dan ook aangeraakt door het tweede deel van deze ruling. Het besluit rust dus op reden 1.

**De cijferconventie, en die is van mij omdat hij het hele corpus raakt: los geschreven, geen koppeltekens.**

Gemeten: `M1.1.S05` schrijft *"Four six two"* op vier plekken (de ijkmissie, plus `M1.1.S99`), `M1.3.S05` schrijft *"Nine one seven"*. **`M1.2.S01`'s gekoppelde vorm is de enige in het corpus.** En een koppelteken is bovendien een TTS-gok: een model kan *"two-six-one"* als samenstelling lezen in plaats van als drie cijfers, en dat is precies de klasse fouten die §18.9 C3 (hardop lezen) moet vangen maar die pas hoorbaar wordt ná generatie.

> **Conventie: cijfers die één voor één worden uitgesproken, worden als losse woorden geschreven.** *"Two six one."* Geen koppeltekens, geen cijfers. Getallen die als getal worden uitgesproken blijven één woord (*"Forty-one"*, *"Seventeen"*).

**Wat er nu moet gebeuren:** `dialogue-writer` M1.2 ontkoppelt de zes voorkomens in `M1.2.S01`; `dialogue-writer` M1.3 zet `.150` op *"Two six one."* — en zijn L2-notitie op r. 50 (*"NEW DETAIL: the number 917 is..."*) gaat mee, want die notitie is de instructie die schrijver drie ook fout zou zetten. **Het nummer staat vanaf nu in het nummerregister** (`ACT1_OVERVIEW` §7), samen met eenenveertig en elf.

---

### L1-R37 — Het geredigeerde personeelsrecord bestaat twee keer, en de verkeerde helft is verplicht

`M1.2.S04.220`–`.250` en `M1.7.S03.250` zijn hetzelfde document, hetzelfde personage, dezelfde laat-het-vallen-beat: een geredigeerd personeelsrecord uit het huishouden van de Arbiter (P2-d, twist 2).

| | `M1.7.S03.250` | `M1.2.S04.220`–`.250` |
|---|---|---|
| omvang | **één** regel | **vier** regels |
| toegang | `condition: 'run.m17_record == true'` | **onvoorwaardelijk** |
| wanneer | vijf missies later | eerst |

**Dit is mijn fout en hij staat in twee van mijn eigen documenten.** `ACT1_OVERVIEW` §4 zet P2-d bij `M1.7.S03`; `BEATS_M1.2.md` §4 zet hem bij `M1.2.S04`. **Beide zeggen "één Dex-regel die het niet begrijpt en verder gaat", en ze noemen twee verschillende scènes.** De M1.2-schrijver heeft het conflict gezien, het volgens zijn eigen stub geschreven en het geëscaleerd — de juiste volgorde. En **L1-R26 is daarna over M1.7 gegaan zonder de M1.2-kopie te zien**, heeft hem gedeclareerd en 96 credits begroot. Beide bestanden staan op `draft`, dus dit is nu nog gratis.

**Ruling: P2-d woont in `M1.7.S03.250` en nergens anders. `M1.2.S04.220`–`.250` vervalt.**

Vier redenen, op volgorde van gewicht:

1. **Optioneel-per-spec en onvoorwaardelijk-in-het-bestand kan niet allebei.** Beide stubs noemen P2-d optionele textuur; alleen de M1.7-versie is dat ook. De M1.2-schrijver kon hem niet poorten omdat er geen vlag voor bestond en heeft dat netjes opgeschreven. **Een beat die per spec optioneel is en die elke speler verplicht hoort, is geen textuur meer.**
2. **De beat moet iets zijn waar niemand twee keer naar kijkt, en vier regels ís twee keer kijken.** *"Not blacked. Lifted. Somebody went back through the index for that."* is een goed beeld en dat is het probleem: het is Dex die geboeid is door iets waarvan hij in de volgende regel zegt *"Anyway. Not ours."* De ene M1.7-regel doet hetzelfde werk en laat het los in dezelfde adem.
3. **P2-d hoort bij P2-b, niet bij P2-a.** In M1.2 leert de speler dat een gestolen custodian-key-token boven celbestrijding gaat (P2-a). Vijf missies later vindt hij in één scherm **twee afwezigheden**: een query-keten die ouder is dan alles erboven (P2-b) en een naam die chirurgisch uit een index is gelicht (P2-d). Dat tweede cluster is waar de speler een tweede jager moet voelen. **In M1.2 is P2-d een curiositeit zonder iets om aan te hechten; in M1.7 is het het tweede bewijs van dezelfde afwezigheid.**
4. **L1-R26 heeft de machinerie al aan de M1.7-kant gezet:** `run.m17_record` gedeclareerd, 96 credits begroot, en de datataak (één optionele objective-rij plus de mapping) staat in `VOICE_LEDGER.md`. Aan de M1.2-kant bestaat daar niets van.

**AR-9 blijft voldaan.** `M1.2.S04.120` is de verplichte gesproken drager van P2-a en die verandert niet; `M1.7.S03.130`/`.140` is de verplichte drager van P2-b. P2-d mag optioneel zijn omdat hij nergens alleen draagt.

**Wat er nu moet gebeuren:** `dialogue-writer` M1.2 verwijdert `.220`–`.250` en de bijbehorende L2-alinea; `.210` loopt schoon door naar `.260`. **`BEATS_M1.2.md` is door mij gecorrigeerd** — dat was de bron van de fout en het is mijn document. Twee bevindingen van de criticus vervallen daarmee vanzelf (`.240` op §18.9 C2, en het ontbreken van een `condition` op een blok dat de beat-sheet optioneel noemt).

---

### L1-R38 — `Story.Clue.*` wordt gezet waar de clue gesproken wordt. Nooit in de debrief

`M1.2.S04`'s kop zegt dat S04 `Story.Clue.CustodianKey` zet, en `ACT1_OVERVIEW` §6 bevestigt dat. `M1.2.S99.280`'s `note:` zegt dat **S99** hem zet. **Twee opslagplekken voor één waarheid (L1-R3), en de lezer staat in act 2, twist 2 — de duurste klasse van L1-R12.**

**Ruling: `M1.2.S04` zet hem, bij de scène-commit, en niets anders.**

De reden is niet dat het register ouder is, maar dat de andere kant een gat maakt dat je kunt uitspelen. **Een clue-vlag registreert dat de speler de clue gehóórd heeft.** Hij wordt op precies één plek uitgesproken: `M1.2.S04.120`. De M1.2-schrijver noteert zelf dat het wardenkantoortje mogelijk niet op het verplichte pad ligt (AR-9-melding, r. 52–54). **Zet je hem in de debrief, dan krijgt de speler die die kamer oversloeg de clue alsnog** — en act 2 rekent hem af op iets wat hij nooit gehoord heeft.

**En dit is geen incident maar een klasse, dus het wordt een regel.** Nageteld tegen het register: `BlightBroadcast` (M1.3.S02), `OutsideQuery` en `AegisDenial` (M1.7.S03), `ThrexVoice_1` (M1.3.S06), `ThrexKnowsSector` (M1.7.S04) — **alle vijf worden gezet in de scène waarin ze klinken.** `CustodianKey` was de enige met een tweede claim, en die claim was de uitzondering.

> **Regel: `Story.Clue.*` wordt gezet door de scène waarin de clue wordt uitgesproken. Debriefs zetten `Story.Beat.*` en de uitkomst van keuzes; ze zetten nooit een clue.** De debrief is onvoorwaardelijk en een clue is dat niet.

**Wat er nu moet gebeuren:** `dialogue-writer` M1.2 corrigeert de `note:` op `M1.2.S99.280` en haalt `Story.Clue.CustodianKey` uit `M1.2.S99`'s `flags-in:` — die scène leest hem in nul condities, dus de kop liegt twee keer over hetzelfde. Ik heb de regel in `ACT1_OVERVIEW` §6 gezet.

---

### L1-R39 — `Story.Choice.M12_Ghost` wordt gezet op de stille tak van S05. Het register had ongelijk

`ACT1_OVERVIEW` §6 r. 237 noemt *"M1.2 debrief"* als zetter; `M1.2.S05` zegt exit van de stille tak. **Het bestand heeft gelijk en mijn register niet.**

`M1.2.S99.030`–`.070` **lezen** de vlag — vijf regels, twee blokken. Een vlag die gezet wordt aan het eind van de scène die hem bij `.030` leest, is bij die lezing nog niet gezet. **Wint het register, dan krijgt de speler na een vlekkeloze ghost-run alsnog *"They have us on a gallery in Block Nine."*** En inhoudelijk hoort hij daar ook: "ghost" is een eigenschap van hoe M1.2's extractie liep, en die wordt in S05 beslist.

**En de validator ziet dit niet, met opzet:** `Story.Choice.M12_Ghost` heeft geen bladeren, en `validate_script.py:924-938` rapporteert setterloze enkelbladvlaggen niet. **Dat is precies waarom L1-R12's kolom *gelezen door* bestaat — en diezelfde rij is daar óók fout, door omissie.** Er stond *"Veil-alertheid act 1"*, een systeem dat niet gebouwd is, terwijl de enige aantoonbare lezer vijf regels in `M1.2.S99` is. **Een cel in die kolom die een ongebouwd systeem noemt in plaats van de vijf regels die er staan, is een lege cel met een jasje aan.**

**Doorgevoerd in `ACT1_OVERVIEW` §6:** zetter `M1.2.S05` (stille tak), lezers `M1.2.S99 (5)` plus de systeemlezer expliciet als *ongebouwd* gemarkeerd. **Wat er nu moet gebeuren:** niets aan het bestand. De schrijver had gelijk.

---

### L1-R40 — Zes maximes uit één mond in één missie, en de tabel telt er één

De criticus richt dit expliciet aan mij omdat het een standaardwijziging is, en hij heeft gelijk dat het er een moet worden.

Mara levert in M1.2 zes gepolijste maximes: `S01.080` *"Paper does not keep."* · `S02.220` *"She is a door we did not have an hour ago."* · `S03.150` *"They mark everything. They come back for what is worth the night."* · `S03.200` *"A marked block is a block nobody looks at twice."* · `S04.200` *"Orders change. Lists do not."* · `S99.240` *"That is what tomorrow is for."* **De T4-aanrakingsinventaris telt er één.**

**Dit is mijn eigen L1-R21-diagnose die nog steeds openstaat.** Ik schreef daar: *"de P4-tabel telt plants en niemand telde de aanrakingen; een tabel die de ene meet en waartegen de andere gecontroleerd wordt, is geen meting."* Ik heb toen de aanrakingsinventaris gemaakt en hem **alleen voor T4-wegverklaringen** gevuld. §18.9 B heeft een rij voor triaden en een voor em-dashes — de standaard wíl dit soort verzadiging meten, alleen niet voor epigrammen.

**Ruling, in twee delen.**

**1. §18.9 B krijgt een rij, en hij krijgt de vorm van de triadenrij omdat schrijvers die al toepassen.**

> | **Maximes** | Meer dan één citeerbare algemene waarheid uit dezelfde mond per scène |

Eén per scène per mond, precies zoals *"Any 'X, Y, and Z' rhetorical triple more than once per scene"*. Schrijvers schrijven nu al *"the scene's one permitted triad (18.9 B)"* in hun L2-blokken; dit hangt aan een gewoonte die er is in plaats van een nieuwe te vragen. **Op M1.2 zakt daarmee precies één scène: S03, met twee.** De andere vijf missiescènes hebben er één en zijn schoon.

**Waarom een scène-cap en geen missie-cap:** zes maximes over vijf scènes is een leider die in stelregels denkt, en dat is Mara. Twee in één scène, vier regels uit elkaar, over dezelfde anomalie, is de vorm die een tell wordt. **Het is dezelfde grens die de triadenrij trekt en om dezelfde reden: de constructie is niet het probleem, de dichtheid is het.**

**Eerlijk over wat ik niet gemeten heb: ik heb geen shell in deze sessie en heb het corpus hier niet op kunnen vegen.** De rij geldt vanaf nu voor nieuw werk; **de retro-sweep over M1.1 en M1.3–M1.8 hoort bij de act-1-continuïteitsdoorloop en moet gemeten worden vóór iemand er een NO-GO op geeft.** Dat is dezelfde behandeling als L1-R14 kreeg, en om dezelfde reden: een regel die op een ongemeten corpus meteen als poort gaat werken, produceert bevindingen die niemand kan wegen.

**2. `M1.2.S03.200` gaat de aanrakingstabel in, en hij wordt vlak.**

Hij staat vier regels na aanraking #1, uit dezelfde mond, over dezelfde anomalie, en in geen enkele rij van welke tabel dan ook. **Het is geen T4-wegverklaring — het is de *exploitatie* van de anomalie**, en de `note:` in het bestand zegt precies waarom dat erger is: *"Dit is waarom ze het nooit onderzoekt, en het is dezelfde blinde vlek die haar K-77 in draagt."* **Dat maakt hem geen mindere aanraking maar een zwaardere**, en L1-R17's drempel (*"twee is toeval"*) is daarmee al binnen één scène opgebruikt, vóór `M1.5.S05.200` er ook maar aan te pas komt.

`M1.2.S03.150` blijft gepolijst — dat is de ontworpen eerste, *"het is competentie"*, en de afbraakcurve heeft een top nodig. **`.200` is de regel die wijkt**, want twee gepolijste zinnen in één scène over hetzelfde raadsel leren de speler de vorm in plaats van het feit. De beat blijft: ze draait de anomalie om in een voordeel en Voss laat haar. Alleen de politoer gaat eraf — dezelfde behandeling en dezelfde reden als `M1.6.S05.410`.

**Wat er nu moet gebeuren:** `dialogue-writer` M1.2 herschrijft `.200` vlak en verwijdert niets. De aanrakingstabel in `ACT1_OVERVIEW` §4 is door mij bijgewerkt en telt er nu acht, in speelvolgorde.

---

### Huishouding — de veegbeurt die bij ronde 7 hoorde, en wat hij extra opleverde

Vier plekken waren gemeld. **Het waren er tien, en dat is het punt.**

| Waar | Wat er stond | Nu |
|---|---|---|
| `M1.5.S05` kop `flags-in:` | `M15_ShiftBossSpared` | de drie bladeren, met de leesplekken erbij |
| `M1.5.S99` kop `flags-in:` | `M15_ShiftBossSpared` — **en S99 leest die vlag in nul condities** | `M15_Terms.*`, de enige die hij wél leest |
| `M1.5.S99` kop `flags-out:` | `M15_IronChorusPact` | `M15_Pact.{Full,Limited,None}` |
| `M1.5.S99` L2 `format:` | idem | idem |
| `BEATS_M1.5.md` r. 94 / 116 / 124 | beide oude namen | bladvorm, plus welke vlag S99 werkelijk leest |
| **`BEATS_M1.1.md` r. 114 / 145 / 174** | **`M11_ConscriptSpared`** — ingetrokken door L1-R3, en L1-R29 heeft in juli alleen `BEATS_HUB_A1` gerepareerd | `M11_Conscript.{Finished,Left,Bound}` |
| **`BEATS_M1.6.md` casttabel** | `M15_IronChorusPact` | `story.m15_pact != "none"`, met het onderscheid met `split` erbij |
| **`BEATS_M1.8.md` §vlaggen** | `M15_IronChorusPact` | `M15_Pact.*` + de ontbrekende `M15_ShiftBoss.Warned` |
| **`M1.6.S02` / `M1.6.S04` koppen** | `M15_IronChorusPact` | bladvorm, mét de regel dat S02 `!= "none"` test en S04 `== "full"` |
| `M1.6.S06` / `M1.6.S99` koppen | vlaggen die de scènes niet lezen, en de vlaggen die ze wél lezen ontbraken | rechtgezet |

**In elk van de tien gevallen waren de `condition`-regels correct en was de validator schoon.** Alleen de instructies logen. **Dat is precies wat L1-R29 zegt** — *"een beat-sheet is ook een lezer"* — en de zes extra vondsten laten zien dat die uitbreiding in juli maar half is doorgevoerd: ik heb toen het beat-sheet gerepareerd dat de escalatie noemde en de andere vijf niet nagelopen. **Een naamveegbeurt is pas af als je hem op de hele boom hebt gedraaid, niet op de melding.**

Verder afgesloten: `M1.5.S99`'s L2-escalatie (open sinds 31-07, opgelost door L1-R13/L1-R3), en `M1.5.S01`'s L2-notitie die een verholpen defect meldde (`mara_sovann` / `dex_callum` / `elin_reyes` komen nergens meer voor behalve in die notitie). **Een afgehandelde escalatie die open blijft staan, is een instructie aan de volgende schrijver** (L1-R30).

---

## Wat ronde 7 niet heeft opgelost, expliciet

- **Ik heb `validate_script.py` opnieuw niet kunnen draaien — geen shell, derde sessie op rij.** Ik heb in deze sessie uitsluitend commentaarblokken, koppen, `note:`-velden en mijn eigen documenten aangeraakt; geen `text:`, geen `condition:`, geen ID. Dat zijn allemaal categorieën waarvan ik in ronde 5 in de parser heb bevestigd dat ze door de bestaande scalaire tak lopen. **Verwachting: de bar blijft op 5 bevindingen (4 REGISTER, 1 BRANCH) — geen daarvan is van mij, en geen enkele wijziging in deze ronde raakt een check aan.** Wijkt dat af, dan is deze ronde het probleem en niet de tool. **Dat drie L1-rondes op rij blind zijn afgesloten, is zelf een bevinding en hij hoort op de statuskaart, niet in een voetnoot.**
- **De veegbeurt voor L1-R40 is niet gedaan.** M1.1 en M1.3–M1.8 zijn niet op maximedichtheid gemeten. Poort pas na meting.
- **`M1.5.S03`'s achttien is een schrijversactie en is niet uitgevoerd.** De ruling staat; de drie regels zijn van de schrijver.
- **`M1.6.S05` is deze ronde niet aangeraakt** — er zat een schrijver aan `.410`. Twee dingen gaan mee in dezelfde beurt als L1-R21 en L1-R25: **(a)** de drawbar-notitie op `.370` (L1-R32), en **(b)** de kop, die `flags-in: Story.Choice.M16_TrainRun` draagt terwijl de scène de drie bladeren `story.m16_train_choice` leest — dezelfde koppenveegbeurt als in `M1.5.S05`/`S99` en `M1.6.S06`/`S99`, en de enige die is blijven liggen.
- **De M1.2-`.yaml`'s zijn niet door mij aangeraakt** (L1-R36, L1-R37, L1-R38, L1-R40 deel 2 en de `.220`–`.250`-verwijdering zijn alle vijf schrijversacties). Afgesproken met de coördinator, want er zat een schrijver aan die missie.
- **Nog open uit eerdere rondes:** L1-R28 (gespecificeerd, niet actief), `M1.5.S99.050`/`.060` in de `none`-tak, `M1.4.S99`'s registerdrempel, de rolsplitsing bij O-16, Embers barkpool van twee registers, en Q-4.

---

## Ronde 8 — 2026-08-02, twee NO-GO's, één BRANCH, en de klasse die acht critic-rondes heeft gedomineerd

**De criticus vat het scherper samen dan ik het had:** *"Acht critic-rondes en zeven L1-rondes, en de terugkerende fout is nooit een slechte regel — het is een notitie die iets beweert dat het bestand tegenspreekt."* Deze ronde bevestigt dat. **Geen enkele bevinding in deze ronde was een slecht geschreven regel.** Twee waren notities die logen over hun eigen bestand, drie waren afgehandelde escalaties die als instructie waren blijven staan, twee waren cellen die naar een bestandsregelnummer wezen alsof het een ID was, en één was een tabel die de praktijk vier vlaggen achterliep.

| # | Onderwerp | Uitkomst | Raakt jou als schrijver? |
|---|---|---|---|
| **L1-R41** | `M1.8.S99`'s BRANCH-bevinding en de geweigerde `silence:` | **Het veld gaat erin.** L1-R24 verwierp hem voor een ánder defect | M1.8 |
| **L1-R42** | De instroom heet *"de veertig"* in een notitie en *"Forty-one"* in de regel | **Het is de eenenveertig.** En *veertig* is in M1.8 al van de bevrijden | M1.6, M1.8 |
| **L1-R43** | `M1.8.S99.090` is een maxime in een slot dat het vlak deelt met `.100` | **`.090` wordt vlak.** `.110` blijft citeerbaar, en waarom | M1.8 |
| **L1-R44** | `M1.4.S04.220` — *"the district allocation"* | **Geen aanraking maar een PLANT.** Geregistreerd als P4-e | M1.4 |
| **L1-R45** | `M1.4.S99.020`–`.050` — het boek zei veertig, er waren er zesentwintig | **Wél een aanraking.** Geregistreerd. Nul tekstwijziging | M1.4 |
| **L1-R46** | `P0.S01.100` draagt zijn syntaxis in aanhalingstekens | **Herschreven, en de goedgekeurde kopij mee** | proloog |
| **L1-R47** | Kaartnummers botsen tussen twee documenten | **Slots ≠ kaarten.** De goedgekeurde kopij wint | proloog |
| **L1-R48** | De drie dode celnamen in `M1.3.S01` | **Ze komen er niet. De schrijver had gelijk** | M1.3 |
| **L1-R49** | `M1.3.S01`'s REGISTER-bevinding | **Sluit met één toegevoegde regel, `.095`** | M1.3 |
| **L1-R50** | Vier `run.`-vlaggen staan niet in de feitentabel | **Gedeclareerd, plus de regel voor de derde soort** | allen |
| **L1-R51** | §18.9 D mist een rij, en de proloog laat zien welke | **Rij toegevoegd. Drie beats op één constructie** | allen |
| **L1-R52** | De aanrakingstabel is T4-gescoped en de vorm generaliseert | **Twist-agnostisch, mét een definitie van "aanraking"** | allen |
| **L1-R53** | Mag L1-R40 nu poorten? | **Nog niet. Meten eerst — en intussen `hold`, geen NO-GO** | `dialogue-critic` |
| **L1-R54** | `dex_workshop.070`'s *"she"* na de Form A-herbouw | **Eén woord. En de regressie is van mij** | hub |
| **L1-R55** | *Acht* heeft vijf referenten en staat niet in het register | **Rij voor *acht maanden*.** Een register beschermt getal **plus eenheid** | M1.7, hub |
| **L1-R56** | Bij Sela wijzen maximes-rij en vingerafdruk tegengesteld | **Structuur ≠ maxime.** De cap blijft; de veeg stelt de smallere vraag | M1.6, hub, veegbeurt |

---

### L1-R41 — `M1.8.S99` krijgt zijn `silence:`, en L1-R24 verwierp een andere vraag dan de gestelde

**Aangenomen, en de criticus heeft gelijk op alle drie zijn gronden.** De belangrijkste is dat hij mijn eigen toets uit L1-R25 correct heeft toegepast in plaats van hem te citeren. Daar wees ik stilte af omdat *"`S05` over één ding gaat: mensen tellen, en in de split-tak is het getal veranderd"* — een stilte die iets verbergt. **Hier verbergt hij niets:** `M1.8.S99` gaat over wie de overlevenden volgen, en de emissaris stemt in **geen enkele tak** vóór de naam. `.180`/`.190` zijn hem die weigert, in een getal. De drie cellen op `.010` zijn onvoorwaardelijk (L1-R24), de stem is onvoorwaardelijk, en de `none`-speler krijgt 20 van de 24 regels plus een lege ramp — **wat precies is hoe het eruitziet als je een bondgenoot hebt afgewezen.**

**Waarom L1-R24 hem verwierp en waarom dat niet meer telt.** Die ruling zei: *"de tak speelt wél; alleen de camera stond verkeerd."* Dat is een antwoord op een andere vraag dan de tool stelt. De tool vraagt niet of de **scène** speelt in `none` — dat doet hij, volledig. Hij vraagt of de **vlag** iets gesproken bijdraagt in `none`, en dat antwoord is nee, met opzet. De cameraclausule was een echt defect en het verplaatsen was juist; het was alleen een ánder defect. **Ik heb in L1-R24 twee bevindingen samengevoegd omdat ze in hetzelfde bestand stonden.**

**En de melding zelf is voor de helft onwaar over dit bestand, wat de criticus als enige gezien heeft.** BRANCH zegt *"gets the beat of a choice he did not make, or none at all"*. De eerste helft geldt hier niet: alle drie de emissaris-regels zijn gepoort en gaan correct samen stil. Er is geen enkele regel die de `none`-speler krijgt en niet had moeten krijgen.

**Derde grond, en die is de onaangenaamste: de scène werd afgestraft omdat hij de beste is.** `M1.8.S99` is het enige bestand in het corpus dat `full` van `limited` onderscheidt. `M1.6.S02` leest alleen `!= "none"` en `M1.6.S04` alleen `== "full"` — één positieve waarde elk, dus `len(positive) >= 2` vuurt daar nooit. **Wie twee takken netjes uit elkaar houdt, krijgt de bevinding; wie ze samenklapt, ontsnapt eraan.** Dat is geen reden om de check weg te doen — hij heeft drie echte defecten gevangen — maar het is wel de reden dat `silence:` bestaat.

**Doorgevoerd.** Het blok stond plakklaar in het bestand en de criticus had de vier voorwaarden van de checker nagelopen; ik heb er één woord in veranderd (*forty* → *forty-one*, L1-R42). **De schrijver heeft het veld gezien, doorgerekend en NIET geplakt omdat hij niet stil over een benoemde ruling heen wilde.** Dat is het gedrag dat dit hele escalatiepad moet opleveren, en het is de reden dat deze ruling bestaat in plaats van dat er stilletjes iets gepatcht is.

> **Gemeten door de coördinator ná doorvoering: de BRANCH-bevinding is weg. De scriptbevindingen staan op 4, alleen REGISTER — voor het eerst onder de vijf.**

**Eén nasleep die van mij is en die ik hier beslis.** De coördinator meldde eerst dat mijn blok aan de verkeerde vlag hing, en heeft dat daarna zelf rechtgezet: een `silence:`-sleutel noemt een **blad**, geen vlag, en `M1.8.S99` leest er twee (`story.m15_pact` met full/limited/none, `story.m15_shiftboss` met killed/prevented/warned). De check draaide per vlag en meldde `none` als onbekende waarde omdat de *andere* vlag hem niet kent. De tool is gerepareerd: een waarde telt nu als bekend zodra één gelezen vlag hem kent.

**Dat is de juiste reparatie voor nu, en er blijft een gat in dat ik benoem in plaats van het te laten liggen.** Met de losse vorm dekt één sleutel élke vlag die de scène leest. Zodra een scène twee vlaggen leest die dezelfde waardenaam delen — en `none` is precies zo'n naam — kan één gedeclareerde stilte er twee afdekken, waarvan de tweede een echt gat is. Vandaag bestaat dat geval niet; het ontstaat op de dag dat iemand een tweede `.none`-blad schrijft.

> **Ruling: `silence:` krijgt een optionele vlaggekwalificeerde sleutelvorm — `story.m15_pact.none: "..."` — die alleen op die vlag slaat. Een sleutel zónder punt blijft betekenen wat hij nu betekent, dus geen enkel bestaand bestand verandert. GESPECIFICEERD, NIET ACTIEF, precies zoals L1-R28: de tool accepteert de vorm nog niet, dus niemand gebruikt hem tot dat zo is.** Ik heb geen shell en heb de tool deze ronde niet aangeraakt; hem specificeren en de adoptie tegenhouden is wat ik wél kan verantwoorden.

---

### L1-R42 — De instroom is *de eenenveertig*. En *veertig* is in M1.8 al bezet, door de andere groep in dezelfde kamer

`M1.8.S99` r. 47 noemde groep 2 **"THE FORTY"** terwijl `.080` in datzelfde bestand *"Forty-one"* spreekt, en L1-R32 eenenveertig al had vastgezet als campagneconstante voor precies die instroom. Dat alleen al is de bekende klasse.

**Maar ik heb geteld in plaats van het bij die correctie te laten, en het is erger dan de melding.** *Veertig* is in M1.8 **al de naam van de andere groep in dezelfde kamer**: de bevrijden uit K-77. `M1.8.S07`'s `want:`, `M1.8.S08`'s `want:`, `M1.8.S09`'s openingsshot en `M1.8.S05.120`'s shot noemen ze alle vier veertig — en `M1.8.S08.140`'s strategic-variant **spreekt** het uit: *"Forty people up a shaft rated for six."*

Dus in `M1.8.S99.010` staan drie cellen in één ruimte, en twee ervan worden geteld met getallen die één uit elkaar liggen: de **eenenveertig** van de trein en de **veertig** uit de cellen. Een notitie die de instroom *"de veertig"* noemt, wijst in de enige scène waar beide groepen samen staan naar de verkeerde.

**Ruling: de instroom heet *de eenenveertig*, ook als groepsnaam, overal. *Veertig* betekent in M1.8 de bevrijden.**

**En de bijna-botsing zelf blijft staan, gemeten en aanvaard — met een grens.** Dit is een *lean*, geen botsing, en het onderscheid is dat van L1-R31's achttien-tegenover-negentien: twee verschillende tokens, twee stabiele referenten, en **precies één gesproken gebruik aan de veertig-kant.** Eenenveertig is onbeweeglijk (veertien plekken, campagneconstante), dus als er iets moest wijken zou het de veertig zijn — en dat hoeft niet zolang hij gesproken één keer voorkomt. **Een tweede gesproken veertig in M1.8 maakt er wél een botsing van, en dan wijkt hij.** Rij toegevoegd aan het nummerregister, met de twee prozareferenten die er óók zijn (`M1.6.S05.310`, de achtergelatenen in `emptied`; `M1.4.S99.020`, het arsenaalboek dat in de fictie liegt) en de regel dat geen van beide ooit *"de veertig"* mag gaan heten.

---

### L1-R43 — `M1.8.S99.090` wordt vlak, en de meetlat ligt vier regels lager in hetzelfde bestand

Dit volgt uit L1-R52 en het is de enige regel die daardoor beweegt.

`.090` — *"The Veil buys people. That's the whole job."* — heeft mededeling-plus-generaliserende-staart, de vorm van rij #4. Zijn eigen `note:` noemt hem *"eight flat words"*, en dat is wat de schrijver dácht te schrijven.

**Wat het beslist is een controleproef die in het bestand zelf ligt.** `.090` en `.100` vullen **hetzelfde slot** — de warned-verwisseling. `.100` is vlak: *"There's a shift boss out there who saw both cells in one kitchen."* Dus de default-tak levert een citeerbare verklaring waar de warned-tak een gewone levert, **voor hetzelfde beat, zonder dat iemand dat gekozen heeft.** Vlak maken zet de twee helften van één slot in hetzelfde register.

**`.110` blijft citeerbaar, en dat is een beslissing en geen coulance.** *"Or he guessed. People guess and then they're right."* is een maxime, en het is de verklaring die **de kamer laat leeglopen** — daar ís het epigram de leegloop. Van de drie verklaringen mag er precies één herhaalbaar zijn. `.080` is de wáre en is de minst afgemaakte zin in de kamer. **Gepolijst-onwaar / vlak-onwaar / onafgemaakt-wáár is een ontworpen asymmetrie; twee gepolijste onwaarheden is alleen nog dichtheid.**

Beat, tak, spreker en volgorde blijven. Kosten nul: er is niets gegenereerd. **De regel is van de schrijver; de eis is van mij.**

---

### L1-R44 — `M1.4.S04.220` is geen aanraking maar een plant, en het onderscheid is het waard

*(Deze ruling is op 02-08 in het scènebestand gezet en pas hier in het log opgenomen — dat is de verkeerde volgorde en het is precies de fout die dit document moet voorkomen. Hersteld.)*

*"Note for the morning shift. The shortfall is written against the district allocation, not against this depot."*

De criticus kon niet vaststellen óf dit een negende aanraking was, en had gelijk dat hij het niet zelf besliste. **Het antwoord is dat het niet die soort regel is.**

Een aanraking heeft drie delen: **de Dominion schiet tekort · een personage verklaart het weg · niemand spreekt het tegen.** `.220` heeft er geen van. In zijn ogen is er niets tekortgeschoten, hij verklaart niemands gedrag, en hij spreekt niet tegen de speler. **Het is de Dominion die zichzelf beschrijft.**

**Wat het wél is: de enige keer in act 1 dat het act-4-vocabulaire hardop klinkt.** §2.4 boekt de Foundry-Collapse-doden als *optimal loss allocation*; de waarheidskolom van P4-a en P4-b luidt *"allocatie, geen incompetentie"*. Hier zegt een administrateur van middelbare leeftijd dat het tekort tegen de **district allocation** gaat en niet tegen zijn depot, en het betekent niets sinisterders dan in wiens kolom het landt. Dat is een **herkenningsplant** van dezelfde vorm als P1-a: geef de speler het woord zestig uur voordat het iets betekent, zodat act 4 verdiend wordt in plaats van aangekondigd. **De twee betekenissen lopen echt uiteen** — de zijne is boekhouding, die van AEGIS is planning — en daarom verraadt hij niets.

**En hij voldoet al aan L1-R17**, wat de criticus zelf gemeten heeft: de regel is vlak. Geen parallellisme, geen antithese, geen slotbeeld. Hij draagt dus niets bij aan de vorm-tell, en dat is het enige waar de aanrakingstelling voor bestaat. **Blijft vlak. Staat als poster goed? Dan herschrijven.**

Geregistreerd als drager van **P4-e**, samen met `M1.4.S99.020`–`.050`. Eén plant, twee dragers, één missie, twee kanten.

---

### L1-R45 — `M1.4.S99.020`–`.050` **is** een aanraking, en de notitie die het ontkende sprak zijn eigen bestand tegen

De andere kant van dezelfde missie, en hier valt het oordeel de andere kant op. Getoetst tegen de definitie in plaats van tegen de notitie:

| Deel | Waar het staat |
|---|---|
| **de Dominion schiet tekort** | `.020` — *"their book said forty were in that hall. There were twenty-six."* · `.030` — *"So where's the other fourteen?"* Veertien ontbrekende lichamen op een verdedigde positie: dezelfde vorm als P4-a en P4-b, één missie eerder dan allebei |
| **een personage verklaart het weg** | tweemaal. `.040` Dex: corruptie. `.050` Mara overschrijft hem met competentie |
| **niemand spreekt het tegen** | de `shot:` op `.060` zegt het met zoveel woorden, en het L2-blok noemt die shot terecht dragend |

**Het L2-blok claimde *"this deliberately does NOT plant T4"* op de grond dat *"the armory was fully defended"* — en dat wordt vier regels eerder door `.020` tegengesproken.** De schrijver adresseerde onder-*reactie*, terwijl de tabel de aanraking breder definieert. Notitie gecorrigeerd; de helft die wél klopte — dat hier vooral Mara's *lezing* van de overwinning geplant wordt — staat er nog, want dat is de helft die het schrijven stuurt.

**Registreren kost nul tekstwijziging, en dat is het argument.** De regel is al vlak: *"We got in because we were better than they are"* heeft geen figuur, geen ritme, geen slotbeat. Precies wat L1-R17 van een aanraking vraagt. Wat erbij komt is **een rij in de telling**, zodat de volgende aanraking tegen tien wordt afgewogen en niet tegen acht.

**Het verbetert Mara's curve in plaats van hem te verstoren.** Deze scène is de **top**: haar zekerste en persoonlijkste verklaring, en het L2-blok had de eis al woordelijk goed — *"not a comfort that reads as a lie later, but a conviction that reads as a cause of death"*. De afbraak begint daarna.

**En de rijm die niemand geregistreerd had, en die het beste argument is om er niets aan te doen:** Dex' wáre verklaring wordt hier midden in een woord afgekapt (`.040`), en zijn wáre verklaring van het K-77-verraad wordt in `M1.8.S99.080` midden in een woord afgekapt. Dezelfde man, dezelfde vorm, zeven missies uit elkaar, beide keren is het juiste antwoord de minst afgemaakte zin in de kamer. **Twee schrijvers die elkaar niet konden zien hebben dat twee keer geschreven.** Niet opruimen.

**Continuïteitsschuld aan act 4**, geregistreerd bij P4-e: de veertien ontbraken niet door diefstal maar stonden elders geboekt, want een drukventiel moet zichzelf kunnen bewapenen. **Niets in act 1 mag dat zeggen, suggereren of iemand erover laten peinzen.**

---

### L1-R46 — `P0.S01.100` wordt herschreven, en de goedgekeurde kopij gaat in dezelfde beurt mee

> was: `Then AEGIS came "pre-compliance" — and took Aunt Petra.`
> nu: `Then AEGIS came for Aunt Petra. The notice said pre-compliance.`

**Aanhalingstekens maken geen geluid.** Zeg hem zoals TTS hem krijgt en *"came"* krijgt een complement dat het niet kan hebben. **De leestekens droegen de hele syntaxis.** Een acteur lost dat op met toonhoogte; een TTS-model niet — en deze kaart is `credit_tier: 2`, de enige tag is `[quietly]` en de leveringsnotitie zegt *"flat, after the fact"*. **Vlak en zacht is precies de levering waarin een ongemarkeerd citaat verdwijnt.** Dezelfde klasse als L1-R36, en erger: een koppelteken heeft tenminste nog een defaultlezing.

**De schrijver heeft terecht geweigerd.** Het is verbatim goedgekeurde kopij (`RECAP_CARDS_M1.md` kaart 3) en zijn eigen L2-blok legt vast dat kopijwijzigingen een architect- plus ownerbesluit zijn. **Ik mag het wel, want de bedoeling ligt vast en alleen de vorm moet de spraakketen overleven:** AEGIS gebruikte dat eufemisme, Petra werd meegenomen, zonder aanklacht. Beide staan er nog.

**Wat veranderde is waar het eufemisme aan hangt: aan een document in plaats van aan leestekens.** Dat overleeft zonder acteerwerk, het haakt aan het vlugschrift dat al in de still ligt, en het is **bureaucratischer** in plaats van minder (§2.2: *"every atrocity has bureaucratic logic"*). `.110` — *"No charge. No date. No office that answers."* — leest daarna als het lézen van dat document, wat de kaart strakker maakt dan hij was. En de inzet was niet stilistisch: **koudlezervraag 3 rust volledig op deze kaart.**

`words:` van 127 naar 129 (het corpus telt em-dashes niet mee; nagerekend over alle zestien regels). `RECAP_CARDS_M1.md` r. 27 is in dezelfde beurt bijgewerkt, mét de reden eronder — **anders staat de oude kopij er nog als bron, en dan transcribeert de volgende agent hem terug.**

---

### L1-R47 — Slots zijn geen kaarten

`P0.S01_recap.yaml` heeft **zeven** ID-blokken; `RECAP_CARDS_M1.md` heeft **zes** goedgekeurde kaarten. Het extra blok is de gereserveerde Enforcer-kaart (Q-2) op positie 3, dus alles daarna ligt in het scriptbestand één hoger. Het bestand noemde ze allemaal *"card N"*. **Gevolg: "kaart 4" is Petra in het ene document en Mara in het andere, en wie *"repareer kaart 4"* kreeg had vijftig procent kans de verkeerde te bewerken.**

**Ruling: de goedgekeurde kopij wint. Kaartnummers zijn die van `RECAP_CARDS_M1.md`; wat het scriptbestand heeft zijn *slots*.** Dat is de goedkoopste kant om te wijken en de enige die niet hernummert: de ID-blokken zijn permanent (`SCRIPT_FORMAT` §2), de kaartnummers staan in een document dat de owner getekend heeft en waartegen de koudlezertest scoort. De mapping staat nu letterlijk in **beide** bestanden, want een offset die maar aan één kant beschreven is, is geen mapping.

Wordt Q-2 met **ja** beantwoord, dan groeit de kopij naar zeven kaarten en lopen beide nummeringen weer gelijk. Tot dan is die tabel de enige plek waar de verschuiving staat.

---

### L1-R48 — De drie dode celnamen komen er niet. De schrijver had gelijk en mijn stub had ongelijk

`BEATS_M1.3` beat 2 vroeg Dex de cellen die dit eerder probeerden **bij naam** op te noemen. De schrijver heeft geweigerd ze te verzinnen en geëscaleerd in plaats van het stil op te lossen. **Hij heeft plaatsen en aantallen geleverd, met een leeftijd op het eind** — *"The overpass, four, and two of the four were fifteen."*

**Aangenomen, en het is de L1-R34-vorm: mijn eis was een proxy en de tekst levert waar de proxy voor stond.**

Drie gronden, op volgorde van gewicht, en de eerste is níét de canonschaarste:

1. **Naamloosheid ís het onderwerp van de akte.** Wat een naam krijgt overleeft. Ember heet Ember; de kamer ruilt in `M1.8.S99` een familienaam voor een symbool; en AR-10 zet **de eerste naam op de muur** in `M1.8.S90`. **Dex' lijst is de reden dat die muur bestaat — hij kan zijn doden alleen tellen.** Geef hem namen en de lijst wordt een eregalerij, en `M1.8.S90` wordt een herhaling in plaats van een antwoord.
2. **De canon heeft ze niet.** `00_INDEX.md` kent op Kessara precies twee cellen, Ember en de Iron Chorus, en de tweede leeft en is M1.5's introductie. Drie dode cellen dopen is canon verzinnen en dus een owner-beslissing.
3. **De leeftijd doet het werk dat de naam moest doen.** Bricks apparaat, één stap verschoven, voor nul canon.

**Gevolg dat de melding niet voorzag: `.080` en `.090` worden dus NIET herschreven**, en de generatiewaarschuwing (L1-R18's les) vervalt. De `obstacle:` in de kop stond op *"is a name Dex can recite"* — die zin was onwaar over zijn eigen scène en is gecorrigeerd; `BEATS_M1.3.md` §4 S01 ook. **Het aanbod *"if L1 wants real names, I will rewrite .080/.090"* is ingetrokken in plaats van ongebruikt gelaten** (L1-R30).

---

### L1-R49 — `M1.3.S01`'s REGISTER sluit met één toegevoegde regel, en die regel is een karakterbeat

De criticus stelde voor de bevinding rood te laten met een herpunte reden. **Ik neem dat niet over, want er is een reparatie die de scène beter maakt en hij zat vast achter L1-R48.**

**Wat gemeten is.** De conditie waarvoor L1-R1 de registercontrole schreef — *"een cutscene waarin niemand ooit een volle zin zegt"* — is hier **niet** vervuld: `.230` (13 w), `.270` (14 w) en `.360` (13 w) zijn volle ondergeschikte zinnen. De twintig-woordsdrempel is een **proxy**, en hij faalt hier terwijl het ding waar hij voor staat standhoudt. Dat is een grond om de bevinding niet als stijlfout te lezen. **Het is geen grond om te concluderen dat de scène niets mist.** De langste regel is bovendien verbatim de verscheepte `BriefingText` uit `setup_story_missions.py`, dus het plafond van die scène ligt in **data** en niet bij de schrijver — nagerekend door de criticus, en het klopt.

**Het tegenbewijs staat in dezelfde missie en het is doorslaggevend.** `M1.3.S99` is dezelfde kamer, dezelfde vier sprekers, dezelfde schrijver, en haalt **29 woorden** op `.055` — daar op 01-08 toegevoegd omdat beat 2 geknepen werd. **De kamer produceert lengte zodra een beat erom vraagt.** S01 heeft precies één beat die erom vraagt, en dat is de beat die op mijn escalatie stond te wachten. De schrijver had dat zelf al gediagnosticeerd — *"Names carry length"* — en zijn diagnose was scherper dan zijn verdediging.

**Ruling: één nieuwe regel, `M1.3.S01.095`, DEX, tussen `.090` en `.100`.** Niets wordt hernummerd (het corpus gebruikt al `.045` en `.262`).

| Eis | Waarom |
|---|---|
| **≥20 woorden, één volle zin met bijzin** | dit is de regel die de drempel haalt, en hij haalt hem omdat de beat hem verdient |
| **Onderwerp: waarom de lijst plaatsen, aantallen en één leeftijd heeft en geen namen** | het antwoord op beat 2, zonder één naam te verzinnen |
| **Zijn langste regel in de scène én zijn enige volle zin** | §18.4 geeft Dex fragmenten. **De man die in fragmenten praat produceert één hele zin, één keer, over het enige dat hem bang maakt.** Machinaal controleerbaar |
| **Geen maxime, geen triade, geen benoemd gevoel** | een citeerbare zin maakt van zijn angst een uitspraak |
| **`.100` en `.110` blijven de knop** | daarom staat hij vóór `.100` |

**De begrenzing, en alleen L1 kan hem zien.** Dit is de **derde** gereserveerde grammaticale breuk in de cast: Mara's ene *"jij"* (AR-5), Petra's ene vráág (L1-R9), Dex' ene volle zin. Drie is een systeem, vier is de auteur (§18.9 D). **Geen vierde personage krijgt er een.** De drie liggen in verschillende grammaticale categorieën — voornaamwoord, taalhandeling, zinslengte — en in verschillende akten. Opgeschreven zodat de volgende schrijver die dit apparaat mooi vindt, weet dat het op is.

*Los meegenomen, beide gratis:* `M1.3.S01` r. 64 zei *"her four lines"* over Reyes en het zijn er **drie** (de samentrekkingsclaim zelf is met een regex over de missie bevestigd; alleen de telling was één te hoog). En M1.3 was de enige act-1-missie **zonder** `# eleven: L1-R14 checked`-notitie terwijl hij schoon is — één gebruik, `M1.3.S99.140`, bunks als maat voor Ember, precies de betekenis die L1-R14 beschermt. Notitie toegevoegd: **een ongeschreven controle is niet te onderscheiden van een niet-uitgevoerde.**

---

### L1-R50 — Vier `run.`-vlaggen zijn gedeclareerd, en de tabel krijgt de soort die eraan ontbrak

`run.m12_drops_bought`, `run.m14_leave_behind`, `run.hub_triage_order` en `run.m18_threx_probe` draaien in productie en stonden geen van vier in `SCRIPT_FORMAT` §4's feitentabel. **De validator klaagde niet en kon dat ook niet: hij vuurt op feiten zónder zetter, en alle vier worden door een `choice:`-blok gezet.** Dat is precies de vorm van L1-R29 — de praktijk en het document zijn uit elkaar gelopen op een plek waar geen enkele controle kijkt. De M1.2-schrijver heeft het in zijn eigen bestand geëscaleerd (*"it needs a row in the mission prep table and in SCRIPT_FORMAT 4's run-fact list"*) en had gelijk.

**Ruling: alle vier gedeclareerd, met hun waarden en hun leesplekken. En er komt een naam bij voor de derde soort.** De tabel kende *latch* en *objective*; deze vier zijn **een gesprekstak die de missie niet hoort te overleven**.

> **De regel, zodat niemand hem nog een keer afleidt: een spelerskeuze die verandert wat er *gezegd* wordt en niet wat er *meegedragen* wordt, is `run.`, altijd.** L1-R27 besliste dat al voor de hub; dezelfde redenering dekt een voorbereidingsaankoop, een wat-laten-we-achter-beslissing onder vuur, en hoe je een ondervrager door glas antwoordt. **Vraag: leest act 2 dit? Nee → `run.`**

Declareren verandert geen enkele controle-uitkomst (deze vier hebben zetters), dus het kost niets en het herstelt het enige dat er kapot aan was: dat de tabel de praktijk beschrijft.

---

### L1-R51 — §18.9 D krijgt de rij die de proloog blootlegde

**Aangenomen, en het voorstel is beter geformuleerd dan wat ik ervan zou hebben gemaakt.** Drie opeenvolgende kaarten glossen een eigennaam in exact dezelfde constructie: `.040` *"The Tithe of Hands — a labor lottery nobody wins."*, `.190` *"Hollow Point — a dead geothermal vault the Dominion forgot."*, `.200` *"Ember Cell: eleven people who still say no."* **Elke regel doet afzonderlijk twee dingen en overleeft de Verklaren-rij.** Het patroon is de tell, niet de regel — en een schrijver die zijn scène regel voor regel controleert kan het per constructie niet zien. Dat is dezelfde blinde vlek waar §18.9 B's Triaden- en Maximes-rij voor bestaan, één niveau hoger.

> | **Dezelfde retorische constructie draagt drie opeenvolgende beats** |

**Het is geen verbod, en dat moet erbij.** Een bewust herhaalde constructie is een refrein en dat is een van de oudste dingen die schrijven doet. Dit vangt de **onbedoelde** versie: drie beats op één vorm omdat de vorm werkte. **Drie is de grens; twee is niets.** En een recap of codex-vermelding mag de uitzondering krijgen — **op schrift, in het bestand**, want een vorm wiens hele taak is om snel eigennamen te glossen heeft een argument.

**De proloog krijgt die uitzondering, met de redenering erbij, en met een grens.** Nagerekend: de drie zijn in speelvolgorde **niet aaneengesloten** — `.040` is kaart 2 en `.190`/`.200` zijn kaart 6, vier kaarten uit elkaar, en alleen `.190`/`.200` lopen echt achter elkaar. **Dat zijn er twee, en twee is niets.** De eerlijke lezing is dus dat de scène **op** de grens staat en er niet overheen. Hij mag ook niet bewegen: niemand voegt een vierde gloss in deze constructie toe, en slot 3's kopij (Q-2) mag er niet in geschreven worden.

---

### L1-R52 — De aanrakingstabel is niet langer T4-gescoped, en hij krijgt eindelijk een definitie

**Aangenomen, en het is dezelfde fout als L1-R21 in een nieuwe jas.** Die ruling zei: *"de P4-tabel telt plants en niemand telde de aanrakingen; een tabel die het ene meet en waartegen het andere gecontroleerd wordt, is geen meting."* Ik heb toen de aanrakingsinventaris gemaakt en hem **alleen voor T4-wegverklaringen** gevuld — en daarmee dezelfde soort grens teruggezet die ik net had afgebroken.

**L1-R17's gevaar is VORM, niet welke wending een regel bedient.** Een speler die leert dat een net spreekwoordje betekent dat het spel iets verbergt, kijkt niet na uit welke tabel de regel kwam. `M1.8.S99.090` heeft exact de vorm van rij #4 en verklaart een **T3**-anomalie weg. Bleef de tabel T4-gescoped, dan telde niemand hem, en dan generaliseert de tell over de tabelgrens heen — precies de faalwijze die L1-R17 beschrijft.

**Ruling: de inventaris wordt twist-agnostisch en krijgt een `wending`-kolom. En hij krijgt een definitie, want acht rondes lang is "is dit een aanraking?" per geval opnieuw afgeleid.**

> **Een aanraking heeft drie delen, alle drie nodig:** er schiet iets tekort dat de speler zou kunnen opvallen · een personage verklaart het weg in een gesproken, verplichte regel · niemand spreekt het tegen.
>
> **Niet meegeteld:** de Dominion of AEGIS die zichzelf beschrijft (dat is plantmateriaal — L1-R44), een document of optionele terminal (AR-9), en een regel die de anomalie benoemt zónder hem te verklaren.

De tabel staat nu op tien rijen in speelvolgorde. **De rijnummers zijn hernummerd en zijn daarmee géén sleutel meer** — L1-R21 en L1-R40 citeren de oude 1–8. **Vanaf nu is het regel-ID de sleutel**, en dat is sowieso de enige verwijzing die niet veroudert.

---

### L1-R53 — L1-R40 mag nog niet poorten. De tegenwerping is weerlegd, de veegbeurt is niet gedaan

**De tegenwerping is dood en dat is winst.** Twee rondes vroegen of de maximes-rij niet gewoon de **vingerafdruk** meet in plaats van de slop — hij vuurde in M1.2 alleen op Mara en in M1.3 alleen op Vex, en dat is precies wat een vingerafdrukmeter zou doen. **De controleproef is gevonden en hij is goed:** Mara levert in `M1.8.S05` **nul** maximes terwijl haar vingerafdruk daar maximaal aanwezig is, en Sela in `M1.8.S99` hoogstens één — en zij is na Vex de meest maxime-gevoelige stem in de cast. **Een epigram-vormige stem kán dus naar nul geschreven worden.** De rij meet dichtheid van politoer, en dat is een schrijfkeuze. Aangenomen.

**Maar de poort gaat nog niet open, en mijn eigen ruling zegt waarom: *poort pas na meting*.**

| Gemeten | Uitkomst |
|---|---|
| M1.2 | zakt op **vier** scènes — waar ik er één voorspelde |
| M1.3 | één (Vex) |
| M1.4 | twee Mara-kandidaten in `S01`, twee in `S99` |
| M1.8 | schoon |
| **M1.1, M1.5, M1.6, M1.7, hub** | **ongemeten** |

**Die eerste rij is zelf een reden om te wachten.** Ik voorspelde één scène en het zijn er vier — dus mijn model van de verdeling klopt niet, en een regel die op een ongemeten corpus meteen als poort werkt, produceert bevindingen die niemand kan wegen. Dat is woordelijk het argument uit L1-R40 en het is nu gemeten in plaats van gevreesd.

**Ruling, in twee delen.**

**1. De rij blijft ongepoort tot M1.1, M1.5, M1.6, M1.7 en de hub gemeten zijn.** Ik heb geen shell en maximes zijn semantisch — een grep vindt ze niet. **Dit is dus een leestaak voor `dialogue-critic`, en hij hoort in dezelfde beurt als de scènes toch open gaan**, niet als aparte doorloop.

**2. En intussen is hij niet nutteloos: een bevinding op de maximes-rij wordt gemeld als `hold`, nooit als NO-GO, tot de veegbeurt sluit.** Dat is het verschil tussen een regel die informatie oplevert en een regel die werk blokkeert op een grond die nog niet gemeten is. **Een poort die sluit voordat je weet hoe vaak hij dicht gaat, meet niet — hij oordeelt.**

---

### L1-R54 — `HUB.A1.dex_workshop.070`'s *"she"* krijgt één woord, en de regressie is van mij

**Aangenomen, en de criticus heeft terecht geen §18.9-rij verzonnen: dit is één regel in één bestand, en het is een gevolg van een wijziging die ik heb opgelegd.**

`.070` — *"Bench comes off first. Room goes there with a door, and she stops treating people on the floor."* — is de zwaarste regel van het bestand, en zijn kracht hangt erop dat *"she"* Reyes is: hij geeft zijn eigen werkbank op voor iemand anders en zegt niet voor wie.

**Vóór Form A stond er geen vrouw op het toneel.** *"She"* had één mogelijke referent en de regel werkte op de lucht. Form A — **door mij opgelegd op 01-08** — zet Mara in de scène en laat haar zes regels eerder weglopen, dus de speler hoort *"she"* met een verse, verkeerde referent binnen bereik. Het corrigeert zichzelf binnen dezelfde bijzin (*"treating people"* is de medic en niemand anders), maar de correctie landt een tel te laat. **Een regel die je moet herlezen is niet privé, hij is verwarrend, en dat is niet hetzelfde.**

**Dit is precies de klasse die `BEATS_HUB_A1` §8.0 beschrijft: een wijziging om reeksredenen die een lezer van één bestand niet kan zien** — en hij is er dan ook doorheen gekomen tot de reeksronde, want alleen de reeksronde kon hem zien. De schrijver heeft niets fout gedaan; ik heb de grond verplaatst onder een regel die ik niet had herlezen.

**Wat níét mag veranderen, en dat is de reden dat het één woord is en geen herschrijving:** hij zegt nooit waaróm, hij noemt geen naam, de handen blijven stil, de ogen blijven op Voss, en de afsluiting maakt de regel privé.

**Wat de reparatie wél mag, en waarom een rolwoord geen verlies is: de regel identificeert haar al door functie.** In die vault is er precies één mens die mensen behandelt. *"She"* verborg dus nooit **wie** — het verbergt **waarom**, en een rolzelfstandignaamwoord kost niets van wat de regel werkelijk bewaarde. Dex laat lidwoorden vallen (`.040`, `.060`, `.070`, `.110`), dus een kaal rolwoord vóór het voornaamwoord past ín zijn vingerafdruk in plaats van ertegen.

**Eis: op het moment dat de speler het voornaamwoord hoort, moet de referent al eenduidig zijn.** Eén woord, in `.070`, vóór *"she"*. Geen naam, geen reden, geen reactie, geen tweede voornaamwoord. **Schrijversactie — ik schrijf het woord niet.** Tot het gebeurd is: `dialogue-critic` houdt **`.070`**, niet het bestand. Het twaalf-scèneverdict draait hier niet op.

---

### L1-R55 — *Acht* krijgt een rij, maar de rij beschermt *acht maanden* en niet het woord *acht*

Gemeten en gemeld: **acht** heeft in act 1 al vijf referenten — dagen (2×), meters, een rekindex, maanden (4×), mensen (2×). Per L1-R33 is dat een **stijl-lean** en geen defect: één woord dat overal iets anders meet levert geen tegenspraak op die de speler kan horen, en daar poort niemand op.

**Maar *"acht maanden"* draagt in vier M1.7-scènes één betekenis** — de ouderdom van het Veil-log, het getal dat de query ouder maakt dan alles erboven (P2-b, twist 2). En §7's eigen regel zegt: **een rij zodra een getal een tweede scène raakt.** Dus de rij hoort er te komen, en hij kost vandaag niets terwijl hij bij een vijfde gebruik niet meer aan te leggen zou zijn zonder iemands regel te breken.

**De valstrik zit in de kolom *"mag niets anders meten"*, en de melding wees hem correct aan:** zou die het kále woord beschermen, dan zakt `HUB.A1.reyes_triage.020` — *"Eight words a line"* — op een botsing die niet bestaat. Een woordtelling is geen duur.

> **Ruling: een register beschermt een getal *in zijn eenheid*, zodra de eenheid de betekenis draagt.** Dezelfde scheiding als L1-R31's *"nineteen doors"* tegenover *"the nineteenth day"*, en L1-R42's *veertig* (bevrijden) tegenover *eenenveertig* (instroom).

**En dat verklaart meteen waarom elf wél het kale woord claimt: de omvang van Ember Cell is een blóót aantal en heeft geen eenheid om zich mee te onderscheiden.** Elf is de **uitzondering**, niet het model — en L1-R14 is daarmee smaller dan hij las. Dat is winst: de regel die het duurst was om te handhaven blijkt maar op één getal te slaan.

---

### L1-R56 — Bij Sela wijzen de maximes-rij en §18.4 tegengesteld, en de veegbeurt mag niet starten voor dat is uitgesproken

**Dit is de belangrijkste van de drie en hij is precies op tijd gemeld.** §18.4 geeft Sela *"rhetorical structure even in private"* — **zij is de enige in de cast bij wie dat in de vingerafdruk staat.** L1-R53 stuurt straks een veegbeurt over vijf missies plus de hub, en `HUB.A1.sela_intake` draagt drie maximes uit één mond in één scène. **Zonder deze ruling zou de veeg als eerste het personage vlakken wiens stem de figuur ís.**

**Ze zijn niet met elkaar in strijd, want ze meten verschillende dingen.**

| | |
|---|---|
| **Structuur** — §18.4, bij haar ongelimiteerd | de **vorm** van de zin: antithese, parallellisme, ontkenning-dan-imperatief, een vraag die aan de kamer wordt teruggegeven, tweede persoon meervoud |
| **Maxime** — §18.9 B, bij iedereen gecapt | een **algemene waarheid**, schoon uit de kamer te tillen, gevormd om herhaald te worden |

**Een personage kan onophoudelijk retorisch zijn en nooit één algemene waarheid uitspreken — door retorisch te zijn over het ding dat vóór haar staat.** Dat is geen theorie; het is haar sterkste regel in act 1, en hij komt uit een ruling die ik al genomen heb. **L1-R34** liet `M1.6.S03.190` staan: *"No. So say it out loud if you're staying, and the rest of us can stop counting on you."* Ontkenning, imperatief, gevolg, tegen een wagon in de tweede persoon meervoud — **en nul algemene waarheid.** Het gaat volledig over déze mensen, in déze wagon, vannacht. Haar vingerafdruk staat op vol vermogen en de maximes-rij vuurt niet.

**Ruling: de cap geldt ook voor haar, en de veegbeurt stelt de smallere vraag.** Niet *hoeveel retorische regels heeft ze* (ongelimiteerd, en dat ís het personage), maar *hoeveel regels beweren iets dat het uittillen uit de kamer zou overleven* (één per scène, zoals bij iedereen).

**En de reparatie is bij haar geen afvlakking maar een aanscherping:** richt de figuur op het specifieke in plaats van op het algemene. Dat kost haar niets en het scherpt wat ze doet — **een organisator wiens macht is dat ze jóúw situatie benoemt, niet de menselijke conditie.** Doorgevoerd in §18.9 B onder de maximes-rij, want dat is waar de veger kijkt.

---

## Wat ronde 8 niet heeft opgelost, expliciet

- **Ik heb géén shell gehad, vierde L1-ronde op rij.** Ik heb `validate_script.py`, `check_ruling_premises.py` en `check_spoken_numbers.py` **niet** kunnen draaien. Wat ik wél gedaan heb: de parser en `check_branches()` gelezen om te bevestigen dat het `silence:`-blok door de bestaande `HEADER_MAPS`-tak loopt en de vier voorwaarden haalt — en de **coördinator heeft het daarna gemeten**, wat de bevindingen van **5 (4 REGISTER, 1 BRANCH)** naar **4 (alleen REGISTER)** bracht. **Dat is voor het eerst een L1-ronde waarvan de kernbewering gemeten is in plaats van afgeleid, en het verschil is dat iemand anders de shell had.** Dat vier rondes op rij blind gesloten hebben, hoort op de statuskaart en niet in een voetnoot; deze is half blind gesloten en dat is de eerste vooruitgang op dat punt.
- **Openstaand na deze ronde, allemaal schrijvers- of criticusacties:** `M1.8.S99.090` vlak (L1-R43) · `M1.3.S01.095` schrijven en `words:` bijtellen (L1-R49) · `dex_workshop.070` één woord (L1-R54) · de maximes-veegbeurt over vijf missies plus de hub, mét L1-R56 in de hand (L1-R53) · de herbeoordeling van `M1.4.S99` en `P0.S01`.
- **De maximes-veegbeurt heeft nu een vrijwaring die hij eerder niet had.** L1-R56 is er gekomen omdat de hub-criticus de botsing bij Sela **vóór** de veeg meldde in plaats van erna. Dat is het verschil tussen een regel die een personage scherpt en een regel die het afvlakt, en het is één ronde eerder gemeld dan het patroon van dit document zou voorspellen. Waard om op te merken, want dit log staat verder vol met het omgekeerde.
- **`silence:` met vlaggekwalificeerde sleutels is gespecificeerd en NIET actief** (L1-R41, staart). Toolwijziging vóór adoptie, zoals L1-R28.
- **De registerdrempel van `M1.4.S99`** (langste 19 tegen 20) staat nog steeds open uit ronde 5 en is deze ronde niet aangeraakt. L1-R49 laat zien hoe zo'n bevinding hoort te sluiten — via een beat die ondervoed is, niet via een twintigste woord — en of `M1.4.S99` zo'n beat heeft is niet onderzocht.
- **Nog open uit eerdere rondes:** L1-R28 (gespecificeerd, niet actief), `M1.5.S99.050`/`.060` in de `none`-tak, de rolsplitsing bij O-16, Embers barkpool van twee registers, en Q-4.
