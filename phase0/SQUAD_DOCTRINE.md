# SQUAD-DOCTRINE — ONTWERP

*Owner-opdracht 26-07 avond, punt 1. Zijn woorden, want het hele ontwerp hangt
eraan:*

> mijn squadleden zijn getrainde mensen met een brein. Standaard doen ze alles wat
> een soldaat doet — ze vuren op wat ze zien, ze lopen mee, en als er op ze
> geschoten wordt volgen ze hun training: dekking zoeken, verplaatsen,
> dekkingsvuur. **Dat is geen feature die je aanzet, dat is de basis.**

*En:*

> Ik stuur intentie, geen voetstappen.

*Eerder die dag stelde ik drie schakelaars voor — autonoom vuren aan/uit,
meelopen aan/uit, dekking zoeken aan/uit. Het antwoord was: "Geen van drieën. Zo
werkt het niet." Dit document begint daar.*

---

## Waarom die drie schakelaars fout waren

Een schakelaar zegt: *dit gedrag bestaat niet tenzij je het aanzet.* Dat is de
omkering van hoe een squad hoort te werken. Een soldaat die pas schiet als je een
vinkje zet, is geen soldaat maar een geweerstandaard.

De juiste vorm is **twee lagen**, en dat is precies wat elke referentie doet:

| Laag | Wat het is | Wie bepaalt het |
|---|---|---|
| **Basiscompetentie** | Wat een soldaat sowieso doet: vuren op wat hij ziet, meelopen, dekking zoeken onder vuur, een gevallen maat helpen | Niemand. Het is er |
| **Doctrine** | Een KADER dat die basis inperkt of loslaat: niet vuren tot ik het zeg, geen dekking zoeken want we stormen door, hier blijven staan | De owner, met één order |

Doctrine **verwijdert of verruimt** gedrag; hij voegt het niet toe. "Kamikaze"
betekent niet "zet aanvallen aan" maar "laat dekking zoeken weg" — precies zoals
de owner het formuleerde.

---

## Research: hoe het genre dit doet

| Spel | Wat het levert | Wat ik overneem |
|---|---|---|
| **Full Spectrum Warrior** | Je bestuurt nooit een individueel wapen. Je geeft een team een positie en een intentie; het team voert de doctrine uit (bounding overwatch, suppress-and-flank). De hele game is "het team is competent, jij geeft richting" | **Het kernidee.** Dit is één op één wat de owner beschrijft |
| **Ghost Recon** (AW / Wildlands / Breakpoint) | Expliciete *Rules of Engagement*: Recon (vuur niet tenzij beschoten), Assault (vuur vrij), Suppress (blijf staan, houd ze plat). Daarnaast losse orders: hold position, regroup, fire at will | **De ROE-set.** Drie modi, niet acht |
| **Mass Effect 2/3** | Teamgenoten vuren en dekken zelf; hun *powers* zijn de enige laag die de speler stuurt. Hoge basiscompetentie, dunne commandolaag | **De verhouding.** De commandolaag is klein omdat de basis groot is |
| **The Division** | AI-teamgenoten gedragen zich als spelers: dekking, flankeren, overeind helpen | **De lat voor "basis"** |
| **SWAT 4 / Rainbow Six** | ROE mét nalevingsregels; één intentie ("stack up, breach and clear") vouwt uit naar veel handelingen | **Eén order = veel handelingen** |
| **XCOM** | Order per soldaat per beurt | **Niet overnemen.** Dat is de tegenpool en het maakt de speler tot micromanager |

**Het gemeenschappelijke patroon:** een kleine ROE-set (drie tot vier), bovenop
een basis die altijd draait. Niemand geeft de speler schakelaars voor
basisgedrag.

---

## Wat er vandaag staat

`EEclipseSquadStance` bestaat al en heeft twee waarden: `Ready` en `Aggressive`.
Hij wisselt de HUD-regel en **verandert geen enkel gedrag** (squad-audit punt 12).
De owner: bouw dít uit, geen tweede systeem ernaast.

Twee velden in de data zijn ook dood: `FollowDistance` en `CoverSearchRadius`.
Die horen bij de basislaag en krijgen daar hun betekenis.

---

## Het ontwerp

### Laag A — basiscompetentie (geen order nodig)

