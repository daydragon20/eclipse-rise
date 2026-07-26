# Jouw taken — werkblad

De editor staat open op **GrayboxDistrict**. Werk deze volgorde af; hij is zo
gekozen dat de downloads op de achtergrond binnenlopen terwijl jij speelt.

> **Beslissing 15.7 (MetaHuman-shading) is al klaar** — je koos vandaag B (hybride).
> Die staat vastgelegd in `phase0/metahuman_recipes.md`. Niets meer aan te doen.

> **Editor open = agent kan niet bouwen.** Sluit de editor als de dev-sessie moet
> compileren. Vraagt hij erom, sluit hem dan even — je verliest niets.

## Welk bestand waarvoor

| Bestand | Wanneer je het nodig hebt |
|---|---|
| **`JOUW_TAKEN.md`** (dit bestand) | je werkblad — begin hier |
| **`SPEEL_ECLIPSE.bat`** | dubbelklikken om te spelen (standalone, geen editor) |
| **`PROMPT_SPELEN_EN_CHECKEN.md`** | kant-en-klare prompts om aan de dev-sessie te plakken |
| `phase0/FEEL_GAUNTLET_P2-02.md` | het volledige draaiboek van stap 2 |
| `phase0/metahuman_recipes.md` | de zes gezichts-recepten van stap 4 |
| `FAB_CLAIMLIJST.md` | optioneel vooruitwerk: assets claimen voor latere planeten |
| `GEMINI_BEELD_PROMPTS.md` | doelbeelden genereren om te zien waar je heen werkt |
| `PROGRESS.html` | je dashboard — dubbelklikken om de stand te zien |

---

## ✅ STAP 1 · Env-packs — KLAAR (25-07)

Geverifieerd op schijf in `Eclipse/Content`:

| Pack | Stand |
|---|---|
| Factory_Pack_V1 | 2,1 GB — 408 assets |
| Uniblocks | 3,7 GB — 3803 assets |
| IBuilding_49 | compleet (mesh + AO/BaseColor/Metallic/Normal/Roughness) |
| LPCharacters_FREE | 16 MB — 25 assets (bonus) |

**Nog te doen:** geef het door aan de dev-sessie met **prompt 3** uit
`PROMPT_SPELEN_EN_CHECKEN.md`. Dat is de trigger waar P2-08 (het fidelity-district)
op wacht — zonder die melding begint de kit-pass niet.

Die prompt vraagt de agent ook te controleren of Sci-Fi Hallway, Sci-Fi Light Pack,
Auto Footsteps, Niagara Footstep VFX, FPS Weapon Bundle en Free Muzzle Flash binnen
zijn. Ik zie ze niet als aparte Content-map — zegt de agent dat ze ontbreken, klik
ze dan alsnog via **Window → Fab → Add to Project**, één voor één.

**Waarom dit het belangrijkste was:** je gebouwen staan op 10%. Belichting (85%),
toon-materiaal (100%) en inktlijnen (85%) zijn al ver — de gebouwen zijn wat de
beelden nog goedkoop laten ogen. Nu kan de agent daar eindelijk aan werken.

---

> ## Stap 2 en 3: wat is het verschil?
>
> **Allebei speel je in de echte game, met dezelfde `SPEEL_ECLIPSE.bat`.** Het
> verschil zit in waar je op let.
>
> | | Stap 2 · gauntlet | Stap 3 · playtest |
> |---|---|---|
> | Wat test je | één systeem: Command Mode | het hele spel |
> | Hoe | tellen en scoren | gewoon voelen |
> | De vraag | *werkt Command Mode?* | *is dit leuk?* |
> | Uitkomst | "true" of "false" | losse notities |
> | Beslist | of Stage B gebouwd wordt | of Fase 1 dicht mag |
>
> **Doe ze achter elkaar in één sessie:** eerst 20 minuten gericht Command Mode
> testen, daarna 30 minuten gewoon doorspelen zonder te tellen. Eén keer
> opstarten, twee taken klaar.

## STAP 2 · Feel-gauntlet Command Mode (~20 min) — het R3-verdict

Dit is een **test**, geen playtest. Je beoordeelt één ding: voelt Command Mode
als *sneller denken* of als *een menu openen*? Het spel is expres nog lelijk —
dat hoort zo, polish mag een verdict nooit fabriceren.

