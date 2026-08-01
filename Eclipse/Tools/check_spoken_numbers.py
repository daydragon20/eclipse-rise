#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""De telling van gesproken getallen, plus EEN poort: een notitie mag niet
beweren dat een getal vrij is terwijl het gesproken wordt.

WAAROM DIT BESTAAT
------------------
Op 01-08 kozen VIER agents onafhankelijk "het enige vrije getal onder de
twintig" om een botsing met L1-R14 op te lossen. M1.1 nam achttien, M1.2 nam
zestien, M1.6 nam zeventien, M1.5 nam opnieuw achttien.

Elk van die vier had GELIJK op het moment dat hij mat. Alle vier de metingen
zijn nu onwaar, want ze maten tegen een corpus dat in dezelfde nacht onder hen
veranderde. En twee van hen schreven hun meting op als een `note:` -- en die
notitie is nu een instructie die de volgende schrijver verkeerd stuurt.

Dat is niet hun fout. Een schrijver kan niet zien wat een parallelle schrijver
op datzelfde moment doet. Wat wel kan: het antwoord uit een tool halen in plaats
van uit een notitie.

DE POORT IS SMAL MET OPZET. Hij oordeelt NIET dat twee gebruiken van hetzelfde
getal een defect zijn -- L1-R33 heeft juist beslist dat herhaling geen defect is
en REFERENTDRIFT wel, en dat onderscheid kan een tool niet maken. Hij vangt
precies een ding: een notitie die beweert dat een getal nergens gesproken wordt,
terwijl deze telling het tegendeel meet.

Draai:  python Eclipse/Tools/check_spoken_numbers.py
        python Eclipse/Tools/check_spoken_numbers.py --zelftest
