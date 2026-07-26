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

*Twee correcties op mezelf. Rij 1 stond eerst als een SOM (22 hp × 6,67 schoten/s)
en niet als een meting, terwijl deze kop belooft dat alles gemeten is. Het harnas
meet de uitkomst rechtstreeks — 308 hp in 2 s aanhoudend vuur — en dat getal ligt
er nu. Een som ziet er net zo overtuigend uit als een meting en is het niet: hij
gaat uit van perfect vuren zonder een enkel gemist frame. En rij 2 noemde
**2,50 s** zonder te zeggen waar hij vandaan kwam; dat is het verschil van twee
gemeten momenten en dat staat er nu bij. Van de vijf vetgedrukte getallen in deze
drie audits is er één rechtstreeks gemeten, zijn er drie afgeleid met de stap
erbij, en het vijfde (0,000 s) komt uit het harnas.*

---

## Ronde 1 — 26-07-2026, 11:05

| # | Onderdeel | Referentie | Wij (gemeten) | Oordeel |
|---|---|---|---|---|
| 1 | **Time-to-kill, speler → vijand** | Division ~1–2 s op gelijk niveau; Gears ~0,5–1 s | **Gemeten: 308 hp in 2 s aanhoudend vuur** = 154 hp/s, dus 100 hp valt in **0,65 s**. Met kopschoten (×2,5) **0,26 s** | Keuze — zit in de band |
| 2 | **Time-to-kill, vijand → speler** | Vrijwel altijd trager dan andersom (de speler moet kunnen reageren) | **2,50 s** van vol naar neer — het verschil van twee gemeten momenten in de speelronde (eerste schade op 28,5 s, neer op 31,0 s) | Keuze — bijna 4× ruimer dan andersom, dat is gezond |
| 3 | **Kopschoten** | ×1,5 tot ×3 | **×2,50** gemeten (44 → 110 hp) | Was een omissie, **vandaag gerepareerd** |
| 4 | **Bevestiging dát je raakt** | Universeel: hitmarker, schadegetal, impactgeluid of alle drie | **Inslaggeluid sinds 26-07** (luider dan het schot, want dit is het signaal dat telt). Nog geen kruisje en geen getal | **Half — het oor weet het nu, het oog nog niet** |
| 5 | **Kopschot voelbaar anders** | Eigen hitmarker-vorm en/of eigen geluid | **Een kopschot klinkt 1,35× harder.** Een eigen cue zou beter zijn maar ligt er niet | **Half — hoorbaar, nog niet zichtbaar** |
| 6 | **Reactie van het slachtoffer** | Flinch-animatie | **Hit-reactie van 0,25 s**, gewicht 0,995 gemeten | Was een omissie, **vandaag gerepareerd** |
| 7 | **Reactie van de schutter** | Schiethouding / terugslag | **Schietpose van 0,12 s** ✓; **geen terugslag** | Half — zie 9 |
| 8 | **Spreiding** | Altijd iets: heupvuur ruimer dan mikken | **Geen.** Elk schot gaat exact naar het kruis | **Omissie** |
| 9 | **Terugslag** | Altijd iets, ook bij lage cadans | **Geen.** Het kruis beweegt niet bij vuren | **Omissie** |
| 10 | **Munitie en herladen** | Altijd | **Geen.** Oneindig vuur, geen magazijn | **Omissie, of bewust Fase 2+** |
| 11 | **Wapenwissel** | Altijd minstens twee wapens | **Eén wapen.** Daarom kreeg RB vandaag een andere taak | Keuze voor nu, met een plek gereserveerd |
| 12 | **Geluid bij een schot** | Altijd | **Klinkt sinds 26-07**, op de plek van het schot. De cue lag al in de repo en werd door niemand afgespeeld | **Was een omissie, dezelfde dag gerepareerd** |
| 13 | **Dood van een vijand** | Death-animatie of ragdoll | **Death-take speelt** ✓; geen ragdoll | Keuze — een take is voorspelbaarder dan physics |
| 14 | **Schade aan de speler zichtbaar** | Schermrand, richtingsindicator, geluid | Alleen de HUD-balk. **Maar de assets liggen er al**: `Screen_Damage_Indicator` bevat `WBP_DamageIndicator` en `T_BloodOverlay`, en geen enkele regel gameplay roept ze aan | **Omissie — en een verworven pack die dood ligt** |
| 15 | **Dekking** | Gears heeft een dekkingssysteem; Division/Borderlands niet | Geen systeem, wel dekkingsgeometrie in het district | Keuze — past bij Division/Borderlands |
| 16 | **Suppression** | Division en Gears hebben het | Geen. De wees-bark `Pinned` beloofde het en is 26-07 opgeruimd | Keuze — bewust niet, en de tekst liegt niet meer |

