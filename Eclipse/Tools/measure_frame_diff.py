"""Compare two frames pixel for pixel and report where luminance moved.

Written for the grounding-coverage step of dressing iteration 3: contact
darkening is a LOCAL effect, so a whole-frame mean says nothing useful about
it. What matters is how many pixels darkened, by how much, and whether they
sit low in the frame (ground) rather than scattered across it.

Same colour discipline as measure_frame_values.py: sRGB EOTF to linear, then
Rec.709 luminance. Differences are reported in linear luminance because that
is the space the darkening actually happens in.
"""

import argparse
import sys

try:
    from PIL import Image
except ImportError:
    sys.exit("Pillow ontbreekt: py -m pip install --user pillow")


def srgb_to_linear(c):
    c = c / 255.0
    return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4


LUT = [srgb_to_linear(i) for i in range(256)]


def luminance(px):
    r, g, b = px[0], px[1], px[2]
    return 0.2126 * LUT[r] + 0.7152 * LUT[g] + 0.0722 * LUT[b]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("before")
    ap.add_argument("after")
    ap.add_argument("--threshold", type=float, default=0.002,
                    help="linear luminance move that counts as a change")
    ap.add_argument("--bands", type=int, default=4,
                    help="horizontal bands to split the frame into")
    ap.add_argument("--smooth-only", type=float, default=None,
                    help="only count pixels whose 3x3 luminance range is below this "
                         "in BOTH frames. The rig is not pixel-deterministic: a hard "
                         "edge that lands a sub-pixel elsewhere flips a pixel by up to "
                         "0.8 lum, which drowns any soft effect. Contact darkening is "
                         "a smooth gradient on smooth asphalt, so masking out edges "
                         "keeps the signal and drops the jitter.")
    ap.add_argument("--sites", type=float, default=None,
                    help="report WHERE the darkening landed: keep smooth pixels that "
                         "dropped more than this, group them into connected sites and "
                         "list each site's centre and strength. Contact shadows are a "
                         "per-object effect, so the useful number is how many distinct "
                         "sites appeared, not a frame average that noise can swamp.")
    ap.add_argument("--min-site", type=int, default=40,
                    help="ignore sites smaller than this many pixels")
    args = ap.parse_args()

    a = Image.open(args.before).convert("RGB")
    b = Image.open(args.after).convert("RGB")
    if a.size != b.size:
        sys.exit(f"maten verschillen: {a.size} vs {b.size}")

    w, h = a.size
    pa, pb = a.load(), b.load()

    lum_a = [[luminance(pa[x, y]) for x in range(w)] for y in range(h)]
    lum_b = [[luminance(pb[x, y]) for x in range(w)] for y in range(h)]

    def is_smooth(lum, x, y, limit):
        lo = hi = lum[y][x]
        for yy in range(max(0, y - 1), min(h, y + 2)):
            row = lum[yy]
            for xx in range(max(0, x - 1), min(w, x + 2)):
                v = row[xx]
                lo = v if v < lo else lo
                hi = v if v > hi else hi
        return (hi - lo) < limit

    darker = brighter = 0
    considered = 0
    sum_dark = 0.0
    worst = 0.0
    worst_at = None
    band_dark = [0] * args.bands
    band_h = h / args.bands

    for y in range(h):
        band = min(int(y / band_h), args.bands - 1)
        for x in range(w):
            if args.smooth_only is not None:
                if not (is_smooth(lum_a, x, y, args.smooth_only)
                        and is_smooth(lum_b, x, y, args.smooth_only)):
                    continue
            considered += 1
            la = lum_a[y][x]
            lb = lum_b[y][x]
            d = lb - la
            if d < -args.threshold:
                darker += 1
                sum_dark += -d
                band_dark[band] += 1
                if -d > worst:
                    worst = -d
                    worst_at = (x, y, round(la, 4), round(lb, 4))
            elif d > args.threshold:
                brighter += 1

    total = considered if args.smooth_only is not None else w * h
    print(f"frame          {w}x{h} = {w*h} pixels")
    if args.smooth_only is not None:
        print(f"vlak genoeg    {considered} ({100.0*considered/(w*h):.1f}% van het frame)")
    print(f"donkerder      {darker} ({100.0*darker/total:.2f}%)")
    print(f"lichter        {brighter} ({100.0*brighter/total:.2f}%)")
    if darker:
        print(f"gem. verdonkering  {sum_dark/darker:.4f} lum")
        print(f"sterkste           {worst:.4f} lum op x={worst_at[0]} y={worst_at[1]}"
              f"  ({worst_at[2]} -> {worst_at[3]})")
    for i, n in enumerate(band_dark):
        y0, y1 = int(i * band_h), int((i + 1) * band_h)
        share = 100.0 * n / darker if darker else 0.0
        print(f"  band {i} (y {y0}-{y1}): {n} donkerder ({share:.1f}% van alle verdonkering)")

    if args.sites is None:
        return

    hit = [[False] * w for _ in range(h)]
    for y in range(h):
        ra, rb = lum_a[y], lum_b[y]
        for x in range(w):
            if ra[x] - rb[x] > args.sites:
                if args.smooth_only is None or (
                        is_smooth(lum_a, x, y, args.smooth_only)
                        and is_smooth(lum_b, x, y, args.smooth_only)):
                    hit[y][x] = True

    sites = []
    for y0 in range(h):
        for x0 in range(w):
            if not hit[y0][x0]:
                continue
            stack = [(x0, y0)]
            hit[y0][x0] = False
            pixels = []
            while stack:
                x, y = stack.pop()
                pixels.append((x, y))
                for dy in (-2, -1, 0, 1, 2):
                    for dx in (-2, -1, 0, 1, 2):
                        nx, ny = x + dx, y + dy
                        if 0 <= nx < w and 0 <= ny < h and hit[ny][nx]:
                            hit[ny][nx] = False
                            stack.append((nx, ny))
            if len(pixels) < args.min_site:
                continue
            cx = sum(p[0] for p in pixels) / len(pixels)
            cy = sum(p[1] for p in pixels) / len(pixels)
            before = sum(lum_a[p[1]][p[0]] for p in pixels) / len(pixels)
            after = sum(lum_b[p[1]][p[0]] for p in pixels) / len(pixels)
            sites.append((len(pixels), cx, cy, before, after))

    sites.sort(reverse=True)
    print(f"\nverdonkeringsplekken (>{args.sites} lum, >={args.min_site} px): {len(sites)}")
    for n, cx, cy, before, after in sites[:25]:
        ratio = before / after if after > 0 else float("inf")
        print(f"  {n:6d} px  op ({cx:6.0f},{cy:5.0f})  {before:.4f} -> {after:.4f}  "
              f"({ratio:.2f}x donkerder)")


if __name__ == "__main__":
    main()
