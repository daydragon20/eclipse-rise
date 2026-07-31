# VOICE LEDGER — wat er echt is uitgegeven
*Eigenaar: `voice-director`. Bijwerken na ELKE generatiebatch, zonder uitzondering.*
*De ladder in `19_voice_production.md` §19.2 is een plan. Dit bestand is de waarheid.*

---

> ## ✅ BRON GEVONDEN — 31-07, en de vraag is dicht
>
> Hier stond een waarschuwing dat dit getal geen herleidbare bron had. **Die bron is er
> wel**, verbatim van de owner: *"het waren geen 310000 credits maar 131000, fout
> onthouden."* Vastgelegd in commit `ebc764e`, waar ook alle tiers opnieuw gesneden zijn —
> niet omgenummerd, want een vlakke zoek-vervang liet de tiers optellen tot 310k.
>
> **Waarom dit hier blijft staan in plaats van weggepoetst te worden:** ik heb dit getal
> bijna teruggedraaid omdat ik de bron niet kon vinden. Eerder op dezelfde avond deed ik dat
> wél met O-2 — en dat bleek fout, want dat antwoord was in een chatvenster gevallen dat de
> repo niet ziet. **De les is niet "vertrouw agents" en ook niet "wantrouw ze", maar: een
> getal dat het plan stuurt hoort zijn bron in de repo te dragen, niet in een gesprek.**
> Daarom staat het citaat nu in het commitbericht én hier.
>
> Gevolg van de correctie, eerlijk: **~17% van de gesproken tekst past deze maand, niet 40%.**
> De Act-1-hub- en companiongesprekken (was 33.000) zijn eraf gevallen — dat zijn de stemmen
> die je *tussen* de missies hoort in Hollow Point.

**Budget:** 131.000 credits · **vervalt 21 augustus 2026, rolt niet door**
**Werkdeadline generatie:** 19 augustus (twee dagen buffer)
**Doel op 19/08: 131.000 besteed, 0 over.** Credits die op 21 augustus ongebruikt zijn, zijn weggegooid.

**De reserve (20.000) is geen spaarpot maar verzekering.** Hij blijft *niet-toegewezen* tijdens de sprint, zodat een misgelopen batch nog te herstellen is. Op **17 augustus** gaat wat er nog van over is alsnog de volgende tier in. Niet eerder toewijzen, niet laten staan.

**Let op — de gratis 10.000/maand tellen NIET mee.** Het gratis niveau geeft geen commerciële rechten en vereist naamsvermelding; die audio mag niet in een verkochte game. Na 21 augustus is de route: **Starter, ~€5/maand voor 30.000 credits mét commerciële rechten** (zie `19_voice_production.md` §19.2).

## Ledger

| Datum | Tier | Scope | Regels | Credits | Saldo na |
|---|---|---|---|---|---|
| 2026-07-31 | 0 | Casting stage 1 — brede screening, 18 rollen × 4–6 kandidaten (premade) | 0 | 0 | 131.000 |
| 2026-07-31 | 0 | Casting stage 1 uitbreiding — Voice Library, 24 kandidaten voor Mara/Kaine/Vex/Callis/Torren | 0 | 0 | 131.000 |
| 2026-07-24 | 3+4 | SFX-set (7 one-shots) + muziek-sting — gegenereerd toen de meter kapot was; bedrag **afgeleid** uit het accountsaldo, niet gemeten tijdens de run | 8 | 2.868 | 128.132 |
| 2026-07-31 | 0 | Stage 2 casting-test — 9 rollen, 17 finalisten, 51 clips. **VERKEERD MODEL** (multilingual_v2 i.p.v. eleven_v3); 21 getagde clips onbruikbaar | 51 | 2.475 | 125.657 |
| 2026-07-31 | 0 | A/B-controleproef: gedraagt een audiotag zich op multilingual_v2? | 1 | 45 | 125.612 |
| 2026-08-01 | 2 | IJkmissie M1.1 — **AFGEBROKEN VÓÓR DE EERSTE CALL**, casting niet vastgelegd (zie hieronder) | 0 | 0 | 125.612 |