**Starten:** druk op de groene **Play ▶** (of Alt+P), en **klik één keer in het
spelbeeld** zodat hij je muis pakt. Dat laatste is waarom je controls eerder
niets deden.

**Besturing:**
- **Q vasthouden** (of LB op de controller) → wereld gaat naar 30%. Loslaten = normaal.
- Tijdens het vasthouden: **Tab** / scroll / **RB** = volgende soldaat · **E** of **X** = soldaat onder je richtkruis
- **1-4** of D-pad = orders geven · **Alt** vasthouden = stance

**Meet deze 5 dingen — wees streng, één gefaald punt = "false":**

1. **Order-round-trip** — geef 10 orders achter elkaar in Command Mode. Krijgt elk
   order binnen 1 seconde échte tijd antwoord (bark of weigering)? Doel: **10/10**
2. **Targeting** — geef 10 orders onder vuur aan een bepáálde soldaat met een
   bepaald doel. Landt hij in één keer bij de juiste? Doel: **minstens 9/10**
3. **Comfort** — speel 3 gevechtsmomenten. Voelt instappen als sneller denken, of
   als een menu? Geen desoriëntatie. Loslaten moet schoon hervatten, zonder
   ingeslikte input
4. **Vertrouwen** — geen enkele stille orderfout tijdens de vertraging. Let er
   actief op: krijgt élk order hoorbaar of zichtbaar antwoord?
5. **Gebruiks-trek** — speel daarna vrij. Stap je uit jezelf minstens 1× per
   gevecht de mode in? **Een mode die je niet gebruikt, is een gefaalde mode.**

**Doorgeven:** zeg tegen de agent **"R3-verdict: true"** of **"R3-verdict: false"**,
met per punt kort wat je zag.

Bij **false** stopt Stage B — dan gaat de agent terugvallen op dilatatie 0.5, of
volledige pauze, of hold-to-order zonder vertraging. Dat is geen mislukking, dat
is precies waar deze test voor is.

---

## STAP 3 · 13.2-playtest (~30 min) — sluit Fase 1 af

Nu wél gewoon spelen. Doe dit met **`SPEEL_ECLIPSE.bat`** (dubbelklikken) voor een
schone standalone build, of gewoon met Play ▶ in de editor.

> **Objectives los testen?** Wil je alleen de missiedoelen nalopen zonder elke keer
> de hele campagne te spelen: gebruik **prompt 4** uit `PROMPT_SPELEN_EN_CHECKEN.md`.
> Die laat de agent een snelle testroute opzetten en documenteren in
> `phase0/TESTROUTE_OBJECTIVES.md` — per missie welk commando, en wat je hoort te
> zien als het klopt. Handig als je één objective wil hertesten na een fix.

Speel ~30 minuten de hele lus: **missie → squad → gevecht → basis → volgende missie.**

Noteer onderweg kort (telefoon of kladblok):
- Wat voelt **goed**? Waar dacht je "oh, dit is leuk"?
- Wat voelt **slecht** of traag? Waar verloor je je aandacht?
- Snapte je steeds wat je moest doen, of raakte je de draad kwijt?
- Deed je squad wat je vroeg, of moest je vechten met de besturing?

Geen lange analyse nodig — losse zinnen zijn prima. Geef ze daarna aan de agent.

---

## STAP 4 · De zes gezichten (~1 uur, mag later)

Dit is de enige taak die niets blokkeert: zolang een gezicht ontbreekt, draait dat
personage op een fallback-body. Je kan dit dus rustig een andere dag doen.

**Per gezicht (~10 min):**
1. Editor → **Window → MetaHuman** → **Create New**
2. Kies het preset-startpunt uit `phase0/metahuman_recipes.md` en pas alleen de
   genoemde punten aan — **niet meer dan dat**. De toon-shader doet de stilering,
   dus geen detailsculpt nodig
3. Sla op met **exact** de naam uit het recept: `MH_Kaya`, `MH_Brick`, `MH_Vale`,
   `MH_Dex`, `MH_Petra`, `MH_Kaine`
4. Exporteer naar het Eclipse-project
5. Zeg: **"MH_<Naam> staat erin"**

