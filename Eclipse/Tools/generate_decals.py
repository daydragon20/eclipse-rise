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

# --- Hazard stripes (256x256, tiling diagonal). ---
hazard = Image.new("L", (256, 256), 30)
d = ImageDraw.Draw(hazard)
for k in range(-4, 9):
    x0 = k * 64
    d.polygon([(x0, 256), (x0 + 64, 256), (x0 + 320, 0), (x0 + 256, 0)], fill=225)
save(hazard, "T_decal_hazard_diff.png")

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
