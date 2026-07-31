# JOUW ACTIES — wat Nathan moet doen, wanneer, en hoe

*Laatst bijgewerkt: 2026-08-01 vroeg · Dit is de uitvoerbare versie van de owner-tabel in `STATUS.md`.*
*Agents: werk dit bij zodra een punt afgehandeld is, en verplaats het naar "Afgerond" onderaan.*

---

## Het overzicht

| Wanneer | Wat | Hoe lang | Blokkeert |
|---|---|---|---|
| **Nu — dit houdt de bar rood** | **O-3** Twee stemmen voor `eclipse_fighter` C en D, plus Petra | 15 min | acht regels die anders als **stilte** gegenereerd worden |
| **Nu — het gaat over geld** | **O-14** Act 1 kost ~97.000 van je 125.612 credits | 10 min | hoe act 1 klinkt |
| **Nu** | **O-12** 2.475 credits gingen naar het verkeerde model — opnieuw of niet? | 5 min | niets, maar het loopt op |
| **Nu** | **O-13** Drie stemmen zijn twee keer gekozen | 10 min | niets |
| **Als je zin hebt** | **O-15** Kijk naar de vier vault-kamers: is dit Hollow Point? | 5 min | niets |
| **Wacht op mij** | **O-4** IJkmissie beluisteren | — | — |

**Niets hiervan houdt het bouwen tegen** — de agents werken door aan de schermlaag, de
kaartlaag en het schrijfwerk terwijl jij dit doet. Maar **O-3 wél voor audio**: zolang twee
`eclipse_fighter`-slots leeg zijn, verdwijnen acht regels zonder foutmelding.

## ~~API-scopes~~ · *afgehandeld — dit was de rem, en hij is weg*

Je hebt de drie scopes (`user_read`, `speech_history_read`, `models_read`) aangezet. Gemeten:
alle drie geven HTTP 200, het saldo is leesbaar (**125.612 van 131.000 over**) en `eleven_v3`
blijkt op dit abonnement te bestaan — dat sluit de open vraag of §19.4's audiotags kunnen
werken.

Wat er meteen uitkwam en wat je moet weten: **de ingebouwde kostenmeting was dood en meldde
dat nergens.** Daardoor zijn er 2.475 credits naar het verkeerde model gegaan zonder dat
iets alarm sloeg. De meter is gerepareerd en weigert nu te genereren zolang de uitgave niet
meetbaar is; de vraag wat er met die 51 clips moet gebeuren staat als **O-12** op je
dashboard.

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

## O-4 · IJkmissie beluisteren *(wacht op mij, niet op jou)*

**Wat dit is.** Missie M1.1 *Thirteen Bullets* gaat als eerste helemaal door de pijplijn:
beats → dialoog → kwaliteitspoort → stem → in de game. Eén missie volledig af vóór er 41
andere gemaakt worden. Als de schrijfstandaard fout is, wil je dat weten bij missie 1 en
niet bij missie 34.

**Stand 01-08: er is nog geen seconde audio, en dat kan ook niet.** De kwaliteitspoort heeft
M1.1 op **NO-GO** gezet — drie van de zeven scènes zijn door, vier gaan terug. Eén voorbeeld
van wat hij vond: in de debrief zegt Mara *"They'll count six men missing"*, terwijl er in
twee van de drie takken vijf doden zijn en één man leeft. Dat is niet te repareren nadat het
ingesproken is.

Die vier reparaties lopen nu. **Je hoeft niets te doen**; de kaart springt vanzelf terug
zodra er iets te horen is.

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
