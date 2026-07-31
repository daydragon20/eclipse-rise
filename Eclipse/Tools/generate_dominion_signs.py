# Authors the DOMINION SIGNAGE FAMILY with Pillow (system python, no UE).
#
# Why this file exists (20_world_dressing_standard.md §20.2, the fiction law):
# the seven placards the builder used to dress the district came from
# FD_WarningSigns_V1 — PHOTOGRAPHS OF REAL-WORLD EARTH SIGNS. STOP in Latin
# capitals, the ISO-361 radiation trefoil, a skull-and-crossbones over the word
# TOXIC. All three were confirmed on the 31-07 recording frames
# (Saved/Screenshots/HUD_spelerlaag/HUD_3e_persoon.png). §20.2: "Er zijn geen
# verkeersborden in de Vantara Expanse."
#
# The old curation pass (ASSET_CURATION.md §8) rejected 53 of 60 signs — but it
# filtered on TONE ("reads suburban, not industrial oppression"), never on
# FICTION. §20.2 says curation is both: "Curatie betekent niet alleen 'past de
# stijl' maar ook 'past de fictie'." That second filter is what this file is.
#
# ---------------------------------------------------------------------------
# THE GRAMMAR — why these seven read as one civilization and not seven doodles
#
# §21_quality_mandate warns against one generic replacement used seven times.
# So the family is a SYSTEM with rules, and each sign is that system saying
# something specific to the people who work in front of it:
#
#   1. THE RADIANCE IS THE ROOT GLYPH. The Dominion's creed is that order is
#      light (02_story_bible.md §2.2). Every state-level sign carries the sun:
#      a disc with rays. Nothing else needs to say "state".
#   2. DANGER IS THE LIGHT BEING WRONG. A sun-worshipping bureaucracy does not
#      warn you with a skull — it tells you the light here is unshielded,
#      occluded, or contained. Hazard = a broken, eaten or bracketed disc.
#   3. THE CARRIER SHAPE ENCODES WHO IS TALKING. Ring = the state, and it is
#      not negotiable. Diamond = an emission hazard. Bracket = a works
#      containment. Bare field = operational instruction from the works.
#   4. NUMBERS ARE TALLIES, NOT DIGITS. Lot numbers, lane codes and shift codes
#      are tick groups. Arabic numerals and the Latin alphabet are Earth
#      (§20.2 bullet 4), so the signs carry information without either.
#   5. EVERY PLATE ENDS IN A CODE STRIP. Same structural slot on all seven,
#      different content per sign — the single strongest "one civilization"
#      cue there is. A leading FILLED mark = Dominion state authority; a
#      HOLLOW mark = works/labour authority. A worker reads that in a glance.
#
# Luminance only, exactly like generate_decals.py: the toon MID's palette
# supplies the hue (15.5 keeps hue authority with the palette), so a sign can
# never introduce an off-palette colour. No fonts, no readable text, no
# real-world marks.
#
# Wear is authored per sign (§20.5): nothing is new and nothing is evenly
# dirty. Flake dropouts, boot/rag streaks, grime speckle, and rust runs that
# bleed DOWNWARD from the mounting bolts — slijtage waar water loopt.
#
# Output: Eclipse/Saved/GeneratedDecals/T_sign_dom_*_diff.png
# Import: Tools/import_generated_decals.py (headless, picks up the whole dir).
# The printed 1/mean per sign is the builder's AlbedoGain — same measurement
# discipline as prepare_warning_signs.py, so texturing never re-meters the
# dusk auto-exposure.
#
# Run:  python Eclipse/Tools/generate_dominion_signs.py

import math
import os

from PIL import Image, ImageDraw

OUT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "Saved", "GeneratedDecals"))
os.makedirs(OUT, exist_ok=True)

S = 1024               # placard edge, matches the old OUT_SIZE so the builder's quads are unchanged
PLATE = 64             # dark steel backing
BAND = 8               # brushed-steel column variation
FRAME = 124            # the inset border frame
BOLT = 150             # mounting bolts
GLYPH = 242            # the mark itself
DIM = 170              # secondary/structural marks
INSET = 46             # frame inset from the plate edge

