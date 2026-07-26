# LOCOMOTIE-AUDIT TEGEN DE GENRE-STANDAARD

*Opdracht van de owner, 26-07-2026, punt 9. Doel in zijn woorden: "je haalt jezelf
uit de rol van bugmelder." Dit is geen eenmalige lijst — hij hoort aan het eind van
elke werkdag opnieuw gedraaid te worden, want elke fix verandert wat de volgende
ronde vindt.*

**Methode.** Per punt: wat doet de referentie, wat doen wij, en is dat verschil een
KEUZE of een OMISSIE. Omissies worden gerepareerd. Keuzes gaan met een aanbeveling
naar de owner. Alle "wij"-getallen komen uit het harnas — `python
Eclipse/Tools/show_measurements.py <woord>` geeft ze vers; er staat hier geen enkel
getal dat niet gemeten is.

**Referenties.** Gears 5 (het enige spel in dit genre dat tuningcijfers publiceert,
TU3-patchnotes), The Division 1/2, Borderlands 2/3. Waar een referentie zwijgt staat
dat er expliciet bij in plaats van dat er een getal verschijnt.

---

## Ronde 1 — 26-07-2026, 10:30

| # | Onderdeel | Referentie | Wij (gemeten) | Oordeel |
|---|---|---|---|---|
| 1 | **Oriëntatie** | Lichaam volgt de CAMERA, beweegt daaromheen. Alle drie identiek | Camera-relatief sinds vandaag; achteruit duwen draait 0,0° (was 180°) | **Was een omissie, vandaag gerepareerd** |
| 2 | **Strafe-snelheid** | Gears 5 TU3: 1,00× vooruit (na een gemeten 0,739 die te sloom bleek) | 420 cm/s = 1,00× | Keuze, met bron |
| 3 | **Strafe-animatie** | Eigen zijcyclus per richting | 3 van de 9 lichamen hebben er een; de speler 3 van de 6 richtingen | **Omissie, deels onoplosbaar** — zie §Assets |
| 4 | **Backpedal-snelheid** | 0,70–0,85× | 357 cm/s = 0,850× | Keuze, met bron |
| 5 | **Draaien in stilstand** | Lichaam draait na ±90° mee, mét draai-animatie | Draait mee boven 90°, zónder animatie (de voeten schuiven kort) | **Half gerepareerd vandaag** — de drempel is er, de animatie niet |
| 6 | **Starten** | 0,2–0,4 s tot topsnelheid | 0,300 s | Keuze |
| 7 | **Stoppen** | Gears ~0,15 s, Division zwaarder ~0,3 s | 0,150 s over 27 cm | Keuze — bewust aan de Gears-kant |
| 8 | **180°-richtingswissel** | 0,3–0,5 s | 0,400 s, lichaam draait 180° | Keuze |
| 9 | **Sprint in** | Ramp van 0,1–0,2 s | 0,150 s | Keuze |
| 10 | **Sprint uit** | Meestal sneller dan erin | 0,042 s — **3,6× sneller dan erin** | Keuze, maar scherp: zie aanbeveling |
| 11 | **Springen** | — | 127,5 cm hoog; coyote 0,110 s; buffer 0,150 s | Keuze, met bron |
| 12 | **Landen** | Landingsdemping of -animatie | Camera-dip van **4,3 cm** bij een normale sprong, geschaald met de valsnelheid; komt exact terug op 65 | **Was een omissie, dezelfde dag gerepareerd** |
| 13 | **Hurken** | Snelheidsstraf + overgangsanimatie | 150 cm/s, capsule 124 cm, en sinds vandaag een overgang van 0,3 s op half gewicht (de hoofd-hitbox zakt mee) | **Was een omissie, dezelfde dag gerepareerd** |
| 14 | **Mikken tijdens bewegen** | Vrijwel altijd een snelheidsstraf (Division ~0,6×, Gears loopt je automatisch langzamer) | **Geen enkele straf** — je sprint even hard met je vizier op | **Omissie** |
| 15 | **Animatie per overgang** | Aparte take per overgang | Alleen idle/gang/klap/schot; geen start-, stop-, land- of draaitake | **Omissie, grotendeels assetwerk** |

---

## Wat ik hiervan zelf repareer

**Vandaag gedaan:** 1 (oriëntatie), 5 (drempel van 90°), 12 (landingsdip), 13
(hurk-overgang). Van de vijf omissies in ronde 1 zijn er dus vier weg; de vijfde
(14, mikken kost geen snelheid) vraagt een owner-beslissing.

**Wat ronde 1 daarmee leert over de methode:** twee van die vier waren pas
zichtbaar dóór een andere fix van diezelfde ochtend. Het oriëntatiemodel maakte
punt 5 zichtbaar (draaien in stilstand was geen vraag zolang het lichaam zijn
looprichting volgde), en het overlay-slot voor schot en klap maakte punt 13
goedkoop. Daarom is dit een dagelijkse ronde en geen lijst.

## Wat jouw beslissing vraagt

| # | Vraag | Mijn aanbeveling |
|---|---|---|
| 14 | **Kost mikken snelheid?** Vandaag niet. | **Doen, 0,6×.** Het is de standaard in dit genre en het geeft mikken een prijs — nu is er geen reden om níét altijd te mikken. Maar het raakt hoe elk gevecht speelt, dus jij beslist. |
| 10 | **Sprint uit in 0,042 s** — 3,6× sneller dan erin. | **Laten staan.** Snel uit een sprint kunnen is responsief; het omgekeerde zou je in gevechten laten doorschieten. Dit stond al op je lijst en de meting bevestigt dat het bewust asymmetrisch mag zijn. |
| 3 | **Vijf lichamen hebben geen zijcycli.** | **Assetwerk, geen tuning.** Zie hieronder. |

## Assets — waar aansluitwerk ophoudt

De vier richtingscycli zitten in **één** pack (SciFiCharacter). De lichamen komen uit
vijf verschillende packs. Gemeten over de negen verscheepte lichamen: `Rebel_A`,
`Veil` en `RadiantGuard` krijgen 6/6 richtingen, de **speler** (ParagonLtBelica) 3/6,
en vijf lichamen 0/6. Er is een kruislingse terugval ingebouwd (mist een gang zijn
zijcyclus, dan wordt die van de andere gang gebruikt), maar voor de vijf kale
lichamen bestaat er niets om op terug te vallen: die moonwalken bij achteruit en
strafen, en dat blijft zo tot er cycli voor hun skelet zijn.

Datzelfde geldt voor de draai-animatie: **geen enkele pack heeft een `Turn_*`-take.**
Zolang die er niet is, is punt 5 half — de drempel voorkomt het ergste, de voetslip
blijft.
