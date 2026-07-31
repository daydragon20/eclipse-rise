#!/usr/bin/env python3
"""
fetch_casting_candidates.py — verse kandidaten uit de échte Voice Library.

Waarom dit bestaat
------------------
De eerste shortlist gebruikte 21 unieke stemmen voor 80 kandidaatplaatsen:
Alice kwam in zes rollen voor, Daniel in zes, Matilda in zes. Dat zijn de
~20 PREMADE standaardstemmen die elk account heeft. De gedeelde Voice
Library — duizenden stemmen — was nauwelijks aangesproken.

Owner: "de kandidaten per rol komen heel de tijd terug (...) de voice
database is wel echt meerdere duizenden stemmen dus je kunt wel verse
kandidaten vinden en dan kan je er ook meer geven."

Wat dit script wel en niet doet
-------------------------------
- Bladert `/v1/shared-voices`. Dat is een LEES-endpoint: **het kost geen
  credits.** Alleen genereren kost credits.
- **Raakt nooit een rol aan waar de owner al twee finalisten koos.** Zijn
  keuzes verwijzen naar kandidaatnummers; die hernummeren zou zijn werk
  weggooien.
- Geeft **meer** kandidaten per rol dan de eerste ronde, en nooit twee keer
  dezelfde stem binnen één rol.
- Gebruikt elke stem in **hoogstens twee** rollen, zodat de lijst niet weer
  dichtslibt met dezelfde namen.

Draaien:
    python Tools/fetch_casting_candidates.py            # toont wat het zou doen
    python Tools/fetch_casting_candidates.py --apply    # schrijft het weg
"""

from __future__ import annotations

import argparse
import configparser
import json
import os
import pathlib
import sys
import urllib.parse
import urllib.request

REPO = pathlib.Path(__file__).resolve().parent.parent
CASTING = REPO / "progress_media" / "casting" / "casting_stage1.json"
KEUZE = REPO / "phase0" / "CASTING_KEUZE.json"

PER_ROL = 8          # meer dan de ~4-5 van de eerste ronde
MAX_HERGEBRUIK = 2   # een stem mag in hoogstens twee rollen opduiken

# Wat elke rol nodig heeft. Leeftijd/geslacht komen uit 02_story_bible.md;
# de trefwoorden uit de fingerprints in 18_writing_standard.md §18.4.
ROLPROFIEL = {
    "reyes":    dict(gender="female", age="middle_aged", wil=["calm", "professional", "serious", "clear"], niet=["sassy", "upbeat"]),
    "torren":   dict(gender="male",   age="middle_aged", wil=["deep", "calm", "authoritative", "mature"], niet=["energetic", "upbeat"]),
    "kaya":     dict(gender="female", age="young",       wil=["energetic", "casual", "expressive", "upbeat"], niet=["calm", "serious"]),
    "whisper":  dict(gender="male",   age="middle_aged", wil=["soft", "calm", "mysterious", "smooth"], niet=["energetic", "rough"]),
    "sela":     dict(gender="female", age="young",       wil=["confident", "clear", "expressive", "warm"], niet=["raspy"]),
    "vex":      dict(gender="male",   age="old",         wil=["deep", "calm", "wise", "authoritative"], niet=["energetic", "young"]),
    "kaine":    dict(gender="female", age="middle_aged", wil=["authoritative", "confident", "serious", "crisp"], niet=["sassy", "upbeat"]),
    "threx":    dict(gender="male",   age="middle_aged", wil=["smooth", "warm", "expressive", "intense"], niet=["rough", "monotone"]),
    "callis":   dict(gender="male",   age="old",         wil=["professional", "smooth", "mature"], niet=["energetic", "rough"]),
    "aegis":    dict(gender="neutral", age="middle_aged", wil=["calm", "monotone", "neutral", "clear"], niet=["expressive", "emotional"]),
}

WAAROM = {
    "reyes":   "Klinisch en beheerst — de arts die onder druk juist preciezer wordt.",
    "torren":  "Kolonel op rust. Geeft bevelen als voorstellen; de langste stiltes van iedereen.",
    "kaya":    "Praat te snel, onderbreekt zichzelf, maakt een serieuze zin nooit af.",
    "whisper":  "Alleen een stem tot act 3. Zegt nooit 'ik', bevestigt nooit — impliceert.",
    "sela":    "Spreekt ook in een kamer alsof er een menigte staat.",
    "vex":     "De zachtste stem in de game, en de langste zinnen. Verheft zich nooit.",
    "kaine":   "Zegt precies wat ze gaat doen, en doet het. Geen eufemisme.",
    "threx":   "Warm en onverdraaglijk. Complimenteert je oprecht terwijl hij je pijn doet.",
    "callis":  "Lijdende vorm, bureaucratische omhaal, verbindt zich nooit in één zin.",
    "aegis":   "Geen zelfverwijzing, tegenwoordige tijd, overtuigt nooit — stelt alleen vast.",
}


def sleutel() -> str | None:
    k = os.environ.get("ELEVENLABS_API_KEY")
    if k:
        return k
    p = REPO / "Eclipse" / "Config" / "UserSecrets.ini"
    if p.is_file():
        c = configparser.ConfigParser()
        try:
            c.read(p, encoding="utf-8")
            for s in c.sections():
                for opt in c[s]:
                    if "eleven" in opt.lower() or "api" in opt.lower():
                        return c[s][opt]
        except Exception:
            pass
    return None


