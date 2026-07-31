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

**Wat dit doet met de ruling zelf: hij wordt dringender, niet zwakker.** L1-R17 zei *"twee is toeval, vier is een systeem"*. Bij drie is de drempel al gepasseerd voordat P4-c geschreven wordt. De criticus zette `M1.7.S03` mede op NO-GO omdat het de vierde zou zijn — **die grond is juist en blijft staan.**

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

**Kosten: nul.** `M1.4.S99` staat op `draft`, `critic: null`. Er is niets gegenereerd en niets te herbetalen.

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

**Wat er nu moet gebeuren:** `dialogue-critic` herscoort `M1.8.S05` — het `critic:`-veld staat nog op NO-GO en dat blijft zo tot hij dat doet; ik heb alleen de reden bijgewerkt naar de waarheid. De architect-actie is uitgevoerd; er is geen schrijversactie.

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

## Wat deze ronde niet heeft opgelost, expliciet

- **Ik heb `validate_script.py` niet kunnen draaien** in deze sessie (geen shell). Elke wijziging die ik in een scènebestand heb gemaakt is daarom beperkt tot commentaarblokken en één scalaire `turn:`-string, en ik heb de parser gelezen om te bevestigen dat beide categorieën door de bestaande tak lopen. **Wie hierna het eerst een shell heeft, draait `python Eclipse/Tools/validate_script.py --no-voice` en meldt het resultaat** — dat is een controle die ik verschuldigd ben, niet een formaliteit.
- **L1-R28 is gespecificeerd, niet actief.** De toolwijziging gaat vóór de adoptie.
- **`M1.5.S99.050`/`.060` in de `none`-tak** — gemeld in L1-R24, niet beslist, op de act-1-continuïteitsdoorloop.
- **`M1.4.S99`'s registerdrempel** (langste 19 tegen 20) — gemeld in L1-R22, niet beslist; scènevraag.
