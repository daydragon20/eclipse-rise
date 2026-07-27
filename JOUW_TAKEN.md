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

## ✅ JE ZES PUNTEN: ALLE ZES GEDIAGNOSTICEERD (27-07)

Suite **184/184 groen**, ValidateData 0 errors. Bij drie punten bleek de
oorspronkelijke formulering niet te kloppen met wat de code doet — dat staat er
telkens bij, want dat verandert wat er gebouwd moet worden.

| # | Stand |
|---|---|
| **1** mikken | **GEBOUWD, niet visueel bevestigd.** De camera-lag ging nooit uit tijdens ADS — je kruis staat schermvast terwijl de camera achterloopt, dus de wéreld schuift eronder door. Nu uit. En de zijoffset (55 cm) kromp niet mee met de arm (300→165), waardoor je personage júíst je vizierlijn in kroop; die schaalt nu mee |
| **2** richtkruis | **BESTAAT EN WORDT GETEKEND** (`richtkruis zichtbaarheid=3 tekst='+'`). Niet te fotograferen: vier opnamemethodes vangen de UI-laag niet |
| **3** camera | **GEMETEN, en de uitslag spreekt de klacht tegen.** Twee bestaande metingen kónden hem niet vinden (negen keer exact x=500). De derde beweegt wel — spreiding 64 px — maar de uitslagen vallen bij **draaien** en **herladen**, niet bij rennen |
| **4** kogels | **ER IS GEEN WERELD-INSLAG om zichtbaar te maken.** De hitscan gooit alles weg wat geen personage is, dus een schot in een muur wordt stil verworpen: geen feit, geen geluid, geen decal. VFX op dat pad blijft bij elke MIS onzichtbaar |
| **5** wapenwissel | **HET WAPEN IS EEN APART MATERIAALSLOT** (`M_Belica_Guns`, slot 4 van 12). Zichtbaar máken vraagt dus geen mesh-operatie, alleen een eigen tint. Wisselbaar maken vraagt het nog steeds wel |
| **6** de gids | **DE KETEN WERKT — end-to-end gemeten.** Toets → actie → handler → paneel open met 13 regels. G kon je niet hebben (toegevoegd ná je sessie); F3 is door de engine geclaimd als `viewmode lit` |

### 🟡 DRIE BESLISSINGEN DIE BIJ JOU LIGGEN

1. **Speel na een verse build en druk G.** Dat sluit punt 6 én bevestigt of je het
   richtkruis ziet. Werkt G niet, dan ligt het aan je toetsenbord — en dat weten we
   dan ook, want de keten is gemeten.
2. **Punt 5 — GUNMETAL KAN, hier is de meting die je voorwaardelijk vroeg.**
   Gemeten op de mik-opname (`HighresScreenshot00980`), helderheid op 0–255:

   | | |
   |---|---|
   | lichaam, mediaan | **50,9** (10% op 43,3 · 90% op 57,6) |
   | asfalt eronder | 58,0 |
   | donkerste 5% van het frame | onder **11,1** |

   Het lichaam is donker (20% van wit) maar er zit ruim marge onder. Twee gebankte
   waardestappen van het district (×0,72) brengen 51 naar **≈26** — duidelijk
   onder het lichaam, en nog steeds ruim boven de donkerste 5% waar het in de
   schaduw zou verdwijnen. Jouw instructie is dus uitvoerbaar zonder
   kleuruitzondering; ik kom niet terug met een verzoek maar met een frame.

   *Kanttekening: dit is één frame van een schemerdistrict. Op een lichtere scène
   ligt het lichaam hoger en wordt de marge alleen maar groter, niet kleiner.*

   **GELAND (`776ffa9`) — het frame is `HighresScreenshot00988`, en het oordeel is
   aan jou.** Slot herkend op naam (`M_Belica_Guns`), twee gebankte waardestappen,
   85% ontzadigd richting de eigen luminantie zodat er geen nieuwe hue bijkomt.

   **Maar ik heb GEEN betrouwbaar getal bij dat frame, en dat zeg ik erbij.** Twee
   pogingen om het wapen te meten zijn allebei mislukt:
   - een handmatig meetvenster pakte de betonnen pilaar mee (gaf 54,1 tegen 44,4 —
     *lichter*, wat het op het frame duidelijk niet is);
   - het verschil tussen het vorige en het nieuwe frame nemen pakte 5,59% van het
     hele beeld (`x 0..1278, y 0..719`), want de ronde is vanaf stap 3 niet
     bit-reproduceerbaar en ik meet dan scèneruis mee.

   Dat er 0,69 uit die tweede rolt — dicht bij de beoogde 0,52 — is dus **geen
   bevestiging**. Het staat hier zodat niemand het later als bewijs oppakt.
   Wil je een hard getal naast je oordeel, dan is de weg een isolatie-opname zoals
   opname 5 (alleen het wapenslot zichtbaar); dat is een build en die staat klaar.

