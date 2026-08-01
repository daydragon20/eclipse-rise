#!/usr/bin/env python3
"""
dump_casting_definitief.py — de castingkeuzes als leesbare lijst.

Waarom dit bestaat
------------------
De castingtab is op 01-08 uit het dashboard gehaald: alle keuzes lagen vast en
een tab die niets meer doet is ruis. Owner: "haal alle keuzes uit de casting
tab en zorg dat VS Claude het weet, dan mag je het weghalen."

De keuzes zelf zaten alleen in JSON. Dit script rendert daaruit één leesbaar
document, zodat een agent die `STATUS.md` leest ziet WAT er gekozen is zonder
drie JSON-bestanden te moeten samenvoegen.

**Dit document is afgeleid, nooit de bron.** De bron blijft:
    phase0/O16_KEUZE.json        de tien slots die Nathan koos (wint altijd)
    phase0/CASTING_KEUZE.json    de posities uit de shortlist
    phase0/CASTING_RESOLVED.json de opgeloste binding (output van resolve_casting_choice.py)

Draaien:  python Tools/dump_casting_definitief.py
"""

from __future__ import annotations

import json
import pathlib
from datetime import datetime

REPO = pathlib.Path(__file__).resolve().parent.parent
UIT = REPO / "phase0" / "CASTING_DEFINITIEF.md"


def laad(naam: str) -> dict:
    p = REPO / "phase0" / naam
    if not p.is_file():
        return {}
    try:
        return json.loads(p.read_text(encoding="utf-8", errors="replace"))
    except Exception:
        return {}


def main() -> int:
    res = laad("CASTING_RESOLVED.json")
    o16 = laad("O16_KEUZE.json")
    rollen = res.get("rollen", {})
    if not rollen:
        print("CASTING_RESOLVED.json bevat geen rollen — draai eerst resolve_casting_choice.py")
        return 1

    # O-16-keuzes op rol groeperen: sleutel is "rol:slot"
    per_rol: dict[str, list] = {}
    for sleutel, waarde in o16.items():
        rol = sleutel.split(":")[0]
        slot = sleutel.split(":")[1] if ":" in sleutel else "-"
        naam = waarde.get("stem") if isinstance(waarde, dict) else str(waarde)
        vid = waarde.get("voice_id") if isinstance(waarde, dict) else ""
        per_rol.setdefault(rol, []).append((slot, naam, vid))

    r = ["# CASTING — DEFINITIEF",
         "",
         f"*Gegenereerd door `Tools/dump_casting_definitief.py` op "
         f"{datetime.now().strftime('%Y-%m-%d %H:%M')}. **Afgeleid, niet de bron.***",
         "",
         "De castingtab is uit het dashboard verwijderd omdat alle keuzes vastliggen "
         "en de poort ze bewaakt. Dit is wat er gekozen is.",
         "",
         "**De bron blijft JSON — bewerk dít bestand nooit met de hand:**",
         "",
         "| Bestand | Wat het is |",
         "|---|---|",
         "| `phase0/O16_KEUZE.json` | De tien slots die Nathan zelf koos. **Wint van alles.** |",
         "| `phase0/CASTING_KEUZE.json` | Posities uit de shortlist (per rol een top-2) |",
         "| `phase0/CASTING_RESOLVED.json` | De opgeloste binding — output van `resolve_casting_choice.py` |",
         "",
         "**Vóór elke generatiebatch:**",
         "",
         "```",
         "python Eclipse/Tools/resolve_casting_choice.py",
         "python Eclipse/Tools/check_voice_resolves.py     # exit 0 = de enige toestemming",
         "```",
         "",
         "Faalt de poort: genereer niets, meld welke rol hangt, stop. "
         "**Val nooit terug op een fallback-stem** — dat maakt de bar groen terwijl "
         "die regels stilte worden.",
         "",
         "---",
         "",
         f"## Nathans eigen keuzes — {len(o16)} slots (O-16)",
         "",
         "Deze winnen van de afgeleide binding.",
         "",
         "| Rol | Slot | Stem | voice_id |",
         "|---|---|---|---|"]

    for rol in sorted(per_rol):
        for slot, naam, vid in sorted(per_rol[rol]):
            r.append(f"| `{rol}` | {slot} | {naam} | `{vid}` |")

    r += ["",
          "---",
          "",
          f"## Alle rollen — {len(rollen)} met {res.get('finalisten_totaal', '?')} finalisten",
          "",
          "| Rol | Tier | Finalisten |",
          "|---|---|---|"]

    for rol, info in sorted(rollen.items(), key=lambda kv: (kv[1].get("prio", 9), kv[0])):
        fin = info.get("finalisten", [])
        namen = ", ".join(f.get("stem", "?") for f in fin) or "—"
        r.append(f"| `{rol}` | {info.get('tier','')} | {namen} |")

    zonder = res.get("library_finalisten_zonder_licentiecheck", [])
    if zonder:
        r += ["",
              "---",
              "",
              f"## ⚠ Licentiecontrole open — {len(zonder)} stemmen",
              "",
              "`19_voice_production.md` §19.1 eist dat de commerciële licentie van een "
              "Voice-Library-stem **met de hand op de stemkaart** gelezen wordt; dat veld "
              "bestaat niet in de API. Dit is owner-werk en staat als kaart **O-20** op het "
              "dashboard. **Genereer niets voor die check rond is** — een stem zonder "
              "licentie moet je later vervangen, en dan herbetaal je élke regel van dat "
              "personage.",
              ""]
        for z in zonder:
            naam = z.get("stem") if isinstance(z, dict) else str(z)
            vid = z.get("voice_id", "") if isinstance(z, dict) else ""
            r.append(f"- **{naam}** `{vid}`")

    conflicten = res.get("conflicten_eerste_keuze") or {}
    r += ["",
          "---",
          "",
          "## Conflicten",
          "",
          f"- Eerste keuze: **{len(conflicten)}** — "
          + ("geen; elke rol heeft een eigen stem." if not conflicten else "zie JSON."),
          f"- Inclusief reserves: **{len(res.get('conflicten_inclusief_reserves') or {})}** — "
          "reserves mogen elkaar overlappen zolang de *gekozen* stem uniek is; "
          "`check_voice_resolves.py` toetst alleen die laatste.",
          "",
          "**Waarom uniekheid telt:** de stem-ID zit in de cachesleutel. Twee personages "
          "op dezelfde stem is niet 'lijkt op elkaar' maar letterlijk dezelfde stem — en "
          "later wisselen herbetaalt elke regel van dat personage.",
          ""]

    UIT.write_text("\n".join(r), encoding="utf-8")
    print(f"Geschreven: {UIT.relative_to(REPO)}")
    print(f"  {len(o16)} O-16-slots · {len(rollen)} rollen · "
          f"{len(zonder)} wachten op licentiecontrole")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
