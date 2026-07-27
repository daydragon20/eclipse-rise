# -*- coding: utf-8 -*-
"""Wat is er VERANDERD aan de opnames sinds de vorige ronde?

Waarom dit bestaat (27-07): de owner-regel is dat er bij elke landing iemand
naar de beelden kijkt. Vannacht heb ik dat een keer of tien niet gedaan - de bar
zei "0 frame-fouten" en ik ging door. Dat is geen slordigheid die je met goede
voornemens oplost: negen beelden per ronde, vijftien rondes, en negentig procent
daarvan is identiek aan de vorige keer.

Dus niet "kijk altijd" maar "kijk als er iets veranderd is". Een ronde waarin
niets beweegt, mag goedkoop zijn; een ronde waarin een beeld kantelt, hoort te
schreeuwen.

De maat is de GEMIDDELDE ABSOLUTE AFWIJKING per pixel, niet het percentage
pixels boven een drempel. Dat tweede telde eerder vannacht 5% "verandering" op
een pilaar die stilstond: anti-aliasing en temporele ruis zetten overal kleine
verschillen neer, en een drempeltelling maakt daar grote getallen van. Een
gemiddelde laat ruis klein en echte verandering groot.
"""
import io
import os
import sys

try:
    from PIL import Image, ImageChops
except ImportError:
    print("shot-diff: PIL ontbreekt, vergelijking overgeslagen.")
    sys.exit(0)

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SHOTS = os.path.join(ROOT, "Saved", "Screenshots", "WindowsEditor")
BASE = os.path.join(ROOT, "Saved", "ShotBaseline")

# DREMPELS PER STAP, en dat is geen verfijning maar een noodzaak.
#
# Twee kalibratierondes zonder enige codewijziging gaven dit:
#
#   stap 1-2 (stilstaand)          0,77 - 1,83
#   stap 3   (lopend + vurend)     9,52 - 10,55
#   stap 4-9 (na verbergen, draaien, herladen)  5,10 - 6,96
#
# Vanaf stap 3 loopt de speler en beweegt de squad, en die twee zijn niet
# bit-reproduceerbaar: hulzen liggen anders, de loopcyclus staat op een andere
# fase. Dat is geen verandering maar beweging.
#
# Eén globale drempel kan hier niet: op 2 gaat hij elke ronde af voor stap 3-9
# (vals alarm, en dan kijkt niemand meer), op 15 is hij blind voor stap 1-2 waar
# een afwijking van 4 al betekenis heeft. Dus per stap, met ruwweg het dubbele
# van de gemeten spreiding als marge.
#
# GEIJKT MET EEN ECHTE RAMP, en dat corrigeerde mijn eerste keuze. De ronde
# gedraaid met "show StaticMeshes" - het complete district onzichtbaar, de
# grofste verandering die er bestaat - gaf 13,5 tot 22,2. Mijn eerste drempel
# van 14 voor stap 4-9 liet stap 8 (13,72) en stap 9 (13,49) er dus DOORHEEN.
# Een alarm dat een verdwenen district niet haalt, is geen alarm.
#
# Vandaar 11 voor die stappen: boven de gemeten ruis (max 7,84) en onder de
# ramp. Die marge is dun en dat hoort hier te staan.
#
# WAT DIT NIET KAN. De onderscheidende kracht verschilt sterk per stap:
#   stap 1-2  ruis ~1-2 tegen ramp ~20   -> tienvoudig verschil, scherp
#   stap 3    ruis ~8-11 tegen ramp 22   -> bruikbaar
#   stap 4-9  ruis ~5-8 tegen ramp 13-18 -> krap; kleine echte veranderingen
#             verdwijnen hier onder de bewegingsruis en worden NIET gevangen
#
# Dit vervangt het kijken dus niet. Het maakt alleen de vraag "is er iets
# gebeurd" goedkoop genoeg om hem elke ronde te stellen.
#
# DE DREMPELS ZIJN VOORLOPIG, en dat is geen slag om de arm maar een meting: de
# ronde direct na het ijken gaf 1,1 tot 2,2 waar de kalibratierondes 5 tot 10
# gaven. De ruis is dus zelf wisselend - hoeveel de squad tussen twee rondes
# verschilt, hangt af van hoe de AI die run uitpakt. Vier rondes is te weinig om
# dat vast te pinnen.
#
# Daarom drukt hij het GETAL elke keer af en niet alleen het oordeel. Gaat hij
# vals alarm geven, dan staat de reden in de kolom ernaast en is de drempel bij
# te stellen op bewijs in plaats van op gevoel.
DREMPEL_PER_STAP = {1: 4.0, 2: 4.0, 3: 20.0}
DREMPEL_OVERIG = 11.0


def drempel(stap):
    return DREMPEL_PER_STAP.get(stap, DREMPEL_OVERIG)


def nieuwste_ronde(aantal=9):
    if not os.path.isdir(SHOTS):
        return []
    pngs = [os.path.join(SHOTS, f) for f in os.listdir(SHOTS) if f.lower().endswith(".png")]
    pngs.sort(key=os.path.getmtime)
    return pngs[-aantal:]


def gemiddelde_afwijking(a, b):
    ia = Image.open(a).convert("RGB")
    ib = Image.open(b).convert("RGB")
    if ia.size != ib.size:
        return None  # ander formaat: niet vergelijkbaar, geen getal verzinnen
    diff = ImageChops.difference(ia, ib)
    hist = diff.convert("L").histogram()
    totaal = sum(i * n for i, n in enumerate(hist))
    pixels = sum(hist)
    return totaal / float(pixels) if pixels else 0.0


def main():
    ronde = nieuwste_ronde()
    if len(ronde) < 9:
        print("shot-diff: minder dan negen opnames, niets te vergelijken.")
        return 0

    os.makedirs(BASE, exist_ok=True)
    kijken = []
    eerste_keer = True
    for index, pad in enumerate(ronde, start=1):
        ijk = os.path.join(BASE, "stap%d.png" % index)
        if os.path.exists(ijk):
            eerste_keer = False
            afwijking = gemiddelde_afwijking(pad, ijk)
            if afwijking is None:
                print("  stap %d: ander beeldformaat — zelf kijken" % index)
                kijken.append(index)
            else:
                grens = drempel(index)
                merk = "  <-- KIJKEN" if afwijking > grens else ""
                print("  stap %d: afwijking %.2f (grens %.0f)%s" % (index, afwijking, grens, merk))
                if afwijking > grens:
                    kijken.append(index)
        # de ijkopname bijwerken, ook als hij niet veranderde
        with open(pad, "rb") as src, open(ijk, "wb") as dst:
            dst.write(src.read())

    if eerste_keer:
        print("shot-diff: eerste ronde, ijkbeelden aangelegd. Volgende keer is er iets te vergelijken.")
    elif kijken:
        print("shot-diff: %d beeld(en) VERANDERD -> %s" % (len(kijken), ", ".join("stap %d" % i for i in kijken)))
    else:
        print("shot-diff: geen enkel beeld noemenswaardig veranderd sinds de vorige ronde.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
