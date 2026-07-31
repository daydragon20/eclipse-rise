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
# NO module-level seed any more. Every map below owns its OWN random.Random
# stream (770 poster, 771 stain, 772 hazard, 773 pool, 774 blob, 775 stencil),
# which is what lets one map be re-authored without silently reshuffling the
# next one's noise. The old `random.seed(503)` stream had exactly one consumer
# left - the stencil - and the stencil now has a stream of its own, so dropping
# the seed leaves every other map bit-identical to the banked round.


def save(img, name):
    path = os.path.abspath(os.path.join(OUT, name))
    img.save(path)
    print(f"wrote {path}")


# --- Dominion propaganda poster (512x768) -----------------------------------
# 20.2 FICTION FIX, 2026-07-31. What stood here was the EYE OF PROVIDENCE:
#
#     d.polygon([(256, 90), (96, 360), (416, 360)], outline=230, width=10)
#     d.ellipse([196, 210, 316, 330], outline=235, width=10)
#     d.ellipse([244, 258, 268, 282], fill=245)
#
# an equilateral triangle enclosing a ringed eye. That is the reverse of the
# Great Seal of the United States (1782), freemasonry, "Illuminati" in popular
# culture - a datable Earth institution mark, and it was the brightest graphic
# element in the district's widest review camera. §20.2 forbids it flat out,
# and line 5 of THIS file already forbade it thirty-odd lines earlier ("no
# real-world marks"): the contract was written and then broken. Same class of
# error as the ISO-361 radiation trefoil replaced the same evening, only older
# and more heavily loaded.
#
# THE REPLACEMENT IS NOT A NEW SYSTEM. Tools/generate_dominion_signs.py had
# already authored the district's signage family hours earlier, and its rule 1
# is "THE RADIANCE IS THE ROOT GLYPH". Seven placards were speaking that
# language while the posters fifteen metres away still carried an eye in a
# triangle - two incompatible Dominion iconographies in one district, which
# breaks the one-style law on top of the fiction. So this poster is that same
# grammar, rule for rule:
#
#   rule 1  the mark is the Radiance: a solid disc with rays.
#   rule 3  the carrier shape says who is talking. A closed RING = the state,
#           and it is not negotiable - the same carrier as the checkpoint seal.
#   rule 4  numbers are tallies, not digits: the rank under the emblem is the
#           lots, filled = called to shift, hollow = standing.
#   rule 5  every plate ends in the same code strip, and a FILLED authority
#           mark means Dominion state authority.
#
# Proportions are the checkpoint seal's at half scale (ring r300/w26, disc
# r104, twelve rays 138->246 on a 1024 plate), so the two read as one hand.
# What makes the poster a different SENTENCE rather than the seal repeated
# (§20.7: the same mark twice is visible) is what sits under the emblem: the
# seal is closed by a halt bar, the poster opens onto the rank it is addressing.
#
# The dense ray field between disc and ring is also what stops a circle inside
# a circle from reading as a pupil inside an iris - the scrubber placard
# learned the same lesson about accidental faces ("symmetry was reading as a
# body"). Trading one eye for another would have been a wasted round.
#
# And it is a PASTED SHEET, not a fresh print (§20.5, nothing is new and
# nothing is evenly dirty): paste wrinkles, bleach where the light hits, grime,
# and a scuffed lower corner at the height where people brush past it. The old
# poster was pixel-perfect clean, which is its own kind of fiction break.
P_W, P_H = 512, 768
P_GROUND = 26                        # the sheet's printed ground
P_GLYPH = 236                        # the mark
P_DIM = 176                          # secondary / structural marks
_poster_rng = random.Random(770)

poster = Image.new("L", (P_W, P_H), P_GROUND)
d = ImageDraw.Draw(poster)
d.rectangle([16, 16, 495, 751], outline=200, width=6)          # the sheet's printed border

# --- the emblem: ring (state) around the Radiance (rules 1 + 3) ---
PCX, PCY, PR = 256, 250, 150
d.ellipse([PCX - PR, PCY - PR, PCX + PR, PCY + PR], outline=P_GLYPH, width=13)
for i in range(12):
    _a = math.radians(15.0 + i * 30.0)
    d.line([PCX + math.cos(_a) * 69, PCY + math.sin(_a) * 69,
            PCX + math.cos(_a) * 123, PCY + math.sin(_a) * 123], fill=P_GLYPH, width=11)
d.ellipse([PCX - 52, PCY - 52, PCX + 52, PCY + 52], fill=P_GLYPH)

