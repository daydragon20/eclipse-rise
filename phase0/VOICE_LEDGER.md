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
