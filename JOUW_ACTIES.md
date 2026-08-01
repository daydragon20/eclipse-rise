# JOUW ACTIES — wat Nathan moet doen, wanneer, en hoe

*Laatst bijgewerkt: 2026-08-01 vroeg · Dit is de uitvoerbare versie van de owner-tabel in `STATUS.md`.*
*Agents: werk dit bij zodra een punt afgehandeld is, en verplaats het naar "Afgerond" onderaan.*

---

## Het overzicht

*Bijgewerkt 01-08 laat. **O-3 en O-13 bestaan niet meer als losse kaarten**: ze zijn samen
één kaart geworden, **O-16**, met één voorstel en één knop. Wat elf losse keuzes waren, is nu
één akkoord — met de mogelijkheid er twee uit te pikken en de rest te laten lopen.*

| Wanneer | Wat | Hoe lang | Blokkeert |
|---|---|---|---|
| **NU — dit is de enige echte rem** | **O-16**: tien stemmen liggen als voorstel klaar, met een fragment per stem en één regel waarom. **Er staan nu 39 scènes / 55.729 credits aan tekst klaar die alleen hierop wacht** — vannacht was dat 26 scènes | 5 min | **alle audio.** Er wordt geen seconde ingesproken tot dit weg is |
| **Nu — het gaat over geld** | **O-14** Act 1 zou 97.659 van je 125.612 kosten — maar je beslist vandaag over **55.729** (wat door de poort is). En de helft van act 1 is één man: Voss kost 41.140 doordat hij in twee stemmen spreekt | 10 min | hoe act 1 klinkt |
| **Nu** | **O-12** 2.475 credits gingen naar het verkeerde model — opnieuw of niet? | 5 min | niets, maar het loopt op |
| **Als je zin hebt** | **O-15** Kijk naar de vier vault-kamers: is dit Hollow Point? | 5 min | niets |
| **Wacht op mij** | **O-4** IJkmissie beluisteren | — | — |

**Dat voorstel ligt er nu.** Tien open slots, tien stemmen, negentien fragmenten om af te
spelen (tien voorstellen plus negen alternatieven), en één regel waarom per keuze. Het staat
op je dashboard als **O-16** en op
`progress_media/casting/CASTING.html` onder *Het voorstel van 01-08*. **Nul credits:** het
saldo stond voor én na deze ronde op 125.612, in beide richtingen gemeten.

**Niets hiervan houdt het bouwen tegen** — de schermlaag, de kaartlaag en het schrijfwerk
lopen door terwijl jij dit doet.

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

## O-16 · de castingblokkade, nu als één voorstel *(5 minuten, en het houdt alles tegen)*

**Wat er mis is.** Drie stem-ID's staan op **twee rollen tegelijk**, en dat is niet
"lijkt op elkaar" — het is dezelfde ID, dus dezelfde cachesleutel, dus **letterlijk
dezelfde stem**:

| stem | rol 1 | rol 2 |
|---|---|---|
| Matilda | **Mara** (328 regels) | een Eclipse-schutter (52) |
| Liam | **Dex** (267 regels) | een Dominion-dienstplichtige (9) |
| Eric | Threx (28) | een Veil-operative (8) |

Mara zou dus klinken als een willekeurige schutter, en Dex als een tegenstander in
hetzelfde vuurgevecht. Daarnaast staan Laura, Alice en Daniel elk op twee personages, en
zijn er **twee `eclipse_fighter`-slots leeg** — acht regels die zonder foutmelding als
**stilte** gegenereerd zouden worden.

**Waarom het nu niet kan wachten.** Casting is **permanent**: de stem-ID zit in de
cachesleutel, dus wie later wisselt **herbetaalt elke regel van dat personage**. Bij de
ijkmissie hing daardoor **66% van de kosten** aan stemmen die nog konden bewegen. Daarom is
die generatie afgeblazen met **nul credits uitgegeven** — dat was de juiste uitkomst, en
mijn opdracht om te generen was fout.

**Dit had ik moeten zien.** Ik meldde "alle acht stemmen resolven" en dat wás waar. Maar
*resolven* is niet *uniek* resolven, en die controle bestond niet. Hij bestaat nu wel.

**Wat jij doet:** één voorstel goedkeuren. Bij elke botsing blijft het **personage** staan en
krijgt de **kant met de minste regels** een verse stem — 52, 9 en 8 regels tegen 328, 267 en
28, en bij de andere drie 0 tegen 325, 0 tegen 92 en 0 tegen 4. Dat is veruit de goedkoopste
kant om later nog te kunnen bewegen.

**Twee dingen die ik onderweg gemeten heb en die het voorstel veranderd hebben.** Eclipse-
vechter B is in het script een **man** (*"It is the man who said On the new one, right"*),
terwijl er nu een vrouwenstem aan hangt — die stond dus los van de botsing al verkeerd. En
vechter **C en D zijn geen mensen van jou**: al hun acht regels staan in M1.5 en het script
noteert bij hun regels *"Iron Chorus: numbers, never names"* tegenover *"Ember: names"*. Ze
zijn daarom gecast als de mensen van de rivaal, niet als jouw squad.

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
