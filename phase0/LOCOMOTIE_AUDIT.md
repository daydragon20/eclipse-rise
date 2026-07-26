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

## Ronde 2 — 26-07-2026, avond

*Alleen de rijen die veranderd zijn. De rest staat onveranderd in ronde 1.*

| # | Onderdeel | Ronde 1 | Nu | Oordeel |
|---|---|---|---|---|
| 5 | **Draaien in stilstand** | Drempel er, animatie niet | **Correctie op mezelf: de animatie BESTAAT.** Ik keek in SciFiCharacter, maar de speler is Belica — en ParagonLtBelica levert `Idle_Turn_90_Left/Right`, `Idle_Turn_180_Left/Right` en drie `TurnInPlace`-varianten | **Niet geblokkeerd. Ik had het ten onrechte doodverklaard** |
| 14 | **Mikken tijdens bewegen** | Geen straf | **145 cm/s als PLAFOND**, niet als factor. Trager dan wandelen (180), zoals de owner besliste | Was een omissie, **gerepareerd** |
| 15 | **Animatie per overgang** | Alleen idle/gang/klap/schot | Sinds vandaag ook **hurkovergang** en **herladen**; nog steeds geen start-, stop- of landtake | Half — en de draaitake is nu wél bereikbaar (zie 5) |

**Nieuw punt dat ronde 1 niet had:**

| # | Onderdeel | Referentie | Wij (gemeten) | Oordeel |
|---|---|---|---|---|
| 16 | **Hoor je waar je loopt?** | Universeel oppervlakgebonden voetstappen | **Beton op het plein, metaal op de dekkingsblokken**, zeven varianten per oppervlak. Er was geen enkel oppervlaktetype in het project | Was er niet, **vandaag gebouwd** |

---

### Ronde 2, aanvulling: punt 3 is ook weg

| # | Onderdeel | Ronde 1 | Nu |
|---|---|---|---|
| 3 | **Strafe-animatie** | 3 van de 9 lichamen hebben er een | **9 van de 9.** De validator eist het nu, en hij staat groen |

**Ik had dit ten onrechte assetwerk genoemd.** De reden die eronder lag was een
comment in het setup-script:

> "MESH AND ANIMS MUST COME FROM THE SAME PATH. Every pack ships its OWN COPY of
> UE4_Mannequin_Skeleton ... a different USkeleton asset"

Dat is waar op ASSETNIVEAU en het is de verkeerde conclusie. Nagemeten: alle acht
de packs dragen een kopie van hetzelfde `UE4_Mannequin_Skeleton` — verschillende
assets, dezelfde botten. Daar heeft UE5 `CompatibleSkeletons` voor. Vijf
koppelingen (`Tools/link_compatible_skeletons.py`) en de anim-arme packs mogen de
takes van een donor afspelen.

Donoren zijn expliciet, niet geraden: een soldaat leent van een soldaat, een
warrior van een warrior. Een loopcyclus draagt de houding van het personage, dus
een zwaargepantserde pas onder een slank lichaam valt op.

**En de meting vond meteen mijn eigen te strenge validator.** Eerste versie eiste
zowel een WANDEL- als een RENcyclus per richting, en zette daarmee de SPELER rood:
Belica levert alleen jog-takes en helemaal geen wandelcyclus. De runtime vult dat
al aan (`FillFrom` laat Walk terugvallen op Run en andersom), dus de validator
eiste data die de code met opzet zelf invult. Nu eist hij per RICHTING dat er in
minstens één tempo iets staat — dat is de regel die de code echt volgt.

### Ronde 2, tweede aanvulling: de speler mist een herlaadpose

| # | Onderdeel | Nu |
|---|---|---|
| 15 | **Animatie per overgang** | Herladen is er als SYSTEEM (veld, pipeline, pose-aanroep) maar **de speler heeft de take niet**: ParagonLtBelica levert nul reload-animaties. Herladen is dus hoorbaar (vier foley-fasen) en leesbaar (de HUD zegt HERLADEN) maar niet zichtbaar |

**Lenen kan hier niet.** Het compatibele-skelettentrucje van vanavond werkt tussen
de SciFi-packs omdat die allemaal een kopie van `UE4_Mannequin_Skeleton` dragen.
Belica draagt haar eigen `Belica_Skeleton` — andere botten, dus retargeting en
geen koppeling.

**En dat maakt het een RUIL, geen defect.** Belica heeft de draaitakes die
SciFiCharacter niet heeft; SciFiCharacter heeft de herlaad- en wandeltakes die
Belica niet heeft. Welke van de twee de speler is, is een owner-keuze en staat in
het kliklijstje. Mijn advies: laten staan tot hij gespeeld heeft — draaien zie je
constant, herladen twee keer per gevecht.

**Wat er wel veranderd is:** een ontbrekende eenmalige pose meldt zich nu. Hij
degradeerde stil, wat betekende dat je het pas merkte door ernaar te kijken. Eén
regel per lichaam, want een schietpose valt 6,67 keer per seconde.

