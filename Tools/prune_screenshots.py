#!/usr/bin/env python3
"""
prune_screenshots.py — houdt de screenshot-map klein.

De agent maakt screenshots om de game te controleren. Die stapelen op:
zonder opruimen groeit Eclipse/Saved/Screenshots ongelimiteerd (stond op
1765 bestanden / 1,9 GB voor de eerste run).

Regel: houd de NIEUWSTE N (default 50) in de werkmap, gooi de rest weg.

BESCHERMD — deze mappen worden NOOIT aangeraakt:
  Eclipse/Saved/ShotBaseline      referentiebeelden voor de art-review (15.8)
  Eclipse/Saved/CurationStaging   asset-curatiebewijs
  Eclipse/Saved/GeneratedDecals   gegenereerde bronassets, geen screenshots
  progress_media                  wat het dashboard toont
  .../prev_round                  vorige-ronde-vergelijking van de art-reviewer

Gebruik:
    python Tools/prune_screenshots.py            # dry-run, toont wat er zou gebeuren
    python Tools/prune_screenshots.py --apply    # voert het uit
    python Tools/prune_screenshots.py --keep 80 --apply
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent

# Map die opgeruimd wordt (alleen bestanden direct hierin, niet in submappen).
TARGET = REPO / "Eclipse" / "Saved" / "Screenshots" / "WindowsEditor"

# Submappen en paden die met rust gelaten worden.
PROTECTED_DIRS = {"prev_round"}

EXTENSIONS = {".png", ".jpg", ".jpeg", ".bmp", ".exr"}


def human(n: int) -> str:
    for unit in ("B", "KB", "MB", "GB"):
        if n < 1024 or unit == "GB":
            return f"{n:.1f} {unit}" if unit != "B" else f"{n} B"
        n /= 1024.0
    return f"{n:.1f} GB"


def collect(target: Path) -> list[Path]:
    """Alle screenshotbestanden direct in target, nieuwste eerst."""
    if not target.is_dir():
        return []
    files = [
        p
        for p in target.iterdir()
        if p.is_file() and p.suffix.lower() in EXTENSIONS
    ]
    files.sort(key=lambda p: p.stat().st_mtime, reverse=True)
    return files


def main() -> int:
    ap = argparse.ArgumentParser(description="Ruimt oude screenshots op.")
    ap.add_argument("--keep", type=int, default=50, help="hoeveel nieuwste bewaren (default 50)")
    ap.add_argument("--apply", action="store_true", help="echt verwijderen (zonder dit: dry-run)")
    ap.add_argument("--target", type=Path, default=TARGET, help="af te ruimen map")
    args = ap.parse_args()

    if args.keep < 1:
        print("--keep moet minstens 1 zijn.", file=sys.stderr)
        return 2

    target: Path = args.target
    for part in target.parts:
        if part in PROTECTED_DIRS:
            print(f"WEIGERING: {target} ligt in een beschermde map.", file=sys.stderr)
            return 2

    files = collect(target)
    if not files:
        print(f"Geen screenshots gevonden in {target}")
        return 0

    keep = files[: args.keep]
    drop = files[args.keep :]

    kept_bytes = sum(p.stat().st_size for p in keep)
    drop_bytes = sum(p.stat().st_size for p in drop)

    print(f"Map        : {target}")
    print(f"Gevonden   : {len(files)} bestanden ({human(kept_bytes + drop_bytes)})")
    print(f"Behouden   : {len(keep)} nieuwste ({human(kept_bytes)})")
    print(f"Verwijderen: {len(drop)} oudere ({human(drop_bytes)})")

    if not drop:
        print("Niets te doen.")
        return 0

    if keep:
        print(f"Nieuwste behouden: {keep[0].name}")
        print(f"Oudste behouden  : {keep[-1].name}")

    if not args.apply:
        print("\nDRY-RUN — er is niets verwijderd. Draai met --apply om het uit te voeren.")
        return 0

    removed = 0
    freed = 0
    for p in drop:
        try:
            size = p.stat().st_size
            p.unlink()
            removed += 1
            freed += size
        except OSError as exc:
            print(f"  kon {p.name} niet verwijderen: {exc}", file=sys.stderr)

    print(f"\nVerwijderd: {removed} bestanden, {human(freed)} vrijgemaakt.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