3. **~~Punt 5: welke kleur krijgt het wapen?~~ (beantwoord — zie hierboven)** Een eigen tint op slot 4 maakt het
   meteen leesbaar in silhouet. Maar wélke kleur raakt de ÉÉN-STIJL-WET en de
   waardehiërarchie die in het district al vastligt (Cover > CoverB > DecoLine).
   Dat is art-directie en dus jouw oordeel, niet iets wat ik 's nachts invul.
3. **~~Punt 4: mag ik het schadepad aanraken?~~ — JA GEGEVEN, STAP 1 GELAND
   (`44fd7b8`).** Een schot dat de wereld raakt krijgt nu een uitkomst: geteld en
   gelogd met impactpunt en physical material. Apart geland zoals je vroeg, verse
   bar, 184/184 groen.

   Ik had die klus te groot ingeschat: de tak die een personage raakt is
   **ongewijzigd**, en de enige branch die verandert deed voorheen niets — die kan
   dus niets breken.

   **Stap 2 (het inslaggeluid) wacht op een verse sessie**, en daar is een
   technische reden voor: audio reageert hier op FEITEN via de bus, en een
   wereldtreffer is een ander feit dan `Event.Combat.HitLanded` (die betekent
   "schade aan een personage" en wordt zo geteld). Er is dus een nieuwe tag nodig
   plus een regel in `EventCatalog.md`, anders valt de 34/34-controle om. Dat is
   één samenhangend blokje van drie bestanden.

4. **~~Oude vraag~~ (hierboven beantwoord)** — Stap 1 is "een schot dat de wereld
   raakt krijgt een uitkomst", en dat zit in het schadepad van élk wapen. Zeg het
   en ik doe het met een verse bar en een frame erbij. En zeg het als je de Muzzle
   Flash uit je Fab-bibliotheek wilt ophalen.

---

## 🔴 NA DE REFERENTIE-OPDRACHT — STAND EN ÉÉN KLIK

**De maatstaf staat: [phase0/REFERENTIE_TPS.md](phase0/REFERENTIE_TPS.md).**
Vijf onderdelen, elk met wat de referentie doet, wat wij nu doen, en of dat een
keuze is of een gat. Hoofdstuk 4 en 5 zijn herzien nadat jij het wapen op mijn
eigen opnames aanwees — die stonden fout omdat ik ze uit codelezing had afgeleid.

### ✅ EEN KLIK VAN JOU, en die beslist een open vraag

**Start het spel pas nadat ik "build klaar" meld.** `SPEEL_ECLIPSE.bat` gebruikt de
bestaande binaries; startte je vorige keer vóór mijn build af was, dan speelde je
oude code en kón het richtkruis er niet zijn.

Waarom dit de vraag beslist: in een draaiende missie meldt het spel zelf
`UI: richtkruis zichtbaarheid=3 tekst='+'` — het kruis bestaat en wordt getekend.
Ik kan het alleen niet fotograferen (zie hieronder). Zie jij het na een verse build
nog steeds niet, dan zit het probleem ergens anders en weten we dat meteen.