## De stand na ronde 2

Van de vier omissies uit ronde 1 zijn er twee weg (14 en deels 15), één is een
**fout van mij gebleken** (5 — de animatie was er wel), en één is nog steeds
assetwerk (3: vijf lichamen zonder zijcycli).

**De les uit rij 5 is groter dan de rij zelf.** Ik verklaarde een taak dood op
grond van een zoekopdracht in de verkeerde map, schreef dat als comment in de
kijkcode ("nagekeken: geen enkele Turn_*-take"), en die comment werd daarna de
reden dat de drempel op 90 graden bleef staan. Een verkeerde bevinding die zich
vastzet in een comment is erger dan geen bevinding: hij ziet eruit als iets wat
al onderzocht is.

**Wat ik daaraan verander:** een "bestaat niet"-conclusie hoort te zeggen WAAR ik
gekeken heb. "Geen Turn_*-take in SciFiCharacter" was waar geweest; "geen enkele
Turn_*-take" was het niet.

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

---

## Ronde 3 — 26-07-2026, laat

**Waarom opnieuw.** Sinds ronde 2 is er veel veranderd dat locomotie raakt: de
additieve poses (een idle-take die als volledige pose het lichaam liet inklappen),
de compatibele skeletten (2948 stil afgewezen animaties naar 0), en turn-in-place
dat daadwerkelijk is aangesloten. Deze ronde meet wat daarvan is blijven staan.

**Nieuw deze ronde: er is voor het eerst naar het SCHERM gekeken.** De vorige twee
rondes staan volledig op getallen. Dat is precies de kritiek die de owner op de
verificatie gaf, en punt 5 hieronder is er niet los van te zien.

| # | Onderdeel | Ronde 2 | Nu (gemeten) | Oordeel |
|---|---|---|---|---|
| 5 | **Draaien in stilstand** | Drempel er, take gevonden maar niet aangesloten | Drempel 60°, take resolvet, en op beeld: camera 118,4° / lichaam 118,4° / verschil 0,0°. De speler heeft de draaipose (`Idle_Turn_90_Right`, **volledige pose**, niet additief) | **Gesloten voor de speler** — met een kanttekening, zie hieronder |
| 15 | **Animatie per overgang** | Idle/gang/klap/schot + hurkovergang + herladen | Ongewijzigd. Nog steeds geen start-, stop- of landtake | Half, ongewijzigd |

### De kanttekening bij punt 5: de take duurt 4,00 seconden

`Idle_Turn_90_Right` is **4,00 s** lang, en de pose wordt precies zo lang
aangehouden — `PlayOneShotPose` neemt de lengte van de take zelf. Het lichaam is
zijn draai in een fractie daarvan rond (de draaisnelheid staat op 500°/s), dus de
draaipose blijft daarna nog seconden op 0,85 gewicht staan, óók als je alweer loopt.

De referentie zit op **0,4–0,8 s** voor een kwartslag-draai in stilstand (Gears,
The Division). Vier seconden is daar een veelvoud van, en dat is geen tuningkeuze
maar een eigenschap van een Paragon-take: die dragen een lange nasleep waarin het
personage uitzakt naar idle.

**Ik heb dit NIET gerepareerd, en dat is bewust.** Er zijn twee wegen — de take
afkappen op een genre-waarde, of hem koppelen aan het moment dat het lichaam
klaar is met draaien — en welke goed is, hangt af van waar in die 4,00 s de draai
zelf zit. Dat heb ik niet gemeten, en een getal kiezen zonder die meting is
precies de fout die deze audit moet vinden. Volgende ronde: bemonster het gewicht
en de bothoek over de duur van de take, en bepaal daaruit het afkappunt.

### Punt 3 en de draaitake: de stand per lichaam

| Lichaam | Poses | Wat ontbreekt |
|---|---|---|
| **Player** (Belica) | 3/5 | herladen, hurken |
| Rebel_A, Rebel_B, Rebel_C | 4/5 | draaien |
| Enforcer, Trooper, Shock | 4/5 | draaien |
| Veil, RadiantGuard | 4/5 | draaien |

**Correctie op de vorige regel van dit document.** Onderaan stond: *"geen enkele
pack heeft een `Turn_*`-take"*. Dat is de fout die in ronde 2 al één keer is
rechtgezet en die hier bleef staan — ParagonLtBelica levert ze wél, en de speler
gebruikt er nu één. Wat klopt is de beperktere vorm: **de acht niet-speler-lichamen
hebben geen draaitake**, want hun packs leveren er geen.

Voor die acht is dat minder erg dan het klinkt: turn-in-place is een
speler-ervaring. Een squadmate die je van opzij ziet bijdraaien valt nauwelijks op,
en een vijand al helemaal niet. Het staat hier omdat een "9/9" ergens anders in dit
document anders zou suggereren dat dit ook rond is.

**De speler mist herladen en hurken**, en dat is de scherpste van de tien: herladen
duurt 2,2 s met vier foley-fasen eronder, dus je hóórt een handeling die je niet
ziet.

