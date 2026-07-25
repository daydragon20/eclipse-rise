# Generates the district's decal textures with Pillow (system python, no UE).
# Design contract (15.5 palette discipline): decals are LUMINANCE patterns -
# the toon material's palette tint supplies the hue (Dominion white-gold,
# rebel red, hazard amber), so a decal can never introduce an off-palette
# color. Abstract glyphs only - no fonts, no readable text, no real-world
# marks. Output: Eclipse/Saved/GeneratedDecals/*.png (import via
# Tools/import_generated_decals.py).
#
# Two kinds of map live here:
#   *_diff  - luminance patterns, sampled as AlbedoTex, and therefore MEASURED
#             (AlbedoGain = 1/linear-mean, Tools/measure_albedo_gain.py) before
#             any builder line may use them.
#   *_mask  - OPACITY falloffs for M_EclipseToonDecal (mask.r * OpacityScale),
#             imported linear + grayscale by the "_mask" branch in
#             import_generated_decals.py. A mask carries no gain by design: it
#             modulates coverage, never luminance, so the builder can place one
#             without a measurement round (dressing-iteratie 2).
#
# Run:  python Eclipse/Tools/generate_decals.py

import math
import os
import random

from PIL import Image, ImageDraw

OUT = os.path.join(os.path.dirname(__file__), "..", "Saved", "GeneratedDecals")
os.makedirs(OUT, exist_ok=True)
random.seed(503)


def save(img, name):
    path = os.path.abspath(os.path.join(OUT, name))
    img.save(path)
    print(f"wrote {path}")


