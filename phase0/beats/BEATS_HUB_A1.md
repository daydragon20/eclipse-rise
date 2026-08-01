# BEAT-SHEET — HUB act 1 · Hollow Point
*L1 | story-architect | 2026-07-31 | twaalf gesprekken tussen de missies door*
*Canon: `19_voice_production.md` §19.2 tier 4 · `05_base_building.md` §5.2 · `18_writing_standard.md` §18.3 (hub-band 12–35 woorden), §18.6 (humor in downtime)*

---

## 1. Waarom deze laag apart bestaat

`19_voice_production.md` §19.2 zet act-1-hubgesprekken op **tier 4** (35.000 credits ≈ 5.800 woorden) en noemt de bezetting expliciet: *"Mara, Dex, Brick, Reyes at Hollow Point."* Ze worden dus later gegenereerd dan de missies — maar ze worden **nu** geschreven, want ze dragen twee dingen die de missies niet kunnen dragen:

1. **De personages worden gezelschap.** De speler brengt 25 uur door met deze mensen. Missiescènes hebben altijd haast; hubscènes niet. Dit is waar Dex grappig mag zijn zonder dat er iemand doodgaat, en waar Reyes de enige vraag stelt die ze in een missie nooit kan stellen.
2. **Eén dragende plant.** `HUB.A1.mara_letters` is de reden dat Mara's opnames in acts 2–4 niet uit de lucht komen vallen (§2.5). Als er één hubscène overleeft bij budgetkrapte, is het die.

**`21_quality_mandate.md` §21.2 is hier expliciet:** *"Elk companion krijgt reacties op wat er gebeurt, niet alleen op wat de plot vereist."* Twaalf scènes is de ondergrens, geen plafond. Waar een schrijver ruimte ziet voor een dertiende, is de dertiende welkom — mits hij zich verantwoordt (§21.3).

---

## 2. De regels van de hub-laag

| Regel | Waarom |
|---|---|
| **Type = `hub`, band 12–35 woorden** | De speler staat stil en heeft gekozen om te luisteren (§18.3) |
| **`credit_tier: 4`** | Na de missies. Bij krapte valt deze laag als eerste af — schrijf hem daarom zo dat hij zonder audio als tekst nog werkt |
| **Elke scène hangt aan een `condition`** | Een hubgesprek dat op het verkeerde moment beschikbaar is, is erger dan geen hubgesprek. Poort ze op `Story.Beat.*` |
| **Een keuze in een hubscène zet een `run.`-vlag. Nooit een `story.`-vlag** | Ruling **L1-R27**. Zie hieronder |
| **De poort wordt het scalaire kopveld `condition:`** | Ruling **L1-R28**. **Nog niet gebruiken** — zie hieronder |

### L1-R27 · hub-takken zijn `run.`

De twaalf scènes hebben elf lege *Zet*-kolommen in §5, en §6 zegt: *"Een speler die de hub overslaat, mist gezelschap — nooit begrip."* Een hubkeuze die de campagne verandert, spreekt dat tegen. De doorslaggevende reden is echter budgettair: **de hub-tier is de eerste die eruit valt en dat is één keer gebeurd (L1-R11).** Alles wat uit de hub persisteert, is dus een afhankelijkheid op een scène die misschien nooit klinkt — **de exacte faalvorm waar AR-9 voor bestaat.**

De enige uitzondering blijft `Story.Thread.MaraLetters_Open`, en die is een *aanwezigheids*draad die `M1.8.S91` **verrijkt** in plaats van vertakt. Dat is de vorm die een uitzondering moet hebben.

**Vorm, aangenomen als standaard** (precedent `M1.8.S06`, uitgevoerd in `HUB.A1.reyes_triage`): **vier keuze-opties in plaats van een vierassige variantenset.** Goedkoper op de as die telt (L1-R15) én beter drama — een variantenset deelt één antwoord, vier opties krijgen vier verschillende antwoorden.

**Denk je een `story.`-rij nodig te hebben, dan heb je óf een beat die in een missie thuishoort, óf een owner-vraag. Escaleren, niet toevoegen.**

### L1-R28 · de poort krijgt een kopveld — en je gebruikt het nog niet

Twaalf schrijvers hebben de poort als **commentaar** opgeschreven (`# condition: story.beat_m14_quartermaster == true`). Dat valideert schoon en betekent niets: geen tool leest het. Het veld wordt scalair, met exact de grammatica van het regelveld:

```yaml
condition:    'story.beat_m14_quartermaster == true'
```

> **NOG NIET GEBRUIKEN.** `SCENE_OPTIONAL` in `validate_script.py` kent het veld niet, dus een scène die het vandaag draagt zakt op SCHEMA. Het commentaar blijft staan tot de tool bij is. De toolwijziging gaat eerst.
| **De speler mag altijd weglopen** | Geen enkele hubscène mag informatie bevatten die nergens anders staat |
| **Eén komische beat per zes regels in downtime, nul in nasleep** (§18.6) | Na M1.8 is de hele hub nasleep. `HUB.A1.dex_after` is geen grappige scène |
| **Geen exposition** | Wie een hubscène gebruikt om de wereld uit te leggen, schrijft een codex-entry met een stem eraan (§18.8) |

---

## 3. De twaalf scènes

| # | Bestand | Na | Wie | Wat het draagt |
|---|---|---|---|---|
| 1 | `HUB.A1.dex_first_conversation` | M1.1 | Dex | zijn stem, zonder haast. **Bestandsnaam staat letterlijk in `SCRIPT_FORMAT` §1** |
| 2 | `HUB.A1.reyes_first_conversation` | M1.1 | Reyes | de vraag die ze in een missie nooit kan stellen |
| 3 | `HUB.A1.mara_map_table` | M1.2 | Mara | waarom zij nog gelooft. De enige keer dat ze over vroeger praat |
| 4 | `HUB.A1.dex_workshop` | M1.3 | Dex **+ Mara** | basisbouw als systeem-leraar (`05_base_building.md`). **Vorm A: begint midden in een ruzie** — §8.1 |
| 5 | `HUB.A1.brick_bunk` | M1.4 | Brick | de nieuwe man die niemand iets vraagt |
| 6 | `HUB.A1.reyes_triage` | M1.4 | Reyes | haar verleden, één laag opengetrokken |
| 7 | `HUB.A1.brick_the_tithe` | M1.6 | Brick | ~~AR-11~~ → **de weigering**. AR-11 is in M1.6 al twee keer betaald. **Vorm C** — §8.1, §8.4 |
| 8 | `HUB.A1.sela_intake` | M1.6 | Sela | politiek als praktijk, niet als toespraak |
| 9 | **`HUB.A1.mara_letters`** | M1.7 | Mara | **de dragende plant.** Zonder deze scène geen brieven in acts 2–4 |
| 10 | `HUB.A1.petra_return` | M1.8 | Petra | wat er terugkwam, en wat niet. **Vorm B: Voss zegt niets** — §8.1 |
| 11 | `HUB.A1.sela_after` | M1.8 | Sela | de eerste keer dat ze macht ziet in plaats van onrecht |
| 12 | `HUB.A1.dex_after` | M1.8 | Dex | de scène waarin hij geen grap maakt |