> **Formaat:** het dashboard leest deze tabel automatisch uit. Houd de kolomvolgorde
> aan en begin de datumkolom met `JJJJ-MM-DD`, anders telt de regel niet mee.

## Tier-voortgang

| Tier | Wat | Begroot | Besteed | Klaar |
|---|---|---|---|---|
| 0 | Casting — fase 1 GRATIS, fase 2 alleen Act-1-rollen | 6.000 | 2.520 | ☐ |
| 1 | Bark- & systeembibliotheek | 48.000 | 0 | ☐ |
| 2 | Act 1 story-dialoog (incl. ijkmissie) | 45.000 | 0 | ☐ |
| 3 | Muziek — thema + never-silent-vloer | 15.000 | 2.868 ᵃ | ☐ |
| 4 | Kern-SFX | 7.000 | (zie ᵃ) | ☐ |
| — | Reserve (niet toewijzen; 17/08 vrijgeven) | 10.000 | 0 | — |
| | **Totaal** | **131.000** | **5.388** | |

ᵃ De run van 24-07 maakte 7 SFX én 1 muziekstuk in één batch, met de meter kapot.
Het totaal (2.868) is af te leiden uit het accountsaldo, maar de **verdeling over
tier 3 en 4 is niet te reconstrueren** — sound-generation en music rekenen niet per
teken af, en `/v1/history` was toen niet leesbaar. Niet alsnog verzinnen; vanaf nu
meet elke run zichzelf.

**Sluitproef 31-07:** account meldt **5.388 van 131.000** gebruikt.
131.000 − 2.868 − 2.475 − 45 = **125.612 over**, en dat is exact wat het account zegt.
De ledger sluit op de cent.

*Herzien 31-07 laat: het budget bleek **131.000** en niet 310.000 — de owner had zich vergist. Alle tiers zijn opnieuw gesneden, niet alleen omgenummerd. Wat eraf viel: de Act-1-hub- en companiongesprekken (was 33.000). Die blijven geschreven en wachten op volgende maand.*

*Tier 0 is twee keer bijgesteld. Eerst 5.000 → 18.000 op owner-instructie "doe moeite om de juiste voice te vinden". Daarna 18.000 → **12.000**, want de owner wees erop dat de brede screening **niets hoeft te kosten**: de Voice Library is gratis doorzoekbaar, met eigenschappen én previewfragmenten per stem. Alleen de diepe test op de twee finalisten kost credits, omdat een generieke preview niet vertelt of een stem Mara's doodsscène overleeft of over geweervuur heen leesbaar blijft. De 6.000 die vrijkwam is terug naar tiers 1–3.*

## Aantekeningen

*Onverwachte spend hier noteren, met de oorzaak. Een batch die credits kost terwijl
hij uit cache had moeten komen, betekent dat tekst of een audio-tag is veranderd —
zoek uit wát vóór je opnieuw draait.*

### 2026-08-01 — M1.1 stond klaar om ingesproken te worden en is NIET gegenereerd

**Gemeten saldo vóór: 125.612. Gemeten saldo ná: 125.612. Delta 0.**
`require_usage_measurement()` slaagde in beide richtingen — de meter werkt, dit is
geen ongemeten run maar een run die niet begonnen is.

**Wat er wél klopte.** Alle zeven scenes `critic: GO`. Alle audio-tags binnen de
goedgekeurde set van §19.4 (`[pause]`×6, `[quietly]`×4, `[shouting]`×3, `[nervous]`×2,
`[exhausted]`×2, `[whispering]`, `[amused]`), nul regels met meer dan twee tags.
`validate_script.py --no-voice` heeft nul bevindingen in M1.1. Gemeten kosten 9.316
credits (niet 9.332 — S03/S05/S06/S99 zijn vannacht nog geraakt; 0,2% drift, ruim
binnen de 25%-grens).

**Waarom het toch niet doorging: casting ligt niet vast, en drie stemmen zijn dubbel
gecast.** Gemeten uit `phase0/CASTING_RESOLVED.json` + `VoiceKeyMap.json`:

