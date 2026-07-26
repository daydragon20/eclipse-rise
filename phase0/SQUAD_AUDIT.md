# SQUAD-AUDIT TEGEN DE GENRE-STANDAARD

*Derde toepassing van de methode uit [LOCOMOTIE_AUDIT.md](LOCOMOTIE_AUDIT.md), nu
op de pijler waar de GDD het hardst over is: **orders zijn beloftes** (8.4). Per
punt: wat doet de referentie, wat doen wij, en is dat een KEUZE of een OMISSIE.*

**Referenties.** Mass Effect 2/3 en Ghost Recon (squad-commando met een pauze- of
vertraagmodus), The Division (AI-teamgenoten in co-op), XCOM (order-per-soldaat).
Gears heeft AI-teamgenoten zonder commandolaag en is hier maar half bruikbaar.

**Getallen.** Uit het harnas — `python Eclipse/Tools/show_measurements.py`.

---

## Ronde 1 — 26-07-2026, 11:45

| # | Onderdeel | Referentie | Wij (gemeten) | Oordeel |
|---|---|---|---|---|
| 1 | **Antwoord op een order** | Hoorbaar en zichtbaar, binnen een seconde | **0,000 s** — order en antwoord vallen in hetzelfde frame. Sinds vandaag ook **hoorbaar**: 8 ingesproken zinnen, rem 2 s per soldaat | Keuze. *Kanttekening:* 0,000 s betekent dat het R3-criterium "antwoord binnen 1 s" vandaag niet kán falen |
| 2 | **Een order wordt VOLGEHOUDEN** | Vanzelfsprekend: een order geldt tot je hem intrekt | Was **1 schot**, is sinds vandaag **14 in 2 s** | **Was een gebroken belofte, vandaag gerepareerd** |
| 3 | **Weigeren met een reden** | Zeldzaam in het genre; meestal doet de AI het gewoon | Vier redenen, alle vier bereikbaar, elk met een eigen zin | **Beter dan de referentie** — dit is een pijler, geen omissie |
| 4 | **Autonoom vuren zonder order** | Universeel: teamgenoten schieten uit zichzelf | **Niet.** Zonder FocusTarget vuurt niemand | **Vraag voor de owner** — drie extra schutters verandert elk gevecht |
| 5 | **Meelopen zonder order** | Universeel | **Niet.** `FollowDistance` staat in de data en wordt door niets gelezen | **Vraag voor de owner** — hangt aan 4 |
| 6 | **Dekking zoeken** | Division en Ghost Recon wel; Mass Effect deels | **Niet.** `CoverSearchRadius` is dood; het district heeft wél dekking | **Vraag voor de owner** |
| 7 | **Neergaan en overeind helpen** | Universeel in co-op-shooters | **Werkt.** Auto-triage dispatcht, `TryStabilizeSoldier` beslist op het venster van de HELPER, en `Event.Squad.SoldierStabilized` vuurt in de suite | Keuze — compleet |
| 8 | **Een gevallen soldaat antwoordt anders** | Zeldzaam | Eigen zinnenpool sinds vandaag: *"I'm hit - can't move."* | **Beter dan de referentie** |
| 9 | **Formatie** | Ghost Recon heeft formaties; Mass Effect niet | Geen | Keuze — past bij het Mass Effect-model |
| 10 | **Klasse-verbs** | Mass Effect: elke teamgenoot heeft eigen krachten | 1 van de 3 (Medic/Stabilize). Twee klassen doen niets bijzonders | **Omissie, staat al op de owner-lijst** |
| 11 | **Per-soldaat richten** | XCOM en Ghost Recon wel | Werkt, via Command Mode; selectie cycelt met RB | Keuze |
| 12 | **Stance / houding** | Ghost Recon: aggressive/hold fire | Wisselt de HUD-regel, verandert **geen gedrag** | **Omissie, staat al op de owner-lijst** |

---

## De rode draad

**De squad is sterk waar het genre zwak is, en zwak waar het genre sterk is.**

Waar wij vóórlopen: elke order krijgt een antwoord, weigeringen hebben een reden
én een eigen zin, en een gevallen soldaat zegt iets anders dan een geblokkeerde.
Dat is de pijler uit 8.4 en die staat.

Waar wij achterlopen: **de squad doet niets uit zichzelf.** Geen autonoom vuur,
geen meelopen, geen dekking. Punten 4, 5 en 6 zijn één vraag in drie vormen: *mag
je squad handelen zonder dat je het zegt?* In Mass Effect en Division is het
antwoord ja; het maakt teamgenoten tot medespelers in plaats van gereedschap.

**Dat is bewust een owner-vraag en niet iets wat ik stilletjes aanzet.** Drie
soldaten die uit zichzelf vuren is de grootste balansverschuiving die er op tafel
ligt — groter dan kopschoten of de vijandopstelling. En het raakt de identiteit
van het spel: een squad die alles zelf doet, maakt het commando-systeem
overbodig.

## Wat er vandaag al uit voortkwam

Punt 2 was een gebroken belofte en is gerepareerd. Dat onderscheid is de reden dat
deze audit bestaat: "de squad doet niets uit zichzelf" is een keuze, maar "de
squad doet niet wat je vroeg" is een defect — en die twee zaten in dezelfde
waarneming verstopt.

## Aanbeveling

1. **Beslis over 4/5/6 als één vraag**, niet los. Mijn advies: **autonoom vuren
   JA, meelopen JA, dekking zoeken nee.** De eerste twee maken je squad een
   medespeler; de derde vraagt een cover-systeem dat er niet is en dat je volgens
   punt 15 van de gevechts-audit ook niet wilt.
2. **Als 4 ja wordt, meet dan opnieuw wat een gevecht kost.** Drie extra schutters
   halveert de tijd waarin een groep vijanden valt, en alle getallen in de
   gevechts-audit zijn zonder hen gemeten.
