# Controller-feel: de waarden, met bron

*Onderzoeksronde 2026-07-25, op verzoek van de owner: "Ik wil geen gegokte getallen."*

Elke rij hieronder draagt een label:

- **[OFFICIEEL]** — uitspraak of broncode van de maker.
- **[GEMETEN]** — reverse-engineering door derden, reproduceerbaar, maar niet
  bevestigd door de maker.
- **[REDENERING]** — geen bron gevonden; gekozen op argument. **Deze mag je
  wantrouwen**, en ze staan er apart bij zodat je weet welke dat zijn.

De belangrijkste uitkomst van deze ronde is een correctie op mezelf: ik had
`StickResponseExponent = 2.0` in de code gezet met de comment *"2.0 is de
console-shooter default"*. **Dat is niet te onderbouwen.** Geen enkele bron noemt
2.0 als verscheept getal; het is forumfolklore. Wat wél gedocumenteerd is, staat
hieronder.

## De tabel

| Parameter | Waarde in ECLIPSE | Conventie-band | Bron | Waarom deze |
|---|---|---|---|---|
| Deadzone kijken (radiaal) | 0.18 | Halo capt zijn deadzone-sliders op 15%; Destiny 1 mat 24% cardinaal / 28% diagonaal | [GEMETEN] EternalDahaka, r/halo; r/DestinyTheGame | Tussen die twee in: ruim boven de gemeten drift, ruim onder Destiny's als traag ervaren 24% |
| Deadzone bewegen (radiaal) | 0.20 | idem, maar loopsticks krijgen in de praktijk meer | [REDENERING] | Jouw stick rust op LY = −0.048; een driftende loopstick draait met `bOrientRotationToMovement` ook je lichaam, dus die faalt zichtbaarder dan een kijkstick |
| Deadzone-vorm | radiaal | Halo: center-deadzone rond, axiale deadzone vierkant, samen "rounded square" | [GEMETEN] EternalDahaka | Puur radiaal: een vierkante zone maakt diagonaal lopen meetbaar anders dan recht (Destiny: 45° stick → 31° beweging) |
| Responscurve | machtscurve, exponent 2.0 | CoD's officiële taxonomie: Standard = machtscurve, Linear = recht, Dynamic = omgekeerde S | [OFFICIEEL] Activision blog | De vorm is officieel; het getal niet — zie hieronder |
| Exponent-waarde | 2.0 | enige gedocumenteerde implementatie gebruikt 3.0; Halo Infinite meet als ~kubisch, oudere Halo's als ~kwadratisch | [GEMETEN] r/CompetitiveHalo; open-source referentie | Kwadratisch = de oudere, als responsiever ervaren Halo-curve. 3.0 is wat Infinite doet, en dat is precies wat spelers traag noemen |
| Yaw bij volle uitslag | 160°/s | Halo Infinite ~157°/s onversneld, klassieke Halo ~240-250°/s, CoD-default ~250°/s, met versnelling tot 642-646°/s (klassiek) en 720°/s (Destiny-cap) | [GEMETEN] EternalDahaka; r/CompetitiveHalo | Zit op Infinite-niveau. **Dit is bewust de ondergrens van de band** — er is nog geen versnelling (zie open punten), dus dit is de topsnelheid en niet het startpunt |
| Pitch bij volle uitslag | 110°/s (0.69× yaw) | Destiny meet verticaal ≈ 60% van horizontaal; Halo ≈ 50% | [GEMETEN] beide EternalDahaka-analyses | Tussen beide in. Verticaal hoort trager: je pitch-bereik is 140° en je yaw-bereik oneindig |
| ADS-multiplier | 0.80 | Destiny laat 0.5–1.5 toe; Halo zet zoom-gevoeligheid per vergrotingstrap (1.4×–10×) | [OFFICIEEL] Bungie TWAB 2021-08-12; [OFFICIEEL] 343 settings-gids | Midden-conservatief: onze ADS is een FOV-versmalling van 0.80, dus de gevoeligheid volgt dezelfde factor en de hoeksnelheid op het scherm blijft gelijk |
| Muis | rauw, geen curve, geen deadzone | Valve verscheept `m_rawinput 1`, `m_customaccel 0`, `m_filter 0`; id verscheept `cl_mouseAccel 0` | [OFFICIEEL] Source SDK 2013 `in_mouse.cpp`; Quake III `cl_main.c` | Een muis heeft geen rustpositie en geen begrensde uitslag, dus geen deadzone en geen curve. Windows' eigen "Enhance pointer precision" legt er al een 3.2×–14× snelheidsafhankelijke gain overheen; die wil je juist omzeilen |
| Muisschaal | 1.0 op 0.022°/count | Quake (1996) `m_yaw 0.022`, Quake III idem, Source idem | [OFFICIEEL] id + Valve broncode | 0.022 is dertig jaar de de-facto standaard en de reden dat cm/360 tussen games vergelijkbaar is |

