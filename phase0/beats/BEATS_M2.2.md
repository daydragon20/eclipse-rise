# BEAT-SHEET — M2.2 *The Strike*
*L1 | story-architect | 2026-08-02 | act 2, beweging I*
*Canon: `02_story_bible.md` §2.9 (*"winning the Krad-9 miners' strike (or arming it — moral fork)"*), §2.5 (Brick), §2.7 (*Allies With Teeth*, *The Cost of Speed*) · `03_world_design.md` §3.3 (KRAD-9), productieregel 4 · `11_missions.md` §11.2 (strike protection, tunnel warfare, breach/pressure)*
*Erft: `ACT2_OVERVIEW.md` §7 (Krad-9-register), §8 (AR-A2-10, AR-A2-11) · `ACT1_OVERVIEW.md` AR-11 · `RULINGS_L1.md` L1-R14, L1-R17*

---

## 1. Dramatische functie

Bijbel: *"winning the Krad-9 miners' strike (or arming it — moral fork)."* `03_world_design.md` §3.3 zegt wat de vork oplevert: *"the miners' strike arc decides if the player gains it as ally (sustainable, slower) or armed uprising (fast, bloodier, Dominion reprisals)."*

> **De missie waarin Voss een overwinning krijgt aangeboden die niet van hem is — en waarin de snelste manier om te helpen de manier is die de mensen die hij helpt kapotmaakt.**

Krad-9 is niet Kessara en niet Tarsis. `03_world_design.md` §3.3: *"~15M pit-clans. Union-brotherhood culture forged by shared air: 'You breathe what your crew breathes.'"* Deze mensen hebben al een beweging. Ze zijn al georganiseerd. Ze staken al. **Eclipse arriveert als de vreemde met de wapens**, en dat is precies het gevaarlijkste dat een rebellie kan zijn voor een vakbond.

Dat is de morele kern en beide kanten zijn in één zin verdedigbaar (§2.7 regel 2):
- **Winnen** (de staking beschermen tot de bedrijfsbeveiliging opgeeft): duurzaam, en het kost weken die de Dominion krijgt om de Armada te sturen.
- **Bewapenen** (de mijnwerkers gewapend laten opstaan): morgen vrij, en de eerste represaille valt op mensen die geen keuze hadden.

**En dit is Bricks thuiswereld.** AR-11: de Tithe of Hands haalde hem hier weg. Hij is niet teruggegaan. Nu wel.

---

## 2. Cast

| Wie | Waar |
|---|---|
| **VOSS** | overal |
| **BRICK** | overal. **Zijn missie**, en hij spreekt nog steeds als laatste |
| **SELA** | mee — een vakbond is haar taal, en het is de eerste keer dat ze mensen ontmoet die haar niet nodig hebben |
| **DEX** | Hollow Point, radio. Hij wil de ertsstroom en zegt dat te vaak |
| **REYES** | Hollow Point, radio — één keer, over drukletsel |
| Een **stakingsleider** | S03, S04, S05, S99 — de tegenspeler. **Fingerprint en casting ontbreken (C-A2-2 / Q-A2-3)** |
| Pit-clanleden | rolsprekers `CIVILIAN_A`/`_B`; mijnwerkers, geen burgers |
| Bedrijfsbeveiliging | barks — **een vijfde vocabulaire, en het goedkoopste van allemaal: bewakers, geen soldaten** |
| Dominion tunnelspecialisten | alleen in de `armed`-tak, S06 |
| Eclipse-vechters | `FIGHTER_A`/`_B` |

**Spreekt niet:** Kaya, Torren, Whisper, Threx, Kaine, Petra, Mara (ook niet als opname — de drie adressen liggen vast in AR-A2-5 en dit is er geen van).

---

## 3. Site & runtime-haak

- **Fictie:** een luchtloze planetoïde, doorboord tot hij hol is. Buiten: vacuüm, harde schaduwen, 0,4 g. Binnen: onder druk staande tunnelsteden en **ertskathedralen** — uitgemijnde holtes zo groot dat er weer is. Elke breuk is een natuurkundig probleem: uitstromende atmosfeer is zowel wapen als bedreiging.
- **`location`-strings** (`ACT2_OVERVIEW` §7): `Kessara / Underworks / Hollow Point — Map Table` · `Krad-9 / Surface` · `Krad-9 / Tunnel City` · `Krad-9 / Tunnel City — Ore Cathedral` · `Krad-9 / Deep Shaft`
- **Regio-pin:** **bestaat niet** — C-A2-3.
- **Runtime:** *Strike protection* (§11.2 Defense) met een breach/pressure-laag. Objectives: `ReachLocation` → `DefendArea` (de kathedraal) → keuzefase → tak-afhankelijke uitvoering. Optional: nul mijnwerkersdoden; geen enkele drukbreuk in bewoond volume.
- **Nieuw:** lage zwaartekracht en drukbeheer. Systeemtaak, C-A2-3. **Geen scène doet alsof het bestaat** — de dialoog beschrijft lucht en gewicht, ze vraagt er geen mechanica voor.