| Stem-ID | Rol 1 | Rol 2 |
|---|---|---|
| `XrExE9yKIg1WjnnlVkGX` (Matilda) | **mara** (328 regels) | **eclipse_fighter_b** (52) |
| `TX3LPaxmHKxFdv7VOQHJ` (Liam) | **dex** (267 regels) | **dominion_conscript_b** (9) |
| `cjVigY5qzO86Huf0OWal` (Eric) | **threx** (28 regels) | **veil_operative_b** (8) |

De eerste twee zitten allebei ín M1.1. Mara en een Eclipse-schutter zouden dezelfde
stem hebben; Dex en een Dominion-conscript ook — dat zijn tegenstanders in hetzelfde
vuurgevecht. Niet *lijkend*: dezelfde stem-ID, dus dezelfde cachesleutel, dus dezelfde
stem.

**Exposure in M1.1: 6.111 van 9.316 credits = 66%** hangt aan stemmen die nog kunnen
wijzigen (de twee botsingen, plus `voss_f`=Laura en `reyes`=Alice, die allebei in de
eerste-keuze-botsingen van O-13 staan).

**Waarom dat een stop is en geen risico dat je neemt.** §19.1 punt 2: casting is
permanent, want de cachesleutel is `hash(voiceId + text + emotion + modelId)`. Wie
later van stem wisselt, herbetaalt élke regel van dat personage. O-13 zegt het zelf,
in de optie "later": *"dan blijft casting open en kan Tier 1 niet starten. De
botsingen moeten voor generatie weg."* De castingtabel in §19.3 is nog volledig leeg
en O-3 én O-13 zijn onbeantwoord — Tier 0 is dus niet af, en §19.2 verbiedt aan een
tier te beginnen voor de vorige klaar is.

**Derde blokkade, los van casting.** `Eclipse/Content/Audio/DialogueSeed.json` bevat
alleen `Squad.VoiceA` en `Squad.VoiceB` (generieke barkstemmen, `eleven_multilingual_v2`).
**Geen enkele M1.1-spreker staat erin** — het readme zegt zelf dat de companions "in
Phase 2" komen. De commandlet-route uit §16.12 kan M1.1 op dit moment dus überhaupt
niet genereren zonder eerst seed-entries te schrijven, en dát is precies het opschrijven
van de castingkeuze die nog niet gemaakt is. Let op: het model in die seed is
`eleven_multilingual_v2`, niet `eleven_v3` — wie hem ongewijzigd draait, herhaalt de
fout van 31-07.

**Wat "alle acht stemmen resolven" wél en niet betekende.** Dat klopte: geen lege
sleutel. Maar resolven is niet hetzelfde als *uniek* resolven, en die controle bestond
niet — `check_voice_resolves.py` meldde beide helften van elk botsend paar als "CAST
AND READY". Dat is nu gerepareerd: de tool groepeert op stem-ID en faalt (exit 1) als
twee **rollen** er een delen. Aliassen van één personage (`threx`/`dahl_threx`) tellen
terecht niet mee. Controleproef gedaan: met de drie botsingen opgelost verdwijnt het
blok en gaat de check groen — hij kan dus echt bewegen en staat niet permanent rood.

**Wat er nodig is om dit alsnog te draaien:** antwoord op O-13 (zes botsingen, waarvan
twee in M1.1) en O-3 (eclipse_fighter slot C en D hebben nog geen stem). Daarna is
M1.1 een run van ~9.316 credits, S05 eerst.

**2026-07-31 — casting stage 1 afgerond, 0 credits.** 18 rollen, 80 kandidaat-slots,
uitsluitend metadata + de gratis previewfragmenten. Geen enkele TTS-call gedaan.
Resultaat staat in `progress_media/casting/` (zie `CASTING.html`). Stage 2 is
**bevroren** tot de owner (a) O-2 beantwoordt en (b) per rol een top 2 kiest.

**~~⚠ Deze ledger kan zichzelf niet controleren.~~ — OPGELOST later op 31-07.**
*Hieronder de meting van eerder op de dag; ze staat er nog omdat ze verklaart
waarom de run van 24-07 ongemeten is en waarom de meter is verbouwd. De scopes
zijn inmiddels toegekend — zie het blok hieronder.*

Toen gemeten: `/v1/voices` en `/v1/shared-voices` gaven 200, maar
`/v1/user/subscription` 401 (`user_read`), `/v1/history` 401
(`speech_history_read`) en `/v1/models` 401 (`models_read`).

