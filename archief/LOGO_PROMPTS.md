# Logo — gekozen beelden en de prompts die ze maakten

## ✅ DE KEUZE (27-07-2026)

Vier beelden gegenereerd met Gemini, elk met een andere seed. Alle vier hadden de
spelling correct — uitzonderlijk voor een beeldgenerator, en het scheelde
nawerk. Ze staan in `brand/`.

| Bestand | Rol | Waarom deze |
|---|---|---|
| **`brand/ECLIPSE_logo_wordmark.png`** | **Het logo** | Schoonste letterwerk van de vier. De eclips zit achter de tekst met de corona ertussenuit, en het silhouet houdt stand bij verkleinen. Dit is wat op je titelscherm hoort |
| **`brand/ECLIPSE_titelkaart.png`** | **Key art / trailerkop** | Doet als enige wat de andere drie niet doen: rechts staan de Dominion-torens in koel wit-goud tegenover het sodium-oranje. Dat is de kleurspanning uit `03_world_design.md` letterlijk in beeld |
| **`brand/ECLIPSE_stencil_ingame.png`** | **In-game element** | Hoort niet in de prullenbak maar op een muur in Kessara. Een gestencild verzetsmerk is precies wat het verzet achterlaat, en het is de tegenhanger van de propagandaborden die er al staan. Niet bedoeld als logo — te veel detail om klein te lezen |
| `brand/ECLIPSE_wordmark_alt_afgevallen.png` | afgevallen | Zelfde compositie als het wordmark maar met meer ruis: strepen door de letters, drukke achtergrond, hardere cyaan rand |

### Wat er nog aan moet vóór productiegebruik

1. **De cyaan randlijn weghalen** (zit op drie van de vier). Cyaan staat niet in
   het Kessara-palet — sodium-oranje, wit-goud en void-black wel; neon-cyan hoort
   bij Vel'Naar. Vervang hem door wit-goud, dan versterkt hij de kleurspanning in
   plaats van een vierde kleur toe te voegen.
2. **De zwarte balk bovenaan de titelkaart** wegsnijden — crop-artefact.
3. **Test op 64 pixels.** Het wordmark overleeft dat waarschijnlijk; de stencil
   zeker niet, en die is daar ook niet voor bedoeld.

### Wat nog ontbreekt

**Variant 2 — alleen het symbool.** Nog niet gegenereerd, en nodig voor de
Steam-tegel, het app-icoon en de favicon. Een wordmark werkt niet op 64×64; daar
moet de zwarte schijf met corona alleen staan. De prompt staat hieronder.

---

## De prompts

*Voor een beeldgenerator (Midjourney, DALL·E, Gemini, Firefly). Plak één prompt
in zijn geheel. Genereer er meerdere per variant — een logo kies je uit twintig,
niet uit één.*

**De identiteit waar alles op rust** (uit `01_game_vision.md` en
`03_world_design.md`):

- Titel: **ECLIPSE: RISE OF THE RESISTANCE**
- Stijl: cel-shaded, Borderlands-familie — dikke inktcontouren, bold shapes,
  graphic-novel-durf. **Nooit fotorealistisch, nooit een 3D-render.**
- De centrale kleurspanning van de hele game: **sodium-oranje (het verzet)
  tegen Dominion wit-goud (het imperium)**
- Het thema: een naamloze burger wordt de generaal van een galactische opstand

De naam doet het werk: een *eclipse* is het moment dat het licht van het
imperium wordt weggenomen. Dat is het symbool, en het is er al.

---

## Variant 1 — Wordmark met eclips-symbool (de hoofdvariant)

```
Game logo for a sci-fi action-strategy game titled "ECLIPSE: RISE OF THE
RESISTANCE". Cel-shaded comic-book style with heavy black ink outlines, bold
flat colour fills, and a graphic-novel confidence — in the visual family of
Borderlands, never photorealistic and never a 3D render.

The word ECLIPSE dominates: wide, heavy, industrial sans-serif letterforms with
thick black outlines, slightly weathered and scratched as if stencil-sprayed on
metal. The letters are filled with a burning sodium-orange gradient. Behind and
between the letters sits an eclipsed sun: a solid black disc with a fierce
sodium-orange corona flaring out from its edge, the disc partially overlapping
the letterforms so the corona light bleeds between them.

Below, in a smaller and cleaner condensed typeface, the subtitle RISE OF THE
RESISTANCE in pale imperial white-gold — a deliberate contrast with the orange
above it.

Colour palette strictly: sodium orange, deep void black, imperial white-gold,
and a thin cold cyan rim light. Dark background, high contrast, strong
silhouette that reads at small size. Centred composition, symmetrical, poster-
ready.

No photorealism, no 3D render, no lens flare photography, no gradients beyond
flat cel bands, no extra text.
```

