# PART 21 — QUALITY MANDATE
**ECLIPSE: RISE OF THE RESISTANCE**
*Game Design Bible — Document 21 of 21 | De staande kwaliteitsopdracht*

---

> **Owner-instructie, 2026-07-31, verbatim:**
> *"Op alles wat het doet zijn graphics en uitgebreid zijn de twee dingen die de hoogste prioriteit hebben, altijd. Dus liever 3 dialogen van 20 zinnen dan 1 dialoog met 2 zinnen, en meer licht-angles of graphic points en stuff. Dus zoals ik al zei: nooit de kortste weg naar het doel, maar het meest kwalitatieve en beste, echt."*

Dit document staat boven elke werkafspraak over tempo. Wie moet kiezen tussen *sneller klaar* en *rijker*, kiest **rijker**.

---

## 21.1 De twee permanente prioriteiten

1. **Graphics** — visuele kwaliteit is nooit "goed genoeg voor nu". Elke ronde zoekt de zwakste visuele schakel en verbetert hem.
2. **Uitgebreidheid** — meer inhoud, meer diepte, meer variatie. Een systeem dat werkt met het minimum is niet af; het is begonnen.

**En de regel die beide draagt: nooit de kortste weg naar het doel.** Als er een goedkope oplossing is en een goede, is de goede de opdracht. Als iets in één versie kan of in drie, worden het er drie.

---

## 21.2 Wat dit concreet betekent, per vakgebied

### Schrijven
- **Liever drie dialogen van twintig regels dan één van twee.** Waar een scène kan bestaan, bestaat hij.
- Bark-varianten gaan naar de **bovenkant** van de band uit §18.5 — twaalf, niet zes.
- Een missie krijgt niet één briefingscène, maar een briefing, twee momenten onderweg, een reactie op de wending en een debrief.
- Elk companion krijgt reacties op wat er gebeurt, niet alleen op wat de plot vereist.
- Voss-varianten worden geschreven voor élke as waarop een scène kan vertakken, niet alleen waar het strikt moet.

### Belichting en beeld
- **Nooit één licht.** Key, fill, rim, praktische lampen in de scène zelf, bounce. Meer hoeken, meer aanleidingen voor licht.
- Elk district heeft meerdere **graphic points**: momenten waar licht, silhouet en compositie samenvallen tot een beeld dat je zou screenshotten.
- Volumetrie, atmosferische lagen, reflecties en post-processing worden gebruikt, niet overgeslagen "voor later".
- Materialen krijgen hun volle behandeling: slijtage, variatie, detailnormals, dieptes.

### Wereld
- Dichtheid is **goed** — mits gecomponeerd. Zie hieronder §21.3, want dit is het enige punt waar deze opdracht botst met een andere.
- Verticaliteit, interieurs die echt in te gaan zijn, doorkijkjes, meerdere routes.

### Systemen
- Bouw de volledige versie uit de spec, niet de helft die de test haalt.
- Randgevallen worden afgehandeld, niet gedocumenteerd als "bekend gedrag".
- Waar een systeem één parameter kan hebben of een datatabel, wordt het een datatabel.

---

## 21.3 De belangrijkste nuance — meer, niet willekeurig

Dit document zegt **meer**. `20_world_dressing_standard.md` §20.3 zegt **geen opvulling**. Die twee spreken elkaar niet tegen, maar het verschil is het hele vak:

| | |
|---|---|
| ✅ **Wat bedoeld is** | Meer inhoud die zich verantwoordt. Twintig props die samen een ruimte bouwen. Twaalf barks die twaalf verschillende toestanden dragen. Vijf lichten die één beeld maken. |
| ❌ **Wat niet bedoeld is** | Twintig props omdat de hoek leeg was. Twaalf barks die hetzelfde zeggen. Vijf lichten zonder bedoeling. |

**De toets is niet "hoeveel", maar "hoeveel dat werkt".** Een dialoog van twintig regels waarvan er vijf leeg zijn, is geen uitgebreide dialoog — dat is een dialoog van vijftien met ruis. De opdracht is twintig regels die alle twintig iets doen.

Zo blijft ook de anti-slop-gate (§18.9) volledig van kracht: **meer schrijven verlaagt de lat niet.** Het is meer werk, niet makkelijker werk.

---

## 21.4 De enige uitzondering — regellengte

Eén ding wordt niet langer: **de individuele dialoogregel**.

Een combat-bark blijft 3–8 woorden, want daarboven hoort de speler hem niet. Dat is geen bezuiniging, dat is techniek. De uitgebreidheid zit in het **aantal** regels, varianten en scènes — niet in langere zinnen.

> **Kort per regel. Gul in aantal.**

---

## 21.5 Waar dit ophoudt

Uitgebreidheid is geen vrijbrief voor:

- **Scope-uitbreiding buiten de GDD.** Meer diepte binnen wat gespecificeerd is, geen nieuwe systemen die niemand vroeg. Nieuwe scope is een owner-beslissing.
- **Een rode bar.** Kwaliteit die de build breekt of de 12.4-budgetten overschrijdt is geen kwaliteit. Groen blijft groen.
- **Credits verspillen.** Meer schrijven kost niets; meer inspreken kost geld. De tier-ladder in §19.2 blijft leidend.
- **Eindeloos doorwerken aan één bug.** `phase0/DEBUG_DISCIPLINE.md` blijft gelden — daar is de time-box juist een kwaliteitsmaatregel, want een agent die zes uur vastzit levert nul kwaliteit.

---

## 21.6 De vraag die elke agent zich stelt vóór hij oplevert

> **"Heb ik de kortste weg genomen?"**

Is het antwoord ja, dan is het werk niet af. Ga terug en doe het goed.
