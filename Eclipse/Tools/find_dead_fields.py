"""Vind UPROPERTY-velden die de owner kan verdraaien zonder dat er iets gebeurt.

Waarom dit bestaat: in de nacht van 25→26 juli bleek de omgekeerde vraag de rijkste
van de hele sessie. Niet "welke code mist een aanroep" — dat vind je met een test —
maar **"welke knop kan de owner verdraaien zonder dat iets hem leest"**. Dat kost hem
echte tijd aan het afstellen, en van buiten is het onzichtbaar: je verdraait iets, je
speelt, er verandert niets, en je concludeert dat de waarde niet uitmaakt.

Die sweep leverde 23 velden op, waarvan er drie zichtbaar zijn tijdens spelen
(EnemySpawns, ShootAnim, InsertionPointIds) en één die het missieontwerp raakt.

Draaien:

    python Eclipse/Tools/find_dead_fields.py           (alle dode velden)
    python Eclipse/Tools/find_dead_fields.py --tel      (alleen het aantal)

Wat "dood" hier betekent: de veldnaam komt in GEEN ENKEL .cpp/.h-bestand voor buiten
zijn eigen header, tests niet meegerekend. Twee bewuste keuzes daarin:

  tests niet meegerekend   een test die een veld uitleest bewijst niet dat het spel
                           er iets mee doet — dat was juist de val bij de camera
  eigen header uitgesloten anders vindt hij zichzelf altijd

Drie dingen die dit script NIET ziet, en die je met de hand moet wegen:

  1. Blueprints. Een BP kan een BlueprintReadOnly-veld lezen; .uasset is binair.
     ECLIPSE is C++-first, dus in de praktijk is dit zeldzaam — maar het is een
     aanname en geen meting.
  2. Python-setupscripts SCHRIJVEN velden (UnlockTag, ShootAnim, InsertionPointIds).
     Dat maakt een veld niet levend; het maakt het erger, want dan is het bewust
     geauthorde data die niets doet.
  3. Een veld dat wél gelezen wordt maar in dode code staat. Dit is een naamzoeker,
     geen bereikbaarheidsanalyse.

De uitvoer is de bron; zet het getal NIET in een document over. Dat is precies de
fout die deze sessie zes keer heeft gemaakt — inclusief in de alinea die uitlegde
waarom tellingen verouderen, en inclusief in de eerste versie van dit overzicht
("negentien" waar het er 23 waren).
"""

import re
import sys
from pathlib import Path

# BEIDE MODULES, en dat was een blinde vlek tot 26-07 avond. De sweep keek alleen
# in Source/Eclipse, dus elk veld dat door de VALIDATOR gelezen wordt (die in
# EclipseEditor staat) meldde zich als dood. RoleSummary stond zo op de lijst
# terwijl er letterlijk een validator op staat.
#
# Zelfde klasse als de vorige blinde vlek van deze tool: hij keek alleen naar
# scalars en miste elke TArray. Een sweep die te veel meldt is net zo onbruikbaar
# als een die te weinig meldt — je leert hem negeren.
SOURCE_ROOT = Path(__file__).resolve().parents[1] / "Source"
ROOT = SOURCE_ROOT / "Eclipse"

# Het type-alternatief is expres breed: de eerste versie van deze sweep keek alleen
# naar getallen en namen, en miste daardoor EnemySpawns — een TArray, en juist de
# belangrijkste vondst van de hele ronde. Een sweep die zijn eigen zwaarste geval
# niet vindt is geen sweep.
TYPES = (
    r"float|int32|bool|FName|uint8|FText|FString"
    r"|TArray<[^>]+>|TSoftObjectPtr<[^>]+>|TObjectPtr<[^>]+>|TMap<[^>]+>"
    r"|FGameplayTag|FVector|FEclipse\w+"
)
FIELD = re.compile(rf"^\s*(?:{TYPES})\s+(\w+)\s*(?:=|;)")


def fields_in(header: Path) -> list:
    """Veldnamen die direct onder een UPROPERTY(EditAnywhere) staan."""
    lines = header.read_text(encoding="utf-8", errors="replace").splitlines()
    out = []
    for index, line in enumerate(lines[:-1]):
        if "UPROPERTY(EditAnywhere" not in line:
            continue
        match = FIELD.match(lines[index + 1])
        if match:
            out.append(match.group(1))
    return out


def main() -> int:
    sources = [p for p in SOURCE_ROOT.rglob("*.cpp")] + [p for p in SOURCE_ROOT.rglob("*.h")]
    bodies = {p: p.read_text(encoding="utf-8", errors="replace") for p in sources}

    dead = []
    for header in sorted(ROOT.rglob("*.h")):
        for name in sorted(set(fields_in(header))):
            word = re.compile(rf"\b{re.escape(name)}\b")
            elsewhere = any(
                path != header
                and "Tests" not in path.parts
                and word.search(text)
                for path, text in bodies.items()
            )
            if not elsewhere:
                dead.append((header.name, name))

    if "--tel" in sys.argv:
        print(len(dead))
        return 0

    if not dead:
        print("Geen dode velden — alles wat de owner kan verdraaien wordt ergens gelezen.")
        return 0

    width = max(len(h) for h, _ in dead)
    current = None
    for header, name in dead:
        if header != current:
            print()
            current = header
        print(f"  {header:<{width}}  ::{name}")
    print(f"\n{len(dead)} velden die niemand leest.")
    print("Elk hoort een NIET GELEZEN-regel in zijn header te hebben, of aangesloten te worden.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
