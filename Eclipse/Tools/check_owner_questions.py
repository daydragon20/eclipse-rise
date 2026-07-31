#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""Controleert of een owner-vraag ook echt UITVOERBAAR is.

WAAROM DIT BESTAAT. Op 01-08 stonden er twee kaarten op het dashboard die Nathan
iets vroegen dat niet kon:

  O-3  "Kies een stem voor Petra Voss"  -- zij stond in geen van de 19
       castingrollen en er was geen enkel fragment om te beluisteren.
  O-4  "Beluister de ijkmissie M1.1"    -- er bestond geen seconde audio van M1.1,
       want alle zeven scenes stonden op critic: null.

Allebei door mij geschreven zonder te controleren of het ding aan de andere kant
bestond. Dat is duurder dan een rode test: de owner opent het dashboard, kan niets
doen, en de fout ligt bij hem tot hij het uitzoekt.

WAT ER GECONTROLEERD WORDT
  1. Structuur   -- verplichte velden; elke optie MOET een `waarde` hebben, want
                    zonder waarde is de knop op het dashboard dood (gebeurd op 31-07).
  2. Paden       -- elk repo-pad dat in de tekst genoemd wordt bestaat.
  3. audio_map   -- de map bestaat EN bevat audio. Een lege map is precies de
                    O-3-fout: de kaart belooft afspeelknopjes die er niet zijn.
  4. Dashboard   -- elke 127.0.0.1:8377-URL geeft 200. Draait het dashboard niet,
                    dan wordt dit als NIET GEDRAAID gemeld en niet als schoon.
  5. Dubbel      -- een vraag die al in OWNER_ANSWERS.md beantwoord is hoort weg.

Draaien:
    python Eclipse/Tools/check_owner_questions.py
    python Eclipse/Tools/check_owner_questions.py --zelftest
