# DE TWEE OPACITEITSMASKERS VAN HET INSLAGSPOOR (DEBUG_DISCIPLINE 4.3, maatronde).
#
# WAAROM DIT BESTAND ER IS EN NIET IN generate_decals.py STAAT. Dat bestand wordt
# op dit moment door een tweede agent aangeraakt (de propagandaposter), en het
# schrijft ALLE maskers van het district in een gedeelde staging-map die de
# importeur vervolgens in zijn geheel inleest. Er iets aan toevoegen betekent dus
# ook de poster van iemand anders opnieuw uitrollen. Eigen generator, eigen
# staging-map, eigen importeur: geen botsing mogelijk.
#
# WAT HET OPLOST — en dit is een GEMETEN oorzaak, geen smaak.
#
# Het spoor gebruikte T_blob_mask. Dat masker is een smoothstep over een
# overgangsband van 0,55: het is pas VOL ondoorzichtig tot r = 0,45 en zakt daarna
# geleidelijk naar nul op de rand van het vlak. Op een spoor van 9 cm betekende dat
# een effectief dekkende kern van ongeveer 4 cm; meer dan de helft van de maat was
# al weg voordat er een pixel getekend werd. Bij een spoor dat vanaf 10 m maar
# negen pixels breed is, valt die zachte flank bovendien volledig binnen de
# antialiasing en levert hij pixels met te weinig contrast om nog van asfalt te
# verschillen.
#
# DAAROM TWEE MASKERS EN NIET EEN GROTERE BLOB:
#
#   T_impact_ring_mask  de INKTRING. Een gesloten annulus met een harde rand.
#                       Dit is tegelijk de stijlwet (15.5: cel + inktlijn) en de
#                       contrastreserve: de ring is DONKER, de vulling is HEET.
#                       Loopt het asfalt licht, dan draagt de donkere ring het
#                       contrast; loopt het donker, dan de hete vulling. Een enkele
#                       amberkleurige vlek heeft die tweede kant niet.
#   T_impact_core_mask  de HETE VULLING. Vol dekkend tot vlakbij de ring, met een
#                       gerafelde spatrand, zodat het als inslag leest en niet als
#                       stip.
#
# Beide zijn OPACITEITSMASKERS (M_EclipseToonDecal bemonstert mask.r * OpacityScale)
# en dus LINEAIRE data, geen kleur — de importeur zet sRGB uit en comprimeert
# grijswaardig. De kleur komt uit het palet, nooit uit deze bestanden.
#
# EN DE RAND WORDT NOOIT AANGEZET, ALLEEN INGEDRUKT. Elke ruisterm vergroot de
# bemonsterde straal en kan de dekking dus alleen maar KLEINER maken. Dat is de les
# van de eerste vlekkenronde: een falloff die op de rand van het vlak nog waarde
# draagt, leest als een tapijttegel — je ziet het vierkant en niet de vlek.
#
# Draaien:  python Eclipse/Tools/generate_impact_masks.py
#           daarna Eclipse/Tools/import_impact_masks.py via UnrealEditor-Cmd.

import math
import os
import random

from PIL import Image

OUT = os.path.join(os.path.dirname(__file__), "..", "Saved", "GeneratedImpactMasks")
os.makedirs(OUT, exist_ok=True)

S = 512


def save(img, name):
    path = os.path.abspath(os.path.join(OUT, name))
    img.save(path)
    print(f"wrote {path}")


def value_noise(size, cells, rng):
    small = Image.new("L", (cells, cells))
    small.putdata([rng.randint(0, 255) for _ in range(cells * cells)])
    return small.resize((size, size), Image.BICUBIC)


def hard_edge(distance, width):
    """1 binnen, 0 buiten, met een overgang van `width` in genormaliseerde straal.

    Geen smoothstep over een halve radius zoals T_blob_mask: `width` is hier
    ongeveer twee texels breed, precies genoeg om de trapjes weg te halen zonder
    contrast in te leveren. Dat verschil IS de reparatie.
    """
    return max(0.0, min(1.0, distance / width))


# --- De inktring ------------------------------------------------------------
# Binnenrand 0,58 / buitenrand 0,96 van de ingeschreven schijf. De buitenrand
# blijft onder 1,0 zodat de dekking op de rand van het vlak gegarandeerd nul is.
#
# De ring is niet rond maar HANDGETROKKEN: de straal wordt per hoek gemoduleerd
# door twee lage harmonischen plus ruis. Een perfecte cirkel leest als een
# vectorvorm en breekt de stijl (§20.5 patroonbreuk) — en drie identieke perfecte
# cirkels naast elkaar op de weg leest als een sjabloon, niet als inslagen.
RING_IN, RING_OUT = 0.58, 0.96
ring_rng = random.Random(781)
ring_noise = value_noise(S, 11, ring_rng).load()
_ring_phase = [ring_rng.uniform(0.0, math.tau) for _ in range(4)]