def haal_bibliotheek(key: str, paginas: int = 12) -> list[dict]:
    """Bladert de gedeelde bibliotheek. Leesverkeer — kost geen credits."""
    alles: list[dict] = []
    gezien: set[str] = set()
    for gender in ("male", "female"):
        page = 0
        while page < paginas:
            q = urllib.parse.urlencode({
                "page_size": 100, "page": page,
                "gender": gender, "language": "en",
            })
            req = urllib.request.Request(
                f"https://api.elevenlabs.io/v1/shared-voices?{q}",
                headers={"xi-api-key": key},
            )
            try:
                with urllib.request.urlopen(req, timeout=30) as r:
                    d = json.load(r)
            except Exception as exc:
                print(f"  pagina {page} ({gender}) overgeslagen: {exc}", file=sys.stderr)
                break
            vs = d.get("voices", [])
            for v in vs:
                vid = v.get("voice_id")
                if vid and vid not in gezien:
                    gezien.add(vid)
                    alles.append(v)
            if not d.get("has_more"):
                break
            page += 1
    return alles


def score(v: dict, prof: dict) -> int:
    """Hoe goed past deze stem bij de rol? Hoger is beter, negatief = weg."""
    s = 0
    g = (v.get("gender") or "").lower()
    if prof["gender"] == "neutral":
        s += 1
    elif g == prof["gender"]:
        s += 6
    else:
        return -1                       # verkeerd geslacht: niet bruikbaar

    leeftijd = (v.get("age") or "").lower().replace("-", "_")
    if leeftijd == prof["age"]:
        s += 5
    elif prof["age"] == "old" and leeftijd == "middle_aged":
        s += 2
    elif prof["age"] == "middle_aged" and leeftijd in ("old", "young"):
        s += 2

    blob = " ".join(str(v.get(k, "")) for k in
                    ("descriptive", "use_case", "description", "name")).lower()
    for w in prof["wil"]:
        if w in blob:
            s += 3
    for w in prof["niet"]:
        if w in blob:
            s -= 4

    # accent-neutraal heeft de voorkeur (§19.3)
    acc = (v.get("accent") or "").lower()
    if acc in ("american", "transatlantic", "neutral"):
        s += 2
    elif acc in ("british", "australian"):
        s += 1

    if (v.get("category") or "") == "professional":
        s += 3
    return s


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--apply", action="store_true", help="wegschrijven (anders alleen tonen)")
    ap.add_argument("--per-rol", type=int, default=PER_ROL)
    args = ap.parse_args()

    key = sleutel()
    if not key:
        print("Geen API-sleutel gevonden.", file=sys.stderr)
        return 2

    data = json.loads(CASTING.read_text(encoding="utf-8"))
    keuze = json.loads(KEUZE.read_text(encoding="utf-8")) if KEUZE.is_file() else {}

    bevroren = {r for r, v in keuze.items() if len(v) >= 2}
    teverversen = [r for r in data["rollen"]
                   if r["rol"] not in bevroren and r["rol"] in ROLPROFIEL]

    print(f"Rollen bevroren (owner koos al twee): {len(bevroren)} — {', '.join(sorted(bevroren))}")
    print(f"Rollen te verversen: {len(teverversen)} — {', '.join(r['rol'] for r in teverversen)}")
    if not teverversen:
        print("Niets te doen.")
        return 0

    # stemmen die al ergens gebruikt worden, blijven buiten de nieuwe lijsten
    gebruikt: set[str] = set()
    for r in data["rollen"]:
        if r["rol"] in bevroren:
            for k in r["kandidaten"]:
                gebruikt.add(k.get("voice_id", ""))

    print("\nBibliotheek ophalen (leesverkeer, kost geen credits)…")
    pool = haal_bibliotheek(key)
    print(f"  {len(pool)} unieke stemmen opgehaald")

    hergebruik: dict[str, int] = {}
    for r in teverversen:
        prof = ROLPROFIEL[r["rol"]]
        kand = []
        for v in sorted(pool, key=lambda x: -score(x, prof)):
            vid = v.get("voice_id", "")
            if score(v, prof) < 4 or vid in gebruikt:
                continue
            if hergebruik.get(vid, 0) >= MAX_HERGEBRUIK:
                continue
            eig = " / ".join(x for x in (
                v.get("gender"), v.get("age"), v.get("accent"), v.get("descriptive")) if x)
            kand.append({
                "nr": len(kand) + 1,
                "stem": v.get("name", "?"),
                "voice_id": vid,
                "eigenschappen": eig,
                "waarom": WAAROM.get(r["rol"], ""),
                "beschrijving": (v.get("description") or "")[:220],
                "preview_url": v.get("preview_url", ""),
                "categorie": v.get("category", "library"),
                "bron": "shared-library",
            })
            hergebruik[vid] = hergebruik.get(vid, 0) + 1
            if len(kand) >= args.per_rol:
                break
        print(f"  {r['rol'].ljust(20)} {len(kand)} verse kandidaten"
              f"  ({', '.join(k['stem'] for k in kand[:4])}…)")
        if args.apply and kand:
            r["kandidaten"] = kand

    if not args.apply:
        print("\nPROEFDRAAI — er is niets weggeschreven. Draai met --apply.")
        return 0

    data["stage"] = "stage1-ververst"
    CASTING.write_text(json.dumps(data, ensure_ascii=False, indent=1), encoding="utf-8")
    print(f"\nWeggeschreven naar {CASTING.relative_to(REPO)}")
    print("De bevroren rollen zijn ongemoeid gebleven — de owner-keuzes kloppen nog.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
