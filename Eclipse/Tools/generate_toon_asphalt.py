"""Genereer een HAND-GEAUTHORDE asfalt-albedo voor de toon-master.

Waarom dit bestaat: de art-review waardeerde de vloer op naar BLOKKEREND onder de
EEN-STIJL-WET. Het district draait op een 4K Megascans-foto, en dat is rauw
fotorealisme in een cel/ink-frame op 60-80% van elk beeld. Het verlagen van de
textuurmix (0.50 -> 0.20) bracht de eigen korrel van 30% naar 13% en dat is de
helft van de fix, maar een mix-waarde koopt alleen LAGE VARIANTIE. De andere twee
eisen van de review - celband-respons en zichtbare ink-reactie op grondranden -
vragen een andere textuur, geen ander getal.

Wat deze generator anders doet dan een foto, en waarom:

1. LAAGFREQUENT IN PLAATS VAN HOOGFREQUENT. Een foto van asfalt is korrel op
   korrel-schaal: op schermafstand middelt dat uit tot ruis en op close-up leest
   het als een foto. Getekend asfalt heeft juist GROTE vlakken met een enkele
   scheur - dat leest op beide afstanden als hetzelfde oppervlak. Daarom bouwt
   dit uit een paar octaven waardenruis met een lange golflengte, niet uit
   per-pixel ruis.

2. GEKWANTISEERD IN WAARDESTAPPEN. De toon-master trekt uit een albedo alleen de
   luminantie en gebruikt die als vermenigvuldiger. Een continue toon geeft dus
   een continue vermenigvuldiger - precies het fotografische verloop dat we niet
   willen. Kwantiseren naar een handvol stappen geeft de cel-respons die de
   review vraagt: het oppervlak krijgt vlakken in plaats van een gradient.

3. EXPLICIETE SCHEUREN. Een paar donkere lijnen op tekenschaal geven de inkpass
   iets om op te reageren en geven het overzichtsframe structuur die niet
   uitmiddelt. Ze zijn dun en schaars: dit is een vloer, geen craquele.

4. GEMETEN VARIANTIE ALS DOEL. De review noemt sd/gem onder ~12%. Dit script
   MEET dat op zijn eigen uitvoer en zegt het, zodat de volgende stap niet op
   een gevoel maar op een getal begint.

EERLIJK OVER DE EERSTE UITVOER (bekeken 2026-07-25): het GETAL klopt - 13,8% eigen
korrel tegen ~60% voor de foto - maar de VORMTAAL nog niet. De vlekken lezen als
camouflage of wolken, niet als asfalt: te groot en te organisch. Asfalt is een
tamelijk uniform oppervlak met subtiele reparatievlakken, niet een landschap. De
scheuren lezen bovendien als dunne krassen in plaats van als scheuren.

Wat er waarschijnlijk moet veranderen in ronde 2: het grove octaaf kleiner in
amplitude (het draagt nu de compositie in plaats van hem te kruiden), een fijner
derde octaaf voor de korrel op tegelschaal, en scheuren die vertakken vanuit een
punt in plaats van te dwalen. Dat is een look-oordeel, dus het hoort langs de
art-reviewer zodra het in een frame staat - niet nog een keer op een getal gestuurd.

Draai (geen editor nodig, alleen Pillow):
  python Eclipse/Tools/generate_toon_asphalt.py
Daarna importeren met Tools/import_generated_decals.py-patroon in een vrije slot.
"""

import math
import os
import random

try:
    from PIL import Image, ImageDraw, ImageFilter
except ImportError:
    raise SystemExit("Pillow ontbreekt: draai met de python die Pillow heeft")

HERE = os.path.dirname(os.path.abspath(__file__))
OUT_DIR = os.path.join(os.path.dirname(HERE), "Saved", "GeneratedDecals")
OUT = os.path.join(OUT_DIR, "T_toon_asphalt_diff.png")

SIZE = 1024          # tegel; de builder schaalt hem met TexWorldScale 700
VALUE_STEPS = 5      # cel-stappen: genoeg voor leven, weinig genoeg voor vlakken
TARGET_MEAN = 168    # sRGB; de gain wordt in de builder gemeten, dit is alleen ruwweg midden
SPREAD = 26          # sRGB-amplitude van de vlekken -> bepaalt de variantie
SEED = 20260725      # vast: dezelfde tegel op elke machine (determinisme, 15.5)


