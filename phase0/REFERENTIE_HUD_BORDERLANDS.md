# REFERENTIE — de Borderlands-HUD, element voor element
*Werkdocument | 2026-07-31 | owner-instructie: "vind een foto van Borderlands van dat en baseer je daarop"*
*Eigenaar: `hud-builder` | Hoort bij owner-vraag O-8 en de stijl-lock O-6 = A*

---

## 0. Waar je de echte beelden vindt

**Kijk hier eerst, vóór je iets bouwt.** Dit zijn verzamelingen met echte in-game screenshots, geen artist impressions:

| Bron | Wat je er vindt |
|---|---|
| [Game UI Database — Borderlands](https://www.gameuidatabase.com/gameData.php?id=860) | Elk scherm apart, in hoge resolutie. De beste bron. |
| [Interface In Game — Borderlands 3](https://interfaceingame.com/games/borderlands-3) | HUD in context, tijdens gevecht |
| [Ghent Bailey — het officiële HUD-ontwerpproces van BL3](https://ghentbailey.artstation.com/projects/RYxk9D) | De *iteraties* van de ontwerpers, inclusief afgekeurde varianten en contrasttests |
| [Borderlands Wiki — User Interface](https://borderlands.fandom.com/wiki/User_Interface) | Wat elk element betekent |

De ArtStation-link is de waardevolste: daar staat hoe ze het hebben ontwikkeld en wat ze hebben weggegooid. Dat scheelt ons dezelfde fouten.

---

## 1. De vormtaal in vijf regels

1. **Dikke inktranden om alles.** Dezelfde lijn als de wereld gebruikt (§15.5). De HUD hoort bij de game, niet erbovenop.
2. **Schuine, hoekige panelen.** Bijna niets is recht of rond. Vlakken lopen taps toe of staan scheef.
3. **Enorme cijfers.** Munitie is het grootste getal op het scherm. Vet, hoekig, hoog contrast.
4. **Textuur in de vlakken.** Geen platte kleur — lichte vervuiling, alsof het gedrukt is.
5. **Alles in de hoeken, het midden vrij.** Het richtkruis staat alleen.

---

## 2. Element voor element — Borderlands, en wat het bij ons wordt

| Borderlands | Hoe het daar werkt | ECLIPSE-vertaling |
|---|---|---|
| **Schild** | Blauwe balk linksonder mét getal; leegt van rechts naar links | Wij hebben geen schild. Deze plek wordt **gezondheid**. |
| **Gezondheid** | Rood/oranje balk onder het schild | Linksonder, met de teamkleur van Eclipse in plaats van rood |
| **Munitie** | **Twee getallen**: magazijn + reserve. Rechtsonder, groot | Zelfde plek, zelfde twee getallen. `AR_Foundry 19 / 30` staat er al — nu de vorm eromheen. |
| **Action skill** | Icoon linksonder. **Wit = klaar**, grijs = aan het laden | Onze klasse-ability. Zelfde aan/uit-taal: fel = klaar, dof = wachten. |
| **Richtkruis** | **Verandert per wapentype**: rechtopstaand kruis voor enkelschot/burst, halfronde haken voor shotguns en launchers | Neem dit over. Het is gratis leesbaarheid: je ziet aan de vorm wat je vasthoudt. |
| **Minimap** | Rechtsboven, kompas-strook bovenaan | Rechtsboven; onze drie hoogtes vragen wel een eigen oplossing voor Command Mode |

---

## 3. De harde tegeneis — en die komt uit hun eigen proces

De BL3-ontwerpers deden **contrasttests** op hun vroege ontwerpen. Dat is geen detail: het is de erkenning dat een mooie HUD waardeloos is als je hem in het vuurgevecht niet leest.

**Onze eis, meetbaar:** je moet je munitie binnen een halve seconde kunnen aflezen **midden in een gevecht**. Toets dat op een screenshot tijdens gevecht, niet in een stilstaand menu. Een leesbaarheidstest op een leeg scherm bewijst niets.

Faalt die test, dan wint leesbaarheid van vorm. Altijd.

---

## 4. Bouwvolgorde

1. **Eerst compleet, dan mooi.** Alle elementen functioneel aanwezig (gezondheid, minimap, squad-kaarten, base- en map-lagen), daarna pas de stijlronde. Stijl op een incomplete HUD is werk dat je twee keer doet.
2. **Eén element als proef.** Doe de munitieteller eerst helemaal af in de Borderlands-taal, laat hem zien, en pas daarna dezelfde behandeling toe op de rest. Zo ontdek je een verkeerde richting bij één element in plaats van bij acht.
3. **Beide perspectieven apart.** Wat in third-person klopt kan in first-person half achter het wapen verdwijnen.

---

## 5. Wat de owner erover zei

> *"De gegevens die rond de speler staan op het scherm moeten veel mooier zijn — kijk naar afbeeldingen van Borderlands, haal daar inspiratie. Veel dingen zijn nog niet goed, maar misschien is dat logisch."*

Dat laatste klopt: de schermlaag is pas net als prioriteit 1 opgepakt. Hij kijkt naar iets halfaf en dat weet hij. Zijn opmerking is dus geen kritiek op het tempo maar een richting voor de stijlronde die nog moet komen.

De hoeveel-vraag staat als **O-8** op zijn dashboard: vol Borderlands, Borderlands-licht, of eerst laten werken.
