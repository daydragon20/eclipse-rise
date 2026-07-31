#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""Controleert of een opname ook echt een GERENDERDE scene bevat.

WAAROM DIT BESTAAT. `verify.ps1` meldde "25 opnames, 0 frame-fouten" terwijl 22
van de 50 editor-frames van diezelfde nacht hun onderste kwart op luminantie NUL
hadden -- geen enkele pixel. De bestaande controle grept het LOGBOEK op fouten die
de game zelf meldt (`PLAYSHOT n FOUT]`, `UI: FOUT`). Een frame waar de grond niet
op staat meldt niets, dus die controle kon het per constructie niet zien.

Dat is dezelfde vorm als de vault die nooit getekend werd: het instrument
beantwoordde "is de ronde gedraaid?" terwijl de vraag "staat er iets op?" was.
Wat die rondes ook moesten aantonen -- ze hebben het niet aangetoond.

DE DREMPEL IS GEMETEN, NIET GEKOZEN. Op vijf frames waarvan bekend is dat ze goed
zijn ligt het maximum van de onderste band tussen 50 en 249; op de 22 verdachte
frames staat het op EXACT 0. Dat is geen gradatie maar een binair verschil, dus de
poort staat op nul en heeft geen marge nodig.

WAT ER BEWUST NIET GEPOORT WORDT: "vlakheid". Een goed frame van het district
haalt 47,6% binnen een smalle kleurband en een verdacht frame 54% -- te dicht bij
elkaar om op te oordelen. Het wordt gemeld als informatie en niet als fout, want
een poort die op 47 tegen 54 beslist, valt vroeg of laat de verkeerde kant op.

EN DONKER IS NIET LEEG. De vault is legitiem donker (onderband-maximum 81). Deze
controle vraagt niet of een frame helder is, alleen of er IETS gerenderd is.

Draaien:
    python Eclipse/Tools/check_shot_sanity.py <map-of-bestand> [...]
    python Eclipse/Tools/check_shot_sanity.py --zelftest