"""
from __future__ import print_function

import io
import json
import os
import re
import sys

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
VRAGEN = os.path.join(REPO, "phase0", "owner_questions.json")
ANTWOORDEN = os.path.join(REPO, "phase0", "OWNER_ANSWERS.md")
DASH = "http://127.0.0.1:8377"

VERPLICHT = ("id", "prio", "vraag", "waarom", "stappen", "opties")
AUDIO_EXT = (".mp3", ".wav", ".ogg", ".flac")

# Een pad in lopende tekst: begint met een bekende repo-map en loopt door tot
# een spatie of een leesteken dat geen padteken is.
RE_PAD = re.compile(r"\b((?:Eclipse|phase0|progress_media|Tools|archief)[/\\][\w./\\-]+)")
RE_URL = re.compile(r"http://127\.0\.0\.1:8377(/[\w./%#-]*)")


def _tekst(vraag):
    """Alle tekst van een kaart aan elkaar, zodat paden ook in meer_info tellen."""
    stukken = []
    for k in ("vraag", "waarom", "meer_info", "advies"):
        v = vraag.get(k)
        if isinstance(v, str):
            stukken.append(v)
    for s in vraag.get("stappen") or []:
        if isinstance(s, str):
            stukken.append(s)
    for o in vraag.get("opties") or []:
        for k in ("label", "gevolg"):
            if isinstance(o.get(k), str):
                stukken.append(o[k])
    return "\n".join(stukken)


def _dashboard_leeft():
    try:
        import urllib.request
        urllib.request.urlopen(DASH + "/progress_data.js", timeout=3).read(64)
        return True
    except Exception:
        return False


def _http_ok(pad):
    try:
        import urllib.request
        r = urllib.request.urlopen(DASH + pad, timeout=5)
        return 200 <= r.getcode() < 400
    except Exception as e:
        code = getattr(e, "code", None)
        return code is not None and 200 <= code < 400


def _beantwoorde_ids(tekst):
    """De vraag-ids uit OWNER_ANSWERS.md, uit de TWEEDE tabelkolom.

    HIER ZAT EEN FOUT IN, EN HET WAS PRECIES DE FOUT DIE DEZE TOOL BESTRIJDT.
    De eerste versie zocht de id aan het BEGIN van een regel. Het echte bestand
    is een markdown-tabel -- `| 2026-07-31 19:32 | O-7 | agent-kiest |` -- dus
    de id staat er nooit vooraan en de controle kon NOOIT vuren. Hij las nul
    beantwoorde vragen waar er tweeentwintig staan. De zelftest gaf hem groen
    omdat die een kopvorm (`## X-1`) voerde die in de repo niet voorkomt:
    een controleproef tegen invoer die niet bestaat.

    Kolom twee, en niet "ergens in de regel", want in de toelichtingskolom staat
    prose als "hoort bij O-5 volledig" -- dat zou elke vraag die door een ander
    antwoord genoemd wordt als beantwoord aanmerken.
    """
    ids = set()
    for regel in tekst.splitlines():
        r = regel.strip()
        if not r.startswith("|"):
            continue
        kolommen = [k.strip() for k in r.strip("|").split("|")]
        if len(kolommen) >= 2 and re.fullmatch(r"[A-Z]{1,5}-\d+", kolommen[1]):
            ids.add(kolommen[1])
    # De kopvorm (`## O-9 beantwoord`) blijft ook gelden: die kwam voor voordat
    # de tabel er was, en een oud bestand hoort niet stil door de controle te vallen.
    for m in re.finditer(r"^\s*#{1,6}\s*([A-Z]{1,5}-\d+)\b", tekst, re.M):
        ids.add(m.group(1))
    return ids


def controleer(pad_vragen=VRAGEN, pad_antwoorden=ANTWOORDEN, doe_http=None):
    bevindingen = []
    overgeslagen = []

    with io.open(pad_vragen, encoding="utf-8-sig") as fh:
        data = json.load(fh)
    vragen = data["vragen"] if isinstance(data, dict) else data

    beantwoord = ""
    if os.path.exists(pad_antwoorden):
        beantwoord = io.open(pad_antwoorden, encoding="utf-8-sig").read()

    if doe_http is None:
        doe_http = _dashboard_leeft()
    if not doe_http:
        overgeslagen.append("dashboard-URL's (server op 8377 antwoordt niet) "
                            "-- NIET GEDRAAID, en dat is iets anders dan schoon")

    for q in vragen:
        vid = q.get("id", "<zonder id>")

        for veld in VERPLICHT:
            if not q.get(veld):
                bevindingen.append((vid, "STRUCTUUR", "veld `%s` ontbreekt of is leeg" % veld))

        for i, o in enumerate(q.get("opties") or []):
            if not o.get("waarde"):
                bevindingen.append((vid, "STRUCTUUR",
                                    "optie %d (%r) heeft geen `waarde` -- die knop doet "
                                    "op het dashboard niets" % (i + 1, o.get("label"))))
            if not o.get("gevolg"):
                bevindingen.append((vid, "STRUCTUUR",
                                    "optie %d (%r) legt geen gevolg uit" % (i + 1, o.get("label"))))

        tekst = _tekst(q)

        for pad in set(RE_PAD.findall(tekst)):
            schoon = pad.replace("\\", "/").rstrip(".,;:)")
            if not os.path.exists(os.path.join(REPO, schoon)):
                bevindingen.append((vid, "PAD", "`%s` bestaat niet" % schoon))

        am = q.get("audio_map")
        if am:
            vol = os.path.join(REPO, am.replace("\\", "/"))
            if not os.path.isdir(vol):
                bevindingen.append((vid, "AUDIO", "audio_map `%s` bestaat niet" % am))
            else:
                n = sum(1 for _, _, fs in os.walk(vol)
                        for f in fs if f.lower().endswith(AUDIO_EXT))
                if n == 0:
                    bevindingen.append((vid, "AUDIO",
                                        "audio_map `%s` bevat GEEN audio -- de kaart belooft "
                                        "afspeelknopjes die er niet zijn (de O-3-fout)" % am))

        if doe_http:
            for pad in set(RE_URL.findall(tekst)):
                if not _http_ok(pad):
                    bevindingen.append((vid, "DASHBOARD",
                                        "de kaart stuurt naar %s%s en dat geeft geen 200"
                                        % (DASH, pad)))

        if vid in _beantwoorde_ids(beantwoord):
            bevindingen.append((vid, "DUBBEL",
                                "staat al beantwoord in OWNER_ANSWERS.md -- je vraagt hem "
                                "iets dat hij al besloten heeft"))

    return vragen, bevindingen, overgeslagen


def rapport():
    vragen, bevindingen, overgeslagen = controleer()
    print("=" * 72)
    print("OWNER-KAARTEN -- %d vragen gecontroleerd op uitvoerbaarheid" % len(vragen))
    print("=" * 72)
    if not bevindingen:
        print("  Alle kaarten wijzen naar iets dat bestaat.")
    for vid, soort, msg in bevindingen:
        print("  %-6s %-10s %s" % (vid, soort, msg))
    for o in overgeslagen:
        print("  OVERGESLAGEN: %s" % o)
    print()
    print("%d bevinding(en)" % len(bevindingen))
    return 1 if bevindingen else 0


def zelftest():
    """Bewijs dat elke controle rood KAN worden voor je zijn groen gelooft."""
    import tempfile
    fouten = []

    def draai(kaart, antwoorden=""):
        with tempfile.TemporaryDirectory() as tmp:
            pv = os.path.join(tmp, "q.json")
            io.open(pv, "w", encoding="utf-8").write(
                json.dumps({"vragen": [kaart]}, ensure_ascii=False))
            pa = os.path.join(tmp, "a.md")
            io.open(pa, "w", encoding="utf-8").write(antwoorden)
            return controleer(pv, pa, doe_http=False)[1]

    goed = {"id": "X-1", "prio": "nu", "vraag": "v", "waarom": "w",
            "stappen": ["s"], "opties": [{"label": "a", "waarde": "a", "gevolg": "g"}]}

    if draai(goed):
        fouten.append("CONTROLE: een correcte kaart geeft bevindingen")

    k = dict(goed, opties=[{"label": "a", "gevolg": "g"}])
    if not any(s == "STRUCTUUR" for _, s, _ in draai(k)):
        fouten.append("optie zonder `waarde` werd niet gemeld (dode knop, 31-07)")

    k = dict(goed, waarom="zie phase0/bestaat_echt_niet_12345.md")
    if not any(s == "PAD" for _, s, _ in draai(k)):
        fouten.append("niet-bestaand pad werd niet gemeld")

    k = dict(goed, waarom="zie phase0/VOICE_LEDGER.md")
    if any(s == "PAD" for _, s, _ in draai(k)):
        fouten.append("VALS ALARM: een pad dat WEL bestaat werd gemeld")

    k = dict(goed, audio_map="progress_media/casting/petra_bestaat_niet")
    if not any(s == "AUDIO" for _, s, _ in draai(k)):
        fouten.append("ontbrekende audio_map werd niet gemeld")

    import tempfile as tf
    with tf.TemporaryDirectory() as leeg:
        rel = os.path.relpath(leeg, REPO).replace("\\", "/")
        if not any(s == "AUDIO" for _, s, _ in draai(dict(goed, audio_map=rel))):
            fouten.append("LEGE audio_map werd niet gemeld -- dit IS de O-3-fout")

    # HET ECHTE FORMAAT EERST. De eerste versie van deze test voerde `## X-1` in,
    # een kopvorm die in OWNER_ANSWERS.md nergens voorkomt -- groen op invoer die
    # niet bestaat is geen bewijs, en de controle las daardoor nul beantwoorde
    # vragen waar er tweeentwintig staan.
    TABEL = ("| Wanneer | Vraag | Antwoord | Toelichting |\n|---|---|---|---|\n"
             "| 2026-07-31 19:32 | X-1 | agent-kiest |  |\n")
    if not any(s == "DUBBEL" for _, s, _ in draai(goed, antwoorden=TABEL)):
        fouten.append("een al beantwoorde vraag werd niet gemeld IN HET ECHTE TABELFORMAAT")

    if not any(s == "DUBBEL" for _, s, _ in draai(goed, antwoorden="## X-1 beantwoord\n")):
        fouten.append("de oude kopvorm wordt niet meer herkend")

    # Negatieve controle: een vraag die alleen in de TOELICHTING van een ander
    # antwoord genoemd wordt, is niet beantwoord.
    VALS = ("| Wanneer | Vraag | Antwoord | Toelichting |\n|---|---|---|---|\n"
            "| 2026-07-31 19:32 | Y-9 | ja | hoort bij X-1, zie daar |\n")
    if any(s == "DUBBEL" for _, s, _ in draai(goed, antwoorden=VALS)):
        fouten.append("VALS ALARM: X-1 gold als beantwoord omdat een ANDER "
                      "antwoord hem in zijn toelichting noemt")

    _, _, over = controleer(doe_http=False)
    if not over:
        fouten.append("STIL FALEN: overgeslagen HTTP-controle werd niet als overgeslagen gemeld")

    for f in fouten:
        print("  ROOD  " + f)
    if not fouten:
        # Het aantal wordt GETELD uit de bron en niet opgeschreven: een
        # hardgecodeerd getal dat niet meebeweegt als er een controle bij komt,
        # is precies zo'n kleine onwaarheid als deze tool moet vangen. Het stond
        # op 8 terwijl er inmiddels meer waren.
        n = len(re.findall(r"fouten\.append\(", io.open(__file__, encoding="utf-8").read()))
        print("  Zelftest groen: %d controles, elk bewijst dat de checker kan bewegen." % n)
    return 1 if fouten else 0


if __name__ == "__main__":
    sys.exit(zelftest() if "--zelftest" in sys.argv else rapport())