CX, CY, R = S // 2, 470, 300          # the glyph field
STRIP_Y = 872                          # the code strip baseline


def srgb_to_linear(v: float) -> float:
    v /= 255.0
    return v / 12.92 if v <= 0.04045 else ((v + 0.055) / 1.055) ** 2.4


# --------------------------------------------------------------- primitives
def ray(d, cx, cy, deg, r0, r1, w, fill):
    a = math.radians(deg)
    d.line([cx + math.cos(a) * r0, cy + math.sin(a) * r0,
            cx + math.cos(a) * r1, cy + math.sin(a) * r1], fill=fill, width=w)


def rays(d, cx, cy, n, r0, r1, w, fill, phase=0.0):
    for i in range(n):
        ray(d, cx, cy, phase + i * 360.0 / n, r0, r1, w, fill)


def disc(d, cx, cy, r, fill):
    d.ellipse([cx - r, cy - r, cx + r, cy + r], fill=fill)


def ring(d, cx, cy, r, w, fill):
    d.ellipse([cx - r, cy - r, cx + r, cy + r], outline=fill, width=w)


def code_strip(d, groups, state_authority, y=STRIP_Y):
    """The shared bottom slot: an authority mark then tally groups.

    Filled mark = Dominion state authority (the ring family). Hollow mark =
    works/labour authority. Same slot on every plate, different content —
    this is rule 5 of the grammar and it is what makes seven signs read as
    one signage system instead of seven unrelated pictures.
    """
    h, tw, gap, sep = 54, 16, 22, 52
    total = sum(g * (tw + gap) - gap for g in groups) + sep * (len(groups) - 1)
    x = CX - (total + 96) // 2
    # authority mark
    if state_authority:
        d.rectangle([x, y, x + 44, y + h], fill=GLYPH)
    else:
        d.rectangle([x, y, x + 44, y + h], outline=GLYPH, width=9)
    x += 96
    for gi, g in enumerate(groups):
        for _ in range(g):
            d.rectangle([x, y, x + tw, y + h], fill=DIM)
            x += tw + gap
        x -= gap
        x += sep


# ------------------------------------------------------------------- plate
def plate(rng):
    """Dark steel board: brushed banding, a bevel, an inset frame, four bolts.

    The old placards read as flat black rectangles pasted on the wall (the
    stencil's black square in HUD_3e_persoon.png is the same failure). A
    bevel, a frame and bolts give the plate object-ness, so it reads as a
    MOUNTED BOARD instead of a floating card.
    """
    img = Image.new("L", (S, S), PLATE)
    d = ImageDraw.Draw(img)
    # brushed vertical banding
    x = 0
    while x < S:
        w = rng.randint(6, 26)
        d.rectangle([x, 0, x + w, S], fill=max(0, PLATE + rng.randint(-BAND, BAND)))
        x += w
    # bevel: light top/left, dark bottom/right
    d.line([0, 0, S, 0], fill=PLATE + 46, width=10)
    d.line([0, 0, 0, S], fill=PLATE + 38, width=10)
    d.line([0, S - 1, S, S - 1], fill=max(0, PLATE - 30), width=10)
    d.line([S - 1, 0, S - 1, S], fill=max(0, PLATE - 26), width=10)
    # inset frame
    d.rectangle([INSET, INSET, S - INSET, S - INSET], outline=FRAME, width=11)
    return img, d


def bolts(d):
    for bx, by in ((96, 96), (S - 96, 96), (96, S - 96), (S - 96, S - 96)):
        disc(d, bx, by, 21, BOLT)
        disc(d, bx, by, 8, max(0, PLATE - 18))