### De echte blokkade: ik kan geen UI fotograferen

Vier methodes geprobeerd, alle vier hetzelfde 3D-beeld zonder widgets:
HighResShot, FScreenshotRequest met bShowUI, een Slate-vensteropname, en het
engine-commando `Shot showui`. Daarmee is jouw eis "controleer het op een
screenshot voordat je het af noemt" voor **geen enkel UI-element** te vervullen.
Dat is een gat in mijn verificatielaag, niet in het richtkruis — en het is de
reden dat punt 2 nog niet af mag heten.

### Stand van jouw zes punten

| # | Punt | Stand |
|---|---|---|
| 1 | Mikken moet een stilstaand vizier geven | **GEBOUWD, NIET VISUEEL BEVESTIGD.** De camera-lag ging nooit uit tijdens het mikken — het kruis staat schermvast terwijl de camera achterloopt, dus de wéreld schuift eronder door. Nu uit bij ADS. En de zijoffset (55 cm) kromp niet mee met de arm (300→165), waardoor je personage juist je vizierlijn in kroop; die schaalt nu mee |
| 2 | Richtkruis dat er is en klopt | **BESTAAT EN WORDT GETEKEND** volgens het zelfrapport. **Niet op een frame aan te wijzen** zolang de opnamelaag geen UI vangt |
| 3 | De camera | Meetopdracht staat in de referentie: **schermpositie** meten bij 0/180/420/650 cm/s. Alle bestaande metingen gaan over grootte en afstand, niet over positie — en jouw klacht gaat over positie |
| 4 | Kogels zichtbaar | **Hulzen zijn er wel** (gezien op 00911). Geen tracer. "Geen inslag" is uitdrukkelijk **niet** geverifieerd: op geen opname wordt op dat moment iets geraakt |
| 5 | Wapenwissel zichtbaar | **VAN OPDRACHT VERANDERD** door jouw correctie: niet "bouw een wapenmesh" maar "haal het wapen uit de mesh". Kosten staan in de referentie. **Herweging is jouw keuze.** Goedkope tussenstap genoteerd: het wapen een eigen tint geven zodat het leesbaar wordt in silhouet |
| 6 | De gids opent niet | Nog niet uitgezocht **waarom** — en dat is wat je vroeg, niet nog een toets |

---

## 🟢 SPEELSESSIE 27-07 — STAND VAN JOUW ACHT PUNTEN

Veilig te spelen: **commit `6933fa7`**. Suite 182 groen, ValidateData 0 fouten.

| # | Jouw punt | Stand |
|---|---|---|
| 5 | Na je dood nergens op kunnen klikken | **FASE 1 KLAAR.** Jouw diagnose klopte niet: de overgang wordt wel gemaakt. CommonUI zet de cursor uit zodra je met een controller speelt. Nu vastgezet zolang de hub staat. **Nog open:** de hub met de controller kunnen bedienen (focusnavigatie) |
| 4 | F3 opent de gids niet | **KLAAR — gebruik nu G.** De engine claimt F1-F5 zelf voor viewmodes (staat in jouw log), en je toetsenbord kan F3 als mediatoets pakken. Welke van de twee doet er niet toe: allebei buiten dit project. F3 blijft ook staan |
| 2 | Er is geen richtkruis | **KLAAR.** Volledig bevestigd — het enige woord „crosshair” in het project was de vorm van de muisaanwijzer. Er staat nu een kruis, en de startbat vraagt er niet meer naar |
| 6 | Lichaam draait mee bij stilstand | **KLAAR.** Drempel van 60° naar 0°, en verplaatst van kijkhoek naar toestand: niet mikken = hij blijft staan, mikken = hij sluit meteen aan. Op beeld gecontroleerd (opname 7: camera 238,7°, lichaam 0,0° — je ziet zijn voorkant) |
| 1 | Schaal-bug is een camera-bug | **GEMETEN, en de uitslag weerlegt de hypothese.** Over de hele aanloop varieert de schijnbare hoogte 1,76% en hij wordt KLEINER (31,52 → 30,97 gr), niet groter. De bestaande cameratest keek alleen naar de eindstand — die is nu aangevuld met de aanloop. **Wat dit niet uitsluit:** 650 cm/s werd niet gehaald, en “pas helemaal zichtbaar” gaat over KADERING (waar hij in beeld staat), niet over grootte. Dat is de volgende meting |
| 3 | Kogelinslagen nauwelijks zichtbaar | **OPEN.** Zeg het als je de Muzzle Flash (Niagara) wilt ophalen — dat is een owner-klik |
| 7 | Camera 180° draaien en vooruit | **NOG NIET GECONTROLEERD** |
| 8 | Alle besturing in de gids, één bron | **OPEN** — grootste post: eerst de tabel, dan drie lezers |