ring = Image.new("L", (S, S), 0)
ring_px = ring.load()
for y in range(S):
    for x in range(S):
        nx, ny = (x + 0.5) / S - 0.5, (y + 0.5) / S - 0.5
        r = math.hypot(nx, ny) * 2.0
        a = math.atan2(ny, nx)
        # Alleen INDRUKKEN: elke term maakt de bemonsterde straal groter, dus de
        # ring krimpt. Zo raakt hij nooit de rand van het vlak.
        wobble = 1.0
        wobble += 0.035 * (0.5 + 0.5 * math.sin(3.0 * a + _ring_phase[0]))
        wobble += 0.028 * (0.5 + 0.5 * math.sin(7.0 * a + _ring_phase[1]))
        wobble += 0.055 * (ring_noise[x, y] / 255.0)
        rr = r * wobble
        m = hard_edge(RING_OUT - rr, 0.018) * hard_edge(rr - RING_IN, 0.016)
        # Vier spatstralen die de ring naar buiten doorbreken: een inslag is een
        # ster, geen o. Ze verlagen de effectieve buitenstraal lokaal, dus ook
        # hier groeit er niets naar de rand toe.
        for k in range(4):
            spoke = math.cos(a - (_ring_phase[k] + k * 1.31))
            if spoke > 0.985:
                reach = (spoke - 0.985) / 0.015
                if RING_IN < rr < RING_OUT + 0.02 * reach:
                    m = max(m, hard_edge(RING_OUT + 0.02 * reach - rr, 0.02))
        ring_px[x, y] = int(max(0.0, min(1.0, m)) * 255.0 + 0.5)
save(ring, "T_impact_ring_mask.png")

# --- De hete vulling --------------------------------------------------------
# Vol tot 0,54 — net binnen de binnenrand van de ring (0,58), zodat de twee
# elkaar raken zonder over elkaar heen te vallen. Twee doorschijnende vlakken die
# elkaar overlappen sorteren onvoorspelbaar; elkaar raken is stabiel.
#
# De rand is gerafeld met een HOGERE frequentie dan de ring, want spat en roet
# hebben een fijnere korrel dan de brandrand eromheen. Zelfde regel: alleen
# indrukken.
CORE_OUT = 0.54
core_rng = random.Random(782)
core_noise = value_noise(S, 26, core_rng).load()
_core_phase = [core_rng.uniform(0.0, math.tau) for _ in range(3)]

core = Image.new("L", (S, S), 0)
core_px = core.load()
for y in range(S):
    for x in range(S):
        nx, ny = (x + 0.5) / S - 0.5, (y + 0.5) / S - 0.5
        r = math.hypot(nx, ny) * 2.0
        a = math.atan2(ny, nx)
        wobble = 1.0
        wobble += 0.06 * (0.5 + 0.5 * math.sin(5.0 * a + _core_phase[0]))
        wobble += 0.10 * (core_noise[x, y] / 255.0)
        rr = r * wobble
        m = hard_edge(CORE_OUT - rr, 0.020)
        core_px[x, y] = int(max(0.0, min(1.0, m)) * 255.0 + 0.5)
save(core, "T_impact_core_mask.png")

# --- De meting op de maskers zelf -------------------------------------------
# EEN GENERATOR DIE ZIJN EIGEN UITVOER NIET MEET, LEVERT EEN BESTAND EN GEEN
# EIGENSCHAP. De hele reden dat deze twee bestaan is dat T_blob_mask te weinig
# DEKKENDE straal had; dan hoort hier te staan hoeveel deze twee er wel hebben,
# in hetzelfde getal, zodat het te vergelijken is.
def dekkende_straal(img):
    """Grootste r waarop de gemiddelde dekking op die ring nog >= 0,5 is."""
    px = img.load()
    buckets = [[0.0, 0] for _ in range(64)]
    for y in range(S):
        for x in range(S):
            nx, ny = (x + 0.5) / S - 0.5, (y + 0.5) / S - 0.5
            r = math.hypot(nx, ny) * 2.0
            b = int(r * 32.0)
            if 0 <= b < 64:
                buckets[b][0] += px[x, y] / 255.0
                buckets[b][1] += 1
    best = 0.0
    for i, (total, count) in enumerate(buckets):
        if count > 0 and total / count >= 0.5:
            best = (i + 1) / 32.0
    return best


print(f"GEMETEN  inktring   dekkend tot r = {dekkende_straal(ring):.2f}")
print(f"GEMETEN  hete kern  dekkend tot r = {dekkende_straal(core):.2f}")
print("GEMETEN  T_blob_mask dekkend tot r = 0.45 (smoothstep, band 0,55) — de nulmeting")
