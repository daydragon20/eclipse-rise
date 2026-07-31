# Meet wat er op de WAPENFRAMES staat, zodat "ik zie een wapen" een getal krijgt.
#
# Aanleiding (31-07): de screenshot-inspecteur meldde dat vijf van de zeven
# wapenframes niets bewijzen omdat "de camera in de karaktermesh staat en een
# roodbruin vlak 60 tot 95 procent van het beeld vult". Mijn eigen lezing van
# diezelfde frames was een personage op afstand dat een derde van de hoogte vult.
#
# Twee lezingen van hetzelfde beeld, en geen van beide is een meting. Dit script
# is de meting, en hij is met opzet dom: tel de pixels.
#
#   - FRACTIE HUID  hoeveel procent van het beeld draagt de factietint van het
#                   lichaam. Vult die 60-95%, dan staat de camera inderdaad in de
#                   mesh. Blijft hij onder ~35%, dan staat het personage op
#                   afstand en is het beeld bruikbaar.
#   - GROOTSTE VLEK  de grootste aaneengesloten huidvlek, plus zijn kader. Een
#                   personage op afstand geeft een compacte vlek; een lens in de
#                   mesh geeft een vlek die tot alle vier de randen loopt.
#   - RAKEN DE RANDEN  hoeveel van de vier beeldranden die vlek raakt. Dit is het
#                   beslissende getal: een lichaam VOOR de lens raakt er vier,
#                   een personage in beeld hoogstens een (de onderrand).
#   - DONKERE VLEK  hetzelfde voor de gunmetal-waarde: dat is het WAPEN, en dat
#                   is uiteindelijk waar elk van deze frames over gaat.
#
# De HUD-band bovenin (tekst op een halfdoorzichtige balk) telt niet mee: die is
# in elk frame gelijk en zou elke vergelijking verwateren.

import os
import sys
from collections import deque

from PIL import Image

FRAMES = r"C:\Dev\ECLIPSE_GDD\Eclipse\Saved\Screenshots\HUD_volledig"
HUD_TOP = 240        # de objectievenbalk loopt tot hier
MIN_BLOB = 400       # kleiner dan dit is ruis


def is_skin(r, g, b):
    """De factietint van het lichaam: warm, roodbruin/oranje, duidelijk verzadigd."""
    return r > 70 and r > g * 1.25 and g >= b and (r - b) > 35


def is_gunmetal(r, g, b):
    """Het wapen: donker en ontzadigd. Twee waardestappen onder het lichaam."""
    return r < 85 and abs(r - g) < 26 and abs(g - b) < 26 and max(r, g, b) > 12


def largest_blob(mask, w, h):
    """Grootste aaneengesloten gebied + kader, met een iteratieve vulling."""
    seen = bytearray(w * h)
    best = (0, None)
    for start in range(w * h):
        if mask[start] and not seen[start]:
            queue = deque([start])
            seen[start] = 1
            cells = []
            while queue:
                idx = queue.popleft()
                cells.append(idx)
                x, y = idx % w, idx // w
                for nx, ny in ((x - 1, y), (x + 1, y), (x, y - 1), (x, y + 1)):
                    if 0 <= nx < w and 0 <= ny < h:
                        n = ny * w + nx
                        if mask[n] and not seen[n]:
                            seen[n] = 1
                            queue.append(n)
            if len(cells) > best[0]:
                xs = [c % w for c in cells]
                ys = [c // w for c in cells]
                best = (len(cells), (min(xs), min(ys), max(xs), max(ys)))
    return best


def measure(path):
    img = Image.open(path).convert("RGB")
    W, H = img.size
    px = img.load()

    band_h = H - HUD_TOP
    skin = bytearray(W * band_h)
    gun = bytearray(W * band_h)
    skin_n = gun_n = 0
    for y in range(HUD_TOP, H):
        row = (y - HUD_TOP) * W
        for x in range(W):
            r, g, b = px[x, y]
            if is_skin(r, g, b):
                skin[row + x] = 1
                skin_n += 1
            elif is_gunmetal(r, g, b):
                gun[row + x] = 1
                gun_n += 1

    total = W * band_h
    s_area, s_box = largest_blob(skin, W, band_h)
    g_area, g_box = largest_blob(gun, W, band_h)

    edges = 0
    if s_box and s_area >= MIN_BLOB:
        x0, y0, x1, y1 = s_box
        edges = sum((x0 <= 2, y0 <= 2, x1 >= W - 3, y1 >= band_h - 3))

    return {
        "naam": os.path.basename(path),
        "huid_pct": 100.0 * skin_n / total,
        "huid_vlek_pct": 100.0 * s_area / total,
        "huid_kader": s_box,
        "randen": edges,
        "wapen_pct": 100.0 * gun_n / total,
        "wapen_vlek": g_area,
        "wapen_kader": g_box if g_area >= MIN_BLOB else None,
    }


def main():
    files = sorted(f for f in os.listdir(FRAMES)
                   if f.startswith("HUD_wapen_") and f.endswith(".png"))
    if not files:
        sys.exit("geen HUD_wapen_*.png in %s" % FRAMES)

    print("=" * 108)
    print("%-38s %7s %7s %6s  %-22s %7s %8s" %
          ("frame", "huid%", "vlek%", "randen", "kader vlek", "wapen%", "wapenvlek"))
    print("-" * 108)
    for name in files:
        m = measure(os.path.join(FRAMES, name))
        box = "%d,%d..%d,%d" % m["huid_kader"] if m["huid_kader"] else "-"
        print("%-38s %6.1f%% %6.1f%% %6d  %-22s %6.1f%% %8d" %
              (m["naam"], m["huid_pct"], m["huid_vlek_pct"], m["randen"],
               box, m["wapen_pct"], m["wapen_vlek"]))
    print("=" * 108)
    print("LEZEN: randen=4 en vlek>50%% -> de camera staat IN de mesh, het frame bewijst niets.")
    print("       randen<=1 en vlek 5-35%% -> personage op afstand, bruikbaar bewijs.")
    print("       wapenvlek is de gunmetal-vlek: dat is waar elk van deze frames over gaat.")


if __name__ == "__main__":
    main()
