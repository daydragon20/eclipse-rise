# VOICE LEDGER — wat er echt is uitgegeven
*Eigenaar: `voice-director`. Bijwerken na ELKE generatiebatch, zonder uitzondering.*
*De ladder in `19_voice_production.md` §19.2 is een plan. Dit bestand is de waarheid.*

---

**Budget:** 310.000 credits · **vervalt 21 augustus 2026, rolt niet door**
**Werkdeadline generatie:** 19 augustus (twee dagen buffer)
**Doel op 19/08: 310.000 besteed, 0 over.** Credits die op 21 augustus ongebruikt zijn, zijn weggegooid.

**De reserve (20.000) is geen spaarpot maar verzekering.** Hij blijft *niet-toegewezen* tijdens de sprint, zodat een misgelopen batch nog te herstellen is. Op **17 augustus** gaat wat er nog van over is alsnog de volgende tier in. Niet eerder toewijzen, niet laten staan.

**Let op — de gratis 10.000/maand tellen NIET mee.** Het gratis niveau geeft geen commerciële rechten en vereist naamsvermelding; die audio mag niet in een verkochte game. Na 21 augustus is de route: **Starter, ~€5/maand voor 30.000 credits mét commerciële rechten** (zie `19_voice_production.md` §19.2).

## Ledger

| Datum | Tier | Scope | Regels | Credits | Saldo na |
|---|---|---|---|---|---|
| 2026-07-31 | 0 | Casting stage 1 — brede screening, 18 rollen × 4–6 kandidaten (premade) | 0 | 0 | 310.000 |
| 2026-07-31 | 0 | Casting stage 1 uitbreiding — Voice Library, 24 kandidaten voor Mara/Kaine/Vex/Callis/Torren | 0 | 0 | 310.000 |

> **Formaat:** het dashboard leest deze tabel automatisch uit. Houd de kolomvolgorde
> aan en begin de datumkolom met `JJJJ-MM-DD`, anders telt de regel niet mee.

## Tier-voortgang

| Tier | Wat | Begroot | Besteed | Klaar |
|---|---|---|---|---|
| 0 | Casting — fase 1 GRATIS (bibliotheek), fase 2 diepe test op finalisten | 12.000 | 0 | ☐ |
| 1 | Bark- & systeembibliotheek | 93.000 | 0 | ☐ |
| 2 | Act 1 story-dialoog | 90.000 | 0 | ☐ |
| 3 | Adaptieve muziek | 43.000 | 0 | ☐ |
| 4 | Act 1 hub & companions | 33.000 | 0 | ☐ |
| 5 | Kern-SFX | 19.000 | 0 | ☐ |
| — | Reserve (niet toewijzen; 17/08 vrijgeven) | 20.000 | 0 | — |
| | **Totaal** | **310.000** | **0** | |

*Tier 0 is twee keer bijgesteld. Eerst 5.000 → 18.000 op owner-instructie "doe moeite om de juiste voice te vinden". Daarna 18.000 → **12.000**, want de owner wees erop dat de brede screening **niets hoeft te kosten**: de Voice Library is gratis doorzoekbaar, met eigenschappen én previewfragmenten per stem. Alleen de diepe test op de twee finalisten kost credits, omdat een generieke preview niet vertelt of een stem Mara's doodsscène overleeft of over geweervuur heen leesbaar blijft. De 6.000 die vrijkwam is terug naar tiers 1–3.*

## Aantekeningen

*Onverwachte spend hier noteren, met de oorzaak. Een batch die credits kost terwijl
hij uit cache had moeten komen, betekent dat tekst of een audio-tag is veranderd —
zoek uit wát vóór je opnieuw draait.*

**2026-07-31 — casting stage 1 afgerond, 0 credits.** 18 rollen, 80 kandidaat-slots,
uitsluitend metadata + de gratis previewfragmenten. Geen enkele TTS-call gedaan.
Resultaat staat in `progress_media/casting/` (zie `CASTING.html`). Stage 2 is
**bevroren** tot de owner (a) O-2 beantwoordt en (b) per rol een top 2 kiest.

**⚠ Deze ledger kan zichzelf niet controleren.** De API-sleutel in
`Eclipse/Config/UserSecrets.ini` is *scoped* en mist de rechten `user_read`,
`speech_history_read` en `models_read`. Gemeten op 31-07:

| Endpoint | Resultaat |
|---|---|
| `/v1/voices`, `/v1/shared-voices` | 200 — werkt |
| `/v1/user/subscription` | 401 — `missing permission user_read` |
| `/v1/history` | 401 — `missing permission speech_history_read` |
| `/v1/models` | 401 — `missing permission models_read` |

**Gevolgen, en ze zijn niet klein:**
1. Het werkelijke saldo is **niet uit te lezen**. De 310.000 is een owner-mededeling,
   geen meting. Elk saldo in dit bestand is dus een *aftreksom*, geen waarneming.
2. `Eclipse/Tools/generate_audio_assets.py` roept `get_usage()` aan om spend te meten.
   Die functie vangt de fout af en geeft `None` terug — het script schrijft dan
   stilzwijgend géén `usage_credits` weg. **De ingebouwde kostenmeting is dood en
   meldt dat niet.**
3. Zonder `models_read` is niet te controleren of `eleven_v3` op dit abonnement
   beschikbaar is. Dat is geen detail: §19.4 hangt volledig op v3-audiotags, en
   `high_quality_base_model_ids` van de premade-stemmen noemt v3 **niet**.

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

### Owner-actie: drie scopes op de API-sleutel

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
