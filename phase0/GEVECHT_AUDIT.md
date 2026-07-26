# GEVECHTS-AUDIT TEGEN DE GENRE-STANDAARD

*Zelfde methode als [LOCOMOTIE_AUDIT.md](LOCOMOTIE_AUDIT.md), toegepast op het
gevecht — het gebied waar op 26-07 het meest aan veranderd is (kopschoten, het
schot-alarm, de hit-reactie, de schietpose). Per punt: wat doet de referentie, wat
doen wij, en is dat een KEUZE of een OMISSIE.*

**Waarom dit er is en niet gevraagd werd.** De owner-opdracht van 26-07 punt 9
vroeg een dagelijkse doorloop van de BEWEGING, met als reden: "ik ga deze kleine
dingen niet één voor één melden... als je Borderlands ernaast zet zie je het
meteen, maar geen enkele test valt erover." Dat argument geldt woordelijk voor het
gevecht, en daar zijn vandaag drie systemen bijgekomen. De hitmarker-bevinding
kwam uit precies deze vraag; de rest van de lijst hoorde erbij.

**Getallen.** Alles uit het harnas — `python Eclipse/Tools/show_measurements.py`.
Waar een referentie geen publiek cijfer heeft staat dat er, in plaats van een
verzonnen getal.

---

## Ronde 1 — 26-07-2026, 11:05

| # | Onderdeel | Referentie | Wij (gemeten) | Oordeel |
|---|---|---|---|---|
| 1 | **Time-to-kill, speler → vijand** | Division ~1–2 s op gelijk niveau; Gears ~0,5–1 s | 22 hp/schot op 6,67 schoten/s tegen 100 hp = **~0,6 s**; met kopschoten **~0,3 s** | Keuze — zit in de band |
| 2 | **Time-to-kill, vijand → speler** | Vrijwel altijd trager dan andersom (de speler moet kunnen reageren) | **2,50 s** van vol naar neer | Keuze — 4× ruimer dan andersom, dat is gezond |
| 3 | **Kopschoten** | ×1,5 tot ×3 | **×2,50** gemeten (44 → 110 hp) | Was een omissie, **vandaag gerepareerd** |
| 4 | **Bevestiging dát je raakt** | Universeel: hitmarker, schadegetal, impactgeluid of alle drie | **Niets.** Geen kruisje, geen getal, geen geluid | **Omissie — de zwaarste van deze lijst** |
| 5 | **Kopschot voelbaar anders** | Eigen hitmarker-vorm en/of eigen geluid | Alleen de schade verschilt | **Omissie** (volgt uit 4) |
| 6 | **Reactie van het slachtoffer** | Flinch-animatie | **Hit-reactie van 0,25 s**, gewicht 0,995 gemeten | Was een omissie, **vandaag gerepareerd** |
| 7 | **Reactie van de schutter** | Schiethouding / terugslag | **Schietpose van 0,12 s** ✓; **geen terugslag** | Half — zie 9 |
| 8 | **Spreiding** | Altijd iets: heupvuur ruimer dan mikken | **Geen.** Elk schot gaat exact naar het kruis | **Omissie** |
| 9 | **Terugslag** | Altijd iets, ook bij lage cadans | **Geen.** Het kruis beweegt niet bij vuren | **Omissie** |
| 10 | **Munitie en herladen** | Altijd | **Geen.** Oneindig vuur, geen magazijn | **Omissie, of bewust Fase 2+** |
| 11 | **Wapenwissel** | Altijd minstens twee wapens | **Eén wapen.** Daarom kreeg RB vandaag een andere taak | Keuze voor nu, met een plek gereserveerd |
| 12 | **Geluid bij een schot** | Altijd | Er is een audiolaag en een schot-event; **geen wapengeluid** | **Omissie** |
| 13 | **Dood van een vijand** | Death-animatie of ragdoll | **Death-take speelt** ✓; geen ragdoll | Keuze — een take is voorspelbaarder dan physics |
| 14 | **Schade aan de speler zichtbaar** | Schermrand, richtingsindicator, geluid | Alleen de HUD-balk; **geen richtingsindicator** | **Omissie** |
| 15 | **Dekking** | Gears heeft een dekkingssysteem; Division/Borderlands niet | Geen systeem, wel dekkingsgeometrie in het district | Keuze — past bij Division/Borderlands |
| 16 | **Suppression** | Division en Gears hebben het | Geen. De wees-bark `Pinned` beloofde het en is 26-07 opgeruimd | Keuze — bewust niet, en de tekst liegt niet meer |

---

## De rode draad

**Vier van de vijf omissies gaan over FEEDBACK, niet over mechaniek.** Het gevecht
rékent goed — de schade klopt, de cadans klopt, kopschoten werken sinds vandaag,
de vijand reageert. Wat ontbreekt is dat de speler het te horen of te zien krijgt:
geen hitmarker (4), geen kopschot-signaal (5), geen wapengeluid (12), geen
richting waar de klap vandaan kwam (14).

Dat is één samenhangend gat en geen vier losse klusjes. Het verklaart ook waarom
vechten leeg aanvoelde toen alles nog groen stond: elk systeem deed zijn werk in
stilte.

**Spreiding en terugslag (8, 9) zijn de tweede groep.** Een hitscan die exact naar
het kruis gaat, elke keer, op 6,67 schoten per seconde, is geen wapen maar een
laserpointer. Dit hoort bij dezelfde beslissing als de aim-snelheidsstraf uit de
locomotie-audit: allebei geven ze mikken een prijs.

## Wat ik zelf kan doen zonder beslissing

Niets van deze lijst zonder de owner. **Feedback is smaak-gevoelig** (hoe groot,
hoe luid, hoe lang) en **spreiding/terugslag verandert elk gevecht**. Wat ik wél
kan is de goedkoopste stap voorbereiden zodra hij ja zegt: de hitmarker gaat via
de bestaande HUD en het bestaande `Event.Combat.ShotFired`, dus daar is geen nieuw
systeem voor nodig.

## Aanbeveling, in volgorde

1. **Hitmarker** — kleinste ingreep, grootste verschil. Zonder bevestiging weet je
   niet of je mist of dat de vijand veel leven heeft.
2. **Wapengeluid** — de audiolaag en het schot-event bestaan allebei al.
3. **Spreiding + terugslag** — samen met de aim-snelheidsstraf; het is één
   ontwerpvraag ("wat kost schieten?").
4. **Richtingsindicator bij schade** — pas zinvol als je vaker geraakt wordt dan
   nu.
