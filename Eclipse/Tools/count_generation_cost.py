#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""Tel wat het kost om het script in te laten spreken.

Dit is de invoer voor Q-7 / owner-vraag O-14. Het getal in VOICE_LEDGER.md is op
31-07 met de hand geteld; deze tool maakt die telling herhaalbaar, omdat er nog
scenes bij komen en een handgeteld bedrag stilletjes veroudert.

WAT ER GETELD WORDT (ruling L1-R15, SCRIPT_FORMAT §4):
  words_generated = elke `text:` PLUS elke variant-tekst, want TTS moet ze
  allemaal produceren. Niet `words` (dat is werklast) en niet `words_heard`
  (dat is een speelbeurt). ElevenLabs rekent per TEKEN, dus tekens == credits.

  De sleutel `voss` is logisch en lost op naar voss_m EN voss_f, dus alles wat
  hij zegt telt DUBBEL.

WAAROM DEZE PARSER GEEN YAML-BIBLIOTHEEK GEBRUIKT: PyYAML staat niet op deze
machine en installeren is een owner-beslissing. De parser is daarom regelgebaseerd
en MELDT ELKE REGEL DIE HIJ NIET SNAPT in plaats van hem over te slaan -- een
teller die stil nul teruggeeft is precies de fout die deze avond zes keer is
opgeruimd.

Draaien:
    python Eclipse/Tools/count_generation_cost.py
    python Eclipse/Tools/count_generation_cost.py --zelftest