Gevolgen die dat destijds had:
1. Het saldo was **niet uit te lezen**; elk getal hier was een aftreksom.
2. `generate_audio_assets.py` ving de 401 af, gaf `None` terug en schreef stil
   géén `usage_credits` weg. **De ingebouwde kostenmeting was dood en meldde dat
   nergens** — daarom is de spend van 24-07 alleen achteraf af te leiden.
3. Zonder `models_read` was niet te controleren of `eleven_v3` bestond. Dat is
   precies de onzekerheid die later 2.475 credits op het verkeerde model kostte.

**De meter is gerepareerd (31-07).** `generate_audio_assets.py` weigert nu te
genereren zolang spend niet meetbaar is: `require_usage_measurement()` draait vóór
de eerste betaalde request en **gooit** een `UnmeasurableSpendError` in plaats van
een lege waarde terug te geven. Exitcodes: **3** = geweigerd vóór generatie,
**4** = wel gegenereerd maar de na-meting faalde (spend staat dan als
`measurement_failed` in het manifest en mag **niet** als gemeten getal in dit
bestand landen). Live geverifieerd tegen de echte sleutel: exitcode 3, niets
gegenereerd. Bewaakt door `Eclipse/Tools/test_credit_meter.py` (7 tests), die in
`Eclipse/Tools/verify.ps1` als gate draait. Controleproef gedaan: die test wordt
**rood** tegen de oude code, dus hij kan echt falen.

### ⚠ Act 1 past niet in tier 2 — gemeten, niet geschat (31-07)

Ik heb de scriptbestanden geteld in plaats van aangenomen: elke `text:` plus elke
`variants:`-tekst, per spreker.

| Spreker | Regels | Tekens | Credits | |
|---|---|---|---|---|
| **voss** | 201 | 16.007 | **32.014** | ×2 — elke regel wordt in M én V ingesproken |
| dex | 194 | 9.459 | 9.459 | |
| mara | 252 | 8.261 | 8.261 | |
| iron_chorus_emissary | 71 | 3.284 | 3.284 | shortlist klaar, nog niet gekozen |
| reyes | 43 | 2.387 | 2.387 | |
| overige sprekers | — | — | ~9.610 | |
| | | | **65.015** | **tier 2 is begroot op 45.000** |

### ✅ ACT 1 IS COMPLEET — gemeten eindstand (01-08 ~05:40)

Nul stubs: acht missies, de proloog en de twaalf hub-gesprekken. **71 bestanden,
1.623 regels (varianten meegeteld), 97.275 credits.**

| Missie | Regels | Credits |
|---|---|---|
| M1.1 Thirteen Bullets | 182 | 9.332 |
| M1.2 The Dead Drop | 181 | 11.333 |
| M1.3 Signal Fire | 211 | 8.793 |
| M1.4 The Quartermaster | 132 | 8.655 |
| M1.5 Cells | 221 | 13.279 |
| M1.6 The Tithe Train | 187 | 12.997 |
| M1.7 Under the Ice | 154 | 10.206 |
| M1.8 Blacksite K-77 | 187 | 10.450 |
| Hub (12 gesprekken) | 152 | 10.780 |
| Proloog | 16 | 1.450 |
| **TOTAAL** | **1.623** | **97.275** |

**Saldo 125.612. Act 1 is dus 77% van alles wat er nog is** — en daarna komen barks,
muziek, SFX en de acts 2 t/m 4. Dat is de vraag die op O-14 ligt.

**De extrapolatie zat er 3% naast.** Vannacht om 02:20 schatte ik ~100.000 op basis van
1.641 credits/scène over zes geschreven missies; het werd 97.275. Dat is precies waarom
het getal **geëxtrapoleerd héétte** in plaats van gemeld te worden als meting.

**Twee handtellingen die 5% te hoog uitkwamen.** Beide hub-schrijvers rekenden met de
hand (5,3 tekens per woord) en kwamen samen op ~11.290 tegen de gemeten 10.780. Ze
meldden allebei uit zichzelf dat ze de tools **niet konden draaien** — die agents hebben
geen shell — en vroegen om een echte meting voor iemand het als groen boekt. Dat is de
juiste vorm: een handtelling die zichzelf een handtelling noemt.

