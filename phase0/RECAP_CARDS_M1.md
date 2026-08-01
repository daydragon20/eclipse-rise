# Recap Cards — opening van de slice (SPEC-P2-04 decision 1)

*Concept-draft 2026-07-25 (main-agent). Dit is de bron-copy die Taak "P2-04 stap 4" materialiseert
in `DA_CampaignSetup.RecapCards` (statische kaarten op de bestaande briefing-debug-surface;
geen cutscene-tech, geen dialoogsysteem). Getoond vóór M1.1's briefing, telkens tot M1.1
voltooid is. De 2.9-prologue (2 u) comprimeert naar deze 6 kaarten (spec staat 5–7 toe).*

*Game-taal = Engels (consistent met alle bestaande in-game strings). Elke kaart: still-frame-regie
+ 2–3 regels. Stills zijn Phase-2-graybox-waardig: een gestileerd stilstaand beeld uit de
bestaande district-assets/palet volstaat (15.5); geen nieuwe art-dependency.*

> **KAARTNUMMER ≠ SLOTNUMMER — ruling L1-R47 (02-08).** Dit document heeft **zes** kaarten.
> Het scriptbestand `Eclipse/Content/Script/act1/prologue/P0.S01_recap.yaml` heeft **zeven**
> ID-blokken, want blok 3 is de gereserveerde Enforcer-kaart die op owner-vraag **Q-2** wacht.
> Alles ná die reservering ligt daar dus één hoger. **Dit document wint**: kaartnummers zijn
> die van hieronder, en wat het scriptbestand heeft zijn **slots**.
>
> | kaart (hier) | slot (script) | | kaart (hier) | slot (script) |
> |---|---|---|---|---|
> | 1 De stad | 1 | | 4 Mara | 5 |
> | 2 De Tithe | 2 | | 5 De container | 6 |
> | 3 Petra | **4** | | 6 Ember Cell | 7 |
> | *(nog geen kaart — Q-2)* | *3, leeg* | | | |
>
> Waarom dit een bevinding was en geen muggenzifterij: het scriptbestand noemde zijn blokken
> tot 02-08 óók *"card N"*, en dan is *"repareer kaart 4"* Petra in het ene document en Mara
> in het andere. Wordt Q-2 met **ja** beantwoord, dan groeit dit document naar zeven kaarten
> en lopen beide nummeringen weer gelijk.

---

## Kaart 1 — De stad
**Still:** rantsoenrij in de schemer, natrium-oranje; een AEGIS-oog-decal boven de rij.
> Kessara. Your city, under Dominion curfew.
> You are Voss. You keep your head down. Everyone does.

## Kaart 2 — De Tithe
**Still:** het loterijbord van de Tithe of Hands; één naam oplicht; niemand kijkt elkaar aan.
> The Tithe of Hands — a labor lottery nobody wins.
> Last month it took your neighbor's son.
> He has not come back.

## Kaart 3 — Petra
**Still:** een opengebroken deur, een omgevallen stoel, een AEGIS-vlugschrift op tafel.
> Then AEGIS came for Aunt Petra. The notice said pre-compliance.
> No charge. No date. No office that answers.
> That was the day keeping your head down stopped working.