> ⚠️ **EEN KLIKJE: sluit de editor / het spel als je klaar bent met spelen.**
> “Unable to build while Live Coding is active” — zolang jij speelt kan ik niet
> compileren. Ik werk ondertussen door aan dingen die geen build nodig hebben,
> maar de volgende codewijziging blijft liggen tot de editor dicht is.

**Squad-barks: jouw JA is genoteerd.** Drie zinnen per stem, zes in totaal, één
betaalde generatie. Staat als eerstvolgende klus na de bugs; ik genereer pas als
de zes regels in de seed staan, zodat het één aanroep blijft.

**Nog rood in de opnameronde, met opzet:** de speler komt in het eerste
loopinterval maar 3 cm vooruit (tweede interval 229 cm). Dat is een echt defect
dat de nieuwe bewaker blootlegde; oorzaak nog niet gevonden. Blokkeert het spelen
niet.

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

Dit is de officiële volgorde uit `SPEC-P2-00`.

> **Deze tabel is op 27-07 tegen de DoD's zelf nagelopen** in plaats van tegen een
> gevoel. Twee correcties, en ze gaan de tegenovergestelde kant op. Stap 7 stond
> op "55% — wiring volgt" terwijl die wiring er al is, mét veertien tests. Maar
> "af" is hij óók niet: zijn DoD vraagt een PIE-controle op de debugkaart, een
> soak die **drie nachten** groen blijft (ik heb er één), en jouw F1/F2-verdicts.
> Zo'n restant is geen percentage maar een lijstje, dus staat het er nu als
> lijstje. Wat er per stap nog open staat, is bijna overal iets dat ALLEEN JIJ
> kunt afvinken.

| # | Stap | Stand |
|---|---|---|
| 1-3 | Milestone-flip, squad van 4 + classes | ✅ geland |
| 4 | Command Mode final feel | 🟡 Stage A geland, Stage B wacht op jou |
| 5 | Hollow Point walkable base | 🟡 code af — rest is JOUW OGEN: "elke bouwstap verandert de vault zichtbaar" moet in PIE |
| 6 | Missies M1.1–M1.4 | 🟡 alle vier speelbaar en getest — open: Brick op het roster, de recap cold-reader, en de F-verdicts |
| 7 | Liberation-instance | 🟡 code + tests compleet (14 tests) — open: PIE-controle, de soak DRIE nachten, jouw F1/F2-verdicts |
| 8 | **Save v1** | 🟡 SPEC-P2-06 ligt als concept op je review; `Eclipse.Save.Report` gebouwd. Autosave en de schemawijziging wachten bewust op jouw oordeel |
| 9 | **UI Stack v1** | 🟡 SPEC-P2-07 ligt als concept op je review; de pariteitsbewaker gebouwd. De CommonUI-stack zelf wacht |
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

  **Op 27-07 opnieuw bekeken, dichterbij:** de botsing zit niet in de maat maar
  in de VERHOUDING. Zo'n figuur heeft een kop die ongeveer even breed is als zijn
  romp; die van jou is normaal geproportioneerd. Ze staan nu vaak binnen twee
  meter van elkaar in beeld, en dan is het geen stijl meer maar een fout die je
  ziet. De maat kloppend maken heeft dat niet opgelost en gaat dat ook niet doen.
