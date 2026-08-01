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

HIER = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HIER))
STATUS = os.path.join(REPO, "STATUS.md")
SOAK = os.path.join(REPO, "phase0", "SOAK_LOG.md")

# "| 2026-08-01 01:34 | `hash` | **ROOD** | 244 tests / 0 gefaald / ... |"
RE_SOAK = re.compile(r"^\|\s*(\d{4}-\d{2}-\d{2}[^|]*)\|[^|]*\|[^|]*\|\s*(\d+) tests", re.M)
RE_CLAIM = re.compile(r"\*\*(\d+) tests")

# "**71 bestanden, 1.623 regels, 97.275 credits**" -- de corpustelling waarop het
# budgetbesluit (O-14) rust.
#
# BEWUST OP DEZE HELE VORM GEANKERD en niet op los "N credits". STATUS.md bevat
# ook "~100.000 credits" (een schatting), "0 credits uitgegeven" en "104
# kandidaten, 0 credits" -- drie getallen die iets ANDERS meten. Een losse regex
# haalde ze alle drie binnen en meldde ze als verlopen. Dat is exact de fout die
# deze reeks tools bestrijdt: een instrument dat iets anders meet dan het belooft.
RE_CORPUS = re.compile(
    r"([\d][\d.,]*)\s*bestanden,\s*([\d][\d.,]*)\s*regels,\s*([\d][\d.,]*)\s*credits")


def _getal(s):
    return int(s.replace(".", "").replace(",", ""))


def _gemeten_corpus():
    """(bestanden, regels, credits) uit DEZELFDE teller die het rapport schrijft.

    Geen tweede implementatie: een controle die zijn eigen som maakt, kan het
    oneens worden met de tool die de owner leest, en dan meet hij niets meer.
    Geeft None als de teller niet te draaien is -- dat is OVERGESLAGEN, niet
    schoon.
    """
    try:
        if HIER not in sys.path:
            sys.path.insert(0, HIER)
        from count_generation_cost import tel
        per_voice, per_missie, _, stubs, bestanden, _ = tel()
        return (bestanden,
                sum(v["regels"] for v in per_voice.values()),
                sum(m["credits"] for m in per_missie.values()))
    except Exception:
        return None


def nieuwste_soak(tekst):
    """Het testaantal uit de LAATSTE regel van het logboek, of None."""
    rijen = RE_SOAK.findall(tekst)
    return int(rijen[-1][1]) if rijen else None