# --- the rank of lots (rule 4): tally groups, filled = called to shift ---
_groups = ((4, True), (3, True), (2, False))
_tw, _tgap, _tsep, _th = 22, 16, 48, 96
_total = sum(n * _tw + (n - 1) * _tgap for n, _ in _groups) + _tsep * (len(_groups) - 1)
_x = PCX - _total // 2
for _n, _called in _groups:
    for _ in range(_n):
        if _called:
            d.rectangle([_x, 540, _x + _tw, 540 + _th], fill=P_GLYPH)
        else:
            d.rectangle([_x, 540, _x + _tw, 540 + _th], outline=P_DIM, width=7)
        _x += _tw + _tgap
    _x += _tsep - _tgap
# The ground the rank stands on.
d.rectangle([64, 648, 448, 680], fill=P_DIM)

# --- the code strip (rule 5): filled authority mark = Dominion state ---
_cw, _cgap, _csep, _ch, _cy = 13, 17, 42, 42, 700
_cgroups = (3, 1)
_ctotal = sum(g * (_cw + _cgap) - _cgap for g in _cgroups) + _csep * (len(_cgroups) - 1)
_cx = PCX - (_ctotal + 76) // 2
d.rectangle([_cx, _cy, _cx + 36, _cy + _ch], fill=P_GLYPH)
_cx += 76
for _gi, _g in enumerate(_cgroups):
    for _ in range(_g):
        d.rectangle([_cx, _cy, _cx + _cw, _cy + _ch], fill=P_DIM)
        _cx += _cw + _cgap
    _cx += _csep - _cgap

# --- §20.5 wear: this is paper that has been on a wall for a while ---
_ppx = poster.load()
# Paste wrinkles: long near-vertical creases where the sheet was smoothed down.
# Drawn as a VALUE SHIFT on the existing pixels rather than as a line of its
# own colour - a crease brightens or darkens whatever it runs across (ground,
# ring, tally), and a flat-coloured line would have painted straight over the
# mark instead.
for _ in range(9):
    _wx = _poster_rng.randrange(30, P_W - 30)
    _wy = _poster_rng.randrange(20, 320)
    _wl = _poster_rng.randint(180, 420)
    _wd = _poster_rng.randint(-26, 26)
    _delta = _poster_rng.choice((-16, -12, 13, 17))
    for _k in range(_wl):
        _t = _k / _wl
        _px_ = int(_wx + _wd * _t)
        _py_ = _wy + _k
        if 0 <= _px_ < P_W and 0 <= _py_ < P_H:
            _ppx[_px_, _py_] = max(0, min(255, _ppx[_px_, _py_] + _delta))
# Bleach: the upper field has faced the light longest, so its ink has lifted
# toward the sheet and its ground has greyed up - contrast loss, not a tint.
for _y in range(20, 300):
    _f = 0.72 + 0.28 * ((_y - 20) / 280.0)          # 0.72 at the top, 1.0 by y=300
    for _x in range(20, P_W - 20):
        _v = _ppx[_x, _y]
        _ppx[_x, _y] = int(96 + (_v - 96) * _f + 0.5)
# Scuffed lower corner: hip height, where people brush past.
for _ in range(34):
    _wx = _poster_rng.randrange(24, 210)
    _wy = _poster_rng.randrange(600, P_H - 24)
    _wl = _poster_rng.randint(20, 90)
    d.line([_wx, _wy, _wx + _wl, _wy + _poster_rng.randint(-7, 7)],
           fill=_poster_rng.randint(40, 120), width=_poster_rng.randint(1, 3))
# Grime speckle over the whole sheet.
for _ in range(7000):
    _wx, _wy = _poster_rng.randrange(P_W), _poster_rng.randrange(P_H)
    _ppx[_wx, _wy] = max(0, min(255, _ppx[_wx, _wy] + _poster_rng.randint(-34, 26)))
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


def _smoothstep(t):
    """Clamped smoothstep. Lives up here, above the first map that needs it -
    it used to sit further down with the light pass, which is fine for a pure
    function but not for a reader. It consumes no rng, so moving it leaves
    every map bit-identical."""
    t = max(0.0, min(1.0, t))
    return t * t * (3.0 - 2.0 * t)


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