---

## 4. Scènelijst

### S01 — *Ore Heart* · briefing
`Kessara / Underworks / Hollow Point — Map Table` · cutscene · tier 5
**Aanwezig:** Voss, Dex, Sela, Brick, Reyes

- **want** — Dex wants Krad-9's ore, and he wants it before anyone asks what it costs.
- **obstacle** — There is already a movement on Krad-9 and it did not ask for help.
- **turn** — Brick says he is going, which is not the same as agreeing.

**Beats:** (1) Dex' argument is strategisch en correct: wie Krad-9 heeft, produceert de ander op termijn kapot (§3.3 letterlijk). (2) Sela's tegenwerping: een staking die door buitenstaanders gewonnen wordt, is geen staking meer. (3) **Bricks eerste regel van de missie, en hij spreekt als laatste** (§18.4: *never speaks first in a group*). Hij zegt niet dat hij daarvandaan komt; iemand anders zegt het. (4) Voss besluit te gaan zonder te besluiten wát hij gaat doen — en dat is de opzet voor S05.
**Vlaggen:** leest `Story.Beat.M21_Carcass`. **Zet niets.**
**Register:** dit is Kessara; hier telt men nog. Vanaf S02 niet meer.

### S02 — *Coming Back* · aankomst
`Krad-9 / Surface` → `Krad-9 / Tunnel City` · walk-and-talk · tier 5
**Aanwezig:** Voss, Brick, Sela + twee vechters

- **want** — Voss wants Brick to walk him in, because a local face opens doors.
- **obstacle** — Brick was taken off this rock by the Tithe and everyone here knows which lottery he lost.
- **turn** — The first person who recognises him does not welcome him back; they ask about somebody else.

**Beats:** (1) Het oppervlak: vacuüm, geen geluid, 0,4 g. **Eén beeld.** (2) De sluis en de eerste ademhaling binnen. *"You breathe what your crew breathes"* wordt niet geciteerd — het wordt gedaan: iemand deelt iets zonder dat erom gevraagd is, en niemand bedankt. (3) **Bricks terugkeer.** Geen omhelzing. Iemand noemt een naam en vraagt wat ermee gebeurd is, en Brick antwoordt op zijn manier: met nog een naam. **Hier landt zijn tic voor het eerst in een kamer die de namen kent.** (4) Sela leest de stad en zegt één ding dat klopt en één ding dat te vroeg komt.
**Vlaggen:** **zet niets, leest niets.** Deze scène draagt geen enkele vlag en dat is opzettelijk — hij bestaat om het register te kantelen (`ACT2_OVERVIEW` §7: **delen**).

### S03 — *The Ore Cathedral* · de staking
`Krad-9 / Tunnel City — Ore Cathedral` · cutscene · tier 5
**Aanwezig:** Voss, Brick, Sela · de stakingsleider + pit-clanleden

- **want** — The strike leader wants Eclipse to hold the line and nothing else.
- **obstacle** — Eclipse cannot afford weeks, and the leader knows exactly how long Eclipse can afford.
- **turn** — She tells him who is guarding the ore heart of the Expanse, and it is not an army.

**Beats:** (1) De kathedraal: een uitgemijnde holte met eigen weer, vol mensen die niet werken. Stil op een manier die duur is. (2) De stakingsleider is **beter georganiseerd dan Eclipse**. Ze heeft een eisenlijst, een ploegrooster en een luchtbudget. Dat laatste is de klok van de missie: een staking eindigt wanneer de lucht op is, niet wanneer de moed op is. (3) **AANRAKING A2-2 — P4-A2-b, en hij is VLAK.** De leider merkt op dat het ertshart van de Expanse bewaakt wordt door **bedrijfsbeveiliging** en niet door de Armada, en ze verklaart het weg: niemand verwacht dat een staking een oorlog wordt. Niemand spreekt haar tegen. **Uit de mond van een buitenstaander**, precies zoals P4-b in `M1.5.S05` — een derde partij maakt het feit hard zonder er een mysterie van te maken. (4) Ze vraagt niet om wapens. Ze vraagt om tijd. (5) Sela en de leider herkennen elkaar als hetzelfde beroep en mogen elkaar niet meteen.
**Vlaggen:** zet niets. **Deze scène draagt de aanraking en verder niets** — één aanraking per missie, nooit twee in één scène (`ACT2_OVERVIEW` §4).

