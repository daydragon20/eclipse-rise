"""Vind assets die niemand aanroept — de asset-tegenhanger van find_dead_fields.py.

Waarom dit bestaat: op 26-07-2026 bleken drie geluiden ongebruikt in de repo te
liggen (het wapenschot, de kogelinslag, twee voetstappen). Ze waren gegenereerd,
geïmporteerd, gecommit — en er was geen enkele regel code die ze afspeelde. Ik vond
ze bij toeval, tijdens het schrijven van de gevechts-audit. Toeval is geen methode.

**Wat "dood" hier betekent.** Een asset waarvan de NAAM in geen enkel `.cpp`, `.h`
of `.py` in het project voorkomt. Dat is precies de goede test voor de categorieën
die de code bij PAD moet laden — geluidscues, data-assets, kaarten — want die
kunnen niet per ongeluk gevonden worden.

**Wat het NIET betekent, en dat is belangrijk.** Assets waarnaar vanuit DATA wordt
verwezen (animaties in DT_BodyDefs, materialen op een mesh, texturen op een
materiaal) zijn hier niet aan te tonen: hun verwijzing staat in een `.uasset`, niet
in tekst. Die categorieën staan daarom in SKIP_DIRS. Een animatie die niemand
gebruikt vind je met `find_dead_fields.py` plus de logregels van
setup_character_data.py, niet hiermee.

Draaien:

    python Eclipse/Tools/find_dead_assets.py

Uitvoer is de bron; schrijf het getal niet over in een document (zie de docstring
van find_dead_fields.py voor waarom dat zes keer is misgegaan).
"""

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CONTENT = ROOT / "Content"
SOURCE = ROOT / "Source"
TOOLS = ROOT / "Tools"

# Alleen mappen waar de code zijn assets bij PAD laadt. Character-, materiaal- en
# texturemappen horen hier niet: die worden vanuit data of vanuit andere assets
# gerefereerd, en dan zegt "de naam staat in geen enkele .cpp" niets.
LOOK_IN = ("Audio", "Data", "Maps", "Abilities")

# Binnen die mappen nog steeds overslaan: gegenereerde audio is per definitie
# hash-genoemd en wordt via het manifest gevonden, niet bij naam.
SKIP_DIRS = ("Generated",)


def main() -> int:
    if not CONTENT.is_dir():
        print(f"Geen Content-map op {CONTENT}.")
        return 1

    haystack = []
    for folder in (SOURCE, TOOLS):
        for pattern in ("*.cpp", "*.h", "*.py", "*.ini", "*.json"):
            for path in folder.rglob(pattern):
                haystack.append(path.read_text(encoding="utf-8", errors="replace"))
    # De config leest ook paden (bv. GameplayCueNotifyPaths).
    for path in (ROOT / "Config").rglob("*.ini"):
        haystack.append(path.read_text(encoding="utf-8", errors="replace"))
    blob = "\n".join(haystack)

    dead = []
    checked = 0
    for asset in sorted(CONTENT.rglob("*.uasset")):
        relative = asset.relative_to(CONTENT)
        if not relative.parts or relative.parts[0] not in LOOK_IN:
            continue
        if any(skip in relative.parts for skip in SKIP_DIRS):
            continue
        checked += 1
        if not re.search(rf"\b{re.escape(asset.stem)}\b", blob):
            dead.append(relative.as_posix())

    print(f"{checked} assets gecontroleerd in {', '.join(LOOK_IN)}.")
    if not dead:
        print("Geen dode assets — alles wat hier ligt wordt ergens bij naam genoemd.")
        return 0

    print(f"\n{len(dead)} die door geen enkele .cpp/.h/.py/.ini bij naam genoemd worden:\n")
    for item in dead:
        print(f"  {item}")
    print("\nElk is óf aan te sluiten óf weg te halen. Betaalde audio weggooien is")
    print("een owner-beslissing; aansluiten meestal niet (zie de memory-regel:")
    print("dood is van mij, onbeslist is van de owner).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