> **Regel 1 gewijzigd 02-08 door `story-architect`, ruling L1-R46. Dit is de enige wijziging
> in de goedgekeurde kopij en de bedoeling is onveranderd.** Er stond: *Then AEGIS came
> "pre-compliance" — and took Aunt Petra.* **Aanhalingstekens maken geen geluid.** Zeg de
> regel zoals TTS hem krijgt en *"came"* krijgt een complement dat het niet kan hebben; de
> leestekens droegen de hele syntaxis. Deze kaart is `credit_tier: 2`, de enige tag is
> `[quietly]` en de leveringsnotitie zegt *"flat, after the fact"* — vlak en zacht is precies
> de levering waarin een ongemarkeerd citaat verdwijnt. Zelfde klasse als L1-R36, en erger,
> want een koppelteken heeft tenminste nog een defaultlezing.
>
> **Wat behouden is, want dát is wat de owner goedkeurde:** AEGIS gebruikte dat eufemisme, en
> Petra werd meegenomen, zonder aanklacht. **Wat veranderde:** het eufemisme hangt nu aan een
> **document** in plaats van aan leestekens. Dat overleeft de spraakketen zonder acteerwerk,
> het haakt aan het vlugschrift dat al in de still ligt, en het is bureaucratischer in plaats
> van minder (`02_story_bible.md` §2.2). Koudlezervraag 3 rust volledig op deze kaart en wordt
> harder beantwoord dan eerst. Doorgevoerd in `P0.S01.100` in dezelfde beurt.

## Kaart 4 — Mara
**Still:** een handschoen schuift een gevouwen routekaart over een tafel; gezicht buiten beeld.
> A stranger found you before the sweeps did. Mara.
> She slipped you a route out of the cordon.
> You took it.

## Kaart 5 — De container
**Still:** binnenkant cargo-container; licht door de naden; twee ogen in het donker.
> Out of the city, folded into a cargo container.
> Breathing quietly through every checkpoint.

## Kaart 6 — Ember Cell
**Still:** de geothermische vault (Hollow Point); elf silhouetten rond een leiding-gloed.
> Hollow Point — a dead geothermal vault the Dominion forgot.
> Ember Cell: eleven people who still say no.
> Tonight, Mara says, the first real strike. Thirteen bullets to our name.

---

## Cold-reader-falsificatie (spec decision 1)

**Protocol:** iemand búiten het project leest uitsluitend de 6 kaarten (geen uitleg vooraf,
geen vervolgvragen) en beantwoordt de 4 vragen. Lat: **4/4 goed.** Fail → herschrijven +
opnieuw testen. De eerste R11-reviewersessie telt als deze check. Harde volgorde-regel:
deze check zweeft nooit voorbij de start van M1.2-authoring.

| # | Vraag | Verwacht antwoord (kern) | Gedekt door kaart |
|---|---|---|---|
| 1 | Who am I? | Voss — een gewone burger van Kessara, geen soldaat | 1 (+3) |
| 2 | Where am I? | Kessara, bezette stad; nu ondergedoken in Hollow Point, de vault van Ember Cell | 1, 6 |
| 3 | Why do I fight? | Tante Petra is zonder aanklacht opgepakt door AEGIS; het dagelijkse onrecht (Tithe, rantsoenen) | 2, 3 |
| 4 | Who is Mara? | De verzetsvrouw/leider van Ember Cell die me de stad uit smokkelde — mijn mentor | 4, 6 |

**Owner-actie (t.z.t. op het kliklijstje):** een cold reader vinden (~10 min) zodra de kaarten
op de briefing-surface staan. Antwoorden noteren; bij <4/4 herschrijft de agent en test opnieuw.

## Implementatienotities (voor de Taak-4-changeset)

- `FEclipseRecapCard { FName StillId; FText Lines; }` in een nieuw Quests-type-header of op het
  setup-asset; `TArray<FEclipseRecapCard> RecapCards` op `UEclipseCampaignSetupAsset` (soft-safe:
  lege array = geen recap, briefing direct — 14.3.5).
- Toon-conditie: recap zichtbaar zolang `Story.Beat.M11_ThirteenBullets` NIET in StoryFlags zit
  (state-derived, geen extra vlag) én de geselecteerde missie MT_M11 is.
- Debug-surface: bestaande briefing-scherm + `Eclipse.Story.Report` breidt uit met de recap-status.
- Stills: `StillId` verwijst naar een screenshot-asset onder `/Game/Art/Recap/` — kán in eerste
  iteratie de tekstkaart zonder beeld zijn (graybox-regel); de shot-rig kan de zes stills later
  deterministisch vangen.