## Twee dingen die ik NIET heb gebouwd, en waarom

**Ramp-up (look-acceleration).** Halo en Destiny hebben het allebei: bij ~98% stick-
uitslag schiet de draaisnelheid omhoog (Halo klassiek ~2.5×, Infinite ~2.3×, en
Destiny haalt er 720°/s mee). Dat is precies waarom die games een lage basissnelheid
kunnen combineren met snel omdraaien. **Maar geen enkele bron noemt de ramp-tijd in
seconden** — de onderzoeker markeerde dat expliciet als het grootste gat. Ik bouw
liever niets dan een verzonnen 0.2s, dus dit staat open met een gemeten doel: eerst
kijken of jij het mist bij 160°/s.

**Aim-assist.** De twee vormen zijn magnetisme (het richtkruis wordt naar een doel
getrokken) en target-slowdown (de stick wordt trager over een doel). Voor een
squad-shooter op commando-afstand is **slowdown** de juiste: magnetisme vecht met je
hand als je langs een squadmate zwaait om een order te geven, slowdown niet. **Maar
de conehoeken en sterktes zijn nergens gepubliceerd** — Destiny's "Aim Assistance"
is een eenheidsloos investment-stat getal, Halo heeft alleen een aan/uit-toggle, en
voor Gears en The Division is er niets. Elk getal dat ik hier zou zetten is verzonnen.
Voorstel: bouwen zodra jij zegt dat je het mist, en dan met een schuifregelaar zodat
we het samen op jouw hand afstellen in plaats van op een geraden constante.

## bOrientRotationToMovement versus bUseControllerRotationYaw

ECLIPSE staat op `bOrientRotationToMovement` (lichaam draait naar zijn loopvector) en
dat blijft zo in hipfire: anders loopt het personage zijwaarts te schaatsen terwijl de
camera ergens anders kijkt. Gears en The Division verschillen hier, en het gangbare
patroon is **wisselen bij ADS**: tijdens het mikken draait het lichaam mee met de
camera zodat schouder, wapen en richtkruis één lijn vormen.

Dat is voor ECLIPSE **nog niet gebouwd**, bewust: de wissel moet samenvallen met een
aim-offset in de animatielaag, en die laag is vandaag pas als locomotie geland. Doe je
de wissel zonder aim-offset, dan klapt het lichaam bij het indrukken van LT naar de
camerarichting zonder dat de pose meebeweegt — dat leest als een bug, niet als mikken.

## Wat er uit deze ronde in de code is veranderd

1. De comment die 2.0 "de console-shooter default" noemde is vervangen door wat er
   werkelijk gedocumenteerd is. De waarde zelf blijft 2.0, nu met de juiste reden:
   kwadratisch is de oudere Halo-curve, en de kubische variant is wat spelers in
   Infinite traag noemen.
2. De ADS-multiplier op het kijken (0.80) is toegevoegd — die ontbrak: mikken
   versmalde wel de FOV maar liet de gevoeligheid staan, dus je richtkruis bewoog bij
   ADS sneller over het scherm dan daarbuiten.