def wear(img, rng):
    """§20.5: slijtage waar handen komen, waar voeten komen, waar water loopt."""
    d = ImageDraw.Draw(img)
    px = img.load()
    # rust runs bleeding down from the bolts
    for bx, by in ((96, 96), (S - 96, 96)):
        for k in range(rng.randint(3, 6)):
            rx = bx + rng.randint(-16, 16)
            ln = rng.randint(70, 260)
            d.line([rx, by + 16, rx + rng.randint(-6, 6), by + 16 + ln],
                   fill=max(0, PLATE - rng.randint(8, 30)), width=rng.randint(3, 11))
    # paint flake: dropouts to the plate. Kept CLOSE to the plate value —
    # the first pass read as grey smudges floating on the board rather than
    # as paint missing from it.
    for _ in range(rng.randint(20, 32)):
        wx, wy = rng.randrange(S), rng.randrange(S)
        wr = rng.randint(7, 28)
        d.ellipse([wx - wr, wy - int(wr * 0.6), wx + wr, wy + int(wr * 0.6)],
                  fill=max(0, PLATE + rng.randint(-11, 11)))
    # abrasion streaks
    for _ in range(rng.randint(40, 70)):
        wx, wy = rng.randrange(S), rng.randrange(S)
        wl = rng.randint(30, 170)
        d.line([wx, wy, wx + wl, wy + rng.randint(-12, 12)],
               fill=rng.randint(60, 160), width=rng.randint(1, 3))
    # grime speckle
    for _ in range(9000):
        wx, wy = rng.randrange(S), rng.randrange(S)
        px[wx, wy] = max(0, min(255, px[wx, wy] + rng.randint(-46, 34)))
    return img


def emit(img, rng, name):
    img = wear(img, rng)
    path = os.path.join(OUT, f"{name}.png")
    img.convert("RGB").save(path)
    small = img.convert("RGB").resize((64, 64))
    mean = sum(srgb_to_linear(sum(p) / 3.0) for p in small.getdata()) / (64 * 64)
    print(f"{name}: linear-mean {mean:.4f}  AlbedoGain = {1.0 / mean:.1f}")
    return 1.0 / mean


import random  # noqa: E402  (kept below the helpers so each sign owns its stream)

gains = {}

# =========================================================================
# 1. THE CHECKPOINT SEAL — replaces STOP, at the gate portal's west beam.
# A closed ring (the state, non-negotiable) around a full Radiance sun, with
# the authority bar struck across its lower third: the approach is closed and
# you present your tithe-card. Same FUNCTION as the old STOP sign — halt here
# — said in Dominion grammar instead of Earth traffic grammar.
# =========================================================================
_r = random.Random(9101)
img, d = plate(_r)
ring(d, CX, CY, R, 26, GLYPH)
disc(d, CX, CY, 104, GLYPH)
rays(d, CX, CY, 12, 138, 246, 22, GLYPH, phase=15.0)
d.rectangle([CX - R - 34, CY + 150, CX + R + 34, CY + 198], fill=GLYPH)   # the halt bar
bolts(d)
code_strip(d, (3, 1), state_authority=True)
gains["dom_seal"] = emit(img, _r, "T_sign_dom_seal_diff")

# =========================================================================
# 2. UNSHIELDED EMISSION — replaces the ISO-361 trefoil, on the crossing lamp.
# The trefoil is a datable Earth institution (UC Berkeley, 1946). Here the
# hazard is stated as the light being WRONG: a disc throwing three long
# unshielded spears while the rest of its rays are stubs, inside the diamond
# that marks an emission hazard.
# =========================================================================
_r = random.Random(9102)
img, d = plate(_r)
d.polygon([(CX, CY - R - 22), (CX + R + 22, CY), (CX, CY + R + 22), (CX - R - 22, CY)],
          outline=GLYPH, width=24)
disc(d, CX, CY, 68, GLYPH)
for deg in (-90, 30, 150):                      # the three unshielded spears
    ray(d, CX, CY, deg, 92, 250, 30, GLYPH)
for i in range(12):                             # the shielded stubs
    deg = i * 30.0
    if deg % 120 != 30 % 120:
        ray(d, CX, CY, deg, 96, 146, 13, DIM)
bolts(d)
code_strip(d, (2, 2), state_authority=True)
gains["dom_flare"] = emit(img, _r, "T_sign_dom_flare_diff")

