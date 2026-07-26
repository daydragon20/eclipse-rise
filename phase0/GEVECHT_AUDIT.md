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
| 11 | **Wapenwissel** | Altijd minstens twee wapens | **Correctie op mijn eigen melding van 26-07.** Er zijn **VIER** wapens in `DT_Weapons` — `AR_Foundry`, `Sidearm_Scrap`, `SMG_Patch`, `DMR_Longsight` — en de game mode rust je uit met `FirstRowOf`, dus **drie ervan zijn voor niemand bereikbaar** (geteld in het asset, niet in het setup-script: dat authordt er maar twee). En je drie loadouts (`DT_LoadoutOptions`) worden netjes opgesomd en gekozen, maar de keuze bereikt het wapen nooit: de loadout-rij draagt een tag, geen wapen | **Omissie in de bedrading, met een ontwerpvraag erin** |
| 12 | **Geluid bij een schot** | Altijd | **Klinkt sinds 26-07**, op de plek van het schot. De cue lag al in de repo en werd door niemand afgespeeld | **Was een omissie, dezelfde dag gerepareerd** |
| 13 | **Dood van een vijand** | Death-animatie of ragdoll | **Death-take speelt** ✓; geen ragdoll | Keuze — een take is voorspelbaarder dan physics |
| 14 | **Schade aan de speler zichtbaar** | Schermrand, richtingsindicator, geluid | Alleen de HUD-balk. **Maar de assets liggen er al**: `Screen_Damage_Indicator` bevat `WBP_DamageIndicator` en `T_BloodOverlay`, en geen enkele regel gameplay roept ze aan | **Omissie — en een verworven pack die dood ligt** |
| 15 | **Dekking** | Gears heeft een dekkingssysteem; Division/Borderlands niet | Geen systeem, wel dekkingsgeometrie in het district | Keuze — past bij Division/Borderlands |
| 16 | **Suppression** | Division en Gears hebben het | Geen. De wees-bark `Pinned` beloofde het en is 26-07 opgeruimd | Keuze — bewust niet, en de tekst liegt niet meer |

---

## Ronde 2 — 26-07-2026, avond

*Zelfde zestien punten, opnieuw langsgelopen na de avondronde (terugslag,
spreiding, magazijn, wapengeluid per familie, voetstappen per oppervlak). Alleen
de rijen die VERANDERD zijn staan hier; de rest staat onveranderd in ronde 1.*

| # | Onderdeel | Ronde 1 | Nu | Oordeel |
|---|---|---|---|---|
| 4 | **Bevestiging dát je raakt** | Alleen hoorbaar | **Hitmarker sinds vanmiddag** — een `+` dat 0,12 s oplicht | **Compleet** |
| 5 | **Kopschot voelbaar anders** | Alleen 1,35× harder | **Eigen vorm en kleur**: `×` in oranje tegen `+` in wit | **Compleet** |
| 7 | **Reactie van de schutter** | Pose ✓, terugslag ✗ | **Terugslag: 0,500° klim per schot, 0,000° over na 1 s rust** | Was half, **nu compleet** |
| 8 | **Spreiding** | Geen | **Heup 2,5° / mikken 0,6° / bewegen +1,5°**, met het eerste schot van een reeks zuiver | Was een omissie, **gerepareerd** |
| 9 | **Terugslag** | Geen | Zie 7 | Was een omissie, **gerepareerd** |
| 10 | **Munitie en herladen** | Geen | **Magazijn van 30, herlaadtijd 2,2 s, automatisch herladen bij leeg.** Vier foley-fasen op de herlaadbeurt, munitieteller rechtsonder | Was een omissie, **gerepareerd** |
| 11 | **Wapenwissel** | Vier wapens, drie onbereikbaar | **Onveranderd.** De wapens verschillen nu wél echt (negen assen plus een demper), maar de loadout-keuze bereikt het wapen nog steeds niet | **Nog steeds de grootste omissie in dit gevecht** |
| 12 | **Geluid bij een schot** | Eén cue | **Drie varianten per wapenfamilie plus een nagalm**, nooit twee dezelfde achter elkaar | Was gerepareerd, **nu compleet** |
| 14 | **Schade aan de speler zichtbaar** | Alleen de balk | **Richtingsindicator aangesloten** (het pack dat dood lag) | Was een omissie, **gerepareerd** |

**Nieuw punt dat ronde 1 niet had:**

| # | Onderdeel | Referentie | Wij (gemeten) | Oordeel |
|---|---|---|---|---|
| 17 | **Hoorbaar onderscheid tussen wapens** | Borderlands, Destiny en Battlefield geven elk wapentype een eigen stem | **Familie per wapen** (AssaultRifle / Handgun), drie schotvarianten elk. De sidearm is bovendien **gedempt**, met een alarmradius van 1200 tegen 5000 | Keuze — compleet voor twee families |
| 18 | **Voetstappen zeggen waar je bent** | Universeel oppervlakgebonden | **Beton op het plein, metaal op de dekkingsblokken**, zeven varianten per oppervlak | Was er niet, **vandaag gebouwd** |

