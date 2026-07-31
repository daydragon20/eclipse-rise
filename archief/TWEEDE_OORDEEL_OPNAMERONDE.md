# Tweede oordeel over de opnameronde — 27-07 09:01

Geschreven door een TWEEDE sessie (935a9b1a) die de werkboom verder niet heeft
aangeraakt. Dit is het enige bestand dat zij heeft geschreven. De hoofdsessie
(2267dc48) draaide door tijdens deze analyse.

Bekeken: stap1 (stilstaand), stap2 (lopend), stap3 (vurend), stap5 (isolatie),
stap9 (herladen), in `Eclipse/Saved/ShotBaseline/`.

---

## Bevestigd — de kernclaim klopt

Op stap5 staat de speler alleen in beeld: rechtop, volle hoogte, menselijke
verhoudingen, ongeveer de helft van de schermhoogte. Op stap1/stap3 staat hij
tussen de squad en is hij even groot als de anderen. Geen zwart beeld.

Het ontbrekende wapenmodel is bevestigd (stap3: lege handen terwijl er hulzen
ejecteren) maar was al bekend uit `d3bcaa1` en staat in het kliklijstje.

---

## VONDST — het loopmoment valt vóór de beweging

Pixelvergelijking van ALLEEN de achtergrond (bovenste strook 1280x200: portaal,
STOP-bord, skyline; geen personages in die strook):

| paar | pixels >16 veranderd | gemiddeld verschil |
|---|---|---|
| stap1 -> stap2 (eerste loopinterval) | **9,6 %** | 5,11 |
| stap2 -> stap3 (tweede loopinterval) | **46,3 %** | 24,28 |
| stap3 -> stap4 (uitlopend) | 21,6 % | 12,34 |

Beide loopintervallen duren 2,0 s met dezelfde invoer: `PlayShotTimer` loopt op
2,0 s (FirstDelay 5,0 s) en `DrivePlayShotInput` duwt `AddMovementInput` door op
50 Hz. Toch verschuift het uitzicht in het eerste interval VIJF KEER MINDER dan
in het tweede.

Het is geen loopanimatie-ter-plaatse: `bPlayShotWalking` stuurt daadwerkelijk
`AddMovementInput` aan (`EclipseGameMode.cpp:266-270`). De verplaatsing komt
alleen te laat op gang, en de opname met het label "tijdens lopen" (stap2) valt
daar nog voor. stap2 is in de praktijk bijna nog een stilstandframe.

### Waarom de ronde dit zelf niet ziet

De bewegingscontrole staat op `EclipseGameMode.cpp:369` en is goed bedacht — de
comment erboven beschrijft exact dit risico ("een speler die wel invoer krijgt
maar niet beweegt"). Maar hij draait alleen bij `ShotIndex == 3` en meet dus
uitsluitend 2->3, met een drempel van 50 cm. Dat is net het paar dat ruim
beweegt (46,3 %). Interval 1->2 wordt niet gecontroleerd — dus de ronde is groen.

Zelfde vorm als de fout uit `d3bcaa1`: de controle stond op het ene punt waar het
al goed ging.

### Voorstel, zonder nieuw systeem

- Log de camera-verplaatsing bij ELK moment, niet alleen bij moment 3.
- Assert ook op 1->2, OF schuif de loopopname een interval naar achteren zodat
  hij valt als de beweging er echt is.

---

## Smaakvraag voor de owner, geen bug

Alle lichamen zijn vlakke egale kleuren zonder kleding of uitrusting, terwijl de
omgeving wel tonale opbouw heeft. De gele figuur staat bovendien in een andere
stijl dan de gedetailleerde oranje/turkooizen lichamen (bolhoofd, wanthanden) —
de schaal daarvan is vannacht gerepareerd (328 -> 180 cm), de stijlbreuk niet.
Dat is een EEN-STIJL-WET-oordeel en dus van de owner.
