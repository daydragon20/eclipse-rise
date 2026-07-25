# In-game testgids — de game begeleidt de test, niet een document

*Owner-verzoek, 2026-07-25: "ik wil dit als info in de game als ik aan het spelen ben,
dat de game mij al begeleidt in hoe en wat ik moet doen en testen, en elke control
eerst uittest voordat het echt vrij wordt — een korte tutorial voor elk ding dat ik
moet testen."*

Dit vervangt de leesrol van `phase0/TESTROUTE_OBJECTIVES.md`: dat document blijft de
naslag en de bron van de verwachte waarden, maar tijdens het spelen hoort de gids in
het beeld te staan. Debug-tier per GDD 14.5, en bewust géén stap naar SPEC-P2-07
(UI Stack v1).

## 1. De ene ontwerpkeuze die jij moet maken

"Elke control eerst uittesten voordat het echt vrij wordt" kan op twee manieren, en
ze verschillen wezenlijk:

**A. Detecteren en aftikken (aanbeveling).** De gids vraagt één ding ("houd Q vast —
de wereld moet vertragen"), merkt via Enhanced Input dat je het deed, vinkt af en
gaat door. Alle controls blijven de hele tijd werken.
*Voordeel:* je kunt nooit vastlopen. Als een detectie faalt, speel je gewoon door en
sla je de stap over.
*Nadeel:* niets dwingt je de stappen in volgorde te doen.

**B. Hard vergrendelen tot de stap gehaald is.** Enhanced Input kan dat met
mapping-contexts en prioriteiten: alleen de te leren actie zit in het actieve
context.
*Voordeel:* de belofte "voordat het vrij wordt" wordt letterlijk waar.
*Risico dat ik niet wil verzwijgen:* als een detectie faalt — verkeerde
controller-binding, een actie die niet triggert in een edge case — **sta je stil in je
eigen game** en is de enige uitweg een console-commando. In een debug-gids die juist
bedoeld is om fouten te vinden, is dat het verkeerde faalgedrag.

**Voorstel: begin met A, en bouw B als losse schakelaar** (`Eclipse.Guide.Strict 1`)
zodra A bewijst dat de detectie per control betrouwbaar is. Dan krijg je de strenge
variant zonder het risico van een dode start.

## 2. Vorm

Eén paneel, gestapelde stappen, altijd zichtbaar tijdens een gids-sessie:

```
TESTGIDS  ·  stap 3/11                        [F2] verberg  [N] sla over
────────────────────────────────────────────────────────────────
✔ 1  Lopen            WASD / linkerstick
✔ 2  Rondkijken       muis / rechterstick
▶ 3  Sprint           houd Shift / L3  —  je moet merkbaar sneller gaan
  4  Hurken           Ctrl / B
  5  Vuren            LMB / RT
  6  Command Mode     houd Q / LB  —  wereld vertraagt naar 30%
  ...
```

De actieve stap staat vet met zijn verwachting erbij ("je moet X zien"), zodat je niet
alleen weet wélke toets maar ook waaraan je ziet dat het klopt. Afgevinkte stappen
schuiven naar boven en verkleinen — je houdt het overzicht zonder te scrollen.

## 3. De stappenlijst

**Deel 1 — controls (detecteerbaar):** lopen, rondkijken, sprint, hurken, vuren,
Command Mode intreden, soldaat wisselen (Tab/RB), onder-kruis-selectie (E/X), orders
1-4 / D-pad, stance (Alt/Y). Elk met zijn zichtbare verwachting.

**Deel 2 — systemen (bevestiging door jou, want dit is een oordeel):**
- Squad doet wat je vroeg → goed / niet goed
- Order-reactie voelt direct → goed / traag *(de wall-clock-meting uit de
  gauntlet-overlay levert hier het getal bij: X/10 binnen 1s)*
- Missie-objectives: één doel afvinken en de HUD zien meebewegen
- Debrief: de bonus zien uitkeren én zien fálen (iemand neer laten gaan)

**Deel 3 — de 13.2-vragen** als afsluiting, inclusief de poort "wil je vrijwillig een
tweede ronde".

De verwachte waarden komen uit `TESTROUTE_OBJECTIVES.md` en staan als asserts in de
M1.1-Gauntlets, dus de gids kan niets beweren wat de tests niet ook controleren.

## 4. Verhouding tot de gauntlet-overlay die al gebouwd wordt

De overlay levert de infrastructuur die deze gids nodig heeft en er is geen dubbel
werk: het paneel-frame, de actieve-device-detectie (welke kolom oplichten), de
wall-clock round-trip-meting, en het wegschrijven van een samenvatting naar
`Saved/Logs`. **De gids is een tweede changeset bovenop die overlay**, niet een
concurrent ervan — één widget, een extra modus.

Daarom eerst de overlay afmaken, dan dit. Anders bouw ik twee halve dingen.

## 5. Regels die blijven gelden

- Niet zichtbaar in shotrondes (`-EclipseShot`), anders staat de gids op elke
  review-shot.
- Geen tick-work in de hot path: de gids reageert op input- en bus-events, hij pollt
  niet.
- Standaard uit; aan via console-var of het gids-commando.
- Geen input stelen als het paneel dicht is.