### S04 — *What Your Crew Breathes* · de breuk
`Krad-9 / Deep Shaft` · in-mission-radio + callout · tier 5
**Aanwezig:** Voss, Brick, Sela · mijnwerkers · bedrijfsbeveiliging

- **want** — Security wants the shaft reopened, and they have decided a small breach will do it.
- **obstacle** — A small breach in a pressurised shaft is not small, and the miners know it and the guards do not.
- **turn** — The clans seal the breach with people instead of equipment, and Eclipse watches a culture pay a price it would not have known how to pay.

**Beats:** (1) De beveiliging forceert een schacht. Ze zijn niet wreed; ze zijn onbekwaam, en dat is erger op een planeet zonder lucht. (2) **De breuk.** Druk, geluid dat wegvalt, mensen die de verkeerde kant op getrokken worden. (3) **De pit-clans reageren als één lichaam.** Niemand geeft een bevel. Dat is het register van deze planeet in actie en het is de scène waar de speler het begrijpt zonder dat iemand het uitlegt (§18.8). (4) Brick doet mee alsof hij nooit weg is geweest — **en dan blijkt dat hij dat wél is**, want hij pakt het verkeerde gereedschap. Eén beat, geen commentaar. (5) Iemand gaat dood en het is geen naam die we kenden. (6) De beveiliging trekt terug, geschrokken van wat ze gedaan hebben.
**Band:** `in-mission-radio` met `band: callout` op de gevechts- en drukregels.
**Vlaggen:** zet niets.

### S05 — *Weeks or Tomorrow* · de vork
`Krad-9 / Tunnel City — Ore Cathedral` · cutscene · tier 5
**Aanwezig:** Voss, Brick, Sela · de stakingsleider · Dex op de radio

- **want** — Voss wants to give these people the thing they need most, and he gets to decide what that is.
- **obstacle** — The leader wants weeks; Dex wants ore; the clans want their dead buried; and Eclipse has a war that starts on someone else's clock.
- **turn** — Whichever way he decides, he has decided for people who had a plan.