**Denk aan het silhouet, niet aan de poriën.** Borderlands leeft van
karakterkoppen: uitgesproken kaaklijnen, herkenbare vormen. De cel-shading laag
gooit fotorealisme er toch overheen.

De zes recepten (volledig uitgewerkt in `metahuman_recipes.md`):

| Naam | Wie | Kern |
|---|---|---|
| MH_Kaya | Kaya Renn, 27, smokkelpiloot | scherp, spottend, kort zwart haar, litteken door wenkbrauw |
| MH_Brick | Oram "Brick" Bex, 34, heavy | blokkaak, gebroken neus, zachtaardige reus |
| MH_Vale | Torren Vale, 45, ex-kolonel | gegroefd gezicht, staalblauw, vermoeide waakzaamheid |
| MH_Dex | Dex Callum, 31, engineer | — zie recept |
| MH_Petra | Petra Voss, ±55, het stille hart | — zie recept |
| MH_Kaine | Sera Kaine, 49, villain | — zie recept |

---

## Samengevat

| Stap | Tijd | Blokkeert |
|---|---|---|
| 1 · Env-packs | 10 min klikken | **de hele graphics-fase** |
| 2 · Feel-gauntlet | 20 min | Stage B van Command Mode |
| 3 · Playtest | 30 min | afsluiting Fase 1 |
| 4 · Zes gezichten | ~1 uur | niets — mag later |

Doe stap 1 nu, laat de downloads lopen, en speel meteen door naar stap 2.

---

# Wat er daarna gebeurt

## Meteen na elke taak (agent-werk, jij hoeft niets)

| Jouw taak | Wat er direct daarna losbreekt |
|---|---|
| Env-packs binnen | **P2-08 mag starten** — het fidelity-district. Kit-pass, verticaliteit, particles, interieurs, meerdere bouwers parallel |
| R3-verdict "true" | **P2-02 Stage B** — volledige 8.4-ordertabel, 5 nieuwe verbs, 3 refusal-redenen, stealth-stance |
| R3-verdict "false" | Geen polish. Eerst spec-amendement: dilatatie 0.5 → volledige pauze → hold-to-order zonder vertraging |
| Playtest | Fase 1 formeel afgesloten; jouw notities sturen de eerste review-ronde |
| Gezichten | Assembly-run, toon-restyle volgens jouw B-keuze, koppeling aan de named-slots, outfits uit de garderobe |

## De resterende bouwstappen van Fase 2

Dit is de officiële volgorde uit `SPEC-P2-00`. Vijf van de dertien staan al groen:

| # | Stap | Stand |
|---|---|---|
| 1-3 | Milestone-flip, squad van 4 + classes | ✅ geland |
| 4 | Command Mode final feel | 🟡 Stage A geland, Stage B wacht op jou |
| 5 | Hollow Point walkable base | 🟡 93% — walkable vault is het restant |
| 6 | Missies M1.1–M1.4 | 🟡 90% — story-laag geland, authoring loopt |
| 7 | Liberation-instance | 🟡 55% — pure core geland, wiring volgt |
| 8 | **Save v1** | ⬜ versioned plugin, migratie, autosaves, checkpoints |
| 9 | **UI Stack v1** | ⬜ debug-UI eruit, CommonUI erin, controller + muis gelijkwaardig |
| 10 | **Fidelity-district Kessara** | ⬜ **de echte graphics** — hangt aan jouw env-packs |
| 11 | **Audio-infrastructuur** | ⬜ adaptieve muziek, combat-audio, squad-barks |
| 12 | Testlagen compleet | ⬜ alle zes lagen incl. nightly soak |
| 13 | **Gate-review Fase 2** | ⬜ jouw eindtest — zie hieronder |

Stap 10 is waar je op wacht. Let op de volgorde: hij staat achter stap 6 (de ruimtes
moeten vastliggen voor je ze aankleedt) én achter jouw Fab-kliks. Daarom weegt stap 1
van je takenlijst zo zwaar — het is letterlijk de enige owner-afhankelijkheid die in
de spec bij P2-08 staat.

## Jouw taken die later terugkomen