# --- Rebel eclipse stencil: an OPACITY MASK, not a plate (20.2/20.5 fix) -----
#
# WHAT WAS WRONG. The old map was `T_decal_stencil_diff`: background value 18,
# a hard 225 annulus, and a bite. Value 18 is not transparent - it is dark
# grey - and the builder put that map on the OPAQUE toon master at AlbedoMix
# 1.0. In frame that is a black rectangle with a perfectly geometric red "C"
# in it (confirmed 31-07 on the wide review camera and on the gate camera).
# Three things go wrong at once and all three are story, not polish:
#
#   1. It reads as PRINTED VINYL. §20.2 asks for the Eclipse sigil "gespoten,
#      gekrast, weggeschuurd en terug". The old spray grain touched ~15% of
#      pixels in a thin ring and was invisible at any play distance: no drips,
#      no overspray, no broken edge.
#   2. On a black plate the mark reads as something the REGIME hung on the
#      wall, not something sprayed onto it. The politics invert.
#   3. A crisp round logo centred on a black square reads as an app icon.
#
# THE CAUSE IS ONE WORD: the stencil was on the opaque path. The ground stains
# next to it already ride M_EclipseToonDecal with a mask in .r driving opacity
# - the same diagnosis generate_dominion_signs.py wrote down for the sign
# plates ("flat black rectangles pasted on the wall") without carrying it over
# to the stencil. So the mark moves to the masked path and this file stops
# emitting a luminance map for it at all.
#
# WHAT THE MASK CARRIES. Coverage IS the paint. Everything that makes spray
# read as spray is alpha, which is why no gain is needed (a mask modulates
# coverage, never luminance - see the header):
#   * a HAND-CUT edge: the outer and inner radii wobble with angle, so the
#     silhouette is knife-cut card, not a vector circle;
#   * STENCIL TIES: three radial bridges of bare wall where the card held the
#     inner island. Nobody cuts a free-floating ring out of cardboard;
#   * BLEED: coverage falls off over ~9 px at every cut edge, jittered per
#     pixel, because paint creeps under the card and breaks up;
#   * OVERSPRAY: a sparse halo of low-alpha dust for ~40 px around the mark,
#     densest at the edge - the single strongest "this came out of a can" cue;
#   * DRIPS: runs from the lowest painted edges, thinning as they fall and
#     ending in a bead;
#   * SCRUBBED BACK: broad diagonal bands where coverage is cut to a third -
#     someone took a rag to it;
#   * AND BACK: a faint ghost of an earlier, larger attempt behind the mark.
#     That is the whole §20.2 sentence in one image: sprayed, scratched,
#     scrubbed away, and sprayed again.
#
# Coverage is forced to zero in the outer 24 px, the same discipline as the
# stain/pool/blob masks: a falloff that still carries value at the quad border
# is exactly what made the first stain round read as carpet tiles.
STEN = 512
_st_rng = random.Random(775)


def _wobble(rng, harmonics=4):
    """A smooth 2pi-periodic wobble in [-1, 1]: the knife's hand."""
    terms = [(rng.uniform(0.45, 1.0) / (k + 1), rng.uniform(0.0, 2.0 * math.pi), k + 2)
             for k in range(harmonics)]
    norm = sum(t[0] for t in terms)
    return lambda a: sum(m * math.sin(k * a + p) for m, p, k in terms) / norm


_cut_out = _wobble(_st_rng)
_cut_in = _wobble(_st_rng)
_cut_bite = _wobble(_st_rng)
_cut_ghost = _wobble(_st_rng, 3)
_dens_coarse = _value_noise(STEN, 7, _st_rng).load()    # where the can lingered
_dens_fine = _value_noise(STEN, 23, _st_rng).load()     # the spatter of the nozzle

SCX, SCY = 256.0, 256.0
R_OUT, R_IN = 176.0, 100.0                 # the sprayed annulus
BITE_X, BITE_Y, BITE_R = 334.0, 150.0, 108.0   # the occulting body: the eclipse
GHOST_DX, GHOST_DY = 19.0, -13.0           # the earlier attempt, off by a hand
TIES = ((math.radians(152.0), math.radians(6.5)),   # where the card bridged
        (math.radians(214.0), math.radians(5.5)),
        (math.radians(286.0), math.radians(6.0)))
BLEED = 9.0                                # how far paint creeps under the card
HALO = 40.0                                # overspray reach