- **Geen wapen in de handen.** Dit is het enige echte van de drie dingen die ik
  eerst meldde. Het schot valt en de munitie loopt (30 → 19, elf schoten gemeten),
  maar er hangt nergens een wapenmesh aan een bot — in de hele module hangt maar
  één ding aan het skelet, en dat is de hoofd-hitbox. Wil je dat ik daar een model
  aan koppel, of wacht dat op de art-pass?

  **Nagekeken op 27-07, en het is concreter dan ik het eerst bracht.** Op het
  schietmoment is te zien dat het schot ALLEEN door de hulzen leest: er komt geen
  mondingsvuur, want er is geen loop om het aan te hangen. De vraag "schiet ik nu
  eigenlijk?" wordt dus beantwoord door drie wegvliegende hulzen en verder niets.
  Een wapenmesh lost daarmee twee dingen tegelijk op — je ziet wát je vasthoudt,
  én je ziet dát je vuurt. Zonder mesh heeft mondingsvuur toevoegen geen zin.

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
kan afvinken.

Wat ik wél heb kunnen scheiden: **de gids zelf werkt.** Via de andere weg
aangezet en nagemeten:

```
Test guide: CVar set after mount -> panel OPEN (13 rows).
HUD: munitieteller zichtbaarheid=3 tekst='AR_Foundry   30 / 30' inViewport=1
```

Het paneel gaat open met dertien stappen, en de HUD tekent. Als de View-knop bij
jou nog steeds niets doet, ligt dat dus **aan de knop en niet aan de gids** — en
dan is de CommonUI-configuratie die ik vannacht heb gezet de eerstvolgende
verdachte om af te strepen.

## Lichtronde 1 ligt klaar om te beoordelen (26-07 laat)

Je vroeg om één review-ronde voordat er meer armaturen bijkomen. Die staat er:
zeven frames op 1920×1080 in `Eclipse/Saved/Screenshots/WindowsEditor/`
(HighresScreenshot00227 t/m 00233). Mijn eigen oordeel staat in
[phase0/LICHTRONDE_REVIEW.md](phase0/LICHTRONDE_REVIEW.md).

Kort: de cel-shading werkt, maar **de armaturen lezen niet als lampen** — ze
gloeien zonder lichtplas eronder, dus ze zien eruit als zelflichtende kratten. En
daardoor leest de helderheid-hiërarchie (extractie 16 / sites 10 / routes 6) op
commando-afstand niet: alle ambervlakken lijken even helder. Wat wél opvalt is de
gele schijf bij extractie, en die is met VORM gemaakt, niet met helderheid.

**CORRECTIE een uur later:** die ambervlakken waren de armaturen niet. Met een
isolatieronde (alleen de dertien armaturen zichtbaar, 423 andere actoren verborgen)
blijken ze kleine donkere dozen **zonder enige gloed** — terwijl alle dertien hun
mesh, basistextuur, emissive-masker en gain wél krijgen (16 / 10 / 6, precies de
bedoelde hiërarchie). Er is dus niets ontbrekend en niets verkeerd ingesteld; het
zit in de materiaalgraaf of het masker.

**EN DE OORZAAK IS GEVONDEN — er is niets kapot.** De armaturen zijn zo groot als
een blikje: de drie kleinste bakens meten **9 × 9 × 40 cm**, de routelampen 47–61 cm.
Jouw personage is 190 cm en het plein is 200 meter breed. Een baken van 9 cm op
50 meter afstand is ongeveer 2 pixels — precies de stipjes op het beeld. De
emissive, het masker en de materiaalgraaf zijn allemaal gemeten en in orde.