---

## De rode draad

**Vier van de vijf omissies gingen over FEEDBACK, niet over mechaniek.** Eén ervan
(12, wapengeluid) is dezelfde dag gerepareerd toen bleek dat het geen smaakvraag
was maar een dood asset: `Cue_SFX_Weapon_RebelRifle_Shot_01` lag al in de repo.
Daarna gold hetzelfde voor `Cue_SFX_Impact_BulletMetal_01`: die lag er ook, en
miste alleen een feit om aan te hangen — `ShotFired` vuurt immers óók bij een
misser. Met `Event.Combat.HitLanded` erbij klinkt een treffer nu, en een kopschot
1,35× harder. Wat er nóg dood ligt: twee voetstapcues, en die vragen
anim-notifies.

**De drie die overblijven horen bij elkaar.** Het gevecht rékent goed — de schade
klopt, de cadans klopt, kopschoten werken sinds vandaag, de vijand flincht, en je
hóórt het schot. Wat nog ontbreekt is de bevestiging dat je RAAKT: geen hitmarker
(4), geen kopschot-signaal (5), geen richting waar de klap vandaan kwam (14).

Dat is één samenhangend gat en geen drie losse klusjes. Het verklaart ook waarom
vechten leeg aanvoelde toen alles nog groen stond: elk systeem deed zijn werk in
stilte.

**Spreiding en terugslag (8, 9) zijn de tweede groep.** Een hitscan die exact naar
het kruis gaat, elke keer, op 6,67 schoten per seconde, is geen wapen maar een
laserpointer. Dit hoort bij dezelfde beslissing als de aim-snelheidsstraf uit de
locomotie-audit: allebei geven ze mikken een prijs.

## Wat ik zelf gedaan heb, en waar het ophoudt

**Gedaan:** 3 (kopschoten), 6 (hit-reactie), 7 (schietpose), 12 (wapengeluid), en de
helft van 4 en 5 (inslaggeluid, luider bij een kopschot).
Die vier hadden gemeen dat er geen smaakvraag in zat: de multiplier stond al in de
data, de takes lagen al in de packs, en de cue lag al in de repo. Ze waren dood,
niet onbeslist.

**Waar het ophoudt.** De rest vraagt een oordeel dat niet van mij is. Hoe groot en
hoe luid een hitmarker moet zijn is smaak; spreiding en terugslag veranderen elk
gevecht. Wel voorbereid: de hitmarker kan via de bestaande HUD en het bestaande
`Event.Combat.ShotFired`, dus daar is geen nieuw systeem voor nodig.

## Aanbeveling, in volgorde

1. **Hitmarker** — nu de kleinste stap die overblijft: je hóórt sinds vandaag dat
   je raakt, je ziet het nog niet. `Event.Combat.HitLanded` draagt al of het een
   kopschot was en hoeveel schade er landde, dus de HUD hoeft alleen te tekenen.
2. **Spreiding + terugslag** — samen met de aim-snelheidsstraf uit de
   locomotie-audit; het is één ontwerpvraag: *wat kost schieten?*
3. **Richtingsindicator bij schade** — en die is goedkoper dan hij lijkt: de pack
   `Screen_Damage_Indicator` staat al in het project met een kant-en-klare
   `WBP_DamageIndicator` en een bloedoverlay. Er is niets aangeroepen. Dit is dus
   geen inkoop maar aansluitwerk — het enige wat het van de owner vraagt is een
   blik op hóé het eruitziet, want dat kan ik niet beoordelen.
4. **Impactgeluid** — de cue ligt klaar, maar er is nog geen treffer-event om hem
   aan te hangen. Klein werk zodra dat er is.
