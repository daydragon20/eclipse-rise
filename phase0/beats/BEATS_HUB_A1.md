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
| 4 | `HUB.A1.dex_workshop` | M1.3 | Dex | basisbouw als systeem-leraar (`05_base_building.md`) |
| 5 | `HUB.A1.brick_bunk` | M1.4 | Brick | de nieuwe man die niemand iets vraagt |
| 6 | `HUB.A1.reyes_triage` | M1.4 | Reyes | haar verleden, één laag opengetrokken |
| 7 | `HUB.A1.brick_the_tithe` | M1.6 | Brick | AR-11: hoe de Tithe hem van Krad-9 haalde |
| 8 | `HUB.A1.sela_intake` | M1.6 | Sela | politiek als praktijk, niet als toespraak |
| 9 | **`HUB.A1.mara_letters`** | M1.7 | Mara | **de dragende plant.** Zonder deze scène geen brieven in acts 2–4 |
| 10 | `HUB.A1.petra_return` | M1.8 | Petra | wat er terugkwam, en wat niet |
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
- Dit is de eerste keer dat het spel de speler laat merken dat er iemand meetelt (§2.5: *"Holds the player accountable for casualty-heavy tactics"*).

### 3 · `mara_map_table` *(na M1.2)*
- **want** — Voss wants to know how long Mara has been doing this.
- **obstacle** — She answers questions about herself by answering a different question.
- **turn** — She gives him a number of years, and then a number of people, and the second number is smaller than he expected.
- **De enige scène in de act waarin Mara over het verleden praat.** Maximaal veertien regels. Zij zegt nog steeds "wij".

### 4 · `dex_workshop` *(na M1.3)*
- **want** — Dex wants to show Voss what the vault could be if anyone gave him a week and some steel.
- **obstacle** — Nobody has a week and there is no steel.
- **turn** — He shows him anyway, and the plan is far too big, and it is the first blueprint of Hollow Point as the player will eventually know it.
- **Systeem-leraar-scène** (`05_base_building.md`: je leert basisbouw van de persoon die ervan houdt). Concreet: sloten, energie, wat een Workshop doet — allemaal via wat hij wíl, nooit als uitleg.

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

### 7 · `brick_the_tithe` *(na M1.6)*
- **want** — Voss wants to know why Brick spoke first at the briefing.
- **obstacle** — Brick does not explain things.
- **turn** — He says the name of the man who was taken off Krad-9 in his place, and that is the answer.
- **AR-11 betaalt hier volledig.** Vijf regels. Geen enkele daarvan legt de Tithe uit.

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

### 10 · `petra_return` *(na M1.8)*
- **want** — Voss wants his aunt to rest.
- **obstacle** — She has already found the kitchen, the water line and three people who are not being fed.
- **turn** — She gives him a job, and he takes it, and that is how they both survive the week.
- **Petra bedankt niet en klaagt niet.** Zij vult de leegte die Mara achterliet met **werk**, niet met troost, en dat is precies waarom het werkt.

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
2. **De hub-band is 12–35 woorden per regel**, maar de variantieregel (§18.3: factor 3) geldt ook hier. Bricks scènes zitten structureel onderaan de band; dat is zijn karakter en het is toegestaan zolang iemand anders in dezelfde scène bovenaan zit.
3. **Elke scène heeft een `condition`.** Ongepoorte hubdialoog is de snelste manier om een game dom te laten klinken.
4. **Na M1.8 is alles nasleep.** Nul grappen in scènes 10–12.
5. **`mara_letters` heeft voorrang** boven alle andere hubscènes, bij het schrijven en bij het genereren.
6. **Voss-varianten:** alleen in 2, 6, 8 en 11 — de scènes waarin hij iets tekent, kiest of toegeeft. Elders neutraal. Zie C-6/Q-7.
</content>
