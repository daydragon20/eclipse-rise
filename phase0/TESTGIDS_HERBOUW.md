# DE F3-GIDS, HERBOUWD OP ÉÉN REGEL

*Owner-opdracht 26-07 avond. De regel staat er woordelijk:*

> **de gids mag alleen bevatten wat JIJ NIET KUNT METEN.** Kun je het met het
> harnas of de speelronde vaststellen, dan test jij het en komt het niet in de
> gids.

*En daarbij: de gids moet zichzelf vullen met wat er NIEUW is sinds de vorige
sessie, één kleine stap tegelijk, alles zelf registreren, één kopieerbare
samenvatting aan het eind. **De LENGTE is het signaal** — een lange gids betekent
dat ik te veel naar de owner schuif. "Als er niets voor mij is: zeg dat dan ook."*

---

## Wat er nu in staat, en wat de regel ermee doet

De gids heeft 23 stappen in drie delen. Langs de regel gelegd:

| Deel | Nu | Meetbaar? | Oordeel |
|---|---|---|---|
| **1 — controls** | 14 stappen, één per rij van de controletabel | **Ja, alle veertien.** Dat de binding bestaat wordt getest (`ControlTableClaimsOnlyBindingsThatExist`), en het effect is gemeten: snelheden, springhoogte, FOV, coyote-venster | **Eruit** — op één uitzondering na, zie hieronder |
| **2 — systemen** | 4 stappen | Drie van de vier: "squad doet wat je vroeg", "objective vinkt af", "debrief betaalt" staan alle drie als assert in de suite | **Drie eruit, één blijft** |
| **3 — 13.2-vragen** | 5 vragen | **Nee, geen enkele.** Dit zijn oordelen | **Blijft, ongewijzigd** |

Dat is 23 → 6. En dat is precies wat de owner met "de lengte is het signaal"
bedoelt: van de 23 stappen waren er 17 dingen die ik zelf had moeten
vaststellen.

## De uitzondering, en waarom hij geen uitzondering is

Deel 1 verdwijnt niet helemaal. Wat blijft is niet "leer de besturing" maar:

> **wat is er VERANDERD sinds jij voor het laatst speelde?**

Dat is geen meting. Ik kan meten dát RB sinds vanavond van wapen wisselt; ik kan
niet meten of de owner dat wéét. Een control die van betekenis is veranderd
terwijl hij weg was, is het enige wat een gids over besturing hoort te zeggen —
en het is ook wat hij vroeg ("vult zichzelf met wat er nieuw is sinds mijn vorige
sessie").

Op 26-07 zou die lijst dit zijn, en niet meer dan dit:

- **RB** buiten Command Mode: was camerastandpunt, is nu **wapenwissel**
- **X / R** buiten Command Mode: was hergroepeer-order, is nu **herladen**
  (en valt terug op de order als je magazijn vol is)
- **1e/3e persoon** heeft geen padknop meer — **C** op het toetsenbord

Drie regels. Volgende sessie zijn het er misschien nul, en dan hoort er "er is
niets veranderd" te staan.

## Hoe "sinds je vorige sessie" werkt

Het eindrapport gaat al naar `Saved/Logs`. Dat bestand is dus de markering: de
gids leest het nieuwste rapport, pakt zijn datum, en toont alleen wijzigingen die
daarna zijn geland. Geen rapport gevonden = eerste sessie = toon alles.

De wijzigingen zelf staan als tabel in de gidscode, met een datum per regel.
Eén regel per landing, geschreven op het moment van landen — niet achteraf
gereconstrueerd, want dan staat er wat ik me herinner in plaats van wat er
gebeurde.

## De vier systeemstappen, langsgelopen

| Stap | Blijft? | Waarom |
|---|---|---|
| Squad doet wat je vroeg | **Nee** | `Eclipse.Squad.*` meet het antwoord én de beweging. Gemeten: 2 van 3 binnen 2,5 s |
| Order-reactie voelt direct | **Ja** | De meting zegt altijd ~0 s (vraag en antwoord vallen in hetzelfde frame) en bewijst daarmee niets. Of het antwoord OPVALT is een oordeel |
| Objective vinkt af | **Nee** | Staat als assert in `M11GauntletOnShippedData` |
| Debrief betaalt en faalt | **Nee** | Bedragen en dagovergang zijn gemeten; de +20-bonus is sinds vanmiddag ook gemeten |

## De gids na de herbouw

```
WAT ER VERANDERD IS SINDS JE VORIGE SESSIE   (0-3 regels, meestal 0)
DEEL 1 — het enige systeemoordeel            (1 stap)
DEEL 2 — de vijf 13.2-vragen                 (5 stappen)
SAMENVATTING                                  (kopieerbaar, naar Saved/Logs)
```

Zes stappen in plaats van 23. Als er niets veranderd is: **vijf**, plus een regel
die zegt dat er niets voor hem klaarligt.

## Status

Ontwerp vastgelegd 26-07 avond. Nog niet gebouwd — de build stond vast op een
open editor (Live Coding) terwijl de owner speelde, en dit is code, geen tekst.
Bouwvolgorde: eerst loadouts groen krijgen, dan dit.