---

## 4. Per scène — de kern

### 1 · `dex_first_conversation` *(na M1.1)*
- **want** — Dex wants Voss to hand him back the rifle so he can see what the ambush did to it.
- **obstacle** — Voss wants to talk about what happened out there and Dex would rather talk about the rifle.
- **turn** — The rifle turns out to be fine, which leaves them with nothing to look at, and Dex says the one honest thing he says in act 1.
- Dit is de scène waar de speler leert dat Dex' grappen een deur zijn. Eén regel achter die deur, dan gaat hij weer dicht.

### 2 · `reyes_first_conversation` *(na M1.1)*
- **want** — Reyes wants to know what Voss did with the wounded conscript.
- **obstacle** — She is not asking as an accusation and Voss cannot tell that yet.
- **turn** — She writes his answer down, and explains that she keeps records because someone should.
- **Drie takken op `story.m11_conscript_choice`** — bladeren `finished` · `left` · `bound`, en geen van de drie is een oordeel. **Alle drie moeten afgehandeld worden**; twee van drie is de L1-R12-vorm.
  > **Gecorrigeerd 01-08, ruling L1-R29.** Hier stond `Story.Choice.M11_ConscriptSpared` — een vlag die **mijn eigen L1-R3 heeft ingetrokken** en die niets zet. De schrijver zag het, repareerde het niet stil, en las in plaats daarvan de drie bladeren die `M1.1.S05.170/.180/.190` werkelijk zetten. Hij had gelijk; dit beat-sheet had ongelijk. **Uitbreiding van de L1-R12-regel: een beat-sheet telt als lezer van een vlag**, want het wordt gelezen door een mens die daarna een `condition` typt.
  >
  > **Erratum op L1-R29 zelf, 01-08.** De ruling zegt dat de schrijver *"één woord corrigeert in zijn eigen **L2**-notitie"*. Dat klopt niet: de ingetrokken vlagnaam stond in het **L1**-blok van `HUB.A1.reyes_first_conversation` (r. 25), dus in **mijn** blok, en de L2-notitie beschrijft de correctie alleen. Een schrijver die de ruling letterlijk volgt, kijkt in het verkeerde blok en vindt niets. **De tekst in het bestand is hier gecorrigeerd; de rulingtekst moet nog één woord mee** — zie §8.5 (a). Dit is exact de fout die L1-R29 zelf beschrijft, één laag hoger.
- Dit is de eerste keer dat het spel de speler laat merken dat er iemand meetelt (§2.5: *"Holds the player accountable for casualty-heavy tactics"*).

### 3 · `mara_map_table` *(na M1.2)*
- **want** — Voss wants to know how long Mara has been doing this.
- **obstacle** — She answers questions about herself by answering a different question.
- **turn** — She gives him a number of years, and then a number of people, and the second number is smaller than he expected.
- **De enige scène in de act waarin Mara over het verleden praat.** Maximaal veertien regels. Zij zegt nog steeds "wij".

### 4 · `dex_workshop` *(na M1.3)* — **VORM A: begint midden in een ruzie**

> **Herzien 01-08 (§8.1).** De oude `obstacle:` was *"Nobody has a week and there is no steel"* — een tekort, geen tegenstander. **Een tekort kan niet terugpraten**, en dat is de reden dat deze scène dezelfde motor kreeg als de andere elf: Voss komt binnen, Dex praat, Voss vraagt door. §18.9 D vraagt om iemand die *ongeïnteressant ongelijk* heeft. Die persoon bestaat al in de canon en het kost geen woord om haar binnen te halen.

- **want** — Dex wants Mara to look at the whole plan before she rules on one piece of it.
- **obstacle** — Mara will not plan past tomorrow. She says so, and she leaves.
- **turn** — He finishes showing it to the man who is still standing there, and the plan is far too big, and it is the first blueprint of Hollow Point as the player will eventually know it.

- **De ruzie is al canon en wordt niet uitgevonden.** `mara_map_table.110`: *"The mast is not down yet."* — zij plant niet voorbij morgen, en dat is de vloer waarop haar ommekeer in M1.7 staat. Dex heeft zeven rantsoenkaarten van een basis die vier acts kost. Dat is een echte botsing tussen twee mensen die allebei gelijk hebben.
- **Mara krijgt maximaal drie regels en ze wint niet door gelijk te hebben — ze wint door weg te lopen.** AR-5/AR-5b gelden onverkort: geen "you", geen "your", geen "I" over de zaak. Zij praat over de kaart, niet over hem.
- **De ruzie gaat over één kaart, nooit over het plan.** Dex legt zijn plan niet ter stemming voor; dat is precies wat hij nooit doet. Hij vraagt haar naar één ding te kijken en ze weigert het hele stapeltje.
- **De scène opent op de tweede helft van een zin.** Niet op een vraag, niet op een begroeting, niet op "Stand where the lamp is."
- **Systeem-leraar-scène blijft volledig intact** (`05_base_building.md`: je leert basisbouw van de persoon die ervan houdt). Sloten, energie, wat een Workshop doet — dezelfde vier klachten en twee wensen, dezelfde zeven kaarten. Alleen het motief eronder verandert: hij maakt af waar hij aan begonnen was voor de vrouw die wegliep, tegen de man die bleef.
- **Woordneutraal.** Wat Mara krijgt, levert Dex in. Geen nieuwe locatie, geen nieuwe stem (`mara` is gecast en staat in `19_voice_production.md` §19.2 in de hub-bezetting).

