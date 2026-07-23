# Generates the district's decal textures with Pillow (system python, no UE).
# Design contract (15.5 palette discipline): decals are LUMINANCE patterns -
# the toon material's palette tint supplies the hue (Dominion white-gold,
# rebel red, hazard amber), so a decal can never introduce an off-palette
# color. Abstract glyphs only - no fonts, no readable text, no real-world
# marks. Output: Eclipse/Saved/GeneratedDecals/*.png (import via
# Tools/import_generated_decals.py).
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