> **Correctie op commit `43c493f`:** het bericht daar zegt "78 scenes". Het zijn er **71**.
> Dat getal heb ik uit een agentrapport overgenomen zonder te tellen; regels (1.623) en
> credits (97.275) kloppen wel.

---

### ✅ ACT 1 IS GESCHREVEN OP DE HUB NA — tussenstand (01-08 ~03:50)

Alle acht missies plus de proloog staan er. Gemeten met
`Eclipse/Tools/count_generation_cost.py`:

| Missie | Regels | Credits |
|---|---|---|
| M1.1 Thirteen Bullets | 182 | 9.332 |
| M1.2 The Dead Drop | 181 | 11.333 |
| M1.3 Signal Fire | 211 | 8.793 |
| M1.4 The Quartermaster | 132 | 8.655 |
| M1.5 Cells | 221 | 13.279 |
| M1.6 The Tithe Train | 187 | 12.997 |
| M1.7 Under the Ice | 154 | 10.206 |
| M1.8 Blacksite K-77 | 187 | 10.450 |
| Proloog | 16 | 1.450 |
| **Gemeten totaal** | **1.471** | **86.495** |
| Hub — 12 gesprekken, nog niet geschreven | — | ~14.000 (schatting) |
| **Act 1 compleet** | | **~100.000** |

Saldo 125.612. **Act 1 alleen is dus ~80% van alles wat er nog is**, en daarna komen
barks, muziek, SFX en de acts 2 t/m 4. Dat is de vraag die op O-14 ligt.

**4.390 van die credits waren tot vannacht onbereikbaar.** Vijf condities hingen aan
vlaggen die niemand zette — 96 regels die gegenereerd én betaald zouden worden en nooit
zouden klinken. M1.1, M1.3 en M1.4 zijn geschreven vóór het `choice:`-veld bestond en de
retrofit was nooit gelopen. Zes keuzeblokken toegevoegd (geen ervan verzonnen: elke
optieregel noemde de vlag en de waarde al in zijn eigen note). Wat overblijft is
`run.m17_record`: 96 credits, en een systemtaak die de M1.7-schrijver zelf meldde.

---

### ⚠⚠ EN DIE 65.015 GAAT OVER ZES VAN DE ACHT MISSIES (01-08)

De telling hierboven klopt op wat ze telde. **Maar ze telde niet heel act 1**, en dat
stond er niet bij — het bedrag las als "act 1 kost dit" terwijl het "de scenes die
op 31-07 al geschreven waren kosten dit" betekende.

Gemeten met `Eclipse/Tools/count_generation_cost.py` (nieuw, herhaalbaar): **31 van
de 71 scriptbestanden hebben geen enkele tekstregel.** Dat zijn M1.7 (6 van 7), heel
**M1.8 (12 scenes — de climax van de act)** en de **hele hub (12 scenes)**.

| | scenes | credits |
|---|---|---|
| Gemeten — M1.1 t/m M1.6 + proloog + de eerste M1.7-scene | 40 | **67.341** |
| Geëxtrapoleerd — rest van M1.7, heel M1.8, hele hub | 30 | ~49.230 |
| **Act 1 compleet** | **70** | **~116.571** |

De extrapolatie is 1.641 credits/scene, het gemeten gemiddelde over de zes
geschreven missies (spreiding 1.256 bij M1.3 tot 2.213 bij M1.5). **Het is een schatting
en heet hier ook zo.** Twee bekende scheeftrekkingen die elkaar deels opheffen: de hub
bestaat uit korte gesprekken en zit er waarschijnlijk te hoog in, M1.8 is de climax met
twaalf scenes en zit er waarschijnlijk te laag in.

**Dat verandert de vraag.** Niet 20.015 tekort op een pot van 45.000, maar: **act 1
alleen kost ~93% van de 125.612 die er nog is.** Barks, muziek, SFX en de acts 2 t/m 4
komen daar nog bij. De opties hieronder zijn doorgerekend op 65.015 en dus op een te
klein probleem.