"""
from __future__ import print_function

import os
import sys

try:
    from PIL import Image
except ImportError:
    sys.exit("Pillow ontbreekt -- deze controle kan niet draaien. "
             "Dat is iets anders dan schoon; meld het als NIET GEDRAAID.")

ONDERBAND = 0.25      # onderste kwart van het beeld
VLAK_MELD = 0.60      # boven dit percentage: melden, niet zakken
SCHAAL = (320, 180)   # downsample; de vraag is grof, de meting hoeft dat ook te zijn


def meet(pad):
    """(max onderband, gemiddelde onderband, max hele frame, vlakheid)."""
    im = Image.open(pad).convert("L")
    im.thumbnail(SCHAAL)
    px = list(im.getdata())
    onder = px[int(len(px) * (1.0 - ONDERBAND)):]
    telling = {}
    for v in px:
        telling[v] = telling.get(v, 0) + 1
    modus = max(telling, key=telling.get)
    vlak = sum(n for v, n in telling.items() if abs(v - modus) <= 5) / float(len(px))
    return max(onder), sum(onder) / float(len(onder)), max(px), vlak


def controleer(paden):
    fouten, meldingen, gezien = [], [], 0
    for pad in paden:
        if os.path.isdir(pad):
            for wortel, _, bestanden in os.walk(pad):
                for b in sorted(bestanden):
                    if b.lower().endswith(".png"):
                        f, m, n = controleer([os.path.join(wortel, b)])
                        fouten += f; meldingen += m; gezien += n
            continue
        if not pad.lower().endswith(".png"):
            continue
        gezien += 1
        try:
            max_onder, gem_onder, max_frame, vlak = meet(pad)
        except Exception as e:
            fouten.append((pad, "onleesbaar: %s" % str(e)[:60]))
            continue
        naam = os.path.basename(pad)
        if max_frame == 0:
            fouten.append((naam, "HELEMAAL ZWART -- er is niets gerenderd"))
        elif max_onder == 0:
            fouten.append((naam,
                           "de onderste %d%% bevat GEEN ENKELE pixel boven nul -- "
                           "de grond staat er niet op, dus wat deze opname moest "
                           "aantonen heeft hij niet aangetoond" % int(ONDERBAND * 100)))
        elif vlak > VLAK_MELD:
            meldingen.append((naam, "%.0f%% van het frame ligt binnen een smalle "
                                    "kleurband (onderband max %d) -- kijk er even naar"
                              % (vlak * 100, max_onder)))
    return fouten, meldingen, gezien


def rapport(paden):
    fouten, meldingen, gezien = controleer(paden)
    print("=" * 72)
    print("OPNAMECONTROLE -- %d frame(s) bekeken" % gezien)
    print("=" * 72)
    if gezien == 0:
        print("  GEEN ENKEL FRAME GEVONDEN. Dat is niet schoon, dat is niet gedraaid.")
        return 1
    for naam, msg in fouten:
        print("  FOUT      %-44s %s" % (naam, msg))
    for naam, msg in meldingen:
        print("  melding   %-44s %s" % (naam, msg))
    if not fouten and not meldingen:
        print("  Elk frame bevat een gerenderde scene.")
    print()
    print("%d fout(en), %d melding(en)" % (len(fouten), len(meldingen)))
    return 1 if fouten else 0


def zelftest():
    """Bewijs dat elke poort rood KAN worden -- en dat donker niet leeg is."""
    import tempfile
    fouten = []

    def maak(tmp, naam, tekenaar):
        im = Image.new("RGB", (320, 180), (0, 0, 0))
        px = im.load()
        tekenaar(px)
        p = os.path.join(tmp, naam)
        im.save(p)
        return p

    with tempfile.TemporaryDirectory() as tmp:
        # 1. een gewoon frame: verloop over de hele hoogte
        goed = maak(tmp, "goed.png", lambda px: [px.__setitem__((x, y), (40 + y, 40 + y, 60 + y))
                    for y in range(180) for x in range(320)])
        f, m, n = controleer([goed])
        if f:
            fouten.append("CONTROLE: een normaal frame zakt (%s)" % f[0][1][:40])

        # 2. DE NEGATIEVE CONTROLE DIE ERTOE DOET: donker maar wel gerenderd.
        #    De vault is legitiem donker; die mag niet zakken.
        donker = maak(tmp, "donker.png", lambda px: [px.__setitem__((x, y), (3, 4, 6) if (x + y) % 7 else (9, 9, 12))
                      for y in range(180) for x in range(320)])
        f, m, n = controleer([donker])
        if f:
            fouten.append("VALS ALARM: een DONKER maar gerenderd frame zakt -- "
                          "dan valt de vault om terwijl hij klopt")

        # 3. onderste kwart zwart -> moet zakken
        halfleeg = maak(tmp, "halfleeg.png", lambda px: [px.__setitem__((x, y), (80, 80, 90))
                        for y in range(0, 135) for x in range(320)])
        f, m, n = controleer([halfleeg])
        if not f:
            fouten.append("een frame zonder grond werd NIET gemeld -- dat is de fout "
                          "waarvoor deze controle bestaat")

        # 4. helemaal zwart -> moet zakken, met een ANDERE melding
        zwart = maak(tmp, "zwart.png", lambda px: None)
        f, m, n = controleer([zwart])
        if not f:
            fouten.append("een volledig zwart frame werd niet gemeld")
        elif "HELEMAAL ZWART" not in f[0][1]:
            fouten.append("volledig zwart en grond-ontbreekt geven dezelfde melding -- "
                          "twee oorzaken die een teller niet scheidt is geen meting")

        # 5. geen frames -> NIET schoon
        leeg = os.path.join(tmp, "leeg")
        os.makedirs(leeg)
        if rapport([leeg]) == 0:
            fouten.append("een lege map gold als geslaagd -- niet gedraaid is niet schoon")

    for f in fouten:
        print("  ROOD  " + f)
    if not fouten:
        print("  Zelftest groen: 5 controles, inclusief de negatieve dat DONKER niet LEEG is.")
    return 1 if fouten else 0


if __name__ == "__main__":
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    if "--zelftest" in sys.argv:
        sys.exit(zelftest())
    if not args:
        wortel = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
        args = [os.path.join(wortel, "Eclipse", "Saved", "Screenshots")]
    sys.exit(rapport(args))
