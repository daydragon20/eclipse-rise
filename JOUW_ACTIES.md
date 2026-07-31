# JOUW ACTIES — wat Nathan moet doen, wanneer, en hoe

*Laatst bijgewerkt: 2026-07-31 · Dit is de uitvoerbare versie van de owner-tabel in `STATUS.md`.*
*Agents: werk dit bij zodra een punt afgehandeld is, en verplaats het naar "Afgerond" onderaan.*

---

## Het overzicht

| Wanneer | Wat | Hoe lang | Blokkeert |
|---|---|---|---|
| **Nu, als je zin hebt** | **O-6** Stijlvraag — "A" of "B" | 5 min | niets |
| **Nu, als je zin hebt** | **O-5** Wapen — goedkoop of volledig | 2 min | niets |
| **Rond 1–2 augustus** | **O-3** Stemmen kiezen | 30–45 min | alles wat gesproken wordt |
| **Rond 7 augustus** | **O-4** IJkmissie beluisteren | 20 min | de massaproductie |
| **Wanneer de HUD staat** | Eén sessie spelen | 30 min | niets |

**Niets hiervan houdt het werk tegen.** De agents bouwen door aan de HUD en de beat-sheets terwijl jij dit doet. O-3 en O-4 komen naar jou toe — daar hoef je niets voor te onthouden.

---

## O-6 · De stijlvraag *(nu, 5 minuten)*

**Waarom dit gevraagd wordt.** Je zei dat de wereld er *"realistisch geloofwaardig"* uit moet zien. Dat kan twee dingen betekenen, en het verschil is weken werk. Deze vraag is al één keer eerder langsgekomen (het `OWNER_MANDATE` vroeg om "realistische materialen") en werd toen opgelost in het voordeel van de stilering. Je formulering brengt hem terug, dus ik vraag het expliciet in plaats van er stil van uit te gaan.

**Stap 1.** Open in het dashboard → tab **Documenten** → `20_world_dressing_standard.md`, en lees **§20.8**. Halve pagina.

**Stap 2.** Kies:

- **A — de gestileerde look blijft.** Jouw klacht ging over verkeersborden en random opvulling. Dat zijn *compositie*problemen, en die bestaan in élke stijl. `20_world_dressing_standard.md` lost ze op. **Geen herwerk.**
- **B — je wilt echt fotorealisme.** Dan vervalt de art-lock uit §15.5, moet elk materiaal opnieuw, vervalt de toon-master, en moeten alle gestileerde assets die al binnen zijn opnieuw beoordeeld. **Weken werk.**

**Stap 3.** Zeg "A" of "B". Meer is niet nodig.

> **Ik ga uit van A.** Wil je B, dan is dat een expliciete her-lock — zeg het dan vóórdat er nog een asset landt.

---

## O-5 · Het wapen *(nu, 2 minuten)*

**Wat er aan de hand is.** Er hángt wel degelijk een wapen — dat had je zelf al aangewezen op een screenshot. Het probleem is een ander: **het wapen zit in de karaktermesh, het is geen los object.** Dat verklaart drie dingen tegelijk: er is geen attachment, er hángt toch een wapen, en de wapenwissel doet visueel niets — er is niets om te wisselen. Dat het wegvalt in het silhouet komt doordat de toon-restyle het dezelfde factietint geeft als het lichaam waar het tegenaan ligt.

Dit is dus geen inkoop maar een weging, en die is van jou:

| Optie | Werk | Lost op | Lost niet op |
|---|---|---|---|
| **Goedkoop** — het wapen een eigen tint geven | uren | "ik zie mijn wapen niet" | de zichtbare wapenwissel |
| **Volledig** — wapen uit de mesh isoleren, los asset per wapenfamilie, socket op `hand_r`, wisselogica eraan | dagen | allebei | — |

**Zeg "goedkoop" of "volledig".**

> **Mijn advies: nu goedkoop.** Dan zie je eindelijk wat je vasthoudt. Of die zichtbare wapenwissel je überhaupt stoort, weet je pas ná je eerste speelronde — en dan pas is de dure variant het waard.