**Waarom de handtelling dit niet kon zien.** Ze telde wat er stond, en wat er stond was
zes missies. Een teller die zwijgt over zijn eigen dekking meldt een stub als nul kosten,
en nul kosten leest als "gratis" in plaats van als "nog niet geschreven". De nieuwe tool
drukt daarom **altijd** de stub-lijst af, ook als die leeg is.

**Kruisproef op de tool, want twee tellingen die verschillen moeten uitgezocht worden.**
Tegen het corpus zoals het op 31-07 stond geeft de tool 65.125 tegen 65.015 met de hand
— 0,17% verschil. Dex (9.459), mara (8.261) en reyes (2.387) reproduceren **tot op het
teken**. De tool meet dus hetzelfde; het restverschil zit in hoe regels met varianten
geteld werden. En de zelftest (`--zelftest`, 5 controles) bewijst eerst dát hij kan
bewegen: varianten meetellen, de Voss-verdubbeling, een onbekende YAML-vorm melden in
plaats van stil overslaan, een regel zonder `voice:` signaleren, en een lege scene als
stub melden in plaats van als nul.

**`words_generated` bestaat in nul van de 71 bestanden.** L1-R15 en `SCRIPT_FORMAT` §4
schrijven het allebei voor als *het* credit-veld. Zolang het nergens staat, is deze tool
de enige bron voor dat getal.

---

**Tekort op de oude telling: 20.015 credits.** Er is 125.612 over, dus het is oplosbaar
— maar niet zonder een keuze, en de keuze is niet van mij.

**Waarom Voss zo duur is.** Twee dingen stapelen. `SCRIPT_FORMAT` §4 regel 196 zegt
dat `voss` een *logische* sleutel is die "resolved to `voss_m` / `voss_f` per player
gender at build" — dus elke Voss-regel wordt twee keer ingesproken. Daarbovenop
dragen 32 regels persoonlijkheidsvarianten (126 varianttoksten, 7.396 tekens), en
die tellen ook dubbel. Samen is Voss **71% van het hele tier-2-budget**.

**Opties, met gemeten bedragen — dit is een owner-keuze:**

| Optie | Bespaart | Wat je inlevert |
|---|---|---|
| Eén Voss-geslacht deze maand inspreken | 16.007 | De andere helft van de spelers hoort zijn eigen personage niet |
| Alleen de basistekst, geen persoonlijkheidsvarianten | 14.792 | De as idealist/pragmatist wordt tekst zonder stem |
| Beide bovenstaande | 22.399 | Past ruim, maar Voss wordt één stem zonder varianten |
| Niets inleveren, 20.015 uit reserve + andere tiers halen | 0 | Muziek of SFX schuift naar volgende maand |

**Niet stil oplossen.** Elke optie hierboven verandert wat de speler hoort.

### ✅ De drie scopes zijn er (31-07) — en wat ze meteen opleverden

Alle drie staan nu op de sleutel. Gemeten, niet aangenomen: `/v1/user/subscription`,
`/v1/models` en `/v1/history` geven alle drie HTTP 200. Daarmee:

- **Het saldo is leesbaar:** 5.388 van 131.000 gebruikt.
- **`eleven_v3` bestaat op dit abonnement.** Dat sluit de open vraag uit de vorige
  ronde: §19.4 leunt volledig op v3-audiotags, en v3 is beschikbaar. **v3 is dus het
  model voor Tier 1 en 2**, en omdat `modelId` in de cachesleutel zit, is dat een
  besluit dat je één keer neemt.

**⚠ De teller loopt achter, dus een delta is geen meting.** Gemeten op 31-07: een
echte generatie van 45 tekens gaf een before/after-delta van **0**, en 341 credits
landden pas minuten later. De teller stabiliseerde daarna en bleef 60 seconden lang
gelijk. Gevolg voor de boekhouding: **het aantal verzonden tekens is het gezaghebbende
getal** (bij TTS geldt 1 credit = 1 teken en dat weten we exact); het accountsaldo is
een *nalopende kruiscontrole*, af te lezen ná een wachttijd. `stage2_casting_test.py`
doet dat nu zo en waarschuwt als de twee meer dan 10% uiteenlopen.

### Fout van 31-07 — 2.475 credits op het verkeerde model