_cov = [[0.0] * STEN for _ in range(STEN)]
for _y in range(STEN):
    _row = _cov[_y]
    for _x in range(STEN):
        _dx, _dy = _x + 0.5 - SCX, _y + 0.5 - SCY
        _r = math.hypot(_dx, _dy)
        _a = math.atan2(_dy, _dx)
        _ro = R_OUT * (1.0 + 0.065 * _cut_out(_a))
        _ri = R_IN * (1.0 + 0.090 * _cut_in(_a))
        _bx, _by = _x + 0.5 - BITE_X, _y + 0.5 - BITE_Y
        _rb = math.hypot(_bx, _by)
        _bite = BITE_R * (1.0 + 0.075 * _cut_bite(math.atan2(_by, _bx)))
        # Signed distance to the mark: positive inside, in pixels.
        _s = min(_ro - _r, _r - _ri, _rb - _bite)
        # The ties are cut in ANGLE, so they leave bare wall right across the band.
        for _ta, _tw in TIES:
            _da = abs(((_a - _ta + math.pi) % (2.0 * math.pi)) - math.pi)
            if _da < _tw:
                _s = min(_s, (_da - _tw * 0.42) * 30.0)
        _m = _smoothstep(_s / BLEED)
        if _m > 0.0:
            _m *= (0.80 + 0.20 * (_dens_coarse[_x, _y] / 255.0)) * \
                  (0.86 + 0.14 * (_dens_fine[_x, _y] / 255.0))
            if _m < 0.97:                      # only the bleeding edge breaks up
                _m += _st_rng.uniform(-0.26, 0.26)
        elif -HALO < _s < 0.0:
            # Overspray: dust, densest against the edge, thinning outward.
            _p = 1.0 + _s / HALO
            if _st_rng.random() < 0.40 * _p * _p:
                _m = _st_rng.uniform(0.05, 0.32) * _p
        # "en terug": the ghost of the attempt before this one.
        _gr = math.hypot(_dx - GHOST_DX, _dy - GHOST_DY)
        _ga = math.atan2(_dy - GHOST_DY, _dx - GHOST_DX)
        _gs = min(214.0 * (1.0 + 0.08 * _cut_ghost(_ga)) - _gr, _gr - 132.0)
        if _gs > 0.0:
            _m = max(_m, 0.13 * _smoothstep(_gs / 14.0) *
                     (0.5 + 0.5 * (_dens_fine[_x, _y] / 255.0)))
        _row[_x] = max(0.0, min(1.0, _m))

# Drips: paint runs from the lowest wet edge, thinning, ending in a bead.
for _dx0 in (118, 147, 186, 231, 268, 312, 349):
    _from = None
    for _y in range(STEN - 1, -1, -1):
        if _cov[_y][_dx0] > 0.55:
            _from = _y
            break
    if _from is None or _from > STEN - 60:
        continue
    _len = _st_rng.randint(34, 132)
    _w0 = _st_rng.randint(2, 5)
    _wander = _st_rng.uniform(-0.05, 0.05)
    for _k in range(_len):
        _t = _k / float(_len)
        _yy = _from + _k
        if _yy >= STEN:
            break
        _xx = int(_dx0 + _wander * _k)
        _ww = max(1, int(round(_w0 * (1.0 - 0.62 * _t))))
        _al = (1.0 - _t) * _st_rng.uniform(0.72, 1.0)
        for _o in range(-_ww, _ww + 1):
            if 0 <= _xx + _o < STEN:
                _cov[_yy][_xx + _o] = max(_cov[_yy][_xx + _o], _al)
    # the bead where the run stopped
    _byy = min(STEN - 1, _from + _len)
    _brr = _st_rng.randint(3, 6)
    for _oy in range(-_brr, _brr + 1):
        for _ox in range(-_brr, _brr + 1):
            if _ox * _ox + _oy * _oy <= _brr * _brr:
                _yy, _xx = _byy + _oy, int(_dx0 + _wander * _len) + _ox
                if 0 <= _yy < STEN and 0 <= _xx < STEN:
                    _cov[_yy][_xx] = max(_cov[_yy][_xx], 0.62)

# Weggeschuurd: three broad diagonal bands where a rag took most of it back.
for _sa, _sb, _sw, _skeep in ((0.62, 74.0, 46.0, 0.30), (0.51, -128.0, 33.0, 0.42),
                              (0.78, 226.0, 27.0, 0.36)):
    _norm = math.sqrt(_sa * _sa + 1.0)
    for _y in range(STEN):
        for _x in range(STEN):
            if _cov[_y][_x] <= 0.0:
                continue
            _dist = abs(_sa * _x - _y + _sb) / _norm
            if _dist < _sw:
                _cov[_y][_x] *= _skeep + (1.0 - _skeep) * _smoothstep(_dist / _sw)

stencil_mask = Image.new("L", (STEN, STEN), 0)
_smpx = stencil_mask.load()
for _y in range(STEN):
    for _x in range(STEN):
        _edge = min(_x, _y, STEN - 1 - _x, STEN - 1 - _y)
        _v = _cov[_y][_x] * (_smoothstep(_edge / 24.0) if _edge < 24 else 1.0)
        _smpx[_x, _y] = int(max(0.0, min(1.0, _v)) * 255.0 + 0.5)
# Gekrast: thin scratches straight through the paint, down to bare wall.
_sd = ImageDraw.Draw(stencil_mask)
for _ in range(7):
    _x0, _y0 = _st_rng.randrange(96, 416), _st_rng.randrange(96, 416)
    _sd.line([_x0, _y0, _x0 + _st_rng.randint(-120, 120), _y0 + _st_rng.randint(-90, 90)],
             fill=0, width=_st_rng.randint(1, 2))
save(stencil_mask, "T_decal_stencil_mask.png")

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
#
# _smoothstep used to be defined here; it now lives beside _value_noise above,
# because the stencil mask needs it too.

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
