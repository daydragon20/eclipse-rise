"""Measure a soft ground effect using a probe render as its own stencil.

The problem this solves: a contact shadow is invisible to the eye and its
exact screen extent is unguessable, so hand-placed measuring rectangles
measure whatever the author hoped was there. Twice in this iteration that
produced a wrong conclusion -- once a rect landed on a pole that carries no
blob at all, and the honest-looking number said "the decal does not render".

So: render the SAME frame once with the decal in an unmistakable colour, use
that frame to find the exact pixels the decal covers, then read those pixels
out of the real frame. The control is a ring around each site at the same
depth, which is what the shadow must be judged against -- a shadow is only
ever "dark" relative to the ground beside it.

  python measure_masked_region.py <probe.png> <real.png> [--ring 14]
"""

import argparse
import sys

try:
    from PIL import Image
except ImportError:
    sys.exit("Pillow ontbreekt: draai met de python die Pillow heeft")


def srgb_to_linear(c):
    c = c / 255.0
    return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4


LUT = [srgb_to_linear(i) for i in range(256)]


def luminance(px):
    return 0.2126 * LUT[px[0]] + 0.7152 * LUT[px[1]] + 0.0722 * LUT[px[2]]


def is_probe_colour(px):
    """Magenta the scene cannot produce: red and blue high, green far below."""
    r, g, b = px
    return r > 150 and b > 150 and g < r - 60 and g < b - 60


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("probe")
    ap.add_argument("real")
    ap.add_argument("--ring", type=int, default=14,
                    help="width in pixels of the control ring around each site")
    ap.add_argument("--min-site", type=int, default=80)
    args = ap.parse_args()

    pi = Image.open(args.probe).convert("RGB")
    ri = Image.open(args.real).convert("RGB")
    if pi.size != ri.size:
        sys.exit(f"maten verschillen: {pi.size} vs {ri.size}")
    w, h = pi.size
    pp, rp = pi.load(), ri.load()

    mask = [[is_probe_colour(pp[x, y]) for x in range(w)] for y in range(h)]

    seen = [row[:] for row in mask]
    sites = []
    for y0 in range(h):
        for x0 in range(w):
            if not seen[y0][x0]:
                continue
            stack = [(x0, y0)]
            seen[y0][x0] = False
            pixels = []
            while stack:
                x, y = stack.pop()
                pixels.append((x, y))
                for dy in (-2, -1, 0, 1, 2):
                    for dx in (-2, -1, 0, 1, 2):
                        nx, ny = x + dx, y + dy
                        if 0 <= nx < w and 0 <= ny < h and seen[ny][nx]:
                            seen[ny][nx] = False
                            stack.append((nx, ny))
            if len(pixels) >= args.min_site:
                sites.append(pixels)

    sites.sort(key=len, reverse=True)
    print(f"{len(sites)} decal-plekken gevonden in {args.probe}\n")
    print("  px    positie        binnen   ring     verhouding")

    all_in = all_ring = 0.0
    n_in = n_ring = 0
    for pixels in sites[:20]:
        xs = [p[0] for p in pixels]
        ys = [p[1] for p in pixels]
        x0, x1 = min(xs), max(xs)
        y0, y1 = min(ys), max(ys)
        inside = {(x, y) for x, y in pixels}
        inner = sum(luminance(rp[x, y]) for x, y in pixels) / len(pixels)

        ring = []
        r = args.ring
        for y in range(max(0, y0 - r), min(h, y1 + r + 1)):
            for x in range(max(0, x0 - r), min(w, x1 + r + 1)):
                if (x, y) in inside or mask[y][x]:
                    continue
                if x0 - r <= x <= x1 + r and y0 - r <= y <= y1 + r:
                    ring.append((x, y))
        if not ring:
            continue
        outer = sum(luminance(rp[x, y]) for x, y in ring) / len(ring)
        ratio = inner / outer if outer > 0 else float("inf")
        print(f"{len(pixels):6d}  ({(x0+x1)//2:5d},{(y0+y1)//2:4d})  "
              f"{inner:.4f}   {outer:.4f}   {ratio:.2f}x")
        all_in += inner * len(pixels)
        n_in += len(pixels)
        all_ring += outer * len(ring)
        n_ring += len(ring)

    if n_in and n_ring:
        gi, go = all_in / n_in, all_ring / n_ring
        print(f"\ngewogen totaal: binnen {gi:.4f}  ring {go:.4f}  -> {gi/go:.2f}x")


if __name__ == "__main__":
    main()
