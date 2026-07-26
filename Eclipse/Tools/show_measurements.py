"""Toon alle gemeten feel-waarden uit de laatste testronde, gegroepeerd per test.

Waarom dit bestaat: de harnas-tests rapporteren hun metingen met Report(), en die
komen als "GEMETEN <label> <waarde> <eenheid> (verwacht: ...)" in Saved/Logs
terecht. Het zijn er veel — wie een getal wil opzoeken ("hoe hoog springt hij ook
alweer?") moet anders door een logbestand van duizenden regels scrollen. Hoeveel
het er precies zijn telt dit script zelf; hier een getal neerzetten zou binnen een
dag verlopen.

Dit script LEEST alleen; het draait niets en verandert niets. Draai de suite eerst
als je verse cijfers wilt:

    UnrealEditor-Cmd.exe <project> -ExecCmds="Automation RunTests Eclipse; Quit" \
        -unattended -nopause -nullrhi -NoSound -log

en daarna:

    python Eclipse/Tools/show_measurements.py            (alles)
    python Eclipse/Tools/show_measurements.py sprint     (filter op woord)

Bewust GEEN eigen kopie van de getallen: de bron is het log, dus dit overzicht kan
niet verouderen ten opzichte van de tests. Dat is dezelfde regel die de nacht van
25→26 juli zes keer heeft gekost toen documenten wél hun eigen kopie bijhielden.
"""

import re
import sys
from pathlib import Path

LOGDIR = Path(__file__).resolve().parents[1] / "Saved" / "Logs"

# "LogAutomationController: GEMETEN  <label>  <waarde> <eenheid>   (verwacht: ...)"
MEASURE = re.compile(r"GEMETEN\s{2}(.+?)\s{2,}(-?[\d.]+)\s*(\S*)\s*(\(verwacht:.*)?$")
TEST_DONE = re.compile(r"Test Completed\. Result=\{(\w+)\} Name=\{([^}]+)\}")


def find_log() -> "Path | None":
    """Het nieuwste log dat echt metingen bevat — niet per se Eclipse.log.

    Waarom niet gewoon Eclipse.log: elke engine-start hernoemt het vorige log naar
    Eclipse-backup-<tijd>.log. Draai je de groene bar, dan komt de validatie- of
    catalogusronde ná de tests, en die overschrijft Eclipse.log met een run die
    nul metingen bevat. Precies wie het netjes doet — eerst alles groen, dan de
    cijfers opvragen — kreeg zo "geen metingen gevonden" te zien.
    """
    logs = sorted(LOGDIR.glob("Eclipse*.log"), key=lambda p: p.stat().st_mtime, reverse=True)
    for path in logs:
        if "GEMETEN" in path.read_text(encoding="utf-8", errors="replace"):
            return path
    return None


def main() -> int:
    log = find_log()
    if log is None:
        print(f"Geen meting gevonden in {LOGDIR} — draai eerst de suite met -log.")
        return 1

    needle = sys.argv[1].lower() if len(sys.argv) > 1 else None

    # De metingen staan NA de "Test Completed"-regel van hun eigen test: de
    # automation controller schrijft eerst de uitslag en daarna de verzamelde
    # logregels van die test. Nagemeten in het log, niet aangenomen — de eerste
    # versie van dit script ging van de omgekeerde volgorde uit en hing elke
    # meting aan de VOLGENDE test.
    current, groups, order = None, {}, []
    for raw in log.read_text(encoding="utf-8", errors="replace").splitlines():
        done = TEST_DONE.search(raw)
        if done:
            current = done.group(2)
            if current not in groups:
                groups[current] = []
                order.append(current)
            continue
        m = MEASURE.search(raw)
        if m and current is not None:
            label, value, unit, expect = m.groups()
            groups[current].append((label.strip(), value, unit or "", (expect or "").strip()))

    print(f"Bron: {log.name}")
    shown = 0
    for name in order:
        rows = groups[name]
        if needle:
            rows = [r for r in rows if needle in r[0].lower() or needle in name.lower()]
        if not rows:
            continue
        print(f"\n=== {name} ===")
        for label, value, unit, expect in rows:
            print(f"  {label:<44} {value:>12} {unit:<6} {expect}")
            shown += 1

    print(f"\n{shown} metingen" + (f" met '{needle}'" if needle else "") + ".")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