De vraag is dus niet meer "waarom lichten ze niet op" maar **"hoe groot hoort een
baken te zijn"** — en dat is jouw keuze. Op maat brengen zoals ik bij de
aankleedfiguren deed (iets van 2–4 m voor een plein van 200 m), of ze juist
dichterbij zetten in plaats van vergroten. Aan gain of materiaal hoeft niets meer
te gebeuren.

Twee vragen voor jou, allebei ontwerpkeuzes en geen bugs:
- Mag een armatuur licht **werpen** (een plas op de grond) in plaats van alleen te
  gloeien?
- Wil je de rangorde op vorm bouwen in plaats van alleen op helderheid?

Tot je erover hebt besloten komen er geen armaturen bij.

## Een vraag voor de save-spec (P2-06), gevonden op 27-07

De campagne-setup — de regiograaf, de missietabellen, de tuning — komt **niet** uit
het savebestand, en dat hoort ook niet: dat is geauthorde inhoud, geen
spelerstoestand. Maar er moet er wel één zijn tegen de tijd dat er geladen wordt,
en dat werd nergens gecontroleerd.

Zonder setup laadt een save keurig: de juiste dag, de juiste credits, de juiste
voortgang — en een strategische kaart waarop **geen enkele regio nog een missie
aanbiedt**. Gemeten in een verse instantie: alle zes leeg, zonder één regel in het
log. Nu zegt hij het hardop.

**SPEC-P2-06 IS NU GESCHREVEN** — als concept, in
[phase0/specs/SPEC-P2-06_save_system_v1.md](phase0/specs/SPEC-P2-06_save_system_v1.md).
Hij is falsificatie-eerst opgezet rond de claim die vannacht al één keer sneuvelde:
*een save die perfect herstelt kan de speler nog steeds met niets te doen
achterlaten.* Er staan drie review-punten in waar jouw oordeel nodig is (blokkeren
of waarschuwen bij een mismatch, hoeveel autosave-slots, hoe breed de content-hash).

De oorspronkelijke vraag hieronder is er besluit 1 van geworden: Dat maakt "verkeerde save bij
verkeerde build" detecteerbaar, maar het is een formaatwijziging (v5 → v6). De
goedkope variant is wat er nu staat: de laadkant gaat ervan uit dat het spel zijn
campagnedata al kent, en klaagt luid als dat niet zo is.

**Waarom ik die spec niet gebouwd heb:** stap 1 van zijn eigen bouwvolgorde ís de
schemawijziging, en daar hangt reviewpunt 1 direct aan. Wel gebouwd is stap 4,
`Eclipse.Save.Report` — het commando dat "waarom is mijn kaart leeg" in één regel
beantwoordt. Autosave (stap 2) heb ik bewust óók laten liggen: een autosave die
vrolijk schrijft terwijl laden de setup niet herstelt, vertelt je dat je
voortgang veilig is en geeft je daarna een leeg bord. De spec zet content-
identiteit niet voor niets vóór autosave.

---

# Wat er vannacht bij is gekomen (27-07) — vijf punten voor jou

## 1 · SPEC-P2-07 (UI Stack) ligt klaar als concept

[phase0/specs/SPEC-P2-07_ui_stack_v1.md](phase0/specs/SPEC-P2-07_ui_stack_v1.md).
Dit is de spec die de debug-UI vervangt door de echte CommonUI-stack: HUD,
command-interface, basis- en missieschermen, kaart, met pad en muis
gelijkwaardig.

De kern ervan is geen feature maar een **beperking die ik gemeten heb**: de
UI-laag is met géén enkele opnamemethode vast te leggen. Daardoor staat er in de
DoD met zoveel woorden dat "ziet er goed uit op een screenshot" een onhaalbaar
criterium is en niet geschreven mag worden — alles wordt tegen een state dump
gecontroleerd.

Drie punten waar jouw oordeel nodig is, elk met mijn aanbeveling:

