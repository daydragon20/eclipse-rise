#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""Toetst of de owner-documenten getallen claimen die nog kloppen.

WAAROM DIT BESTAAT. Op 01-08 stonden VIER owner-gerichte teksten verlopen, alle
vier op dezelfde manier -- ze HERHAALDEN de stand in plaats van ernaar te
verwijzen:

  NIEUWE_CHAT_PROMPT.txt   een rode test die groen was, een HUD-poort die als
                           teruggedraaid beschreven stond terwijl hij geland was,
                           310.000 credits waar er 131.000 zijn
  STATUS.md                de ijkmissie als NO-GO terwijl hij door de poort is
  JOUW_ACTIES.md           de API-scopes als "de enige echte rem" terwijl ze aan
                           staan
  progress_data.js         veertig taken op "bezig" terwijl er twee liepen

Het project heeft hier al een PRINCIPE voor ("verwijzen, niet herhalen") en dat
heeft het vier keer niet tegengehouden. Een principe stopt dit niet; een controle
wel -- maar alleen voor het deel dat mechanisch te toetsen is.

WAT ER GETOETST WORDT, EN WAAROM ZO WEINIG. Alleen claims met een GEZAGHEBBENDE
BRON ernaast. Het testaantal in STATUS.md kan tegen `phase0/SOAK_LOG.md` gehouden
worden, want dat logboek wordt door `verify.ps1` zelf geschreven. "De ijkmissie is
NO-GO" is niet mechanisch te toetsen zonder de betekenis te kennen, dus dat staat
hier NIET in -- een controle die doet alsof hij meer dekt dan hij kan, is precies
de fout die deze hele reeks tools bestrijdt.

Draaien:
    python Eclipse/Tools/check_owner_docs.py
    python Eclipse/Tools/check_owner_docs.py --zelftest
"""
from __future__ import print_function

import io
import os
import re
import sys

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
STATUS = os.path.join(REPO, "STATUS.md")
SOAK = os.path.join(REPO, "phase0", "SOAK_LOG.md")

# "| 2026-08-01 01:34 | `hash` | **ROOD** | 244 tests / 0 gefaald / ... |"
RE_SOAK = re.compile(r"^\|\s*(\d{4}-\d{2}-\d{2}[^|]*)\|[^|]*\|[^|]*\|\s*(\d+) tests", re.M)
RE_CLAIM = re.compile(r"\*\*(\d+) tests")


def nieuwste_soak(tekst):
    """Het testaantal uit de LAATSTE regel van het logboek, of None."""
    rijen = RE_SOAK.findall(tekst)
    return int(rijen[-1][1]) if rijen else None


def controleer(status_tekst, soak_tekst):
    bevindingen, overgeslagen = [], []
    waar = nieuwste_soak(soak_tekst)
    if waar is None:
        overgeslagen.append("SOAK_LOG.md bevat geen leesbare bar-regel -- "
                            "NIET GEDRAAID, en dat is iets anders dan schoon")
        return bevindingen, overgeslagen
    geclaimd = RE_CLAIM.findall(status_tekst)
    if not geclaimd:
        overgeslagen.append("STATUS.md claimt geen testaantal -- niets te toetsen")
        return bevindingen, overgeslagen
    for g in set(geclaimd):
        if int(g) != waar:
            bevindingen.append(
                "STATUS.md claimt **%s tests** terwijl de laatste bar-regel in "
                "SOAK_LOG.md %d zegt. Een statuskaart die de stand HERHAALT in "
                "plaats van ernaar te verwijzen, veroudert -- en een verse sessie "
                "leest hem als eerste." % (g, waar))
    return bevindingen, overgeslagen


def rapport():
    st = io.open(STATUS, encoding="utf-8-sig").read()
    sk = io.open(SOAK, encoding="utf-8-sig").read()
    bev, over = controleer(st, sk)
    print("=" * 72)
    print("OWNER-DOCUMENTEN -- getallen tegen hun gezaghebbende bron")
    print("=" * 72)
    for b in bev:
        print("  VEROUDERD  " + b)
    for o in over:
        print("  OVERGESLAGEN: " + o)
    if not bev and not over:
        print("  Het testaantal in STATUS.md klopt met de laatste bar-regel.")
    print()
    print("%d bevinding(en)" % len(bev))
    return 1 if bev else 0


def zelftest():
    """Bewijs dat hij rood KAN worden, en dat hij niet altijd rood staat."""
    SOAK_OK = ("| Wanneer | Commit | Uitslag | Suite | Opnames | Waarop |\n"
               "|---|---|---|---|---|---|\n"
               "| 2026-08-01 01:00 | `aaa` | **GROEN** | 200 tests / 0 gefaald | 9 | - |\n"
               "| 2026-08-01 02:00 | `bbb` | **ROOD** | 244 tests / 0 gefaald | 0 | x |\n")
    fouten = []

    def draai(status, soak=SOAK_OK):
        return controleer(status, soak)[0]

    # 1. gelijk = schoon
    if draai("Bar: **244 tests**, 0 gefaald"):
        fouten.append("CONTROLE: een KLOPPEND getal werd als verouderd gemeld")
    # 2. ongelijk = bevinding
    if not draai("Bar: **237 tests**, 0 gefaald"):
        fouten.append("een verouderd testaantal werd NIET gemeld -- dat is de fout "
                      "waarvoor deze controle bestaat")
    # 3. hij leest de LAATSTE regel, niet de eerste
    if draai("Bar: **200 tests**"):
        pass
    else:
        fouten.append("hij pakte de eerste bar-regel in plaats van de laatste")
    # 4. geen claim = overslaan, niet groen liegen
    b, o = controleer("Geen enkel getal hier.", SOAK_OK)
    if b or not o:
        fouten.append("zonder claim moet hij OVERSLAAN melden, niet stil slagen")
    # 5. leeg logboek = overslaan, niet groen
    b, o = controleer("Bar: **244 tests**", "geen tabel hier")
    if b or not o:
        fouten.append("zonder bar-regel moet hij OVERSLAAN melden -- niet gedraaid "
                      "is niet hetzelfde als schoon")

    for f in fouten:
        print("  ROOD  " + f)
    if not fouten:
        print("  Zelftest groen: 5 controles, inclusief de negatieve dat een "
              "kloppend getal NIET gemeld wordt.")
    return 1 if fouten else 0


if __name__ == "__main__":
    sys.exit(zelftest() if "--zelftest" in sys.argv else rapport())
