#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Controleert EEN ding: rulings die redeneren vanuit een poortstatus, tegen de
poortstatus die de scenebestanden echt dragen.

WAAROM DIT BESTAAT
------------------
Op 01-08 bleek dat twee rulings (L1-R14 en L1-R21) allebei redeneerden met
"M1.5 is GO en wordt niet heropend" terwijl alle zes M1.5-bestanden `critic:
null` droegen. Voor L1-R21 gaf dat niets -- hij had nog twee andere gronden.
Voor L1-R14 wel: die had EEN grond, en die was onwaar. De uitzondering die
daarop stond hield "eleven days" als verhoorduur in stand, en vijftien andere
bestanden zijn intussen op dat getal geveegd -- waarvan een (M1.6.S03) zijn
EIGEN getal inleverde om die regel te beschermen.

De fout is dus niet duur omdat een document ernaast zit. Hij is duur omdat een
ANDERE missie ervoor betaalt, en niemand die dat kan zien.

Het is een keer met de hand nagelopen (RULINGS_L1.md, slotregel: "elke claim in
dit document ... is machinaal tegen de scenebestanden gehouden"). Dat is precies
het soort controle dat veroudert: M1.5 is sinds vannacht geen `null` meer, en er
komen rulings bij.

WAT HIJ NIET DOET
-----------------
Hij leest geen argumenten en beoordeelt geen redeneringen. Hij vergelijkt een
BEWEERDE poortstatus met een GEMETEN poortstatus. Een ruling mag best een missie
noemen; hij mag alleen niet beweren dat die missie ergens staat waar hij niet
staat.

Vrijstellen kan, met een reden erbij:

    <!-- premisse-nagelopen: deze regel MELDT de botsing, hij beweert hem niet -->

Het aantal vrijstellingen wordt ALTIJD geprint, ook als het er nul zijn. Een
vrijstelling die stil is, is een gat.

Draai:  python Eclipse/Tools/check_ruling_premises.py
        python Eclipse/Tools/check_ruling_premises.py --zelftest
"""
import glob
import os
import re
import sys

WORTEL = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RULINGS = os.path.join(os.path.dirname(WORTEL), "phase0", "beats", "RULINGS_L1.md")
SCENES = os.path.join(WORTEL, "Content", "Script", "act1")

VRIJSTELLING = re.compile(r"<!--\s*premisse-nagelopen:\s*(.+?)\s*-->")

# "M1.5 is GO", "M1.5 staat op draft", "M1.4.S99 staat op draft, critic: null".
# Het venster is klein met opzet: hoe verder de bewering van de missienaam staat,
# hoe groter de kans dat we een zin verkeerd lezen.
CLAIM = re.compile(
    r"\bM1\.(\d)(?:\.S\d{2})?\b"          # de missie (of scene) waarover iets beweerd wordt
    r"(?P<tussen>[^.!?|]{0,40}?)"          # kort venster, niet over een zinseinde heen
    r"\b(?P<status>GO|NO-GO|draft|null)\b",
    re.I,
)


def _lees(pad):
    with open(pad, "rb") as f:
        return f.read().decode("utf-8-sig")


def poortstand(missie, wortel=SCENES):
    """De gemeten verdicts van een missie: {'GO': n, 'NO-GO': n, 'null': n}."""
    stand = {"GO": 0, "NO-GO": 0, "null": 0}
    patroon = os.path.join(wortel, "M1.%s_*" % missie, "*.yaml")
    for pad in sorted(glob.glob(patroon)):
        m = re.search(r"(?m)^critic:\s*(.+?)\s*$", _lees(pad))
        if not m:
            continue
        waarde = m.group(1)
        if waarde == "null":
            stand["null"] += 1
        elif waarde.strip('"').upper().startswith("NO-GO"):
            stand["NO-GO"] += 1
        else:
            stand["GO"] += 1
    return stand


def spreekt_tegen(status, stand):
    """Spreekt een beweerde status de gemeten stand tegen?

    Bewust smal. We vangen alleen de vorm die op 01-08 geld kostte: een ruling
    die een missie GO noemt terwijl er nog scenes ongepoort of afgekeurd zijn,
    of die hem ongepoort noemt terwijl alles al beoordeeld is.
    """
    totaal = sum(stand.values())
    if totaal == 0:
        return None                                   # missie bestaat niet -- niet ons werk
    status = status.lower()
    if status == "go" and (stand["null"] or stand["NO-GO"]):
        return "beweert GO, maar %d scene(s) staan op null en %d op NO-GO" % (
            stand["null"], stand["NO-GO"])
    if status in ("draft", "null") and stand["null"] == 0:
        return "beweert ongepoort, maar alle %d scene(s) dragen een verdict" % totaal
    return None


def controleer(rulings_pad=RULINGS, scenes_wortel=SCENES):
    """Geeft (bevindingen, vrijgesteld, gecontroleerd) terug."""
    tekst = _lees(rulings_pad)
    bevindingen, vrijgesteld, gecontroleerd = [], [], 0

    for nr, regel in enumerate(tekst.splitlines(), 1):
        vrij = VRIJSTELLING.search(regel)
        for m in CLAIM.finditer(regel):
            missie, status = m.group(1), m.group("status")
            stand = poortstand(missie, scenes_wortel)
            gecontroleerd += 1
            botsing = spreekt_tegen(status, stand)
            if not botsing:
                continue
            if vrij:
                vrijgesteld.append((nr, "M1.%s" % missie, botsing, vrij.group(1)))
            else:
                bevindingen.append((nr, "M1.%s" % missie, status, botsing))
    return bevindingen, vrijgesteld, gecontroleerd


# ---------------------------------------------------------------- zelftest ---

def _zelftest():
    """Bewijst dat de controle rood KAN worden voor je een groen vertrouwt."""
    import shutil
    import tempfile

    tijdelijk = tempfile.mkdtemp(prefix="premisse_")
    try:
        def scene(missie, naam, critic):
            d = os.path.join(tijdelijk, "M1.%s_test" % missie)
            if not os.path.isdir(d):
                os.makedirs(d)
            with open(os.path.join(d, naam), "w", encoding="utf-8") as f:
                f.write("id: M1.%s.S01\nstatus: draft\ncritic: %s\n" % (missie, critic))

        def rulings(inhoud):
            p = os.path.join(tijdelijk, "R.md")
            with open(p, "w", encoding="utf-8") as f:
                f.write(inhoud)
            return p

        gezakt = []

        # 1. DE CONTROLEPROEF: precies de fout van 01-08 moet rood worden.
        scene("5", "a.yaml", "null")
        scene("5", "b.yaml", "null")
        r = rulings("| De verhoorduur | Blijft staan. M1.5 is GO en wordt niet heropend. |\n")
        b, v, n = controleer(r, tijdelijk)
        if len(b) != 1:
            gezakt.append("1: de historische fout wordt niet gevangen (%d bevindingen)" % len(b))

        # 2. Klopt de bewering, dan blijft hij stil.
        r = rulings("M1.5 staat op draft, critic: null.\n")
        b, v, n = controleer(r, tijdelijk)
        if b:
            gezakt.append("2: een JUISTE bewering wordt gemeld (%s)" % (b,))

        # 3. Een missie die WEL door de poort is, mag GO heten.
        scene("7", "a.yaml", "GO")
        r = rulings("M1.7 is GO.\n")
        b, v, n = controleer(r, tijdelijk)
        if b:
            gezakt.append("3: een terechte GO-claim wordt gemeld (%s)" % (b,))

        # 4. Een NO-GO in de missie ontkracht een GO-claim ook.
        scene("7", "b.yaml", '"NO-GO: iets"')
        r = rulings("M1.7 is GO.\n")
        b, v, n = controleer(r, tijdelijk)
        if len(b) != 1:
            gezakt.append("4: NO-GO ontkracht de GO-claim niet")

        # 5. Vrijstelling werkt EN wordt geteld -- een stille vrijstelling is een gat.
        scene("6", "a.yaml", "null")
        r = rulings("M1.6 is GO. <!-- premisse-nagelopen: dit MELDT de botsing -->\n")
        b, v, n = controleer(r, tijdelijk)
        if b or len(v) != 1:
            gezakt.append("5: vrijstelling telt niet mee (%d bevindingen, %d vrij)" % (len(b), len(v)))

        # 6. Een missie die niet bestaat is niet ons werk -- geen valse rode.
        r = rulings("M1.9 is GO.\n")
        b, v, n = controleer(r, tijdelijk)
        if b:
            gezakt.append("6: een onbekende missie levert een valse bevinding")

        # 7. De tool mag niet over een zinseinde heen lezen.
        r = rulings("M1.5 wordt hier genoemd. Een andere zin zegt GO over iets anders.\n")
        b, v, n = controleer(r, tijdelijk)
        if b:
            gezakt.append("7: leest over een zinseinde heen (%s)" % (b,))

        print("ZELFTEST check_ruling_premises -- 7 controles")
        for r_ in gezakt:
            print("  GEZAKT  %s" % r_)
        if gezakt:
            print("\n%d van 7 gezakt." % len(gezakt))
            return 1
        print("  alle 7 geslaagd -- de controle kan rood worden en blijft stil als het klopt")
        return 0
    finally:
        shutil.rmtree(tijdelijk, ignore_errors=True)


def main(argv):
    if "--zelftest" in argv:
        return _zelftest()

    if not os.path.exists(RULINGS):
        print("check_ruling_premises: %s bestaat niet" % RULINGS)
        return 1

    bevindingen, vrijgesteld, gecontroleerd = controleer()

    print("check_ruling_premises -- %d statusbewering(en) in RULINGS_L1.md getoetst "
          "tegen de scenebestanden." % gecontroleerd)

    # ALTIJD printen, ook bij nul: een vrijstelling die stil is, is een gat.
    print("%d vrijstelling(en) van kracht%s" % (len(vrijgesteld), ":" if vrijgesteld else "."))
    for nr, missie, botsing, reden in vrijgesteld:
        print("  r.%-4d %-6s %s  --  %s" % (nr, missie, botsing, reden))

    if not bevindingen:
        print("\nGeen ruling redeneert vanuit een poortstatus die de bestanden niet dragen.")
        return 0

    print("\n%d ruling(s) redeneren vanuit een poortstatus die niet klopt:" % len(bevindingen))
    for nr, missie, status, botsing in bevindingen:
        print("  RULINGS_L1.md:%d  %s  %s" % (nr, missie, botsing))
    print("\nEen ruling die op een onjuiste premisse staat, kan gelijk hebben om een andere")
    print("reden -- maar dat moet dan OPGESCHREVEN worden, niet aangenomen. Herbevestig de")
    print("ruling op een grond die wel klopt, of laat hem vallen.")
    return 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
