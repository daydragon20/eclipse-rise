#!/usr/bin/env python3
"""
dump_asset_inventaris.py — welke assets bestaan er, en wat doet ermee.

Waarom dit bestaat
------------------
Owner 01-08: *"bekijk al de gedownloade assets en zorg dat VS Claude weet welke
allemaal bestaan, want er zijn wel veel goede. Toon dat in een nieuwe tab in het
dashboard: alle assets die ik van Fab heb gehaald, welke VS Claude heeft of aan
het gebruiken is, en wat hij zei over al de andere."*

Er staat ~18 GB aan packs op schijf en niemand had overzicht. Een agent die niet
weet dat `Sci_fi_hallway` bestaat, bouwt een gang met de hand.

Wat het meet
------------
- **op schijf**: elke top-level map in `Eclipse/Content` met `.uasset`-bestanden
- **in gebruik**: wordt `/Game/<pack>` genoemd in Source, Tools of Config? Dat is
  een ONDERGRENS, geen waarheid: een pack dat alleen vanuit een level of een
  DataTable verwezen wordt, telt hier als ongebruikt. Daarom heet de kolom
  "genoemd in code" en niet "gebruikt".
- **oordeel**: wat agents er eerder over schreven, uit de curatiedocumenten.

Schrijft twee dingen:
    phase0/ASSET_INVENTARIS.md     leesbaar, voor agents en de owner
    phase0/asset_inventaris.json   voor het dashboard

Draaien:  python Tools/dump_asset_inventaris.py
"""

from __future__ import annotations

import json
import pathlib
import re
import subprocess
from datetime import datetime

REPO = pathlib.Path(__file__).resolve().parent.parent
CONTENT = REPO / "Eclipse" / "Content"
UIT_MD = REPO / "phase0" / "ASSET_INVENTARIS.md"
UIT_JSON = REPO / "phase0" / "asset_inventaris.json"

# Mappen die geen asset-pack zijn maar eigen werk of data
EIGEN = {
    "Script", "Audio", "Data", "Maps", "Blueprints", "UI", "Developers",
    "__ExternalActors__", "__ExternalObjects__", "Art", "CharacterAssemblies",
    "Cinematics", "Levels",
}

# Waar agents hun oordelen opschreven
OORDEEL_BRONNEN = [
    "phase0/ASSET_CURATION.md",
    "phase0/ASSET_CLEANUP.md",
    "phase0/CURATIE_ENVPACKS_2026-07-25.md",
    "phase0/KITPASS_P2-08.md",
    "ASSETS_IN_WACHT.md",
    "Eclipse/Saved/RejectedAssets/WAAROM_AFGEWEZEN.md",
    "Eclipse/Content/Art/Imported/SOURCES.md",
    "Eclipse/Content/Art/Characters/SOURCES.md",
    "Eclipse/Content/Art/Textures/SOURCES.md",
]


def mb(pad: pathlib.Path) -> int:
    tot = 0
    for f in pad.rglob("*"):
        try:
            if f.is_file():
                tot += f.stat().st_size
        except OSError:
            pass
    return round(tot / 1048576)


def genoemd_in_code(pack: str) -> list[str]:
    """Waar wordt /Game/<pack> genoemd? Ondergrens, geen waarheid."""
    try:
        r = subprocess.run(
            ["grep", "-ril", f"/Game/{pack}",
             "Eclipse/Source", "Eclipse/Tools", "Tools", "Eclipse/Config"],
            cwd=REPO, capture_output=True, text=True, timeout=60,
            encoding="utf-8", errors="replace",
        )
        return [l.strip().replace("\\", "/") for l in r.stdout.splitlines() if l.strip()]
    except Exception:
        return []


def oordelen() -> dict[str, list[str]]:
    """Zinnen uit de curatiedocumenten, gegroepeerd op pack-naam."""
    uit: dict[str, list[str]] = {}
    packs = [d.name for d in CONTENT.iterdir() if d.is_dir()] if CONTENT.is_dir() else []
    for rel in OORDEEL_BRONNEN:
        p = REPO / rel
        if not p.is_file():
            continue
        try:
            tekst = p.read_text(encoding="utf-8", errors="replace")
        except OSError:
            continue
        for regel in tekst.splitlines():
            schoon = re.sub(r"[*`#>|]", "", regel).strip()
            if len(schoon) < 25:
                continue
            for pack in packs:
                los = pack.replace("_", " ")
                if pack.lower() in schoon.lower() or (len(los) > 6 and los.lower() in schoon.lower()):
                    uit.setdefault(pack, [])
                    if len(uit[pack]) < 3 and schoon not in uit[pack]:
                        uit[pack].append(f"{schoon[:300]}  — *{rel}*")
    return uit