# =========================================================================
# 3. SEALED-ATMOSPHERE ZONE — replaces the skull-and-crossbones TOXIC placard,
# on the west wall inner face. Kessara is an industrial world: the danger is
# the air, and the sign says so the way a works says it — the sun seen THROUGH
# bad atmosphere (a disc cut by smog bands), vapour descending, and the
# scrubber louvres you are downwind of. Works authority, so a hollow mark.
# =========================================================================
_r = random.Random(9103)
img, d = plate(_r)
# The sun sits OFF-CENTRE and the vapour falls diagonally across the plate.
# A centred sun over a centred wedge over a centred grate stacked into a
# figure-on-a-plinth silhouette at district distance (seen in frame,
# HighresScreenshot00051.png) — symmetry was reading as a body, not as weather.
_sx, _sy = CX - 132, CY - 166
disc(d, _sx, _sy, 88, GLYPH)                             # the occluded sun
for yy in (_sy - 44, _sy, _sy + 44):                     # smog bands eating it
    d.rectangle([_sx - 140, yy - 10, _sx + 140, yy + 10], fill=max(0, PLATE - 6))
for x0, y0, y1 in ((CX - 26, CY - 54, CY + 104),         # vapour falling downwind
                   (CX + 96, CY - 14, CY + 118),
                   (CX + 200, CY - 74, CY + 88)):
    d.polygon([(x0 - 27, y0), (x0 + 27, y0), (x0 + 54, y1)], fill=GLYPH)
for k in range(4):                                       # the scrubber louvres
    yy = CY + 172 + k * 46
    d.rectangle([CX - 252 + k * 16, yy, CX + 252 - k * 16, yy + 24], fill=DIM)
bolts(d)
code_strip(d, (1, 3), state_authority=False)
gains["dom_scrubber"] = emit(img, _r, "T_sign_dom_scrubber_diff")

# =========================================================================
# 4. LANE CODE — replaces the route arrow, on the second crossing lamp.
# §20.2 names the replacement explicitly: "Lane-codes en sectornummers, niet
# straatnamen." A spine that forks, and each terminus carries its own tally.
# A worker reads "lane two peels off, lane five runs on" without one glyph
# that an Earth driver would recognise.
# =========================================================================
_r = random.Random(9104)
img, d = plate(_r)
d.rectangle([CX - 17, CY - 40, CX + 17, CY + R], fill=GLYPH)          # the spine
d.line([CX, CY + 20, CX - 232, CY - 190], fill=GLYPH, width=34)       # the peel-off
d.rectangle([CX - 249, CY - 250, CX - 215, CY - 174], fill=GLYPH)     # its run-out
d.rectangle([CX - 17, CY - 250, CX + 17, CY - 20], fill=GLYPH)        # the through-lane
# Per-lane tallies sit ABOVE their own run-out, inside the field: this is the
# lane's number, and it is the whole point of the sign (§20.2 lane-codes).
for tx, n in ((CX - 232, 2), (CX, 5)):
    w, g = 15, 13
    x = tx - (n * w + (n - 1) * g) // 2
    for _ in range(n):
        d.rectangle([x, CY - 320, x + w, CY - 284], fill=DIM)
        x += w + g
bolts(d)
code_strip(d, (2, 5), state_authority=False)
gains["dom_lane"] = emit(img, _r, "T_sign_dom_lane_diff")

# =========================================================================
# 5. SHIFT ROSTER — replaces LABOR, beside the warehouse yard's east gate.
# §20.2: "Tithe-lotnummers, rantsoenkaart-iconografie, ploegcodes." A called-
# lot board: filled cells are the lots on shift, hollow cells are not, and the
# arc above is where the sun stands in the cycle — which shift is running.
# This is the sign that says PEOPLE WORK HERE, and it is the one a worker
# would actually stop and read.
# =========================================================================
_r = random.Random(9105)
img, d = plate(_r)
# The cycle arc rides ABOVE the roster, clear of it: where the sun stands is
# which shift is running. Centre (CX, CY+180) r=330 puts the apex at CY-150,
# a clean 110 px above the first row of lots.
_ac, _ar = CY + 180, 330
d.arc([CX - _ar, _ac - _ar, CX + _ar, _ac + _ar], start=235, end=305, fill=GLYPH, width=20)
_a = math.radians(305)
disc(d, CX + math.cos(_a) * _ar, _ac + math.sin(_a) * _ar, 30, GLYPH)   # the shift marker
_called = {0, 1, 4, 5, 7, 10, 11, 13, 16, 18}
for i in range(20):
    col, row = i % 5, i // 5
    x0 = CX - 260 + col * 108
    y0 = CY - 40 + row * 108
    if i in _called:
        d.rectangle([x0, y0, x0 + 74, y0 + 74], fill=GLYPH)
    else:
        d.rectangle([x0, y0, x0 + 74, y0 + 74], outline=DIM, width=10)