## Variant 2 — Alleen het symbool (voor Steam-tegel, icoon, pictogram)

```
Minimal game icon for a sci-fi rebellion game. Cel-shaded comic style with
heavy black ink outlines and flat bold colour — Borderlands visual family, not
photorealistic, not a 3D render.

A single powerful emblem: a solid black circular disc — an eclipsed sun — with
a fierce sodium-orange corona flaring asymmetrically from behind it, brightest
at the lower left. Cutting through the disc, a stylised clenched fist rendered
as a hard negative-space silhouette, so the fist is formed by the orange light
breaking through the black disc rather than drawn on top of it.

Thick uneven ink outline around the whole emblem, slightly hand-drawn. Flat
cel-shaded bands, no soft gradients. Deep void-black background.

Palette: sodium orange, void black, a single thin imperial white-gold accent
ring at the disc's edge.

Square composition, centred, high contrast, reads clearly at 64x64 pixels. No
text, no letters, no photorealism.
```

## Variant 3 — Gestencild verzetsmerk (ruwer, straat-kant)

```
Resistance faction logo for a sci-fi game, painted as stencil graffiti on a
dirty industrial metal wall. Cel-shaded comic-book rendering with heavy black
ink outlines and flat colour — Borderlands visual family, never photorealistic.

The word ECLIPSE stencil-sprayed in blocky military letterforms, sodium-orange
paint with sprayed overspray edges, drips running from the bottom of the
letters, and small gaps where the stencil bridges were. Above the word, a small
eclipsed-sun mark: black disc, orange corona.

The wall behind is cold grey-blue corrugated metal in flat cel bands, with
sparse grime and a few rivets. A single warm sodium light source from the lower
left rakes across it.

Palette: sodium orange, cold grey-blue, void black, one small imperial
white-gold detail. High contrast, strong silhouette.

No photorealism, no 3D render, no photographic texture, no extra text.
```

## Variant 4 — Titelkaart, breed (voor een trailer of de kop van een pagina)

```
Wide title card for a sci-fi action-strategy game, 16:9. Cel-shaded comic-book
style, heavy black ink outlines, flat bold colour fills — Borderlands visual
family, never photorealistic, never a 3D render.

Centre: the title ECLIPSE in massive weathered industrial letterforms filled
with sodium orange, thick black outlines. An eclipsed sun — black disc, flaring
orange corona — sits behind the letters, its corona light spilling between them.
Below: RISE OF THE RESISTANCE in small clean condensed white-gold type, widely
letterspaced.

Behind everything, far back and low-contrast so it never competes: the flat
silhouette of a smog-choked industrial skyline in deep blue-grey, with a few
tiny warm sodium window lights, and the black geometric silhouettes of imperial
towers rising on the right in cold white-gold rim light.

Palette: sodium orange, void black, imperial white-gold, deep blue-grey. Strong
horizontal composition, empty space at the edges, everything readable at a
glance.

No photorealism, no 3D render, no characters, no extra text.
```

---

## Wat je moet controleren aan het resultaat

1. **Leest het op klein formaat?** Verklein het naar 64 pixels. Zie je nog wat
   het is, dan klopt het silhouet. Zo niet: te veel detail.
2. **Zijn er echte inktcontouren?** Geen contour betekent dat de generator naar
   fotorealisme is afgedwaald — hergenereer met "heavy black ink outlines"
   vooraan.
3. **Klopt de kleurspanning?** Sodium-oranje moet vechten tegen wit-goud. Wordt
   alles oranje, dan verdwijnt het verhaal uit het beeld.
4. **Is de tekst correct gespeld?** Beeldgeneratoren verhaspelen letters. Bijna
   altijd moet de tekst er achteraf overheen — genereer dan variant 2 (alleen
   het symbool) en zet de titel er zelf bij.

## Waarom deze richting

De naam is het concept. Een eclips is het moment dat licht wordt weggenomen —
precies wat het Dominion met de melkweg doet. De corona die er omheen blijft
branden is het verzet: het licht dat er nog is omdat het niet volledig bedekt
kán worden. Daarom staat de zon zwart en de rand oranje, en niet andersom.