def value_noise(size, cells, rng):
    """Laagfrequente ruis: een klein raster, bilineair opgeblazen.

    Bewust GEEN per-pixel ruis. Een klein raster dat wordt uitgerekt geeft grote
    zachte vlekken - de schaal waarop een tekenaar asfalt zou variëren.
    """
    grid = Image.new("L", (cells, cells))
    grid.putdata([rng.randrange(256) for _ in range(cells * cells)])
    return grid.resize((size, size), Image.BICUBIC)


def main():
    os.makedirs(OUT_DIR, exist_ok=True)
    rng = random.Random(SEED)

    # Twee octaven: de grove vlekken dragen, de fijne breken de regelmaat.
    coarse = value_noise(SIZE, 6, rng)
    medium = value_noise(SIZE, 14, rng)
    coarse_px, medium_px = coarse.load(), medium.load()

    img = Image.new("L", (SIZE, SIZE))
    px = img.load()
    for y in range(SIZE):
        for x in range(SIZE):
            # 0..1, zwaartepunt op de grove laag
            n = (0.70 * coarse_px[x, y] + 0.30 * medium_px[x, y]) / 255.0
            v = TARGET_MEAN + (n - 0.5) * 2.0 * SPREAD
            # Kwantiseren NA het mengen: dat geeft vlakken met rafelige randen in
            # plaats van een schaakbord.
            step = 2.0 * SPREAD / (VALUE_STEPS - 1)
            v = TARGET_MEAN - SPREAD + round((v - (TARGET_MEAN - SPREAD)) / step) * step
            px[x, y] = int(max(0, min(255, v)))

    # Scheuren: dun, donker, en met een knik zodat ze niet als een lijnstuk lezen.
    draw = ImageDraw.Draw(img)
    for _ in range(7):
        x, y = rng.randrange(SIZE), rng.randrange(SIZE)
        angle = rng.uniform(0, math.tau)
        for _segment in range(rng.randint(3, 6)):
            length = rng.randint(60, 200)
            nx = x + math.cos(angle) * length
            ny = y + math.sin(angle) * length
            draw.line([(x, y), (nx, ny)], fill=max(0, TARGET_MEAN - SPREAD * 2), width=rng.randint(1, 2))
            x, y = nx, ny
            angle += rng.uniform(-0.6, 0.6)

    # Eén zachte pass: de kwantisatieranden mogen niet als vectorvlakken lezen,
    # maar hij moet ook niet terug naar een verloop. Radius 1 is precies genoeg.
    img = img.filter(ImageFilter.GaussianBlur(radius=1.0))

    img.convert("RGB").save(OUT)

    # Meten, niet hopen (15.5). Dit is dezelfde grootheid als de frame-probe:
    # sRGB -> lineair, en dan sd/gem.
    lin = [(c / 255.0) / 12.92 if (c / 255.0) <= 0.04045 else (((c / 255.0) + 0.055) / 1.055) ** 2.4
           for c in range(256)]
    values = [lin[v] for v in img.tobytes()]
    mean = sum(values) / len(values)
    sd = math.sqrt(sum((v - mean) ** 2 for v in values) / len(values))
    print(f"geschreven: {OUT}  ({SIZE}x{SIZE})")
    print(f"lineair gemiddelde {mean:.4f}   sd {sd:.4f}   sd/gem {100 * sd / mean:.1f}%")
    print(f"gemeten gain voor de builder (1/gemiddelde): {1.0 / mean:.2f}")
    print(f"waardestappen: {VALUE_STEPS}   doel review: sd/gem onder ~12%")
    # De Megascans-foto meet ~60% eigen korrel (afgeleid: mix 0.50 gaf 30% in het
    # frame). Deze tegel is dus ruim vier keer rustiger, en dat is het punt: de
    # MIX kan straks terug omhoog naar 0.50 zodat de vlakken en scheuren echt
    # lezen, terwijl de framevariantie ruim onder het doel blijft. Lagere mix was
    # een noodgreep om een foto te dempen; met een getekende tegel is dempen niet
    # meer nodig.
    print(f"voorspelling frame-variantie bij mix 0.50: ~{100 * sd / mean * 0.5:.0f}% "
          f"(foto gaf 30%) - te verifieren met een shotronde")


if __name__ == "__main__":
    main()