**Beats:** (1) De rekening na S04: de staking is nu ook een begrafenis. (2) **De twee opties, allebei in één zin verdedigbaar:**
- *Winnen* — de staking beschermen tot de beveiliging het opgeeft. Duurzaam. Het kost weken die de Dominion gebruikt.
- *Bewapenen* — de mijnwerkers krijgen wapens en staan morgen op. Snel. De represaille valt op mensen die niet gekozen hebben.
(3) De leider zegt welke van de twee ze wil, en **ze zegt het één keer en dringt niet aan** — dat is het verschil tussen een vakbondsleider en een aanvoerder. (4) **Brick zegt hier zijn twee tellende woorden.** Hij pleit niet; hij noemt iets. Wat het is, kiest de schrijver — maar het is een zelfstandig naamwoord, want Brick geeft je een zelfstandig naamwoord (§18.4, L1-R9's scheiding met Petra). (5) Sela's positie hangt af van de tak en ze zegt hem vooraf, niet achteraf.
**Systeem:** `Story.Choice.M22_Krad.{Won,Armed}` ✚ — script-zijde `story.m22_krad` = `won` | `armed`. **Twee bladeren, niet drie.** De bijbel kent twee uitkomsten met twee gevolgketens en een verzoenende derde zou beide onbepaald maken.
**Gelezen door:** `M2.2.S99` (2 takken) · `M2.9.S04` · act 3 coalitiepolitiek (§2.9: *"unify miners"*) · Sela-reputatie. **Volledig register: `ACT2_OVERVIEW` §6.**
**Voss-varianten:** volledige dekking op beide assen. Dit is de duurste scène van de missie en de enige die hem waard is.

### S06 — *The Price of the Answer* · de uitvoering
`Krad-9 / Tunnel City` (tak `won`) / `Krad-9 / Deep Shaft` (tak `armed`) · in-mission-radio + callout · tier 5
**Aanwezig:** Voss, Brick, Sela + vechters · bedrijfsbeveiliging (`won`) / Dominion-tunnelspecialisten (`armed`)

- **want** — Eclipse wants the strike to end the way Voss chose.
- **obstacle** — The tak he chose has a cost the other one does not, and it arrives immediately.
- **turn** — It works. That is the problem.

**Beats, per tak — en de twee takken zijn niet symmetrisch, met opzet:**
- **`won`:** een verdedigingsgevecht dat lang is en waarin niemand wint. De beveiliging komt in golven en geeft het uiteindelijk op omdat het te duur wordt. **Er valt één Eclipse-dode en die naam gaat naar de muur.** De laatste beat is stilte en lucht.
- **`armed`:** een snelle, effectieve, lelijke opstand. Hij lukt binnen minuten. **En dan komen de Dominion-tunnelspecialisten** — het eerste échte militaire eenheidstype dat de speler ziet, en de opzet voor M2.6. De laatste beat is een mijnwerker met een geweer die niet weet hoe hij het moet neerleggen.
**Vlaggen:** **leest `story.m22_krad`** (2 takken). Zet niets.
**Waarschuwing voor de schrijver:** de `armed`-tak mag **niet** als straf gemonteerd worden. §2.7 regel 2 geldt: hij is sneller, hij werkt, en de speler die hem koos moet zich niet bekocht voelen. De prijs komt in M2.9, niet hier.

### S99 — *Letters* · debrief
`Krad-9 / Tunnel City — Ore Cathedral` · cutscene · tier 5
**Aanwezig:** Voss, Brick, Sela · de stakingsleider · Dex op de radio

- **want** — Voss wants to leave with an ally.
- **obstacle** — What he leaves with depends on what he decided, and the leader will say it plainly either way.
- **turn** — Someone hands Brick a letter that came back, and it is not from Krad-9.

**Beats:** (1) De uitkomst in de mond van de leider, **twee takken, allebei kort**. In `won` is ze een bondgenoot met voorwaarden; in `armed` is ze iets anders geworden en ze zegt zelf wat. (2) Dex krijgt zijn erts en zegt er iets over dat te vrolijk is voor de kamer. Zijn beste grap van de missie, en hij landt verkeerd — dat is de grap (§18.6: *"Dex jokes hardest when he's most frightened"*). (3) **De draad *Conscript Letters* raakt hier de grond.** De conscripten die in `M1.6.S06` mochten schrijven, krijgen antwoord — en de eerste brieven komen uit de Tithe-bestemmingen, dus uit Krad-9 en Vorn. **Eén regel, geen voorlezing.** Wie in M1.6 nee zei, krijgt hier de stilte en een `silence:`-blok met de reden. (4) **Brick.** Hij zegt niets over de brief. Hij noemt een naam. (5) Sela's slotoordeel over de tak, en het is het scherpst in de tak waarin Voss gelijk had.
**Systeem:** zet **`Story.Beat.M22_Strike`** ✚. Leest **`story.m22_krad`** (2 takken) en **`Story.Choice.M16_LettersAllowed`** (1 tak + `silence:` op de andere).
**`silence:`-eis:** de tak waarin er geen brieven zijn, krijgt een reden van ≥40 tekens die uitlegt waarom er niets speelt in plaats van dat het vergeten is (`SCRIPT_FORMAT` §4).

---

## 5. Vlaggen

| In | Uit |
|---|---|
| `Story.Beat.M21_Carcass` (S01) · `Story.Choice.M16_LettersAllowed` (S99, 1 tak + `silence:`) | **`Story.Beat.M22_Strike`** ✚ (S99) · **`Story.Choice.M22_Krad.{Won,Armed}`** ✚ (S05) |
| `story.m22_krad` — gezet S05, gelezen S06 (2) + S99 (2) | — |

**Kanonieke spelling en *gelezen door*: `ACT2_OVERVIEW` §6.** Geen tag bestaat al in code; geen `condition:` voordat hij er is.

---

## 6. Wendingen

| Wending | Handeling |
|---|---|
| **T4** | **geplant, dragend** (S03, P4-A2-b, aanraking **A2-2**): het ertshart van de Expanse wordt door bewakers bewaakt. **Vlak, uit de mond van een buitenstaander.** Tweede van zes in de akte |
| **T5** | **geplant, dragend** (S05, P5-A2-a): de vork. Dit is de zwaarste burgerbeslissing van beweging I en hij telt mee voor Kaine's geweten (§2.8 twist 5) |
| **T3** | niet aangeraakt. **En dat is een eis** — act 2 heeft precies vier T3-plants en ze staan alle vier ergens anders (AR-A2-9) |
| **T1, T2** | niet aangeraakt |
| **Threx-ladder AR-6** | **geen trede** (AR-A2-3) |

---

## 7. Draden

- **The Conscript Letters** — **aangeraakt** in S99, één regel. Geopend in `M1.6.S06`, **sluit in act 4** op het muiterijpad. Act 2 maakt hem levend en maakt hem niet af.
- **Letters from the Wall** — in de `won`-tak valt er één Eclipse-dode; die naam gaat naar de muur en de muur staat nog op Hollow Point. Dat is de opzet voor M2.6.S99.
- **Mara's brieven** — **niet aangeraakt.** Deze missie is geen van de drie adressen (AR-A2-5).

---

## 8. Groei

- **Voss** — hij neemt voor het eerst een beslissing *namens* mensen die hun eigen beslissing al genomen hadden. Dat is een andere soort macht dan act 1 hem leerde en het is de soort die in act 3 de coalitiepolitiek draagt.
- **Brick** — hij komt thuis en het is niet zijn thuis meer. Zijn tic (namen van doden) landt hier voor het eerst in een kamer die de namen herkent, en dat maakt hem groter zonder dat hij één woord extra zegt. **Zijn "nooit als eerste spreken" blijft intact** — er is geen enkele reden om dat hier uit te geven.
- **Sela** — ze ontmoet iemand die haar vak beter beheerst dan zij, en dat is de eerste barst in de zekerheid waarmee ze act 1 uit kwam. §18.9 B's maximes-cap geldt ook voor haar (L1-R56): **richt de figuur op deze mensen, in deze kathedraal, vanavond.**
- **Dex** — hij krijgt zijn erts en het maakt hem niet blij, en hij weet niet waarom. Hij zegt het niet.

---

## 9. Instructies voor de dialogue-writer

1. **Het register van deze planeet is DELEN.** Niemand zegt "ik" over iets dat een ploeg raakt; lucht en gereedschap wisselen zonder dat erom gevraagd wordt; er wordt niet bedankt. **Het spreekwoord *"You breathe what your crew breathes"* wordt NERGENS geciteerd.** Het wordt gedaan. Een personage dat het uitspreekt, is een codex-regel met een mond eromheen en zakt op §18.9 B (Explaining).
2. **De stakingsleider heeft geen naam** tot Q-A2-3. **Verzin er geen.** En ze is geen slachtoffer: ze is beter georganiseerd dan Eclipse en dat moet de speler merken in haar eerste drie regels.
3. **De `armed`-tak wordt niet gestraft in S06.** Hij werkt. §2.7 regel 2 is niet onderhandelbaar: een speler moet beide kanten in één zin kunnen verdedigen. De prijs komt in M2.9.
4. **AANRAKING A2-2 in S03 is vlak** en er is er precies één in deze missie. Zou hij goed op een poster staan, dan herschrijf je hem (L1-R17). **Er staat één gepolijste aanraking in de hele akte en die is van Kaya in M2.5 — niet van jou.**
5. **Brick spreekt nooit als eerste in een groep** (§18.4, "never"). Dat blijft in zijn thuiswereld ook zo, en juist daar is het het meest waard. Zijn tic vuurt hier op maximale dichtheid, maar het is dezelfde tic, niet een nieuwe.
6. **Geen totaal aantal mensen** (AR-A2-10). Ook niet van de stakers.
7. **Getallen:** het enige beschermde getal van deze missie is **nul komma vier** (de zwaartekracht, `03_world_design.md` §3.3, met eenheid). Alles onder de twintig is in act 1 bezet — draai `python Eclipse/Tools/check_spoken_numbers.py` en **schrijf geen vrijheidsclaim in een `note:`**.
8. **Voss-varianten:** S05 krijgt volledige dekking op beide assen; S03 alleen idealist/pragmatist. De rest draait op de basisregel.

---

## 10. Barks

**Nieuw: bedrijfsbeveiliging.** Vijfde faction-vocabulaire. Register: **werknemers**. Ze roepen procedures en ploegwissels, ze zijn niet bang van jou maar van hun leidinggevende, en ze vluchten eerder dan conscripten. Het goedkoopste onderscheid van alle vijf en het scheidt scherp van zowel Dominion-conscripten (angst + procedure) als de Ashline Cartel (kosten).

**Nieuw: pit-clan-omgevingsbarks.** Drukwaarschuwingen, luchtcontroles, gereedschap dat doorgegeven wordt. **Deze dragen de planeet-identiteit voor nul verhaalkosten** en ze zijn de plek waar het deel-register het duidelijkst hoorbaar is.

**Bestaand hergebruik:** de Dominion-tunnelspecialisten in de `armed`-tak spreken het bestaande Dominion-vocabulaire, maar **zonder de angst** — dat zijn geen conscripten. Of dat een eigen pool verdient, is een `voice-director`-afweging (C-A2-2 / Q-A2-3) en **wordt niet opgelost door een bestaande pool te lenen** (L1-R30).