**Wat er gebeurde.** Ik had in `stage2_casting_test.py` een *waarschuwing* gezet dat
de §19.4-audiotags alleen op v3 werken, maar geen *poort*. Daarna draaide ik de echte
run met de uitvoer naar `/dev/null`, zag de waarschuwing dus niet, en het script ging
gewoon door op `eleven_multilingual_v2`. Resultaat: 51 auditieclips, waarvan **21 met
audiotags die dat model niet ondersteunt**.

**Hoe erg is het.** Gemeten met een A/B op dezelfde stem, hetzelfde model, dezelfde
zin — alleen de tag weg: mét `[grieving]` 4,44 s, zonder 2,14 s. Een verschil van
**+2,30 s op een basis van 2,14 s**. Dat is veel meer dan het uitspreken van één woord,
dus multilingual_v2 doet iets groots en onbedoelds met blokhaken. Wat het precies doet
— de tag voorlezen of de voordracht veranderen — scheidt deze meting níét, en dat moet
je dus ook niet beweren. Wel vaststaand: **die 21 clips zijn geen eerlijke basis voor
een castingbesluit**, en casting is permanent.

**Wat er is gerepareerd.** De waarschuwing is nu een **harde poort** (exitcode 6): het
script weigert getagde regels op een niet-v3-model. Standaardmodel is `eleven_v3`.

**Wat er nog moet.** De 30 ongetagde clips (signature + gevechtsregel) zijn bruikbaar.
Voor de 21 getagde clips zijn er twee opties, beide binnen het Tier-0-plafond:

| Optie | Kosten | Tier 0 totaal daarna |
|---|---|---|
| Alleen de 21 getagde clips opnieuw op v3 | 1.439 | 3.959 van 6.000 |
| Alle 51 clips opnieuw op v3 (één model, eerlijke vergelijking) | 2.470 | 4.990 van 6.000 |

**Dit is niet stil doorgezet.** Na onverwachte spend hoort de regel te zijn: stoppen en
melden. Wacht op akkoord.

### Owner-actie (afgerond): drie scopes op de API-sleutel

ElevenLabs-dashboard → API Keys → de sleutel bewerken. Per scope waaróm:

| Scope | Nodig voor | Wat er zonder gebeurt |
|---|---|---|
| `user_read` | `/v1/user/subscription` — het saldo vóór en ná elke batch | **Generatie is nu geblokkeerd** (exitcode 3). Dit is de enige die de sprint tegenhoudt. |
| `speech_history_read` | `/v1/history` — spend per losse regel achteraf | Een batch is alleen als totaal te controleren; een uitschieter is niet naar één regel te herleiden. |
| `models_read` | `/v1/models` — is `eleven_v3` beschikbaar? | Het model achter §19.4 (audio-tags) is onbevestigd, en `modelId` zit in de cachesleutel: een verkeerde keuze herbetaalt alles. |

Zolang `user_read` ontbreekt is elk getal in de kolom *Saldo na* een **schatting**
op basis van tekens-in-de-request, en hoort het als zodanig gelezen te worden.

**Incident 31-07 — zelf veroorzaakt, opgeruimd.** Bij het bouwen van de
controleproef voor bovenstaande test draaide ik de oude codeversie met een
gestubde API, maar zonder `STAGING` om te leiden. Gevolg: zeven stubbestanden van
62 bytes in `Eclipse/Saved/AudioStaging/SFX/` en een overschreven
`manifest.json`. Die map is gitignored, dus de originele provenance van de
SFX-run van 24-07 is weg. **Geen credits verloren en geen game-assets beschadigd:**
de geïmporteerde `.uasset`-bestanden in `Eclipse/Content/Audio/{SFX,Music}` zijn
onaangeroerd en staan in git. Stub en manifest zijn verwijderd, staging is leeg.
Het gevaar zat in het manifest: `process()` ziet een manifestregel plus een bestand
op schijf als cache-hit, dus een latere echte run zou "0 credits" hebben gemeld en
de generatie stil hebben overgeslagen. Zowel de test als de controleproef schrijven
nu uitsluitend in een tempmap, en `test_blind_run_writes_nothing_to_staging` pint
dat vast.