"""
from __future__ import print_function

import glob
import io
import os
import re
import sys
import collections

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
CORPUS = os.path.join(REPO, "Eclipse", "Content", "Script")

# Sleutels die naar meer dan een stem oplossen. SCRIPT_FORMAT §4 regel 196.
MULTIPLIERS = {"voss": 2}

RE_VOICE = re.compile(r"^\s{4}voice:\s*(\S+)\s*$")
RE_TEXT = re.compile(r'^\s{4}text:\s*(.*)$')
RE_VARIANTS = re.compile(r"^\s{4}variants:\s*$")
RE_VARIANT_LINE = re.compile(r'^\s{6}([a-z_0-9]+):\s*(.*)$')
RE_ID = re.compile(r"^\s{2}-\s+id:\s*(\S+)\s*$")


def _unquote(raw):
    """Haal de YAML-quotes eraf. Geeft None als de vorm niet herkend wordt."""
    raw = raw.strip()
    if not raw:
        return None
    if raw[0] == '"' and raw.endswith('"') and len(raw) >= 2:
        return raw[1:-1].replace('\\"', '"').replace("\\\\", "\\")
    if raw[0] == "'" and raw.endswith("'") and len(raw) >= 2:
        return raw[1:-1].replace("''", "'")
    if raw[0] in "|>":
        return None  # blok-scalair: niet ondersteund, wordt gemeld
    return raw


def scan_file(path):
    """Geef (regels, onbegrepen) terug. regels = [(voice, tekst, is_variant)]."""
    out = []
    onbegrepen = []
    voice = None
    line_id = None
    in_variants = False
    with io.open(path, encoding="utf-8-sig") as fh:
        for lineno, line in enumerate(fh.readlines(), 1):
            line = line.rstrip("\n")

            m = RE_ID.match(line)
            if m:
                line_id, voice, in_variants = m.group(1), None, False
                continue

            m = RE_VOICE.match(line)
            if m:
                voice = m.group(1)
                in_variants = False
                continue

            if RE_VARIANTS.match(line):
                in_variants = True
                continue

            m = RE_TEXT.match(line)
            if m:
                in_variants = False
                txt = _unquote(m.group(1))
                if txt is None:
                    onbegrepen.append((path, lineno, line.strip()[:70]))
                else:
                    out.append((voice, txt, False))
                continue

            if in_variants:
                m = RE_VARIANT_LINE.match(line)
                if m:
                    txt = _unquote(m.group(2))
                    if txt is None:
                        onbegrepen.append((path, lineno, line.strip()[:70]))
                    else:
                        out.append((voice, txt, True))
                    continue
                if line.strip() and not line.strip().startswith("#"):
                    in_variants = False
    return out, onbegrepen


def tel(root=CORPUS):
    per_voice = collections.defaultdict(lambda: {"regels": 0, "varianten": 0, "tekens": 0})
    per_missie = collections.defaultdict(lambda: {"regels": 0, "tekens": 0, "credits": 0})
    onbegrepen = []
    stubs = []
    bestanden = 0
    naamloos = 0

    for path in sorted(glob.glob(os.path.join(root, "**", "*.yaml"), recursive=True)):
        bestanden += 1
        rel = os.path.relpath(path, root).replace("\\", "/")
        missie = rel.split("/")[1] if "/" in rel and rel.count("/") > 1 else rel.split("/")[0]
        regels, slecht = scan_file(path)
        onbegrepen.extend(slecht)
        if not regels:
            stubs.append(rel)
            continue
        for voice, txt, is_var in regels:
            if voice is None:
                naamloos += 1
                voice = "(GEEN voice-veld)"
            mult = MULTIPLIERS.get(voice, 1)
            n = len(txt)
            v = per_voice[voice]
            v["regels"] += 1
            v["varianten"] += 1 if is_var else 0
            v["tekens"] += n
            m = per_missie[missie]
            m["regels"] += 1
            m["tekens"] += n
            m["credits"] += n * mult

    return per_voice, per_missie, onbegrepen, stubs, bestanden, naamloos


def rapport():
    per_voice, per_missie, onbegrepen, stubs, bestanden, naamloos = tel()

    totaal = sum(
        v["tekens"] * MULTIPLIERS.get(k, 1) for k, v in per_voice.items()
    )

    print("=" * 74)
    print("GENERATIEKOSTEN — words_generated per stem (tekens == credits)")
    print("=" * 74)
    print("%-26s %7s %9s %9s %10s" % ("stem", "regels", "waarvan", "tekens", "credits"))
    print("%-26s %7s %9s %9s %10s" % ("", "", "variant", "", ""))
    print("-" * 74)
    for k in sorted(per_voice, key=lambda x: -per_voice[x]["tekens"] * MULTIPLIERS.get(x, 1)):
        v = per_voice[k]
        mult = MULTIPLIERS.get(k, 1)
        vlag = "  x%d" % mult if mult > 1 else ""
        print("%-26s %7d %9d %9d %10d%s" % (
            k, v["regels"], v["varianten"], v["tekens"], v["tekens"] * mult, vlag))
    print("-" * 74)
    print("%-26s %7s %9s %9s %10d" % ("TOTAAL", "", "", "", totaal))
    print()

    print("PER MISSIE")
    print("-" * 74)
    for k in sorted(per_missie):
        m = per_missie[k]
        print("  %-34s %5d regels  %8d credits" % (k, m["regels"], m["credits"]))
    print()

    print("DEKKING — waar dit getal NIET over gaat")
    print("-" * 74)
    print("  %d scriptbestanden gescand, %d zonder ook maar een tekstregel:" % (bestanden, len(stubs)))
    for s in stubs:
        print("      stub: %s" % s)
    if naamloos:
        print("  !! %d tekstregels zonder voice-veld — die kunnen NIET gegenereerd worden" % naamloos)
    if onbegrepen:
        print("  !! %d regels die deze parser niet las (NIET meegeteld):" % len(onbegrepen))
        for p, ln, txt in onbegrepen[:15]:
            print("      %s:%d  %s" % (os.path.relpath(p, REPO), ln, txt))
    else:
        print("  Geen onbegrepen regels — elke text: en variant is gelezen.")
    return totaal, onbegrepen, naamloos


def zelftest():
    """Bewijs dat de teller kan bewegen VOOR je zijn uitkomst gelooft.

    Een teller die altijd hetzelfde getal geeft is geen meting. Deze test voert
    bekende invoer in en controleert dat elk onderdeel het bedrag verandert.
    """
    import tempfile
    fouten = []

    def maak(tmp, body):
        p = os.path.join(tmp, "act1", "MX_test")
        os.makedirs(p)
        with io.open(os.path.join(p, "MX.S01_t.yaml"), "w", encoding="utf-8") as fh:
            fh.write(body)
        return tmp

    basis = (
        "scene: MX.S01_t\nlines:\n"
        '  - id:      MX.S01.010\n    speaker: MARA\n    voice:   mara\n'
        '    text:    "abcde"\n'
    )
    with tempfile.TemporaryDirectory() as tmp:
        maak(tmp, basis)
        pv, _, bad, _, _, _ = tel(tmp)
        if pv["mara"]["tekens"] != 5:
            fouten.append("basis: 5 tekens verwacht, kreeg %d" % pv["mara"]["tekens"])
        if bad:
            fouten.append("basis: parser struikelde")

    # 1. varianten MOETEN meetellen
    met_var = basis + (
        '  - id:      MX.S01.020\n    speaker: VOSS\n    voice:   voss\n'
        '    text:    "abcd"\n    variants:\n'
        '      pragmatist: "xy"\n      idealist:   "xyz"\n'
    )
    with tempfile.TemporaryDirectory() as tmp:
        maak(tmp, met_var)
        pv, _, _, _, _, _ = tel(tmp)
        if pv["voss"]["tekens"] != 4 + 2 + 3:
            fouten.append("varianten niet meegeteld: kreeg %d, verwacht 9" % pv["voss"]["tekens"])
        if pv["voss"]["varianten"] != 2:
            fouten.append("variantteller klopt niet: %d" % pv["voss"]["varianten"])

    # 2. de voss-verdubbeling MOET het bedrag veranderen
    with tempfile.TemporaryDirectory() as tmp:
        maak(tmp, met_var)
        pv, pm, _, _, _, _ = tel(tmp)
        verwacht = 5 * 1 + 9 * 2
        gekregen = sum(m["credits"] for m in pm.values())
        if gekregen != verwacht:
            fouten.append("voss-verdubbeling: %d credits, verwacht %d" % (gekregen, verwacht))

    # 3. een vorm die de parser NIET kent moet MELDEN, niet stil 0 tellen
    blok = basis + (
        '  - id:      MX.S01.030\n    speaker: DEX\n    voice:   dex\n'
        "    text:    >\n      een blok-scalair\n"
    )
    with tempfile.TemporaryDirectory() as tmp:
        maak(tmp, blok)
        _, _, bad, _, _, _ = tel(tmp)
        if not bad:
            fouten.append("STIL FALEN: blok-scalair werd niet gemeld")

    # 4. een regel zonder voice-veld moet als niet-genereerbaar opvallen
    geenstem = "scene: MX\nlines:\n  - id:      MX.S01.010\n    speaker: X\n    text:    \"abc\"\n"
    with tempfile.TemporaryDirectory() as tmp:
        maak(tmp, geenstem)
        _, _, _, _, _, naamloos = tel(tmp)
        if naamloos != 1:
            fouten.append("regel zonder voice werd niet gesignaleerd")

    # 5. een leeg bestand is een STUB en geen kosten van nul die als feit leest
    with tempfile.TemporaryDirectory() as tmp:
        maak(tmp, "scene: MX\nlines: []\n")
        _, _, _, stubs, _, _ = tel(tmp)
        if len(stubs) != 1:
            fouten.append("lege scene niet als stub gemeld")

    for f in fouten:
        print("  ROOD  " + f)
    if not fouten:
        print("  Zelftest groen: 5 controles, elk bewijst dat de teller kan bewegen.")
    return 0 if not fouten else 1


if __name__ == "__main__":
    if "--zelftest" in sys.argv:
        sys.exit(zelftest())
    totaal, onbegrepen, naamloos = rapport()
    sys.exit(1 if (onbegrepen or naamloos) else 0)
