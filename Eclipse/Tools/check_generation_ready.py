#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Splitst het corpus in wat NU gegenereerd kan worden en wat niet, met de reden.

WAAROM DIT BESTAAT
------------------
`count_generation_cost.py` telt het hele corpus. Dat is het juiste antwoord op
"wat kost act 1", en het is 97.275 credits. Maar het wordt gelezen als "wat sta
ik op het punt uit te geven", en dat is een ander getal -- want een deel van die
regels staat achter een dichte poort of achter een ongecaste stem.

Dat is precies het patroon dat dit project al vaker geld had kunnen kosten: een
instrument dat iets anders meet dan waarvoor het gelezen wordt. Deze tool bouwt
de teller NIET na -- hij importeert `scan_file` uit dezelfde module, want twee
tellers die het oneens kunnen zijn is de fout die hij moet voorkomen.

DRIE BAKKEN
-----------
  KLAAR    critic staat op GO, en elke stem in de scene is gecast.
  POORT    critic is null, NO-GO, of GEHOUDEN (bv. de hub-reeks).
  CASTING  poort is groen maar minstens een stem heeft geen stem-id.

Een scene die in twee bakken hoort, valt in POORT -- de duurste eerst.

WAT HIJ NIET MEET
-----------------
Hij leest geen escalaties en geen openstaande rulings. Een scene die hier KLAAR
heet, kan nog steeds op een architect-beslissing wachten die alleen in proza
bestaat. Hij zegt: de poort is groen en de stemmen bestaan. Niet meer.

Draai:  python Eclipse/Tools/check_generation_ready.py
        python Eclipse/Tools/check_generation_ready.py --zelftest