# --- Dominion propaganda poster (512x768): stern geometry, bright emblem. ---
poster = Image.new("L", (512, 768), 26)
d = ImageDraw.Draw(poster)
d.rectangle([16, 16, 495, 751], outline=200, width=6)  # frame
# The AEGIS eye: triangle + circle + slit pupil.
d.polygon([(256, 90), (96, 360), (416, 360)], outline=230, width=10)
d.ellipse([196, 210, 316, 330], outline=235, width=10)
d.ellipse([244, 258, 268, 282], fill=245)
# Authority bars: three "text" blocks, unreadable by design.
for i, y in enumerate((430, 500, 570)):
    w = 360 - i * 60
    d.rectangle([(512 - w) // 2, y, (512 + w) // 2, y + 34], fill=205)
# Ground bar.
d.rectangle([64, 660, 448, 700], fill=160)
save(poster, "T_decal_poster_diff.png")

# --- Hazard stripes (256x256): worn paint pad. ---
# 15.8 art-fix: the original loop stepped x0 by the stripe WIDTH (64), so the
# bands tiled edge-to-edge and the whole texture was a solid 225 plate — the
# pads rendered as flat pure-yellow quads (review shot 00013; the banked gain
# 1.3 = 1/0.753 is exactly solid-225's mean, confirming the bug). Now: 64 on /
# 64 off, plus paint wear from an OWN rng stream (the stencil below keeps the
# module-level 503 stream untouched, bit-identical).
hazard = Image.new("L", (256, 256), 30)
d = ImageDraw.Draw(hazard)
for k in range(-2, 5):
    x0 = k * 128
    d.polygon([(x0, 256), (x0 + 64, 256), (x0 + 320, 0), (x0 + 256, 0)], fill=225)
_wear_rng = random.Random(772)
# Dropout patches: paint flaked to the dark base.
for _ in range(26):
    wx, wy = _wear_rng.randrange(256), _wear_rng.randrange(256)
    wr = _wear_rng.randint(5, 22)
    d.ellipse([wx - wr, wy - int(wr * 0.55), wx + wr, wy + int(wr * 0.55)],
              fill=_wear_rng.randint(24, 60))
# Abrasion streaks: tire/boot scuffs across the paint.
for _ in range(42):
    wx, wy = _wear_rng.randrange(256), _wear_rng.randrange(256)
    wl = _wear_rng.randint(18, 80)
    d.line([wx, wy, wx + wl, wy + _wear_rng.randint(-10, 10)],
           fill=_wear_rng.randint(55, 150), width=_wear_rng.randint(1, 2))
# Grime speckle over everything.
_hpx = hazard.load()
for _ in range(2600):
    wx, wy = _wear_rng.randrange(256), _wear_rng.randrange(256)
    _hpx[wx, wy] = max(0, min(255, _hpx[wx, wy] + _wear_rng.randint(-60, 40)))
save(hazard, "T_decal_hazard_diff.png")

# --- Stain opacity mask (512x512, 15.8 art-fix): radial/noise falloff for the
# M_EclipseToonDecal ground stains. NOT a luminance decal - imported linear
# (sRGB off, see import_generated_decals.py) and sampled as opacity, so the
# stains fade out organically instead of reading as hard dark rectangles
# ("carpet tiles", review shots 00008-00013). Own rng stream: the decals above
# keep consuming the module-level seed 503 stream untouched (bit-identical). ---
_mask_rng = random.Random(771)


def _value_noise(size, cells, rng):
    small = Image.new("L", (cells, cells))
    small.putdata([rng.randint(0, 255) for _ in range(cells * cells)])
    return small.resize((size, size), Image.BICUBIC)


S = 512
_n_coarse = _value_noise(S, 6, _mask_rng).load()   # big blotch shape
_n_fine = _value_noise(S, 28, _mask_rng).load()    # edge raggedness
stain_mask = Image.new("L", (S, S), 0)
_pm = stain_mask.load()
for _y in range(S):
    for _x in range(S):
        nx, ny = (_x + 0.5) / S - 0.5, (_y + 0.5) / S - 0.5
        r = math.hypot(nx, ny) * 2.0  # 0 center, 1 at edge midpoints, ~1.41 corners
        base = max(0.0, min(1.0, (1.02 - r) / 0.55))
        noise = 0.62 * (_n_coarse[_x, _y] / 255.0) + 0.38 * (_n_fine[_x, _y] / 255.0)
        m = base * (0.30 + 0.95 * noise)
        m = max(0.0, min(1.0, (m - 0.10) / 0.72))  # soft cut: the quad edge truly reaches 0
        _pm[_x, _y] = int(m * 255.0 + 0.5)
save(stain_mask, "T_stain_mask.png")

# --- Rebel eclipse stencil (512x512): sprayed annulus with a bite. ---
stencil = Image.new("L", (512, 512), 18)
d = ImageDraw.Draw(stencil)
d.ellipse([96, 96, 416, 416], fill=225)
d.ellipse([176, 176, 336, 336], fill=18)          # hollow core
d.ellipse([280, 60, 470, 250], fill=18)           # the eclipse "bite"
# Spray grain: bright speckle outside, dark speckle inside the mark.
px = stencil.load()
for _ in range(9000):
    x, y = random.randrange(512), random.randrange(512)
    r = math.hypot(x - 256, y - 256)
    if 150 < r < 175 or random.random() < 0.15:
        px[x, y] = max(0, min(255, px[x, y] + random.randint(-70, 70)))
save(stencil, "T_decal_stencil_diff.png")

# --- The light pass (dressing-iteratie 2, phase0/DRESSING_ITERATIE_2.md) -------
# The district renders UNLIT, so its "lighting" is decals: warm pools under the
# sodium lamps and blob shadows under the dressing masses. Both are OPACITY
# falloffs on M_EclipseToonDecal - the tint comes from the palette (pool =
# sodium hue, blob = the floor tint x0.4), these files only say WHERE the cue
# has coverage. Written last, each on its OWN rng stream, so every map above
# stays bit-identical to the banked round.
#
# Both discs are INSCRIBED in their quad: r is normalized so it reaches 1.0 at
# the edge midpoints, and coverage is forced to 0 there. A falloff that still
# carries value at the border is exactly what made the first stain round read as
# "carpet tiles" (review shots 00008-00013).


def _smoothstep(t):
    t = max(0.0, min(1.0, t))
    return t * t * (3.0 - 2.0 * t)


# Lamp pool: wide soft disc, small plateau core, faint smog mottle so the light
# is not a CG-perfect gradient. Falloff span 0.78 -> core out to r 0.22.
POOL = 512
_pool_rng = random.Random(773)
_pool_mottle = _value_noise(POOL, 5, _pool_rng).load()
pool = Image.new("L", (POOL, POOL), 0)
_poolpx = pool.load()
for _y in range(POOL):
    for _x in range(POOL):
        nx, ny = (_x + 0.5) / POOL - 0.5, (_y + 0.5) / POOL - 0.5
        r = math.hypot(nx, ny) * 2.0
        m = _smoothstep((1.0 - r) / 0.78)
        m *= 0.90 + 0.10 * (_pool_mottle[_x, _y] / 255.0)
        _poolpx[_x, _y] = int(max(0.0, min(1.0, m)) * 255.0 + 0.5)
save(pool, "T_pool_mask.png")

# Blob shadow: dense core (plateau out to r ~0.45, falloff span 0.55) whose
# radius is ERODED by up to 14% from a mid-frequency noise, so the contact
# shadow reads hand-inked instead of as a vector ellipse. Erosion only ever
# inflates the sampled radius, never shrinks it - that is what keeps coverage at
# exactly 0 on the quad edge (a wobble in both directions would leave a faint
# hard border, the carpet-tile failure again).
BLOB = 512
_blob_rng = random.Random(774)
_blob_edge = _value_noise(BLOB, 9, _blob_rng).load()
blob = Image.new("L", (BLOB, BLOB), 0)
_blobpx = blob.load()
for _y in range(BLOB):
    for _x in range(BLOB):
        nx, ny = (_x + 0.5) / BLOB - 0.5, (_y + 0.5) / BLOB - 0.5
        r = math.hypot(nx, ny) * 2.0
        r *= 1.0 + 0.14 * (_blob_edge[_x, _y] / 255.0)
        _blobpx[_x, _y] = int(_smoothstep((1.0 - r) / 0.55) * 255.0 + 0.5)
save(blob, "T_blob_mask.png")