def main() -> int:
    if not CONTENT.is_dir():
        print("Eclipse/Content niet gevonden")
        return 1

    verdict = oordelen()
    packs = []
    for d in sorted(CONTENT.iterdir()):
        if not d.is_dir() or d.name in EIGEN:
            continue
        n = len(list(d.rglob("*.uasset")))
        if n == 0:
            continue
        refs = genoemd_in_code(d.name)
        packs.append({
            "naam": d.name,
            "assets": n,
            "mb": mb(d),
            "refs": refs,
            "gebruikt": bool(refs),
            "oordeel": verdict.get(d.name, []),
        })

    eigen = []
    for naam in ("Art", "CharacterAssemblies"):
        d = CONTENT / naam
        if d.is_dir():
            eigen.append({"naam": naam, "assets": len(list(d.rglob("*.uasset"))), "mb": mb(d)})

    afgewezen = []
    rej = REPO / "Eclipse" / "Saved" / "RejectedAssets"
    if rej.is_dir():
        for d in rej.iterdir():
            if d.is_dir():
                afgewezen.append({"naam": d.name, "assets": len(list(d.rglob("*.uasset")))})

    data = {
        "gemeten": datetime.now().strftime("%Y-%m-%d %H:%M"),
        "packs": sorted(packs, key=lambda p: -p["mb"]),
        "eigen": eigen,
        "afgewezen": afgewezen,
        "totaal_mb": sum(p["mb"] for p in packs),
        "totaal_assets": sum(p["assets"] for p in packs),
        "ongebruikt": [p["naam"] for p in packs if not p["gebruikt"]],
    }
    UIT_JSON.write_text(json.dumps(data, ensure_ascii=False, indent=1), encoding="utf-8")

    # ---- leesbaar document ----
    r = [
        "# ASSET-INVENTARIS — wat er op schijf staat",
        "",
        f"*Gegenereerd door `Tools/dump_asset_inventaris.py` op {data['gemeten']}. "
        "**Afgeleid — draai het script opnieuw in plaats van dit met de hand bij te werken.***",
        "",
        f"**{len(packs)} packs · {data['totaal_assets']} assets · {data['totaal_mb']} MB.**",
        "",
        "> **Lees dit voor je iets met de hand bouwt.** Een gang, een muzzle flash, een "
        "voetstapgeluid, een auto, een planeet aan de hemel — de kans is groot dat het er al "
        "ligt. Alles hieronder is al betaald en al binnengehaald.",
        "",
        "**\"Genoemd in code\" is een ondergrens, geen waarheid.** Het meet of `/Game/<pack>` "
        "voorkomt in Source, Tools of Config. Een pack dat alleen vanuit een level of een "
        "DataTable gebruikt wordt, staat hier ten onrechte als ongebruikt. Gebruik het als "
        "aanwijzing, niet als bewijs.",
        "",
        "## Packs op schijf",
        "",
        "| Pack | Assets | MB | Genoemd in code |",
        "|---|---:|---:|---|",
    ]
    for p in packs:
        merk = f"ja ({len(p['refs'])})" if p["gebruikt"] else "**nee**"
        r.append(f"| `{p['naam']}` | {p['assets']} | {p['mb']} | {merk} |")

    ong = [p for p in packs if not p["gebruikt"]]
    if ong:
        r += ["", "## Nergens in code genoemd", "",
              "Deze liggen er wel maar worden nergens aangeroepen. Dat is geen verwijt — "
              "het is een lijst met kansen.", ""]
        for p in ong:
            r.append(f"- **`{p['naam']}`** — {p['assets']} assets, {p['mb']} MB")

    r += ["", "## Wat agents er eerder over schreven", ""]
    met = [p for p in packs if p["oordeel"]]
    if met:
        for p in met:
            r.append(f"### `{p['naam']}`")
            for o in p["oordeel"]:
                r.append(f"- {o}")
            r.append("")
    else:
        r.append("*Nog geen oordelen gevonden in de curatiedocumenten.*")

    if afgewezen:
        r += ["## Afgewezen — bewust niet gebruikt", "",
              "Staat in `Eclipse/Saved/RejectedAssets`. Niets is verwijderd; terugzetten is "
              "één verplaatsing. Reden per pack: `WAAROM_AFGEWEZEN.md`.", ""]
        for a in afgewezen:
            r.append(f"- **{a['naam']}** — {a['assets']} assets")
        r.append("")

    r += ["## Eigen werk (geen pack)", ""]
    for e in eigen:
        r.append(f"- **`{e['naam']}`** — {e['assets']} assets, {e['mb']} MB")

    r += ["", "---", "",
          "**De stijlwet geldt onverkort** (`15_visual_quality_charter.md` §15.5): niets "
          "hieruit rendert rauw. Elke plaatsing gaat door de toon-master. En "
          "`20_world_dressing_standard.md` §20.2 komt daar bovenop: een pack-asset dat "
          "aardse semiotiek draagt — verkeersborden, moderne meubels, Latijnse tekst — "
          "wordt vervangen, niet geplaatst.", ""]

    UIT_MD.write_text("\n".join(r), encoding="utf-8")
    print(f"Geschreven: {UIT_MD.relative_to(REPO)} en {UIT_JSON.relative_to(REPO)}")
    print(f"  {len(packs)} packs · {data['totaal_assets']} assets · {data['totaal_mb']} MB")
    print(f"  nergens genoemd: {len(ong)} — {', '.join(data['ongebruikt']) or 'geen'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