def controleer_credits(status_tekst, gemeten):
    """De tweede claim: de corpustelling waarop O-14 rust.

    Aparte functie met opzet -- hij mag de testcontrole niet kunnen laten vallen
    als de teller onbruikbaar is.
    """
    bevindingen, overgeslagen = [], []
    if gemeten is None:
        overgeslagen.append("count_generation_cost is niet te draaien -- "
                            "NIET GEDRAAID, en dat is iets anders dan schoon")
        return bevindingen, overgeslagen
    treffers = RE_CORPUS.findall(status_tekst)
    if not treffers:
        overgeslagen.append("STATUS.md draagt geen corpustelling van de vorm "
                            "'N bestanden, M regels, K credits' -- niets te toetsen")
        return bevindingen, overgeslagen
    namen = ("bestanden", "regels", "credits")
    for treffer in set(treffers):
        for i, (geclaimd, waar) in enumerate(zip(treffer, gemeten)):
            if _getal(geclaimd) != waar:
                bevindingen.append(
                    "STATUS.md claimt %s %s terwijl het corpus er nu %s telt. Dit is "
                    "de telling waarop O-14 rust: elke scenebewerking schuift hem, dus "
                    "een herhaald getal veroudert bij de eerste reparatie."
                    % (geclaimd, namen[i], format(waar, ",d").replace(",", ".")))
    return bevindingen, overgeslagen


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
    cbev, cover = controleer_credits(st, _gemeten_corpus())
    bev, over = bev + cbev, over + cover
    print("=" * 72)
    print("OWNER-DOCUMENTEN -- getallen tegen hun gezaghebbende bron")
    print("=" * 72)
    for b in bev:
        print("  VEROUDERD  " + b)
    for o in over:
        print("  OVERGESLAGEN: " + o)
    if not bev and not over:
        print("  Het testaantal en het creditbedrag in STATUS.md kloppen met hun bron.")
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

    # --- de tweede claim: de corpustelling ---------------------------------
    ECHT = (71, 1623, 97815)
    VERLOPEN = "**71 bestanden, 1.623 regels, 97.275 credits**"

    # 6. DE CONTROLEPROEF: precies wat op 01-08 in de repo stond moet rood worden.
    b, o = controleer_credits(VERLOPEN, ECHT)
    if len(b) != 1:
        fouten.append("een VERLOPEN corpustelling werd niet (of te vaak) gemeld -- "
                      "dat is de fout waarvoor deze tweede controle bestaat")
    # 7. een kloppende telling blijft stil
    b, o = controleer_credits("**71 bestanden, 1.623 regels, 97.815 credits**", ECHT)
    if b:
        fouten.append("CONTROLE: een KLOPPENDE corpustelling werd als verouderd gemeld")
    # 8. duizendtalscheiders zijn geen afwijking
    b, o = controleer_credits("**71 bestanden, 1,623 regels, 97,815 credits**", ECHT)
    if b:
        fouten.append("een duizendtalscheider werd voor een afwijking aangezien")
    # 9. ALLE DRIE de getallen worden getoetst, niet alleen het laatste.
    b, o = controleer_credits("**70 bestanden, 1.600 regels, 97.815 credits**", ECHT)
    if len(b) != 2:
        fouten.append("bestanden en regels worden niet meegetoetst (%d bevindingen)" % len(b))
    # 10. DE VALSE-POSITIEF-PROEF: losse creditgetallen die iets ANDERS meten
    #     mogen niet meetellen. Dit is de fout die deze controle zelf maakte.
    b, o = controleer_credits(
        "Act 1 kost ~100.000 credits. Afgeblazen met 0 credits uitgegeven. "
        "104 kandidaten over 19 rollen, 0 credits.", ECHT)
    if b or not o:
        fouten.append("losse creditgetallen die iets anders meten worden als "
                      "corpustelling gelezen -- de fout die dit gereedschap bestrijdt")
    # 11. geen teller = OVERSLAAN, niet groen liegen
    b, o = controleer_credits(VERLOPEN, None)
    if b or not o:
        fouten.append("zonder teller moet hij OVERSLAAN melden -- niet gedraaid "
                      "is niet hetzelfde als schoon")

    # 12. HET PAD DAT DE ECHTE RUN NEEMT. De controles hierboven spuiten hun
    #     meting in en raken _gemeten_corpus() nooit -- dus een hernoemde of
    #     kapotte meetfunctie liet de zelftest GROEN en de echte run crashen.
    #     Dat is precies gebeurd. Deze controle loopt daarom het echte pad af.
    gemeten = _gemeten_corpus()
    if gemeten is None:
        fouten.append("_gemeten_corpus() geeft niets terug -- de echte run kan "
                      "niet meten, ook al slagen de controles hierboven")
    elif len(gemeten) != 3 or not all(isinstance(x, int) and x > 0 for x in gemeten):
        fouten.append("_gemeten_corpus() gaf %r, geen drietal positieve getallen" % (gemeten,))

    for f in fouten:
        print("  ROOD  " + f)
    if not fouten:
        print("  Zelftest groen: 12 controles over TWEE claims, inclusief de "
              "negatieve dat een kloppend getal NIET gemeld wordt.")
    return 1 if fouten else 0


if __name__ == "__main__":
    sys.exit(zelftest() if "--zelftest" in sys.argv else rapport())
