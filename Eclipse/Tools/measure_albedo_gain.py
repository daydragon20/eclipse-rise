#!/usr/bin/env python3
"""
measure_albedo_gain.py - AlbedoGain meting voor de toon-master (15.5).

M_EclipseToon vermenigvuldigt de cel-band met de lineaire luminantie van de
albedo-texture maal AlbedoGain; de gain wordt per texture gekozen als
1 / gemiddelde-lineaire-luminantie zodat de gemiddelde multiplier 1.0 is en
texturing de dusk auto-exposure nooit hermetert (EclipseGrayboxBuilder,
System.Drawing-pass 2026-07-22: asfalt .059, beton .049, metaal .031).

Meet hier (Pillow, geen UE nodig):
  * Saved/CC0Staging/ambientcg/<id>/<id>_*_Color.jpg   (ambientCG staging)
  * Saved/CurationStaging/raw/*.png                    (UE-exports, curatiepass)

sRGB -> lineair via de exacte piecewise EOTF; luminantie Rec.709. Beelden
worden eerst naar 512px box-gedownsampled (box filter middelt in sRGB; de
afwijking op de lineaire mean is voor deze ruisachtige tileables < 1% en de
meting blijft zo dependency-vrij, geen numpy).

Gebruik:  python Eclipse/Tools/measure_albedo_gain.py
"""
import os
import sys

from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
PROJECT = os.path.abspath(os.path.join(HERE, ".."))
CC0 = os.path.join(PROJECT, "Saved", "CC0Staging", "ambientcg")
RAW = os.path.join(PROJECT, "Saved", "CurationStaging", "raw")

_LIN = [(c / 255.0) / 12.92 if (c / 255.0) <= 0.04045
        else (((c / 255.0) + 0.055) / 1.055) ** 2.4 for c in range(256)]


def mean_linear_luminance(path):
    img = Image.open(path).convert("RGB")
    if max(img.size) > 512:
        img = img.resize((512, 512), Image.BOX)
    hist = img.histogram()  # 3x256: R, G, B
    n = img.size[0] * img.size[1]
    mean_lin = [sum(hist[ch * 256 + c] * _LIN[c] for c in range(256)) / n
                for ch in range(3)]
    return 0.2126 * mean_lin[0] + 0.7152 * mean_lin[1] + 0.0722 * mean_lin[2]


def collect():
    paths = []
    if os.path.isdir(CC0):
        for cc0_id in sorted(os.listdir(CC0)):
            d = os.path.join(CC0, cc0_id)
            if not os.path.isdir(d):
                continue
            for name in sorted(os.listdir(d)):
                if name.endswith("_Color.jpg"):
                    paths.append(os.path.join(d, name))
    if os.path.isdir(RAW):
        for name in sorted(os.listdir(RAW)):
            if name.lower().endswith((".png", ".jpg")):
                paths.append(os.path.join(RAW, name))
    return paths


def main():
    paths = collect()
    if not paths:
        print("niets te meten (staging leeg)", file=sys.stderr)
        return 1
    print(f"{'texture':44s} {'mean-lin':>9s} {'gain=1/mean':>11s}")
    for path in paths:
        mean = mean_linear_luminance(path)
        gain = (1.0 / mean) if mean > 1e-6 else 0.0
        print(f"{os.path.basename(path):44s} {mean:9.4f} {gain:11.2f}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