| Vraag | Mijn voorstel |
|---|---|
| Hoe ver gaat "diegetisch leunend" (8.8) voor de HUD? | Munitie en trefteken schermvast houden — op 1920×1080 in een donkere wijk kost dat anders leesbaarheid. Alarmstaat en squadstatus wél diegetisch |
| Blijven F2/F3/H permanent of alleen tijdens ontwikkeling? | Permanent, achter de bestaande CVar. Ze kosten niets en zijn de enige UI die per ontwerp verifieerbaar is |
| Moet P2-07 op P2-06 wachten? | Nee. Alleen het save/load-scherm hangt eraan; de rest is onafhankelijk |

## 2 · De vierde bron: startbat genereren of een vertaaltabel?

Je klaagde er terecht over dat dezelfde knop op vier plekken anders beschreven
stond. Het knoppenschema is nu data, en drie van de vier bronnen hangen eraan.
De startbat nog niet, en daar zit een keuze in.

De **padknoppen** zijn wél hard te bewaken — RT/LT/LB/RB/A/B/X/Y heten in de bat
exact zoals in het schema. De **handelingen** niet: de bat schrijft "Soldaat
onder kruis" waar het schema "Onder kruis" zegt, en "HERLADEN" plus een losse
regel "Squad hergroeperen" tegen één schemaregel. Dat verschil is opzet — de bat
praat tegen een speler.

Een vertaaltabel zou een **vijfde** bron van waarheid zijn. En zonder handelingen
vangt de bewaker juist de fout van vandaag niet: LT was gekoppeld aan "vorige
soldaat" terwijl LT gewoon in het schema staat — de knop klopte, het páár niet.

**Keuze:** de bat genereren uit het schema (dan is de vriendelijke toon weg), of
de vertaaltabel accepteren. Tot je kiest laat ik het met de hand kloppen.

## 3 · Kijk of de bevrijdingszin aankomt

Haal je de **Foothold** (na Signal Fire), dan staat er nu een zin in het debrief
die uitlegt wáárom de wijk kantelde — niet alleen welke vakken omdraaiden. Hij
komt ná de vakregels, en dat is met opzet: een uitleg vóór de commit zou een
uitkomst aankondigen die nog afgewezen kon worden.

**Zeg het als die zin niet aankomt, of als hij te vroeg komt.**

## 4 · Wanneer telt een nacht als "groen"? (nieuw, 27-07)

`verify.ps1` schrijft nu elke draai weg in
[phase0/SOAK_LOG.md](phase0/SOAK_LOG.md), groen én rood. Daarmee wordt de eis van
SPEC-P2-05 — *soak drie nachten achtereen groen* — voor het eerst aantoonbaar.

Maar er zit een leesregel onder die ik niet in mijn eentje wil vastleggen, want
hij bepaalt wanneer een DoD-regel afgevinkt mag worden. Ik draai die bar tien
keer per sessie; midden in het werk is een rode draai gewoon normaal.

| Regel | Gevolg |
|---|---|
| **De LAATSTE volledige draai van een dag telt** *(mijn voorstel)* | Sluit aan bij wat "nightly soak" betekent: de stand aan het eind van de dag. Rode draaien tijdens het sleutelen tellen niet mee, maar staan er wel |
| Élke draai van die dag moet groen zijn | Strenger en niet te sjoemelen, maar dan haalt geen enkele werkdag het ooit — dan is de eis opnieuw onvervulbaar |

De data ondersteunt allebei: een draai met `-SkipShots` staat er met **0 opnames**
in, dus een volledige draai is herkenbaar zonder extra administratie.

## 5 · Nog steeds open van eerder

- De **View-knop** testen (ik kan een gamepad niet headless indrukken)
- Hoe groot een **baken** hoort te zijn (ze zijn 9 × 9 × 40 cm)
- Of er een **wapenmesh** aan het skelet moet
- De **stijlbotsing** tussen de blokkerige aankleedfiguren en jouw personage
- De drie **squad-stemregels** — dat kost betaalde generatie, dus dat vraag ik eerst