---

## Ronde 3 — 26-07-2026, laat op de avond: de squad vecht mee

*Ronde 2 sloot af met "punt 11 is de laatste omissie". Die is dezelfde avond nog
gedicht (loadouts), en daarna kwam er iets bij dat elk getal in dit document
raakt: de squad vuurt sinds vanavond uit zichzelf.*

**Alle getallen in ronde 1 zijn gemeten met een squad die niets deed.** Dat staat
er nu bij, want zonder die zin liegen ze.

| # | Onderdeel | Ronde 1 (squad zweeg) | Nu (squad vecht mee) |
|---|---|---|---|
| 1 | **Time-to-kill, speler → vijand** | 100 hp valt in 0,65 s | **300 hp valt in 0,758 s.** Dezelfde 300 hp kost de speler alléén **2,108 s** |
| 11 | **Wapenwissel** | Vier wapens, drie onbereikbaar | **Gedicht.** Loadout geeft primair + sidearm, RB wisselt, elk slot houdt zijn magazijn |

**De squad maakt een gevecht 2,78× korter.** Dat is geen som maar een A/B: zelfde
opstelling, zelfde doelwit van 300 hp, één verschil — de doctrine. Onder `recon`
zwijgt de squad, onder `ready` vuurt hij mee. Dat de doctrine-laag die meting
mogelijk maakte, is een prettig neveneffect van in lagen bouwen.

### En wat het de squad kost

| | Gemeten in de verscheepte speelronde |
|---|---|
| Squadleden nog overeind | **3 van 3** |
| Squadleden neergegaan | **0** |

Vóór vanavond kon dit getal niet anders dan nul zijn: ze vochten niet mee. Nu
staan ze in het vuurgevecht en kunnen ze vallen — en in deze ronde vallen ze niet.

Dat heeft een neveneffect dat de moeite waard is: **de +20 voor een ronde zonder
gewonden is eindelijk te VERLIEZEN.** Tot vandaag was hij gratis, want er stond
niemand in de vuurlinie. Een bonus die je niet kunt mislopen is geen bonus.

### Wat dat betekent

| Vraag | Antwoord |
|---|---|
| Is het gevecht nu te makkelijk? | **Waarschijnlijk.** Een factor 2,78 is groot. Maar dit is precies het soort ding dat je moet VOELEN — ik kan meten dat het sneller is, niet of het lekker is |
| Moeten de vijanden sterker? | **Niet zomaar.** Het genre-antwoord is meer vijanden of betere posities, niet meer hp: een kogelspons voelt nooit goed. De vijandopstelling per missie bestaat sinds vanmorgen en is de knop die hierbij hoort |
| Is de speler nu overbodig? | **Nee.** Hij levert nog steeds ~36% van de schade in dit gevecht (2,108 tegen 0,758 betekent dat de squad ongeveer 1,8× de speler doet). En de doctrine is van hem |

### De rode draad na drie rondes

De zestien punten uit ronde 1 hadden zes omissies. **Er is er nul over.**

Wat ervoor in de plaats komt is geen lijst gaten meer maar een balansvraag, en
dat is een ander soort werk: van "dit ontbreekt" naar "dit is te sterk". Die
vraag kan ik niet in mijn eentje beantwoorden — hij hoort in een speelronde.

## De stand na ronde 2

**Van de zestien punten uit ronde 1 waren er zes een omissie. Er is er nog één
over, en het is dezelfde die er 's ochtends al uitsprong: punt 11.**

Vier wapens die nu echt verschillen — schade, tempo, magazijn, afval, drie
soorten spreiding, terugslag, stabiliteit, handling, kopschot, en of ze gedempt
zijn — en drie ervan kan niemand vasthouden. Elke as die ik vandaag heb
toegevoegd maakt dat gat *groter*, niet kleiner: hoe beter de wapens van elkaar
verschillen, hoe zonder betekent het dat je er maar één krijgt.

**Dat is dus de volgende taak, en hij staat al op de owner-lijst als punt 5
(loadouts).** Niet omdat de lijst hem noemt, maar omdat deze audit er twee
rondes achter elkaar op uitkomt.

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
4. **Impactgeluid** — gedaan op 26-07; deze regel stond hier nog van vóór die fix.
5. **Loadout laten meetellen.** Je kiest er een in de prep-fase en het verandert
   niets. Twee dingen zitten in de weg en ze zijn ongelijk van aard: de tweede
   wapenrij is puur bedrading (`FirstRowOf` pakt altijd de eerste), maar WELK
   wapen bij welke loadout hoort staat nergens — de rij draagt een tag en geen
   wapenverwijzing. Dat tweede is een ontwerpkeuze en daarom ligt dit bij jou.