---

## O-3 · Stemmen kiezen *(rond 1–2 augustus, 30–45 minuten)*

**Je krijgt bericht als het zover is.** `voice-director` maakt eerst een shortlist uit de Voice Library — dat kost geen credits, want bladeren en previews beluisteren is gratis.

**Wat jij dan krijgt:** per personage twee finalisten, die elk drie regels inspreken — de signature-regel, een emotioneel uiterste dat het personage echt haalt, en een korte gevechtsregel.

**Waar je op let:**

1. **Klinkt dit als het personage?** Lees de vingerafdruk in `18_writing_standard.md` §18.4 als je twijfelt. Brick zegt bijna niets; Kaya praat te snel; Vex verheft nooit zijn stem.
2. **Houdt hij stand bij het uiterste?** Een stem die een rustige zin prachtig draagt maar instort bij Mara's doodsscène is de verkeerde stem — hoe mooi die eerste zin ook was.
3. **Hoor je hem nog over geweervuur heen?**
4. **Klinken twee personages die samen scènes hebben niet hetzelfde?** Die krijg je daarom naast elkaar te horen.

> **Casting is permanent.** De cache-sleutel bevat de stem-ID, dus een personage later omcasten maakt élke regel van dat personage ongeldig en herbetaalt alles. Neem de tijd — je luistert er een jaar naar.

---

## O-4 · IJkmissie beluisteren *(rond 7 augustus, 20 minuten)*

**Wat dit is.** Missie M1.1 *Thirteen Bullets* gaat als eerste helemaal door de pijplijn: beats → dialoog → kwaliteitspoort → stem → in de game. Eén missie, volledig af, vóór er 41 andere gemaakt worden.

**Waarom het zo werkt.** Als de schrijfstandaard fout is, wil je dat weten bij missie 1 — niet bij missie 34. Je giet ook eerst één proefstuk voor je er 42 giet.

**Wat jij doet:** de missie spelen of beluisteren en één ding zeggen: **"zo moet het klinken" of "nee, want…"**.

Bij "nee" wordt de standaard bijgesteld en gaat M1.1 er nog één keer doorheen. **Pas na jouw akkoord** start de massaproductie van de andere 41 missies.

---

## Eén sessie spelen *(wanneer de HUD staat)*

Je speelt pas als de schermlaag op niveau is — dat is jouw eigen voorwaarde en die staat. `hud-builder` werkt er nu aan: de drie hoogtes (boots, base, map) plus Command Mode, en alles rond het vizier moet in eerste én derde persoon leesbaar zijn.

**Als het zover is:** dubbelklik **`1 - SPEEL ECLIPSE`** op je bureaublad. Druk **G** voor de ingebouwde gids.

Wat de agents daarna van je willen weten, staat op dat moment in het dashboard onder **Nu**.

---

## Afgerond

| # | Wat | Antwoord |
|---|---|---|
| **O-1** | Verloopdatum credits | **21 augustus 2026.** Werkdeadline voor generatie is 19 augustus — twee dagen buffer, want een mislukte batch op de laatste avond is niet over te doen. |
| **O-2** | Commerciële rechten | **Bevestigd 31-07.** Abonnement heeft ze, én per stem is in de Voice Library zichtbaar of die rechten draagt. `voice-director` weigert elke stem zonder licentie. |

---

## Voor agents — hoe je dit bestand gebruikt

- Een owner-actie hoort **hier** te staan met concrete stappen, en **verkort** in `STATUS.md` en in `ownerActies` in `progress_data.js`.
- Zodra Nathan antwoordt: verplaats het punt naar **Afgerond** hierboven, **mét zijn antwoord erbij geciteerd**. Een antwoord dat alleen in een chatvenster viel, bestaat voor de volgende sessie niet — dat is op 31-07 misgegaan met O-2.
- Zet hier **nooit** meer dan vijf open punten neer. Een lijst die te lang wordt, wordt niet gelezen; dat is de reden dat de vorige lijst van 22 naar 4 moest.
