"""Count how loud one hue family is in a frame, so desaturating can be bisected.

Written for the cyan question in dressing-iteratie 3 (par. 1g/6.2). The teal
family reads as neon in cam 3 while the palette authors it at saturation 0.50 —
the grade (ColorSaturation 1.38) pushes it back up, the same shape as the
tonemapper finding for value. So authored saturation does not translate 1:1 and
the only honest way to pick a number is to change it, re-shoot and re-measure.

Reports pixel count, share of frame, and the worst pixel, so a bisection step
has something to compare against.

  python measure_hue_family.py <shot.png> --hue 194 --tolerance 25 --min-sat 0.55
"""

import argparse
import colorsys
import sys

try:
    from PIL import Image
except ImportError:
    sys.exit("Pillow ontbreekt: draai met de python die Pillow heeft")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("frame")
    ap.add_argument("--hue", type=float, default=194.0,
                    help="centre of the family in degrees (194 = the measured teal)")
    ap.add_argument("--tolerance", type=float, default=25.0)
    ap.add_argument("--min-sat", type=float, default=0.55,
                    help="at or above this saturation the family reads as neon")
    ap.add_argument("--min-value", type=float, default=0.12,
                    help="ignore near-black pixels, whose hue is meaningless")
    args = ap.parse_args()

    img = Image.open(args.frame).convert("RGB")
    w, h = img.size
    px = img.load()

    hits = 0
    worst_sat = 0.0
    worst = None
    sat_sum = 0.0
    for y in range(h):
        for x in range(w):
            r, g, b = px[x, y]
            hue, light, sat = colorsys.rgb_to_hls(r / 255.0, g / 255.0, b / 255.0)
            value = max(r, g, b) / 255.0
            if value < args.min_value:
                continue
            # HLS saturation misreads very light pixels; use the HSV definition,
            # which is what "how far from grey is this" means on screen.
            hsv_sat = 0.0 if value == 0 else (value - min(r, g, b) / 255.0) / value
            degrees = hue * 360.0
            if abs(degrees - args.hue) > args.tolerance:
                continue
            if hsv_sat < args.min_sat:
                continue
            hits += 1
            sat_sum += hsv_sat
            if hsv_sat > worst_sat:
                worst_sat = hsv_sat
                worst = (x, y, r, g, b, degrees)

    total = w * h
    print(f"{args.frame}  {w}x{h}")
    print(f"familie hue {args.hue}+-{args.tolerance}, sat >= {args.min_sat}")
    print(f"  pixels   {hits}  ({100.0 * hits / total:.2f}% van het frame)")
    if hits:
        print(f"  gem. sat {sat_sum / hits:.3f}")
        x, y, r, g, b, deg = worst
        print(f"  ergste   sat {worst_sat:.3f} op ({x},{y}) rgb({r},{g},{b}) hue {deg:.1f}")


if __name__ == "__main__":
    main()
