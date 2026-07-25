"""Crop and upscale a region of a banked shot so a soft effect can be SEEN.

Contact darkening is a low-amplitude gradient: it is invisible at 1920x1080
in a review, and a frame average cannot find it either. Cutting the region
out and blowing it up nearest-neighbour keeps the exact pixel values while
making the shape readable.
"""

import argparse
import sys

try:
    from PIL import Image
except ImportError:
    sys.exit("Pillow ontbreekt: draai met de python die Pillow heeft")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("source")
    ap.add_argument("out")
    ap.add_argument("box", help="x0,y0,x1,y1")
    ap.add_argument("--scale", type=int, default=4)
    args = ap.parse_args()

    x0, y0, x1, y1 = (int(v) for v in args.box.split(","))
    img = Image.open(args.source).convert("RGB").crop((x0, y0, x1, y1))
    img = img.resize(((x1 - x0) * args.scale, (y1 - y0) * args.scale), Image.NEAREST)
    img.save(args.out)
    print(f"{args.out}  {img.size[0]}x{img.size[1]}  (bron {x0},{y0}-{x1},{y1})")


if __name__ == "__main__":
    main()