- **Shotrondes beoordelen.** Na elke dressing-ronde levert de agent screenshots op de
  vaste camera's. Jij zegt wat het zwakste punt is. Dit herhaalt zich — het is de
  15.8-lus, en die stopt pas als een scène "AAA-ready" leest.
- **De gate-review van Fase 2.** Externe mensen spelen de eerste 3 uur koud. De vraag
  is er maar één: *"wil ik de rest van deze game?"* Is het antwoord "mooie systemen,
  maar…", dan gaat de fidelity-kloof eerst dicht.
- **Fase 0-restjes** die nooit af zijn geraakt: CI-runner, 10 concept-beelden,
  5 feel-referentieclips. Geen haast, maar ze staan nog open.

## Daarna: Fase 3 en verder

| Fase | Wat | Dashboard-doel |
|---|---|---|
| 3 · Early Build | Kessara + Tarsis compleet, Act 1 volledig, alle 9 klassen, ~25 uur speeltijd | 31 aug |
| 4 · Alpha | Alle 10 planeten, hele campagne uitspeelbaar | 20 sep |
| 5 · Beta | Content lock, balans, VO-opnames, performance, localisatie | 10 okt |
| 6 · Release | Gold, Steam / Epic Games Store | 31 okt |

## Eén eerlijke noot over die datums

Je dashboard zegt Fase 2 af op 12 augustus. Het GDD (`13_roadmap.md`) zegt voor
dezelfde fase **9 maanden**, en voor het geheel ~4,5 jaar — bij een team van 3 tot 8
mensen.

Dat verschil is geen fout: de dashboard-datums zijn de AI-versnelde inschatting op
basis van het huidige tempo. Het GDD zegt er zelf bij dat AI-versnelling *"upside is,
geen plan-of-record"*. De eerlijke lezing: de datums op je dashboard zijn een
ambitie om naartoe te werken, geen belofte. Wat telt is dat de volgorde klopt — en
die klopt.

## Twee vragen uit de eerste echte spelbeelden (26-07)

Ik heb voor het eerst screenshots vanuit jouw camera bekeken. Twee dingen die ik
zag zijn geen bug maar een keuze, en die is van jou:

- **Stijlbotsing.** De aankleedfiguren (Quaternius) zijn blokkerig laag-poly met
  grote koppen; jouw personage is realistisch geproportioneerd. Naast elkaar in
  hetzelfde frame vloeken die twee. Hun maat heb ik wel gerepareerd — ze stonden
  op 328 cm naast jouw 190 cm. Wat wil je: de figuren vervangen, jouw personage
  stileren, of laten staan tot de art-pass?
- **Geen wapen in de handen.** Dit is het enige echte van de drie dingen die ik
  eerst meldde. Het schot valt en de munitie loopt (30 → 19, elf schoten gemeten),
  maar er hangt nergens een wapenmesh aan een bot — in de hele module hangt maar
  één ding aan het skelet, en dat is de hoofd-hitbox. Wil je dat ik daar een model
  aan koppel, of wacht dat op de art-pass?

  *De HUD ontbrak niet: die mount netjes en de teller hoort zichtbaar te zijn. Dat
  hij niet op mijn beelden staat, komt doordat screenshots de UMG-laag niet
  meenemen — mijn meetmethode, geen bug. Idem "nul schoten": verkeerd zoekwoord.*

## Werkt de View-knop nu wel? (26-07 laat)

Je meldde op 25-07 dat twaalf gameplay-acties de gamepad bereikten, maar dat de
testgids op de **View-knop** niet in het log verscheen. In de code stond het
vermoeden al: "View/Menu zijn precies de knoppen die een UI-laag pleegt op te
eten."

De oorzaak lag in je opstartlog, als ERROR, bij elke start:

> *Using CommonUI without a CommonGameViewportClient derived game viewport client.
> **CommonUI Input routing will not function correctly.***

CommonUI komt binnen als plugin-afhankelijkheid, en het project draaide in een
configuratie die de plugin zelf ongeldig noemt. Dat is nu gezet.

**Wil je bij je volgende sessie even proberen of de View-knop de gids opent?** Ik
kan een gamepad niet headless indrukken, dus dit is het enige stuk dat ik niet zelf
kan afvinken. Werkt het nog steeds niet, dan weten we dat het hier níét aan lag —
ook dat is winst.