"""
import collections
import glob
import io
import json
import os
import re
import sys

HIER = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HIER)
from count_generation_cost import scan_file, MULTIPLIERS  # noqa: E402  -- EEN teller, niet twee

WORTEL = os.path.dirname(HIER)
REPO = os.path.dirname(WORTEL)
CORPUS = os.path.join(WORTEL, "Content", "Script")
VOICEMAP = os.path.join(WORTEL, "Content", "Audio", "VoiceKeyMap.json")
CASTING = os.path.join(REPO, "phase0", "CASTING_RESOLVED.json")

RE_CRITIC = re.compile(r"(?m)^critic:\s*(.+?)\s*$")


def _lees(pad):
    with io.open(pad, encoding="utf-8-sig") as fh:
        return fh.read()


def poortstatus(tekst):
    """('GO'|'NO-GO'|'GEHOUDEN'|'ONGEPOORT', ruwe waarde)."""
    m = RE_CRITIC.search(tekst)
    if not m:
        return "ONGEPOORT", "(geen critic-veld)"
    ruw = m.group(1)
    kaal = ruw.strip('"').strip()
    if kaal == "null":
        return "ONGEPOORT", "critic: null"
    if kaal.upper().startswith("NO-GO"):
        return "NO-GO", kaal
    # "GO op 18.9 (scene) -- GEHOUDEN op de reeks" telt NIET als groen.
    if "GEHOUDEN" in kaal.upper() or "REEKS NO-GO" in kaal.upper():
        return "GEHOUDEN", kaal
    if kaal.upper().startswith("GO"):
        return "GO", kaal
    return "ONGEPOORT", kaal


def gecaste_stemmen(voicemap_pad=VOICEMAP, casting_pad=CASTING):
    """De set script-`voice:`-sleutels waarvoor een echte stem-id bestaat.

    Geeft ook terug welke sleutels bekend zijn maar ongecast, zodat een lege
    uitkomst te onderscheiden is van een ontbrekend bestand.
    """
    vm = json.loads(_lees(voicemap_pad))
    kaart = vm.get("map", {})

    rol_naar_ids = {}
    if os.path.exists(casting_pad):
        cast = json.loads(_lees(casting_pad))
        for rol, blok in (cast.get("rollen") or {}).items():
            ids = set()
            if isinstance(blok, dict):
                for slot, waarde in blok.items():
                    if isinstance(waarde, str) and waarde:
                        ids.add(slot)
                    elif isinstance(waarde, dict) and waarde.get("voice_id"):
                        ids.add(slot)
            rol_naar_ids[rol] = ids

    gecast, ongecast = set(), set()
    for sleutel, blok in kaart.items():
        rollen = blok.get("variants") or ([blok["role"]] if "role" in blok else [])
        if rollen and all(rol_naar_ids.get(r) for r in rollen):
            gecast.add(sleutel)
        else:
            ongecast.add(sleutel)
    return gecast, ongecast, os.path.exists(casting_pad)


def analyseer(corpus=CORPUS, voicemap_pad=VOICEMAP, casting_pad=CASTING):
    gecast, ongecast, casting_bestaat = gecaste_stemmen(voicemap_pad, casting_pad)
    bakken = collections.defaultdict(list)
    onbegrepen_totaal = []

    for pad in sorted(glob.glob(os.path.join(corpus, "**", "*.yaml"), recursive=True)):
        tekst = _lees(pad)
        status, reden = poortstatus(tekst)
        regels, onbegrepen = scan_file(pad)
        onbegrepen_totaal.extend(onbegrepen)
        credits = sum(len(t) * MULTIPLIERS.get(v, 1) for v, t, _ in regels)
        stemmen = set(v for v, _, _ in regels if v)
        mist = sorted(s for s in stemmen if s not in gecast)

        rel = os.path.relpath(pad, corpus).replace("\\", "/")
        if status != "GO":
            bakken["POORT"].append((rel, credits, status, reden))
        elif mist:
            bakken["CASTING"].append((rel, credits, "ongecast", ", ".join(mist)))
        else:
            bakken["KLAAR"].append((rel, credits, "GO", ""))

    return bakken, onbegrepen_totaal, casting_bestaat, sorted(ongecast)


# ---------------------------------------------------------------- zelftest ---

def _zelftest():
    import shutil
    import tempfile
    tijdelijk = tempfile.mkdtemp(prefix="genready_")
    gezakt = []
    try:
        corpus = os.path.join(tijdelijk, "act1", "M1.X")
        os.makedirs(corpus)

        def scene(naam, critic, voice, tekst):
            with io.open(os.path.join(corpus, naam), "w", encoding="utf-8") as f:
                f.write("critic: %s\nlines:\n  - id:      M1.X.S01.010\n"
                        "    voice:   %s\n    text:    \"%s\"\n" % (critic, voice, tekst))

        vm = os.path.join(tijdelijk, "vm.json")
        with io.open(vm, "w", encoding="utf-8") as f:
            json.dump({"map": {"mara": {"role": "mara"}, "brick": {"role": "brick"}}}, f)
        cast = os.path.join(tijdelijk, "cast.json")
        with io.open(cast, "w", encoding="utf-8") as f:
            json.dump({"rollen": {"mara": {"A": "id-1"}}}, f)      # brick BEWUST niet gecast

        wortel = os.path.join(tijdelijk, "act1")

        # 1. Groene poort + gecaste stem = KLAAR.
        scene("a.yaml", "GO", "mara", "twaalf tekens")
        b, o, ce, on = analyseer(wortel, vm, cast)
        if len(b["KLAAR"]) != 1:
            gezakt.append("1: een generatieklare scene wordt niet als KLAAR geteld")

        # 2. DE CONTROLEPROEF: een NO-GO mag NOOIT in KLAAR belanden.
        scene("b.yaml", '"NO-GO: iets"', "mara", "twaalf tekens")
        b, o, ce, on = analyseer(wortel, vm, cast)
        if any(r[0].endswith("b.yaml") for r in b["KLAAR"]):
            gezakt.append("2: een NO-GO-scene komt in de KLAAR-bak")

        # 3. `critic: null` telt niet als groen.
        scene("c.yaml", "null", "mara", "twaalf tekens")
        b, o, ce, on = analyseer(wortel, vm, cast)
        if any(r[0].endswith("c.yaml") for r in b["KLAAR"]):
            gezakt.append("3: een ongepoorte scene komt in de KLAAR-bak")

        # 4. GEHOUDEN (de hubvorm) is geen GO -- dit is de subtielste, want de
        #    waarde BEGINT met "GO".
        scene("d.yaml", '"GO op 18.9 (scene) -- GEHOUDEN op de reeks"', "mara", "twaalf tekens")
        b, o, ce, on = analyseer(wortel, vm, cast)
        if any(r[0].endswith("d.yaml") for r in b["KLAAR"]):
            gezakt.append("4: een GEHOUDEN scene wordt als GO gelezen")

        # 5. Groene poort maar ongecaste stem = CASTING, niet KLAAR.
        scene("e.yaml", "GO", "brick", "twaalf tekens")
        b, o, ce, on = analyseer(wortel, vm, cast)
        if any(r[0].endswith("e.yaml") for r in b["KLAAR"]):
            gezakt.append("5: een scene met ongecaste stem komt in de KLAAR-bak")
        if not any(r[0].endswith("e.yaml") for r in b["CASTING"]):
            gezakt.append("5b: die scene belandt ook niet in de CASTING-bak")

        # 6. Poort gaat VOOR casting: een NO-GO met ongecaste stem hoort in POORT.
        scene("f.yaml", '"NO-GO: iets"', "brick", "twaalf tekens")
        b, o, ce, on = analyseer(wortel, vm, cast)
        if not any(r[0].endswith("f.yaml") for r in b["POORT"]):
            gezakt.append("6: dubbel geblokkeerde scene valt niet in de duurste bak")

        # 7. De credits komen uit DEZELFDE teller -- geen tweede implementatie.
        scene("g.yaml", "GO", "mara", "abcde")
        b, o, ce, on = analyseer(wortel, vm, cast)
        g = [r for r in b["KLAAR"] if r[0].endswith("g.yaml")]
        if not g or g[0][1] != 5:
            gezakt.append("7: credits wijken af van len(tekst) (%s)" % (g,))

        print("ZELFTEST check_generation_ready -- 7 controles")
        for r in gezakt:
            print("  GEZAKT  %s" % r)
        if gezakt:
            print("\n%d van 7 gezakt." % len(gezakt))
            return 1
        print("  alle 7 geslaagd -- geen ongepoorte of ongecaste scene kan in de KLAAR-bak komen")
        return 0
    finally:
        shutil.rmtree(tijdelijk, ignore_errors=True)


def main(argv):
    if "--zelftest" in argv:
        return _zelftest()

    bakken, onbegrepen, casting_bestaat, ongecast = analyseer()

    tot = {k: sum(r[1] for r in v) for k, v in bakken.items()}
    n = {k: len(v) for k, v in bakken.items()}
    alles = sum(tot.values())

    print("check_generation_ready -- %d scenebestanden, %s credits als ALLES zou verschepen."
          % (sum(n.values()), format(alles, ",d").replace(",", ".")))
    print("Dat laatste getal is wat count_generation_cost telt. Hieronder staat wat daarvan")
    print("vandaag daadwerkelijk uitgegeven kan worden.\n")

    for bak, kop in (("KLAAR", "KLAAR om te genereren"),
                     ("POORT", "GEHOUDEN door de kwaliteitspoort"),
                     ("CASTING", "GEHOUDEN door casting")):
        c = format(tot.get(bak, 0), ",d").replace(",", ".")
        print("  %-28s %3d scene(s)  %10s credits" % (kop, n.get(bak, 0), c))

    if not casting_bestaat:
        print("\nLET OP: phase0/CASTING_RESOLVED.json bestaat niet, dus GEEN enkele stem geldt")
        print("als gecast. Dat is geen meting van de scripts maar van de casting.")

    # ALTIJD tonen wat er is uitgesloten en waarom -- een stille uitsluiting leest
    # als "alles gedekt".
    if bakken.get("CASTING"):
        print("\nDe casting-bak, met de stem die ontbreekt:")
        for rel, credits, _, mist in sorted(bakken["CASTING"], key=lambda r: -r[1])[:12]:
            print("  %-52s %7s cr   mist: %s"
                  % (rel, format(credits, ",d").replace(",", "."), mist))
        rest = len(bakken["CASTING"]) - 12
        if rest > 0:
            print("  ... en nog %d scene(s)" % rest)

    if ongecast:
        print("\n%d stemsleutel(s) in VoiceKeyMap zonder gecaste stem: %s"
              % (len(ongecast), ", ".join(ongecast)))

    if onbegrepen:
        print("\n%d regel(s) die de teller niet kon lezen -- GEMELD, niet stil overgeslagen:"
              % len(onbegrepen))
        for pad, lineno, fragment in onbegrepen[:5]:
            print("  %s:%d  %s" % (os.path.basename(pad), lineno, fragment))

    print("\nDeze controle leest GEEN escalaties en GEEN openstaande rulings. Een scene die")
    print("hier KLAAR heet, kan nog op een architect-beslissing wachten die alleen in proza")
    print("bestaat. Hij zegt: de poort is groen en de stemmen bestaan. Niet meer.")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