"""
import collections
import glob
import io
import os
import re
import sys

WORTEL = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CORPUS = os.path.join(WORTEL, "Content", "Script")

WOORDEN = ["one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten",
           "eleven", "twelve", "thirteen", "fourteen", "fifteen", "sixteen",
           "seventeen", "eighteen", "nineteen", "twenty"]

NL = {"een": "one", "twee": "two", "drie": "three", "vier": "four", "vijf": "five",
      "zes": "six", "zeven": "seven", "acht": "eight", "negen": "nine", "tien": "ten",
      "elf": "eleven", "twaalf": "twelve", "dertien": "thirteen", "veertien": "fourteen",
      "vijftien": "fifteen", "zestien": "sixteen", "zeventien": "seventeen",
      "achttien": "eighteen", "negentien": "nineteen", "twintig": "twenty"}

RE_TEXT = re.compile(r"^\s*(?:-\s*)?text:\s*(.+)$")
RE_VARIANT = re.compile(r"^\s*(pragmatist|idealist|personal|strategic):\s*(.+)$")

# "nergens in act 1 gesproken", "nowhere spoken", "is vrij", "waren de enige twee vrije"
# Een notitie mag een ACHTERHAALDE claim citeren -- dat is geschiedenis, geen
# bewering. Markeer hem dan, met de reden erbij. Het aantal wordt ALTIJD geprint.
RE_VRIJSTELLING = re.compile(r"getal-nagelopen:\s*(.+?)\s*(?:-->|$)")

RE_VRIJHEIDSCLAIM = re.compile(
    r"(nergens[^.]{0,40}gesproken|nowhere[^.]{0,30}spoken|vrij[a-z]*\s+getal|free\s+number"
    r"|enige[^.]{0,30}vrije)", re.I)


def _lees(pad):
    with io.open(pad, encoding="utf-8-sig") as fh:
        return fh.readlines()


def telling(corpus=CORPUS):
    """{engelswoord: [plaats, ...]} over gesproken tekst en varianten."""
    uit = collections.defaultdict(list)
    for pad in sorted(glob.glob(os.path.join(corpus, "**", "*.yaml"), recursive=True)):
        rel = os.path.relpath(pad, corpus).replace("\\", "/")
        for nr, regel in enumerate(_lees(pad), 1):
            m = RE_TEXT.match(regel) or RE_VARIANT.match(regel)
            if not m:
                continue
            tekst = m.group(m.lastindex)
            for w in WOORDEN:
                if re.search(r"\b%s\b" % w, tekst, re.I):
                    uit[w].append("%s:%d" % (rel, nr))
    return uit


def valse_claims(corpus=CORPUS, gemeten=None):
    """Notities die een getal vrij noemen terwijl de telling het hoort."""
    gemeten = telling(corpus) if gemeten is None else gemeten
    bevindingen, vrijgesteld = [], []
    for pad in sorted(glob.glob(os.path.join(corpus, "**", "*.yaml"), recursive=True)):
        rel = os.path.relpath(pad, corpus).replace("\\", "/")
        for nr, regel in enumerate(_lees(pad), 1):
            if RE_TEXT.match(regel) or RE_VARIANT.match(regel):
                continue                       # gesproken tekst is geen claim
            if not RE_VRIJHEIDSCLAIM.search(regel):
                continue
            # HET ONDERWERP van de claim is het getal dat er vlak VOOR staat,
            # niet het eerste getal in de regel. Zonder die eis meldde deze
            # controle `eleven` en `twenty` op een notitie die over ACHTTIEN
            # gaat -- de getallen stonden er terloops in ("de enige twee vrije
            # getallen onder de twintig"). Dat is exact de fout die dit
            # gereedschap moet vangen, en hij zat in het gereedschap zelf.
            claim = RE_VRIJHEIDSCLAIM.search(regel)
            kandidaten = [(m.start(), m.group()) for m in re.finditer(r"[A-Za-z]+", regel)
                          if NL.get(m.group().lower(), m.group().lower()) in WOORDEN]
            voor = [k for k in kandidaten if k[0] < claim.start()]
            if not voor:
                continue
            woord = voor[-1][1]                      # het dichtstbijzijnde ervoor
            eng = NL.get(woord.lower(), woord.lower())
            if not gemeten.get(eng):
                continue
            elders = [x for x in gemeten[eng] if not x.startswith(rel.split(":")[0])]
            if not elders:
                continue
            vrij = RE_VRIJSTELLING.search(regel)
            if vrij:
                vrijgesteld.append((rel, nr, woord, vrij.group(1)))
            else:
                bevindingen.append((rel, nr, woord, eng, len(gemeten[eng]), elders[0]))
    return bevindingen, vrijgesteld


def _zelftest():
    import shutil
    import tempfile
    t = tempfile.mkdtemp(prefix="getallen_")
    gezakt = []
    try:
        d = os.path.join(t, "m")
        os.makedirs(d)

        def schrijf(naam, inhoud):
            with io.open(os.path.join(d, naam), "w", encoding="utf-8") as f:
                f.write(inhoud)

        # 1. de telling ziet gesproken tekst
        schrijf("a.yaml", '  - id: X\n    text:    "He gave you eighteen days."\n')
        g = telling(t)
        if len(g.get("eighteen", [])) != 1:
            gezakt.append("1: gesproken getal niet geteld")

        # 2. DE CONTROLEPROEF: precies wat op 01-08 gebeurde moet rood worden --
        #    een notitie in het ENE bestand die het getal vrij noemt terwijl het
        #    in een ANDER bestand gesproken wordt.
        schrijf("b.yaml", '  - id: Y\n    note:    "ACHTTIEN is gekozen omdat het nergens '
                          'in act 1 gesproken wordt."\n')
        b, _ = valse_claims(t)
        if len(b) != 1:
            gezakt.append("2: de historische fout wordt niet gevangen (%d)" % len(b))

        # 3. een claim over een getal dat ECHT nergens klinkt, blijft stil
        schrijf("c.yaml", '  - id: Z\n    note:    "TWINTIG is vrij getal, nergens gesproken."\n')
        b, _ = valse_claims(t)
        if any(x[3] == "twenty" for x in b):
            gezakt.append("3: een JUISTE vrijheidsclaim wordt gemeld")

        # 4. gesproken tekst is zelf geen claim -- anders meldt hij elke regel
        #    waarin iemand het woord 'vrij' zegt.
        schrijf("d.yaml", '  - id: W\n    text:    "Eighteen is a free number, nowhere spoken."\n')
        b, _ = valse_claims(t)
        if any(x[0].endswith("d.yaml") for x in b):
            gezakt.append("4: gesproken tekst wordt als notitie-claim gelezen")

        # 5. een notitie die zijn EIGEN regel beschrijft is geen fout: het getal
        #    klinkt daar per definitie. Alleen elders telt.
        schrijf("e.yaml", '  - id: V\n    text:    "Nineteen."\n'
                          '    note:    "NEGENTIEN is vrij getal."\n')
        b, _ = valse_claims(t)
        if any(x[0].endswith("e.yaml") for x in b):
            gezakt.append("5: een notitie over zijn eigen bestand wordt gemeld")

        # 6. DE VALSE-POSITIEF-PROEF: de claim gaat over ACHTTIEN; `sixteen` en
        #    `twintig` staan er terloops in. Alleen achttien mag gemeld worden.
        schrijf("f.yaml", '  - id: U\n    note:    "ACHTTIEN is gekozen omdat het nergens '
                          'gesproken wordt (sixteen en eighteen waren de enige vrije getallen '
                          'onder de twintig)."\n')
        b, _ = valse_claims(t)
        fout = [x for x in b if x[0].endswith("f.yaml") and x[3] != "eighteen"]
        if fout:
            gezakt.append("6: pakt een terloops getal in plaats van het ONDERWERP (%s)"
                          % [x[3] for x in fout])

        print("ZELFTEST check_spoken_numbers -- 6 controles")
        for r in gezakt:
            print("  GEZAKT  %s" % r)
        if gezakt:
            print("\n%d van 5 gezakt." % len(gezakt))
            return 1
        print("  alle 6 geslaagd -- de poort vangt de fout van 01-08 en zwijgt als het klopt")
        return 0
    finally:
        shutil.rmtree(t, ignore_errors=True)


def main(argv):
    if "--zelftest" in argv:
        return _zelftest()

    gemeten = telling()
    print("GESPROKEN GETALLEN IN ACT 1 -- gemeten over `text:` en varianten")
    print("(deze telling is het antwoord op 'is dit getal nog vrij'. Een `note:` is dat niet:")
    print(" vier agents schreven op 01-08 elk hun eigen meting op en alle vier zijn nu onwaar.)")
    print()
    vrij = []
    for w in WOORDEN:
        n = len(gemeten.get(w, []))
        if n == 0:
            vrij.append(w)
            continue
        merk = "  <-- 1 gebruik, nog te claimen" if n == 1 else ""
        print("  %-10s %2d%s" % (w, n, merk))
        if n <= 3:
            for p in gemeten[w]:
                print("               %s" % p)
    print()
    print("VRIJ (nergens gesproken): %s" % (", ".join(vrij) if vrij else "GEEN ENKEL GETAL"))

    bev, vrij = valse_claims(gemeten=gemeten)
    print("\n%d vrijstelling(en) van kracht%s" % (len(vrij), ":" if vrij else "."))
    for rel, nr, woord, reden in vrij:
        print("  %s:%d  %s  --  %s" % (rel, nr, woord, reden))
    if not bev:
        print("\nGeen notitie beweert dat een gesproken getal vrij is.")
        return 0
    print("\n%d notitie(s) noemen een getal vrij dat elders gesproken wordt:" % len(bev))
    for rel, nr, woord, eng, n, elders in bev:
        print("  %s:%d  noemt %r vrij, maar `%s` klinkt %dx -- o.a. %s"
              % (rel, nr, woord, eng, n, elders))
    print("\nDe meting was waar toen ze werd opgeschreven. Dat is precies het probleem:")
    print("een notitie bevriest een telling die verder loopt. Verwijs naar deze tool.")
    return 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
