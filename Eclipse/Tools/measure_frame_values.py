#!/usr/bin/env python3
"""Frame-probe voor de 15.8-reviewloop: meet echte waarden uit een gebankte shot.

Waarom dit bestaat: in dressing-iteratie 2 werd de vloer "naar schemer-waarden"
geschoven en bleek na terugmeten 16% LICHTER te zijn geworden, terwijl de
automatische belichting alles bovendien x1.79 rendert. Zo'n omkering blijft
onzichtbaar zolang "te helder / te donker" een gevoel is. Vanaf nu is het een
getal, en kan een claim over een waarde worden nagerekend in plaats van geloofd.

Rekent per rechthoek de sRGB-waarden om naar lineair (echte sRGB-EOTF, geen
gamma-2.2-benadering) en geeft de Rec.709-luminantie -- dezelfde grootheden
waarin de palet-tints in EclipseGrayboxBuilder.cpp zijn geauthord, zodat
frame en code direct vergelijkbaar zijn.

Gebruik:
  python measure_frame_values.py <shot.png> vloer=760,700,900,780 pool=1150,340,1250,385
  python measure_frame_values.py <shot.png> --strip-y 700     # horizontale scan
Rechthoek = naam=x0,y0,x1,y1 (pixels, x1/y1 exclusief).
"""
from __future__ import annotations

import sys
from pathlib import Path

try:
    from PIL import Image
except ImportError:
    sys.exit("Pillow ontbreekt: pip install Pillow (of draai in de env die generate_decals.py gebruikt)")

# Rec.709 luma-weging: dezelfde die de art-review gebruikte.
LUMA = (0.2126, 0.7152, 0.0722)


def srgb_to_linear(channel_0_255: float) -> float:
    """Echte sRGB-EOTF. De 2.2-benadering wijkt tot ~5% af in de schaduwen -
    precies de band waar de schemer-beslissingen liggen, dus die kan niet."""
    c = channel_0_255 / 255.0
    return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4


def measure(img: Image.Image, box: tuple[int, int, int, int]) -> dict[str, float]:
    x0, y0, x1, y1 = box
    x0, y0 = max(0, x0), max(0, y0)
    x1, y1 = min(img.width, x1), min(img.height, y1)
    if x1 <= x0 or y1 <= y0:
        raise ValueError(f"lege rechthoek na clampen: {box} op {img.width}x{img.height}")

    # tobytes() i.p.v. getdata(): stabiel over Pillow-versies (getdata is
    # deprecated vanaf 14) en sneller op grote rechthoeken.
    raw = img.crop((x0, y0, x1, y1)).convert("RGB").tobytes()
    pixels = [(raw[i], raw[i + 1], raw[i + 2]) for i in range(0, len(raw), 3)]
    n = len(pixels)
    # Gemiddelde in LINEAIRE ruimte: gemiddelden in sRGB zijn natuurkundig
    # onjuist en verschuiven donkere vlakken systematisch te licht.
    lin = [tuple(srgb_to_linear(c) for c in p) for p in pixels]
    lin_mean = tuple(sum(p[i] for p in lin) / n for i in range(3))
    srgb_mean = tuple(sum(p[i] for p in pixels) / n for i in range(3))
    return {
        "n": n,
        "srgb": srgb_mean,
        "linear": lin_mean,
        "lum_lin": sum(lin_mean[i] * LUMA[i] for i in range(3)),
        "max_srgb": max(max(p) for p in pixels),
        "clipped": sum(1 for p in pixels if max(p) >= 254) / n,
    }


def main(argv: list[str]) -> int:
    if len(argv) < 2:
        sys.exit(__doc__)
    path = Path(argv[1])
    if not path.exists():
        sys.exit(f"bestaat niet: {path}")
    img = Image.open(path)
    print(f"{path.name}  {img.width}x{img.height}")

    args = argv[2:]
    if args and args[0] == "--strip-y":
        y = int(args[1])
        band, step = 40, img.width // 8
        print(f"\nhorizontale scan op y={y} (luchtperspectief-check: neemt de waarde AF met afstand?)")
        for x in range(0, img.width - step, step):
            m = measure(img, (x, y, x + step, y + band))
            print(f"  x={x:5d}  lum_lin={m['lum_lin']:.4f}  lin=({m['linear'][0]:.4f},{m['linear'][1]:.4f},{m['linear'][2]:.4f})")
        return 0

    if not args:
        print("\ngeen rechthoeken opgegeven - alleen beeldinfo. Voorbeeld: vloer=760,700,900,780")
        return 0

    for spec in args:
        if "=" not in spec:
            print(f"  overgeslagen (geen naam=x0,y0,x1,y1): {spec}")
            continue
        name, coords = spec.split("=", 1)
        box = tuple(int(v) for v in coords.split(","))
        if len(box) != 4:
            print(f"  overgeslagen (4 coordinaten nodig): {spec}")
            continue
        m = measure(img, box)  # type: ignore[arg-type]
        print(
            f"\n{name}:  lum_lin={m['lum_lin']:.4f}\n"
            f"  lineair    ({m['linear'][0]:.4f}, {m['linear'][1]:.4f}, {m['linear'][2]:.4f})\n"
            f"  sRGB 0-255 ({m['srgb'][0]:.1f}, {m['srgb'][1]:.1f}, {m['srgb'][2]:.1f})  max={m['max_srgb']}\n"
            f"  geclipt    {m['clipped'] * 100:.1f}% van {m['n']} pixels"
        )
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