bolts(d)
code_strip(d, (4, 1, 2), state_authority=False)
gains["dom_shift"] = emit(img, _r, "T_sign_dom_shift_diff")

# =========================================================================
# 6. CHARGE STORE — replaces BLAST, on the Dominion post's west face.
# A contained burst: a solid core with short thick spikes, held inside the
# bracket that means works containment. The two heavy jambs are the blast-door
# returns. Reads "charge store, no open flame" to anyone who works the post.
# =========================================================================
_r = random.Random(9106)
img, d = plate(_r)
for sx in (-1, 1):                                   # the containment bracket
    x0 = CX + sx * 268
    d.rectangle([x0 - 20, CY - 250, x0 + 20, CY + 250], fill=GLYPH)
    # the returns fold INWARD, toward the core they contain
    ra, rb = sorted((x0 - sx * 20, x0 - sx * 108))
    d.rectangle([ra, CY - 250, rb, CY - 210], fill=GLYPH)
    d.rectangle([ra, CY + 210, rb, CY + 250], fill=GLYPH)
disc(d, CX, CY, 78, GLYPH)
rays(d, CX, CY, 8, 96, 188, 34, GLYPH, phase=22.5)
bolts(d)
code_strip(d, (1, 1, 3), state_authority=False)
gains["dom_charge"] = emit(img, _r, "T_sign_dom_charge_diff")

# =========================================================================
# 7. CORE EXCLUSION — replaces the second radiation triangle, on the west
# perimeter wall. It must NOT re-read as sign 2 (§20.7 check 7: dezelfde mesh
# 3x is zichtbaar — and two trefoils were exactly that). So the grammar
# inverts: where 2 EMITS (diamond, broken rays throwing outward), 7 is SEALED
# — concentric standoff rings, a solid unbroken core, corner ticks closing the
# perimeter, and the exclusion bar struck across the whole plate.
# =========================================================================
_r = random.Random(9107)
img, d = plate(_r)
for cx0, cy0, dx, dy in ((124, 124, 1, 1), (S - 124, 124, -1, 1),
                         (124, S - 124, 1, -1), (S - 124, S - 124, -1, -1)):
    # L-shaped corner ticks, each arm folding into the plate
    hx = sorted((cx0, cx0 + dx * 116)); hy = sorted((cy0, cy0 + dy * 18))
    d.rectangle([hx[0], hy[0], hx[1], hy[1]], fill=DIM)
    vx = sorted((cx0, cx0 + dx * 18)); vy = sorted((cy0, cy0 + dy * 116))
    d.rectangle([vx[0], vy[0], vx[1], vy[1]], fill=DIM)
ring(d, CX, CY, 288, 20, GLYPH)
ring(d, CX, CY, 214, 14, DIM)
disc(d, CX, CY, 138, GLYPH)
d.line([CX - 330, CY + 330, CX + 330, CY - 330], fill=GLYPH, width=46)   # the exclusion bar
bolts(d)
code_strip(d, (5, 2), state_authority=True)
gains["dom_core"] = emit(img, _r, "T_sign_dom_core_diff")

print()
print("Builder AlbedoGain table (EclipseGrayboxBuilder.cpp sign block):")
for k, v in gains.items():
    print(f"  {k:14s} {v:5.1f}f")
print(f"\ndone — {len(gains)} placards in {OUT}")
print("import: UnrealEditor-Cmd <project> -run=pythonscript -script=Tools/import_generated_decals.py")
