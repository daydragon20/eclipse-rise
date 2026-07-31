# JOUW ACTIES — wat Nathan moet doen, wanneer, en hoe

*Laatst bijgewerkt: 2026-07-31 laat · Dit is de uitvoerbare versie van de owner-tabel in `STATUS.md`.*
*Agents: werk dit bij zodra een punt afgehandeld is, en verplaats het naar "Afgerond" onderaan.*

---

## Het overzicht

| Wanneer | Wat | Hoe lang | Blokkeert |
|---|---|---|---|
| **Nu — dit houdt wél iets tegen** | **API-scopes** aanzetten bij ElevenLabs | 3 min | **alle generatie staat stil** |
| **Nu, het staat klaar** | **O-3** Stemmen kiezen — 104 kandidaten | 30–45 min | alles wat gesproken wordt |
| **Als je zin hebt** | **O-8** Hoe Borderlands mag de HUD worden | 5 min | de vormgeving van de schermlaag |
| **Rond 6–7 augustus** | **O-4** IJkmissie beluisteren | 20 min | de massaproductie |
| **Wanneer de HUD af is** | Eén sessie spelen | 30 min | niets |

**Op één na houdt niets hiervan het werk tegen.** De agents bouwen door aan het wapen, de
schermlaag, Command Mode en de verkeersborden terwijl jij dit doet. Maar **de scopes wél**:
zonder die komt er geen enkele regel audio uit, ook niet als je vanavond stemmen kiest.

---

## API-scopes · *dit is de enige echte rem (nu, 3 minuten)*

**Wat er aan de hand is, en het is erger dan het klinkt.** De ingebouwde creditmeter was
stuk **en zei dat niet**. `generate_audio_assets.py` vroeg bij ElevenLabs op wat een batch
kost, kreeg daar een `401` op, **slikte die fout** en schreef stil geen verbruik weg. De
ledger las dus als waarheid terwijl er niets gemeten was — en die 310.000 was daarmee een
mededeling van jou, geen meting.

Dat is nu omgedraaid: generatie **weigert** te draaien zonder gemeten spend (exit-code 3),
en die weigering hangt als poort in `verify.ps1` zodat hij niet stilletjes terug kan komen.

**Stap 1.** Ga naar elevenlabs.io → je avatar rechtsboven → **API Keys**.

**Stap 2.** Zet op de sleutel van dit project drie scopes aan:

| Scope | Waarvoor | Zonder |
|---|---|---|
| `user_read` | saldo lezen vóór en ná elke batch | **dit is de blokkade — er komt niets uit** |
| `speech_history_read` | achteraf per regel controleren wat het kostte | een uitschieter is niet te herleiden |
| `models_read` | checkt of `eleven_v3` beschikbaar is | de audio-tags uit §19.4 hangen eraan, en `modelId` zit in de cache-sleutel — een verkeerde gok herbetaalt alles |

**Stap 3.** Zeg het als het staat. Dan meet ik het echte saldo na en zet dat in de ledger.

> **Goed nieuws:** je saldo staat nog op de volle **310.000**. Er is deze maand geen enkele
> credit uitgegeven — ook casting fase 1 niet.

---

## O-3 · Stemmen kiezen *(staat klaar, 30–45 minuten)*

**104 kandidaten over 18 rollen, en het kostte nul credits.** Dat is jouw eigen verdienste:
je wees erop dat je voor het kiezen van een stem niet hoeft te betalen omdat de Voice
Library gratis doorzoekbaar is. Er lag een script klaar om er 2.842 credits doorheen te
jagen; dat is niet gedraaid.

**Open:** http://127.0.0.1:8377/progress_media/casting/CASTING.html — één pagina, per
personage gegroepeerd, speelt direct af.

**Wat je hoort, en dit stuurt je verwachting:** de **gratis previews** met algemene
voorbeeldtekst, **geen ECLIPSE-dialoog**. Je beoordeelt dus timbre, leeftijd, accent en
register — geen acteerwerk. Dat is de prijs van nul credits, en precies waarom er een
tweede ronde bestaat.

**Geef per rol een TOP 2, geen winnaar.** Die twee finalisten krijgen daarna je échte tekst
te spreken — de signature-regel, een emotioneel uiterste dat het personage echt haalt, en
een korte gevechtsregel — en pas dán wordt er gelockt.

**Waar je op let:**

1. **Klinkt dit als het personage?** Vingerafdrukken staan in `18_writing_standard.md`
   §18.4. Brick zegt bijna niets; Kaya praat te snel; Vex verheft nooit zijn stem.
2. **Houdt hij stand bij het uiterste?** Een stem die een rustige zin prachtig draagt maar
   instort bij Mara's doodsscène is de verkeerde stem, hoe mooi die eerste zin ook was.
3. **Hoor je hem nog over geweervuur heen?**
4. **Klinken twee personages die samen scènes hebben niet hetzelfde?**

> **Casting is permanent.** De cache-sleutel bevat de stem-ID, dus een personage later
> omcasten maakt élke regel van dat personage ongeldig en herbetaalt alles. Neem de tijd —
> je luistert er een jaar naar.