### 5 · `brick_bunk` *(na M1.4)*
- **want** — Voss wants to know if Brick needs anything.
- **obstacle** — Brick needs nothing and says so in one word.
- **turn** — He asks Voss for the names of the people who died before he arrived, and writes them somewhere, and that is the entire conversation.
- **Zeven regels maximaal.** Dit is de scène waarin de speler begrijpt wat Brick dóét, drie missies voordat hij de muur last.

### 6 · `reyes_triage` *(na M1.4)*
- **want** — Reyes wants Voss to approve a standing order about who gets treated first.
- **obstacle** — Any order she writes is a version of the order she once refused to follow.
- **turn** — She makes him choose, and then tells him what the Dominion's version of that order was.
- **Haar canon-achtergrond** (§2.5: *"Defected after ordered triage-by-loyalty-score"*) landt hier voor het eerst — en alleen omdat Voss zelf net iets vergelijkbaars heeft moeten tekenen.
- **Uitzondering §18.9 A:** Reyes mag klinisch een emotie benoemen. Als er ergens één zo'n regel in act 1 staat, staat hij hier.
  > **Uitgevoerd en vastgelegd 01-08.** De regel is **`reyes_triage.170`** — *"I am frightened of that plate. That is not a useful thing to be and it will pass."* De schrijver las §18.9 A correct: de uitzondering kent **twee** vrijstellingen die met "or" verbonden zijn, en hij gebruikte de eerste (Reyes, klinisch), niet de tweede (AR-12's gerantsoeneerde `direct_beat`). **Dat is juist en het blijft zo.** `direct_beat: true` zou act 1 een tweede rantsoen geven of `M1.8.S90.050` heropenen, en beide zijn fout.
  >
  > **Dit is de enige klinische emotiebenoeming van Reyes in act 1. Een tweede in act 1 is een NO-GO die ik zelf uitschrijf.** Of acts 2–4 een plafond per act krijgen is een **owner-vraag**: §18.9 A's uitzondering is staand en ongerantsoeneerd, en zodra er twee zijn is het geen uitzondering meer maar een register. Niet opnieuw afleiden — hier staat het.
- **De sluiting wijkt af** — zie §8.2. Zij gaat níét terug naar de kit.

### 7 · `brick_the_tithe` *(na M1.6)* — **VORM C: hij weigert te praten, en de scène gaat daarover**

> **Volledig herzien 01-08. Zie §8.4 voor de canon-uitspraak.** De oude `want:` was al beantwoord voordat de poort openging en de oude `turn:` sprak drie plaatsen in het corpus tegen. **Beide fouten waren van mij, niet van de schrijver.**

- **want** — Voss wants Brick to say what the seventeen days were.
- **obstacle** — Brick will not say. He does not deflect and he does not name anybody; he simply does not answer, twice.
- **turn** — Voss stops asking. On his way out he sees what Brick has been doing for the forty-one all night, and it is the sentence Brick said at the map table, done instead of said.

- **Brick IS meegenomen.** `M1.6.S03.220` (*"Krad-9. Seventeen days."*), `M1.6.S01` en `M1.8.S02` zeggen het alle drie. Niets in deze scène mag suggereren dat iemand anders in zijn plaats ging. **Zie §8.4.**
- **De tic wordt niet gebruikt en niet uitgegeven.** Hij noemt geen dode. De M1.8-schrijver geeft het middel voorgoed uit in `M1.8.S90.050`; deze scène raakt dat niet, want er valt hier geen naam.
- **De betaling is zijn eigen regel.** `M1.6.S01.140` — *"They water them at the yard. Not before."* Het enige dat hij ooit ongevraagd zei. Nu liggen er eenenveertig mensen uit zo'n wagon op de vloer van dit gewelf en draagt hij water rond, en heeft dat de hele nacht gedaan. **Niemand legt het verband. Niemand zegt het woord Tithe.** De speler heeft de zin vier scènes geleden gehoord en herkent hem of niet.
- **Vier regels, niet vijf.** Brick krijgt er twee en geen van beide is een antwoord. Er komt geen tweede beat na de weigering.
- **De sluiting bevat geen van beide sprekers** — zie §8.2.
- **AR-11 is hier niet meer aan de orde.** Die is in M1.6 twee keer volledig betaald (`S01.140` eerste betaling, `S03.220` landing). Deze scène was in het oude beat-sheet als derde betaling ingeboekt en had daarom niets te doen; dat is de oorzaak van de hele fout.

### 8 · `sela_intake` *(na M1.6)*
- **want** — Sela wants Voss to sign off on how the forty newcomers get fed, housed and asked.
- **obstacle** — Ember has never had a rule about anything, and rules are how movements become the thing they fought.
- **turn** — They write the first rule of the movement together, and it is about food, and it is short.
- Politiek als **praktijk**. Sela houdt geen betoog (AR-7); ze schrijft een regel op een stuk plaat. Later blijkt dat de eerste zin van het Free Vantara Concord.

### 9 · `mara_letters` *(na M1.7)* — **DRAGEND**
- **want** — Voss wants to ask Mara whether K-77 is a mistake.
- **obstacle** — He finds her recording something, alone, and she shuts it off when he comes in.
- **turn** — She tells him what it is in one sentence, deflects the second question, and asks him to leave the light on.

> **Dit is de plant voor `M1.8.S91` en voor Mara's opnames in acts 2–4 (§2.5).** Zonder deze scène duiken die brieven in act 2 uit het niets op.
>
> **Regels:** Mara legt niet uit waarom ze ze maakt. Voss vraagt niet door. De speler ziet één seconde van een scherm met **elf** bestanden erop en het aantal wordt niet genoemd. Maximaal negen regels.
>
> **`Story.Thread.MaraLetters_Open`.** M1.8.S91 leest die vlag en voegt een voorwaardelijk regelpaar toe voor de speler die hier was.
>
> **Als er bij budgetkrapte één hubscène overleeft, is het deze.** Meld dat expliciet aan `voice-director`.

### 10 · `petra_return` *(na M1.8)* — **VORM B: Voss zegt niets**

> **Herzien 01-08 (§8.1).** Dit is de enige scène van de twaalf waar zwijgen kárakter is en niet een ontbrekende regel — en het is toevallig ook de enige scène die §18.7's spelersinput miste. Eén ingreep repareert beide.

- **want** — Voss wants his aunt to rest, and he does not manage to say it.
- **obstacle** — She has already found the kitchen, the water line and three people who are not being fed, and she does not need an answer from anybody.
- **turn** — He picks up the spoon. Nobody says anything about it, and that is how they both survive the week.

- **Voss heeft nul `text:`-regels in dit bestand.** Hij is de hele scène in beeld. Elke Petra-regel wordt beantwoord met een beweging, en die beweging staat in `shot:` — zeven beurten, waarvan er zes lichamelijk zijn. `shot:` kost niets.
- **Haar laatste inhoudelijke regel beantwoordt een vraag die hij niet stelt.** *"Until it stops smelling like water."* is nu het antwoord op iets wat hij nog niet gevraagd heeft. Dat is haar vingerafdruk op volle sterkte (§18.4: ze beantwoordt de vraag die ze niet wil beantwoorden met een taak) en het is de enige plek in act 1 waar iemand Voss kent zonder hem te laten praten.
- **De verplichte onderbreking uit §18.7 vervalt hier, en alleen hier.** Een onderbreking heeft twee sprekende partijen nodig. Zie §8.1, vorm B.
- **Petra bedankt niet en klaagt niet.** Zij vult de leegte die Mara achterliet met **werk**, niet met troost, en dat is precies waarom het werkt.
- **De sluiting blijft bij Voss, niet bij de pot** — zie §8.2.
- **Dit levert credits óp.** Vijf Voss-regels verdwijnen, en Voss-regels zijn de enige die voor gender verdubbeld worden.

### 11 · `sela_after` *(na M1.8)*
- **want** — Sela wants to know what "Cinder" is going to mean, since it now means something.
- **obstacle** — Voss did not choose the name and cannot control what people put in it.
- **turn** — She tells him what the district is already saying about him, and it is not true, and she advises him not to correct it.
- **Haar eerste compromis** (§2.5: *"learns power's compromises"*). Klein, verdedigbaar, en het is het zaadje van alles wat ze in acts 3–4 moet slikken.

### 12 · `dex_after` *(na M1.8)*
- **want** — Dex wants to finish the repair he started before K-77.
- **obstacle** — It was Mara's and it did not need repairing.
- **turn** — He works on it anyway, and when Voss offers to help he says yes, which he has never done.
- **Nul grappen.** §18.6: geen komische beat in nasleep. Maximaal acht regels, en drie ervan zijn gereedschapsnamen.

---

## 5. Vlaggen

| Scène | Leest | Zet |
|---|---|---|
| 1 | `Story.Beat.M11_ThirteenBullets` | — |
| 2 | `Story.Beat.M11_ThirteenBullets`, **`Story.Choice.M11_Conscript.{Finished,Left,Bound}`** | — |
| 3 | `Story.Beat.M12_DeadDrop` | — |
| 4 | `Story.Beat.M13_SignalFire` | — |
| 5, 6 | `Story.Beat.M14_Quartermaster`, `Story.Beat.BrickRecruited` | — |
| 7, 8 | `Story.Beat.M16_TitheTrain`, `Story.Char.SelaMet`, `Story.Choice.M16_LettersAllowed` | — |
| **9** | `Story.Beat.M17_UnderTheIce` | **`Story.Thread.MaraLetters_Open`** |
| 10, 11, 12 | `Story.Beat.Act1Complete`, `Story.Char.MaraDead`, `Story.Char.PetraRescued`, `Story.Char.CinderNamed` | — |

---

## 6. Wat de hub-laag **niet** doet

- **Geen wendingen planten.** Alle vijf de wendingen worden in de missies geplant (AR-9: verplichte dragers). Een speler die de hub overslaat, mist gezelschap — nooit begrip.
- **Geen enkele hubscène is verplicht.** Behalve mechanisch: `mara_letters` verrijkt M1.8.S91, maar M1.8.S91 werkt ook zonder.
- **Geen nieuwe personages.** De hub is de bestaande cast, langzamer.
- **Geen orations** (AR-7).

---

## 7. Instructies voor de dialogue-writer

1. **Twaalf scènes, twaalf verschillende gespreksvormen.** Als er twee zijn die allebei "Voss vraagt X, personage antwoordt Y" zijn, is er één te veel. Laat er één beginnen midden in een ruzie, één zonder dat Voss iets zegt, één waarin het personage weigert te praten en de scène daarover gaat.
   > **Deze instructie is in de eerste ronde niet uitgevoerd, en dat was voorspelbaar: geen van beide schrijvers zag meer dan zijn eigen zes bestanden. Ze is nu niet langer een instructie maar een toewijzing — §8.1 zegt welke scène welke vorm draagt.** Een vorm die aan niemand is toegewezen, wordt door niemand gebouwd.
2. **De hub-band is 12–35 woorden per regel**, maar de variantieregel (§18.3: factor 3) geldt ook hier. Bricks scènes zitten structureel onderaan de band; dat is zijn karakter en het is toegestaan zolang iemand anders in dezelfde scène bovenaan zit.
3. **Elke scène heeft een `condition`.** Ongepoorte hubdialoog is de snelste manier om een game dom te laten klinken.
4. **Na M1.8 is alles nasleep.** Nul grappen in scènes 10–12.
5. **`mara_letters` heeft voorrang** boven alle andere hubscènes, bij het schrijven en bij het genereren.
6. **Voss-varianten:** alleen in 2, 6, 8 en 11 — de scènes waarin hij iets tekent, kiest of toegeeft. Elders neutraal. Zie C-6/Q-7.
7. **Lees §8 vóór je één regel aanraakt.** Daar staat wat er per scène veranderd moet worden en wat er met rust gelaten moet worden.

---

## 8. DE REEKS — vonnis en herstelopdracht *(story-architect, 01-08)*

### 8.0 Waarom dit L1 is

Elf van de twaalf scènes halen §18.9 individueel. **De reeks zakt.** Dat is geen schrijversfout: een schrijver ziet zes bestanden, ik zie twaalf. De drie gronden staan hieronder in de volgorde waarin ze mij aanrekenbaar zijn.

1. **§7.1 was een instructie zonder adres.** Drie vormen geëist, nul gebouwd. Een vorm die aan geen enkele scène is toegewezen, wordt door geen enkele schrijver gebouwd — en beide schrijvers hadden gelijk dat hún scène de vorm niet kon dragen, want ze konden niet weten welke wel.
2. **Negen van twaalf delen één motor** (Voss benadert, personage buigt af, de scène draait op de vraag of de afbuiging standhoudt). De *functies* variëren wél — Voss wordt gevraagd, kiest, geeft, wordt geweigerd — maar de **vorm** is twaalf keer dezelfde. Dat is de reden dat het per scène onzichtbaar is.
3. **Elf van twaalf sluiten op dezelfde beweging**, en negen dragen *"does not look up"* / *"goes back to"* op de zwaarste regel. Bij **beide** schrijvers, onafhankelijk. Dat staat in `shot:` en kost dus nul credits — maar het is het instructieblad voor de cinematic pass, en dat is straks twaalf keer hetzelfde beeld.

De schrijver van `sela_after` schreef het zelf op bij `.150`: *"She goes back to work inside one line, **which is how everybody in this game ends a conversation they cannot finish.**"* Hij bedoelde het als bewijs dat het klopte. **Het is de diagnose, en het is de eerlijkste regel in de hele oplevering.**

> **Alles wat hieronder staat is woordneutraal of goedkoper.** Zie §8.7. Van vorm veranderen kost geen woord.

---

### 8.1 De drie vormen, toegewezen

De toewijzing is een oordeel over de akte: **welk personage kan weigeren te praten zonder zijn arc te breken, en in welk gesprek is een zwijgende Voss geloofwaardig in plaats van ontbrekend.** Dat is niet uit één missie af te leiden. Daarom staat het hier en niet in een schrijversbriefing.

| Vorm | Scène | Waarom deze en geen andere |
|---|---|---|
| **A · begint midden in een ruzie** | **4 · `dex_workshop`** | Een ruzie heeft twee mensen nodig die allebei gelijk hebben. Dex' plan duurt vier acts; Mara plant niet voorbij morgen (`mara_map_table.110`). Die botsing bestáát al, is nergens uitgespeeld, en Mara is gecast en hoort in de hub-bezetting. |
| **B · Voss zegt niets** | **10 · `petra_return`** | De enige scène waarin zwijgen van Voss *karakter* is en geen ontbrekende regel — hij heeft haar niets te zeggen, en dat is de scène. Petra heeft per definitie geen antwoord nodig (§18.4: nooit "you", nooit dank, altijd een taak). |
| **C · weigert te praten, en de scène gáát daarover** | **7 · `brick_the_tithe`** | Brick is de enige die kan weigeren zonder dat het als karigheid leest, en dit is de enige scène waarin een weigering *inhoud* is: de speler wíl hier iets weten. Zie §8.4 — deze scène moest toch al herbouwd worden. |

**Wat NIET van vorm verandert, en waarom dat een besluit is en geen verzuim:**

- **`mara_letters`** — dragend (§4.9). Raak niets aan. `.040` is de sterkste regel van de twaalf en het bewijs dat dit oplosbaar is zonder budget.
- **`sela_intake`** — de beste scène van de twaalf op §18.7 en de enige met een echt gezamenlijk schrijfproces in beeld. Een ruzievorm zou Sela's overwinning-zonder-argument slopen.
- **`brick_bunk`** — de omkering (hij *vraagt* om namen) is echt en betaalt `M1.8.S90.020` voor nul woorden. Dit is het model, niet het probleem.
- **`reyes_first_conversation`** — de sluiting wijkt al af: de scène eindigt op **haar vraag aan hem**, waardoor blijkt dat hij voor iets anders kwam. **Niet normaliseren.** Dit is de enige van de twaalf die niet op een taak-over-een-voorwerp eindigt.

#### Vorm A — de opdracht bij `dex_workshop`

De volledige stub-opdracht staat in §4.4. Kern: **de scène opent op de tweede helft van een zin**, Mara krijgt maximaal drie regels en wint door weg te lopen in plaats van door gelijk te hebben, de ruzie gaat over **één** kaart, en de zeven kaarten / vier klachten / twee wensen blijven ongewijzigd. Wat Mara krijgt, levert Dex in.

#### Vorm B — de opdracht bij `petra_return`

De volledige stub-opdracht staat in §4.10. Kern: **Voss heeft nul `text:`-regels**, hij is de hele scène in beeld, en elke Petra-regel wordt beantwoord met een beweging in `shot:`.

> **Uitspraak: een scène met één sprekende partij is vrijgesteld van §18.7's onderbrekingseis.** Een onderbreking veronderstelt twee sprekende partijen; de eis kan hier niet gehaald worden en het niet halen is geen defect. In ruil is de §18.7-spelersinput hier **verplicht** en het is een bewegingsbeat: de lepel. Dat repareert tegelijk het feit dat `petra_return` de enige echt passieve scène van de twaalf was — elf beurten, geen `choice:`, geen variant, geen gemarkeerde beat. De lepel op `.100` was al een beweging; hij was alleen niet als spelersbeat geschreven.

#### Vorm C — de opdracht bij `brick_the_tithe`

Zie §4.7 en §8.4.

---

### 8.2 De sluitingen — welke wijken, en hoe

Twee regels, allebei volledig in `shot:` en dus **nul credits**.

**(i) De stilstand wordt gerantsoeneerd.** *"does not look up"* / *"does not stop working"* / *"goes back to X"* mag op de **zwaarste regel van de scène** in ten hoogste **vijf** van de twaalf staan. Dit zijn ze, en dit is waarom:

| Scène | Reden dat de stilstand daar hoort |
|---|---|
| `brick_bunk` | Het is zijn vingerafdruk, en de `shot:` betaalt `M1.8.S90.020` |
| `brick_the_tithe` | De weigering **is** het niet-opkijken. De vorm en het beeld vallen samen |
| `mara_letters` | `M1.8.S91.140` hangt eraan: het scherm staat weggedraaid en de lamp is uit |
| `dex_after` | De hele scène is niet-opkijken; de "Yes." kost hem juist daarom iets |
| `mara_map_table` | Het tweede getal is één, en de stub eist dat niemand hardop reageert. **De stilstand staat hier aan Voss' kant** — dat staat er al ("Voss does not move") en dat blijft |

**In de andere zeven wordt de zwaarste regel afgeleverd met de handen stil óf met de ogen op Voss, en de `shot:` zegt welke van de twee.** Dat is één zin per bestand.

> Ik verbied het middel niet overal, en dat is met opzet: een architect die een middel volledig schrapt, maakt een nieuwe uniformiteit. Vijf is genoeg om het als handschrift te laten lezen en weinig genoeg om het te laten opvallen.

**(ii) Drie sluitingen wijken van "taak over een voorwerp, spreker terug aan het werk".**

| Scène | Nieuwe sluiting | Wat het kost |
|---|---|---|
| **`reyes_triage`** | Zij gáát niet terug naar de kit. Ze heeft de punch weggegeven en heeft niets meer om handen — de eerste keer in act 1 dat de speler deze vrouw werkloos ziet. Het is ook de enige eerlijke afloop: teruggaan naar de kit maakt de overdracht ongedaan | één `shot:`-zin, nul woorden |
| **`dex_workshop`** | Het plan wordt **bedekt**. Er loopt iemand door de werkplaats naar de bunks — een naamloze schutter, **geen regel** — en Dex veegt de zeven kaarten zijn kit in. Hij heeft dit aan één persoon laten zien | één `shot:`-zin, nul woorden, nul stemmen |
| **`petra_return`** | De camera blijft bij **Voss**, niet bij de pot. Zij is met de kommen weg vóór de laatste beat; vasthouden op hem bij de brander, met opgerolde mouwen | één `shot:`-zin, nul woorden |

En **`brick_the_tithe`** wijkt door constructie: het laatste beeld bevat **geen van beide sprekers**. Het bevat het water en de eenenveertig.

**"Geen afscheid" blijft** — dat is §18.7 en het is goed. Maar geen afscheid is niet hetzelfde als geen einde: na (i) en (ii) eindigen vier van de twaalf met iemand die het beeld verlaat, en het is niet elke keer Voss.

---

### 8.3 Voss — van functie naar as

**De diagnose is scherper dan "hij is vlak".** Elke niet-toewijsbare regel in de striptest is van hem: *"That's not what I asked."* · *"Who was it?"* · *"What am I looking at?"* · *"What order?"* · *"For how long?"* · *"That's not what happened."* §18.4 geeft hem twee vaste eigenschappen en **allebei worden ze bediend** — maar alleen de tweede (*claimt nooit krediet voor het werk van anderen*) wordt als *gedrag* gebruikt. De eerste (*technicusvocabulaire: ziet systemen, benoemt mechanismen*) wordt als **woordkeus** gebruikt in plaats van als handeling, op één regel na.

Die ene regel is `mara_letters.040`:

> *"One door, four beds, three days. Somebody in that room should have said out loud whether that adds up."*

Dat is de K-77-vraag als rekensom, opgebouwd uit Dex' deur, Reyes' bedden en Mara's dagen. **Er zijn er twee zoals die in twaalf bestanden.** Dit is geen nieuwe eigenschap die ik toevoeg — het is de eigenschap die er al staat, hoorbaar gemaakt.

**De regel, en hij is met de hand te controleren:**

> **Een Voss-regel in de hub die zonder verlies vervangen kan worden door "Go on", is geen Voss-regel.** Herbouwen als een meting van het ding dat voor hem staat, of schrappen.

**Toegepast, regel voor regel:**

| Regel | Vonnis |
|---|---|
| `dex_workshop.020` *"What am I looking at?"* | **Herbouwen.** Er ligt een rantsoenkaart op een bank; hij telt of meet hem. Woordneutraal tot +6 |
| `reyes_triage.020` *"What order?"* | **Herbouwen.** Er staat een plaat tegen een krat met niets erop; hij benoemt de plaat, niet de vraag. Woordneutraal tot +6 |
| `mara_map_table.020` *"That's not what I asked."* | **Blijft.** Het is de derde poging na twee afbuigingen en daarmee verdiend. Dit is de regel die de speler onthoudt |
| `mara_map_table.080` *"Who was it?"* | **Blijft.** Vier woorden, en het levert een echte weigering op |
| `sela_after.130` *"For how long?"* | **Blijft.** Het is een duur, dus een meting, en de vier varianten dragen het karakter |
| `brick_bunk.050` *"Before you got here?"* | **Blijft.** De inhoud van de eerste helft van die scène is dat Voss te veel praat |
| `brick_the_tithe.030` *"That isn't what I asked."* | **Vervalt met de herbouw** (§8.4) |
| `petra_return` — alle vijf | **Vervallen** (vorm B) |

> **De dubbele regel lost zichzelf op.** *"That isn't what I asked."* (`brick_the_tithe.030`) en *"That's not what I asked."* (`mara_map_table.020`) komen in het hele corpus van 71 bestanden verder nergens voor. `mara_map_table` gaat voor — hij komt eerder en is daar verdiend. Omdat `brick_the_tithe` toch herbouwd wordt, is er **geen uitzonderingslijst nodig**; de botsing verdwijnt.

Na de reparatie houdt Voss ongeveer **38** regels over de twaalf scènes in plaats van ~45, en drie ervan rekenen.

---

### 8.4 `brick_the_tithe` — de canon-uitspraak

#### De uitspraak

> **Brick is zélf meegenomen. De Tithe heeft hém van Krad-9 gehaald. Niemand ging in zijn plaats.**

Dit is geen smaakvraag en het is niet aan een schrijver. Het corpus zegt het drie keer, en de eerste is een **gesproken** regel die de speler al gehoord heeft voordat deze scène überhaupt beschikbaar is (de poort is `story.beat_m16_tithetrain == true`):

| Bron | Wat er staat |
|---|---|
| `M1.6.S03.220` | BRICK: *"Krad-9. Seventeen days."* — noot: *"AR-11 lands."* Zeventien dagen in een wagon |
| `M1.6.S01` r. 31 | *"AR-11 pays here for the first time — de Tithe nam Brick van Krad-9."* |
| `M1.8.S02` r. 48 | *"Brick — the man the Tithe shipped off Krad-9 in a box — says NOTHING here"* |

`HUB.A1.brick_the_tithe.040` — **BRICK: *"Tam went in my place."*** — spreekt dat alle drie tegen.

**De bron van de fout is mijn `turn:`-veld**, niet de schrijver: *"He says the name of the man who was taken off Krad-9 in his place."* De schrijver heeft dat loyaal uitgevoerd, geen naam verzonnen, en de botsing niet kunnen zien omdat de tegenspraak in drie andere bestanden staat. Zelfde familie als L1-R18 en L1-R23.

#### Wat er over Tam vastligt, en wat niet

Vastgelegd zodat niemand het opnieuw hoeft te zoeken:

- Tam is **dood** (`M1.7.S01.210`, noot: *"a dead man's name"*)
- **iemand is bij hem gebleven** (`M1.7.S01.210` — *"Somebody stayed with Tam."*; wie, staat nergens en blijft nergens staan)
- hij is één van de drie namen die Brick **drie maanden lang telde in een holding block** op Kessara (`M1.4.S99.150`–`.180`: *"Three months in a holding block. What was there to do with the time?"* / *"Counted."* / *"Tam. Oyelaran. Vic."*)

**Niets daarvan sluit een Krad-9-bodem uit — maar *"in my place"* wel**, want dat maakt Brick tot iemand die níét is meegenomen, en dat is hij. **De reparatie is niet: een andere naam. De reparatie is: geen naam.**

#### Waarom de scène toch al niet werkte

Twee dingen die het vonnis mede dragen en die met de canon-fout dezelfde oorzaak hebben:

1. **De `want:` was al beantwoord voordat de scène speelde.** *"Voss wants to know why Brick spoke first at the briefing"* — en `M1.6.S03.220`'s eigen noot zegt: *"The player now knows why he spoke first in S01."* De scène duwde tegen een open deur, in het bijzijn van de man die de deur al open had zien gaan.
2. **De omkering werkt hier niet, en in de andere Brick-scène wél.** `brick_bunk.040` (*"The ones before me."*) is een echte omkering — hij *vraagt* om namen, ander werkwoord, en de `shot:` betaalt `M1.8.S90.020` voor nul woorden. Maar `brick_the_tithe.040` had dezelfde beweging, lengte en niet-reactie als `M1.4.S03.040`, `M1.7.S01.210` en `M1.8.S01.120`, en de `shot:` was bijna woordelijk die van `M1.7.S01.210`. **De M1.8-schrijver stelt dat het middel in `M1.8.S90` op is; dit zou de zesde uitgave zijn geweest.**

#### De afgeleide regel, en die geldt voor alle 42 missies

> **Een hubscène die op beat X gepoort staat, mag niet willen wat beat X geleverd heeft — en de toets is de *hoorbaarheid*, niet de speler.** Een `want:` faalt als hij beantwoord is **in het bijzijn van het personage dat hem heeft**. Dat `reyes_first_conversation` iets wil dat de speler allang weet, is geen probleem: **Reyes** was er niet bij. Dat Voss iets wil dat in zijn eigen aanwezigheid beantwoord is, is dat wel.

Nageteld tegen alle twaalf: **één faalt**, en dat is deze. De andere elf houden stand.

#### De herbouw

Zie §4.7 voor de volledige stub. Kern: **de weigering is de scène**, het middel (een dode naam) wordt niet uitgegeven, en de betaling is **Bricks eigen regel** uit `M1.6.S01.140` — *"They water them at the yard. Not before."* — die hij nu uitvoert in plaats van uitspreekt, voor de eenenveertig die op de vloer van dit gewelf liggen. Nul nieuwe canon, nul nieuwe eigennamen, vier regels in plaats van vijf, en goedkoper dan de huidige versie.

**Titel wijzigt mee:** `"In My Place"` → `"Not Before"`.

---

### 8.5 De vijf losse bevindingen

#### (a) L1-R29 wijst naar het verkeerde blok — *gecorrigeerd in het bestand, rulingtekst nog open*

`HUB.A1.reyes_first_conversation` **r. 25** droeg nog `story.choice_m11_conscriptspared (killed / left / bound and left)` — een door L1-R3 ingetrokken vlag én een blad dat `finished` heet. **Dat staat in het L1-blok**, dus in het mijne; L1-R29 noemt het een "L2-notitie". Een schrijver die de ruling letterlijk volgt kijkt in het verkeerde blok en vindt niets. De `condition:`-regels waren correct; alleen het commentaar loog — **exact de fout die L1-R29 zelf beschrijft.**

**Gedaan:** het L1-blok in het bestand is gecorrigeerd naar de drie levende bladeren.
**Nog te doen (serieel, niet door mij):** in `RULINGS_L1.md` L1-R29 het woord **L2** vervangen door **L1**, met de verwijzing naar r. 25. Zie de erratum-noot in §4.2.

#### (b) `Story.Thread.MaraLetters_Open` — het is geen spellingsprobleem, het is een ontbrekende zetter

**Eerst wat géén defect is.** De twee spellingen zijn het **gesanctioneerde paar** uit `SCRIPT_FORMAT` §4: schrijvers gebruiken altijd de kleine-letterform, `script_to_seed.py` mapt naar de gameplay-tag. Precies zoals `story.brick_recruited == true ⇄ Story.Beat.BrickRecruited`. **Niet repareren.**

**Wat wél het defect is, en het is groter dan gemeld — geteld, niet aangenomen:**

| | |
|---|---|
| machine-leesbare **lezers** | **2** — `M1.8.S91.157` en `.164` (`condition:`) |
| machine-leesbare **zetters** | **0** |
| zetters in **proza** | **2** — `HUB.A1.mara_letters.080` `shot:` én `M1.8.S91.160` `shot:` |

**Twee gevolgen, en het tweede is het dure.**

1. **De draad raakt niet verweesd als de hub eruit valt.** `M1.8.S91.160` opent hem voor iedereen; acts 2–4 houden hun draad. Dat is een geruststelling die niemand had opgeschreven en die ik hier vastleg zodat niemand hem opnieuw hoeft af te leiden.
2. **Maar `.157/.164` zijn dan permanent dood, en wel *na* generatie.** Dat zijn twee gegenereerde regels waarvoor betaald is en die nooit klinken. **Instructie aan `voice-director`: `M1.8.S91.157` en `.164` worden niet gegenereerd vóór `HUB.A1.mara_letters` gegenereerd is.** Dat is de enige maatregel die hier werkelijk geld bespaart, en hij kost niets.

**En de format-lacune, want die is de eigenlijke oorzaak.** De schrijver redeneerde correct: er is geen `set:` buiten `choice:`, en een `choice:` met één optie zakt op choice-integrity. Er is dus geen manier om *"deze scène heeft gespeeld"* uit te drukken — en dat is niet hub-specifiek, dat geldt voor elke aanwezigheidsdraad in 42 missies.

**Specificatie, in de vorm van L1-R28 (specificeren, nog niet aanzetten):** een scalair kopveld dat commit op de laatste regel van de scène.

```yaml
type:              hub
credit_tier:       4
sets_on_complete:  'story.thread_mara_letters_open'
```

Zelfde plek en zelfde grammatica als `condition:`, dus dezelfde toolwijziging: opnemen in `SCENE_OPTIONAL`, controleren met `COND_GRAMMAR`, **en meetellen als schrijver in de feitentabel** — anders is het decoratie en blijft `CONDITION resolves` blind. **NOG NIET GEBRUIKEN** tot `validate_script.py` het veld kent; tot dan blijft de proza-zin in `.080` staan.

> **Dit is een `SCRIPT_FORMAT.md`-wijziging en dus van mij — maar hij hoort in dezelfde seriële ronde als L1-R28's toolwijziging en niet ervoor.** Twee onaangezette kopvelden tegelijk invoeren is één toolwijziging, geen twee.

#### (c) `petra_return` was de enige passieve scène — *opgelost door vorm B*

Elf beurten, tier 4, geen `choice:`, geen variant, geen gemarkeerde bewegingsbeat. De andere negen zonder `choice:` dekken §18.7 wél af (vijf met `PLAYER BEAT`, vier met assen-varianten) en de twee korte Brick-scènes zijn vrijgesteld. Deze viel tussen twee schrijverssystemen door. **De lepel op `.100` wás al een beweging; hij was alleen niet als spelersbeat geschreven.** Nul credits. Zie §4.10.

#### (d) `mara_letters`' L2-blok verwijst twee keer naar `.090`; het bestand eindigt op `.080` — *gecorrigeerd*

Zelfde klasse als (a): het commentaar liegt terwijl de regels kloppen. Beide verwijzingen zijn gecorrigeerd naar `.080`. **Geen dialoogregel geraakt.**

#### (e) `reyes_triage.170` — vastgelegd, niet heropend

Zie de blokcitaat-noot in §4.6. De schrijver handelde juist en meldde het; het besluit staat nu op papier zodat het niet opnieuw afgeleid hoeft te worden, en de vraag naar een tweede in latere acts is expliciet een **owner-vraag**.

---

### 8.6 Vier defectklassen, nul rode regels — wat is mechanisch te vangen?

De criticus wijst erop dat alle vier de defecten van deze poort buiten het bereik van de bar vielen: *"de twaalf poorten staan als **commentaar** (L1-R28, bewust), de vlagcommit staat in **proza**, de canon-tegenspraak staat **tussen twee missies**, en de dubbele Voss-regel staat **tussen twee bestanden**."*

**Dat is geen bewijs dat de bar het verkeerde instrument is. Het is een werklijst — drie van de vier zijn mechanisch, en één niet.**

| Klasse | Mechanisch? | Wat er moet gebeuren |
|---|---|---|
| **1 · poort als commentaar** | **Ja, en het is al gespecificeerd** | L1-R28's toolwijziging. Voeg daarna één lint toe die faalt op een `# condition:`-comment in een bestand dat het echte veld kan dragen — anders blijft de oude vorm eeuwig naast de nieuwe bestaan |
| **2 · vlagcommit in proza** | **Ja — en dit is de verdachte** | `Condition resolves` in `validate_script.py` zou vandaag al rood moeten staan op `M1.8.S91.157/.164`: een `condition` op een vlag die **niets** zet. Als hij zwijgt, telt de tool iets als zetter dat er geen is (proza, of een rij in `ACT1_OVERVIEW` §6). **Dit eerst uitzoeken, vóór `sets_on_complete:`** — een check die niet rood wordt waar hij rood hoort te staan, is erger dan geen check |
| **3 · canon-tegenspraak tussen twee missies** | **Nee — het óórdeel niet. Het verzamelen wél** | Geen validator weet dat *"in my place"* en *"seventeen days in a car"* elkaar uitsluiten. Maar een tool kan per eigennaam uit de glossary (`Tam`, `Krad-9`, `Oyelaran`, `Vic`, `K-77` …) **elke regel én elke noot in alle 71 bestanden op één pagina zetten.** Dat detecteert niets en toont alles: de zes Tam-regels naast elkaar en een mens ziet het in tien seconden. **Wat hier faalde was niet het oordeel maar het verzamelen, en verzamelen is mechanisch** |
| **4 · dubbele regel tussen twee bestanden** | **Ja, triviaal** | Genormaliseerde `text:`-vergelijking over het hele corpus (kleine letters, leestekens weg). Nieuwe check `DUPLICATE`. **Met een witte lijst**, want opzettelijke herhaling bestaat: `"Tam. Oyelaran. Vic."` staat twee keer en dat is de bedoeling. Een check zonder witte lijst wordt uitgezet en dan vangt hij niets |

**De les voor de volgende ronde:** vertrouw op eigen ogen bij **klasse 3, en alleen bij klasse 3.** Voor de andere drie is "met de hand gevonden" een symptoom, geen prestatie.

---

### 8.7 Budget

De twaalf kosten samen **10.780 credits** van een act van 97.275 tegen een saldo van 125.612 (`phase0/VOICE_LEDGER.md`). Een herschrijving die het aantal woorden gelijk houdt is gratis; een die uitbreidt niet.

**Deze herstelopdracht is netto goedkoper dan de huidige oplevering.** Ruwe raming, en het is arithmetiek uit de bestanden, geen meting:

| Ingreep | `words_generated` |
|---|---|
| `petra_return`: vijf Voss-regels weg (Voss verdubbelt voor gender), Petra mag ≤20 woorden groeien | **−55 à −60** |
| `brick_the_tithe`: vier regels in plaats van vijf, één Voss-regel minder | **−20 à −25** |
| `dex_workshop`: Mara krijgt wat Dex inlevert; `.020` herbouwd | **≈ 0** |
| `reyes_triage.020` herbouwd | **+10 à +12** |
| Alle sluitingen en shot-wijzigingen uit §8.2 | **0** |
| **Totaal** | **ongeveer −70 gegenereerde woorden, circa −350 credits** |

> **Dat cijfer is van mij en niet van een tool.** `Eclipse/Tools/count_generation_cost.py` telt dit exact uit de tekst; **de definitieve cijfers komen daarvandaan, ná de herschrijving, niet uit deze tabel.** Ik had geen shell in deze ronde (zie het rapport) en heb dus niets gemeten.

**`mara_letters` houdt absolute voorrang** bij schrijven én genereren (§4.9). Als er één hubscène overleeft, is het die.
</content>