| Gedrag | Wat het doet | Data die het al heeft |
|---|---|---|
| **Meelopen** | Blijft binnen `FollowDistance` van de speler als er geen andere order staat | `FollowDistance` (dood) |
| **Autonoom vuren** | Vuurt op de dichtstbijzijnde vijand binnen wapenbereik en zichtlijn | wapenbereik uit DT_Weapons |
| **Dekking zoeken onder vuur** | Beschoten worden → dichtstbijzijnde dekking binnen `CoverSearchRadius` | `CoverSearchRadius` (dood) |
| **Overeind helpen** | Werkt al (auto-triage, medic) | — |

Dit is de laag waar de owner om vroeg, en het is meteen de grootste
balansverschuiving die er ligt: **drie extra schutters halveert hoe lang een
groep vijanden overeind blijft.** Daarom staat in de gevechts-audit al dat alle
getallen daar opnieuw gemeten moeten worden zodra deze laag draait.

### Laag B — doctrine (`EEclipseSquadStance` uitgebouwd)

Vier waarden, uit de Ghost Recon-set plus het voorbeeld van de owner zelf:

| Doctrine | Wat het met de basis doet | Bron |
|---|---|---|
| `Recon` | **Vuurt niet** tenzij er op hem geschoten wordt. Zoekt dekking. Blijft dicht | Ghost Recon "Recon" |
| `Ready` *(default)* | De volledige basis: vuurt op wat hij ziet, zoekt dekking, loopt mee | Ghost Recon "Assault" |
| `Overwatch` | **Blijft staan** waar hij staat. Vuurt vrij. Loopt niet mee | Full Spectrum Warrior / GR "Suppress" |
| `Aggressive` | **Zoekt geen dekking**, sluit af op de dichtstbijzijnde vijand | Het "kamikaze"-voorbeeld van de owner |

`Ready` en `Aggressive` bestaan al als naam — die blijven, met dezelfde
betekenis die ze nu suggereren en die ze dan eindelijk waarmaken.

### Waar de klasse-verbs landen

`Momentum` en `Killzone` bestaan als tag plus getal en worden nergens afgevuurd
(squad-audit punt 10). Ze horen **in de basislaag**, niet als aparte knop: een
Assault-soldaat gebruikt Momentum wanneer zijn doctrine hem laat opsluiten, een
Sniper zet Killzone wanneer hij in Overwatch staat. Dat is hoe Mass Effect het
doet — de powers zijn autonoom, de speler kan ze desgewenst overrulen.

---

## Bouwvolgorde (in lagen landen, elk apart meetbaar)

| # | Laag | Meting die hem bewijst |
|---|---|---|
| 1 | **Meelopen** (`FollowDistance`) | Afstand tot de speler over 10 s lopen; blijft onder de geauthorde waarde |
| 2 | **Autonoom vuren** | Schoten door squadmates zonder enige order > 0; en de tijd waarin een groep vijanden valt, opnieuw gemeten |
| 3 | **Dekking onder vuur** (`CoverSearchRadius`) | Positie vóór en ná beschoten worden; afstand tot de dichtstbijzijnde dekkingsactor daalt |
| 4 | **Doctrine** (stance uitgebouwd) | Per doctrine één meting die het VERSCHIL toont: Recon vuurt 0 schoten waar Ready er >0 vuurt; Aggressive's afstand tot dekking daalt níét |
| 5 | **Klasse-verbs erin** | Momentum/Killzone vuren af zonder dat de speler ze aanroept |
| 6 | **Hermeting van het gevecht** | De hele gevechts-audit ronde 3, want elk getal daarin is zonder deze squad gemeten |

Elke laag landt apart. Dat is niet alleen de owner-regel maar hier ook
noodzakelijk: laag 2 verandert de balans zo hard dat hem samen met laag 3 landen
zou betekenen dat je niet meer weet welke van de twee wat deed.

---

## Wat dit met Command Mode doet

De owner: *"Dit maakt Command Mode belangrijker, niet overbodig: ik stuur
intentie, geen voetstappen."*

Dat klopt en het is het omgekeerde van wat ik er 's ochtends over schreef. Ik
noteerde in de squad-audit dat "een squad die alles zelf doet het
commando-systeem overbodig maakt". Dat is de XCOM-lezing: als commando's
voetstappen zijn, dan maakt autonomie ze overbodig. Maar als commando's
**doctrine** zijn, gebeurt het tegenovergestelde — hoe competenter de squad, hoe
meer een kaderwissel oplevert. In Full Spectrum Warrior is het team maximaal
competent en is de commandolaag het hele spel.

## Status

Ontwerp vastgelegd 26-07 avond, nog niets gebouwd. Volgende taak na loadouts.