**Eén eerlijke kanttekening.** Bij de bibliotheekstemmen kon één poort níét geautomatiseerd
worden: of een stem commerciële rechten draagt staat niet in de API — dat veld bestaat niet,
en een verzonnen parameter geeft byte-identieke resultaten. Wat wél automatisch ging is
toegepast: alleen makers met status *professional* of *high_quality*, accent-neutraal, en een
opzegtermijn van minstens 365 dagen. Die laatste poort verdiende zich meteen terug — een
sterke Kaine-kandidaat had opzegtermijn 0, dus die stem kan van de ene op de andere dag
verdwijnen. Elke bibliotheekstem draagt daarom `NIET_GEVERIFIEERD` plus een link naar zijn
kaart: **één klik per finalist, niet per kandidaat.**

---

## O-8 · Hoe Borderlands mag de HUD worden *(5 minuten)*

Je hebt met **O-6 = A** de gestileerde lock bevestigd. De vervolgvraag is hoe ver dat in de
schermlaag mag doorwerken: dikke inktlijnen en comic-panelen, of een strakke leesbare laag
die alleen de kleurtaal deelt. Dat bepaalt hoeveel werk het is én hoe leesbaar het blijft
tijdens een gevecht.

De vraag staat als kaart met knoppen op je dashboard.

---

## O-4 · IJkmissie beluisteren *(rond 6–7 augustus, 20 minuten)*

**Wat dit is.** Missie M1.1 *Thirteen Bullets* gaat als eerste helemaal door de pijplijn:
beats → dialoog → kwaliteitspoort → stem → in de game. Eén missie volledig af vóór er 41
andere gemaakt worden. De beats liggen er al.

**Waarom het zo werkt.** Als de schrijfstandaard fout is, wil je dat weten bij missie 1 —
niet bij missie 34. Je giet ook eerst één proefstuk voor je er 42 giet.

**Wat jij doet:** de missie spelen of beluisteren en één ding zeggen: **"zo moet het
klinken" of "nee, want…"**. Bij "nee" wordt de standaard bijgesteld en gaat M1.1 er nog één
keer doorheen. **Pas na jouw akkoord** start de massaproductie.

---

## Eén sessie spelen *(wanneer de HUD af is)*

Je speelt pas als de schermlaag op niveau is — je eigen voorwaarde, en die staat.

**Stand:** de poort is gehaald. De spelerlaag stond per constructie niet op een opname en
was dus niet te controleren; dat is opgelost en op frames geverifieerd. Je ziet nu in beide
perspectieven het richtkruis, en in derde persoon je wapennaam en munitie. Wat er nog niet
is: gezondheid, minimap, squad-kaarten, en de schermlagen van **base** en **map** — die twee
zijn nu nog kale tekstlijsten.

**Als het zover is:** dubbelklik **`1 - SPEEL ECLIPSE`** op je bureaublad. Druk **G** voor
de ingebouwde gids.

---

## Afgerond

| # | Wat | Antwoord |
|---|---|---|
| **O-1** | Verloopdatum credits | **21 augustus 2026.** Werkdeadline voor generatie is 19 augustus — twee dagen buffer, want een mislukte batch op de laatste avond is niet over te doen. |
| **O-2** | Commerciële rechten | **Bevestigd 31-07.** Abonnement heeft ze, én per stem is in de Voice Library zichtbaar of die rechten draagt. |
| **O-5** | Wapen | **"Volledig"** — het wapen gaat écht uit de karaktermesh, niet de goedkope tint-stap. Vier falsificeerbare stappen; loopt bij een element-builder. Het first-person-frame bevestigde de keuze: daar is *geen enkel* wapen zichtbaar, want zodra het lichaam niet gerenderd wordt verdwijnt het geweer mee. |
| **O-6** | Stijlvraag | **"A"** — de Borderlands-lock blijft. Geen her-lock, geen materiaal opnieuw, toon-master blijft. Deblokkeerde meteen de vormgeving van base en map. |
| **O-7** | GPU-crash SM5/SM6 | **"Kies jij"** — en dat kan, want de crash is herleid tot de SkyAtmosphere-pass en niet tot de zware belichtingspaden. |
| **T-11** | R3-verdict feel-gauntlet | **"True"** — Command Mode overleeft echte gevechten. Opende SPEC-P2-02 Stage B, dat een week dichtstond. |
| **T-2 / T-10** | Env-packs / downloads | **Gedaan.** Tien packs staan binnen. |

---

## Voor agents — hoe je dit bestand gebruikt

- Een owner-actie hoort **hier** te staan met concrete stappen, en **verkort** in
  `STATUS.md` en in `ownerActies` in `progress_data.js`.
- Zodra Nathan antwoordt: verplaats het punt naar **Afgerond** hierboven **mét zijn
  antwoord**, én zet het in `phase0/OWNER_ANSWERS.md`. Een antwoord dat alleen in een
  chatvenster viel bestaat voor de volgende sessie niet — dat ging op 31-07 mis met O-2, en
  later diezelfde avond nog eens met O-5 en O-6, die daardoor opnieuw op zijn kliklijst
  verschenen.
- Zet hier **nooit** meer dan vijf open punten neer. Een lijst die te lang wordt, wordt niet
  gelezen; dat is de reden dat de vorige lijst van 22 naar 4 moest.
