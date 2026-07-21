#!/usr/bin/env python3
"""CI check (GDD 14.2): every native Event.* gameplay tag declared in Source/
must have a row in Docs/EventCatalog.md, and vice versa once implemented.

Pre-Phase 1 there are no native tags yet, so the source side is empty by
design; the check still runs so the contract is enforced from the first tag.
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
CATALOG = ROOT / "Docs" / "EventCatalog.md"
TAG_DECL = re.compile(r'UE_DEFINE_GAMEPLAY_TAG\w*\(\s*\w+\s*,\s*"(Event\.[\w.]+)"')
CATALOG_ROW = re.compile(r"`(Event\.[\w.]+)`")


def main() -> int:
    catalog_tags = set(CATALOG_ROW.findall(CATALOG.read_text(encoding="utf-8")))
    source_tags = set()
    for cpp in (ROOT / "Source").rglob("*.cpp"):
        source_tags |= set(TAG_DECL.findall(cpp.read_text(encoding="utf-8", errors="ignore")))

    missing = sorted(source_tags - catalog_tags)
    if missing:
        print("EventCatalog.md is missing documented rows for native tags:")
        for tag in missing:
            print(f"  {tag}")
        return 1

    print(f"Event catalog OK: {len(catalog_tags)} documented, {len(source_tags)} implemented.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
