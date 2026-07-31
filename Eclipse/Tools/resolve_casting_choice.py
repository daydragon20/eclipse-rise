#!/usr/bin/env python3
"""Resolve the owner's casting picks (phase0/CASTING_KEUZE.json) into real
ElevenLabs voice ids, verifiably.

The picks are *positions on the casting page* ("role X, candidates 5 and 2").
Positions are only meaningful against the exact candidate ordering that page was
built from, so this script does two things:

  1. rebuilds the ordering the same way CASTING.html did -- premade candidates
     from casting_stage1.json in order, then any Voice Library candidates -- and
  2. writes an `ordering_fingerprint` per role: a hash of the ordered voice ids.

If the shortlist is ever regenerated in a different order, the fingerprint moves
and a stale pick is caught instead of being silently misread as a different
voice. That matters because casting is permanent: a misread index would cast the
wrong actor for the whole game and nobody would notice until they heard it.

Writes phase0/CASTING_RESOLVED.json. Costs nothing, calls nothing.

Run:  python Eclipse/Tools/resolve_casting_choice.py
Exit: 0 = all picks resolved, 1 = a pick could not be resolved
"""
import hashlib
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent.parent
CASTING = ROOT / "progress_media" / "casting"
CHOICE = ROOT / "phase0" / "CASTING_KEUZE.json"
OUT = ROOT / "phase0" / "CASTING_RESOLVED.json"


def ordered_candidates():
    """Rebuild the page ordering: premade first, then Voice Library."""
    stage1 = json.loads((CASTING / "casting_stage1.json").read_text("utf-8"))
    libp = CASTING / "casting_stage1_library.json"
    lib = json.loads(libp.read_text("utf-8")) if libp.is_file() else {}

    out = {}
    for role in stage1["rollen"]:
        rid = role["rol"]
        cands = [{
            "stem": c["stem"], "voice_id": c["voice_id"],
            "bron": c.get("categorie", "premade"),
            # A candidate without an audio file cannot have been listened to.
            "bestand": c.get("bestand"),
            "eigenschappen": c.get("eigenschappen", ""),
        } for c in role["kandidaten"]]
        for c in lib.get(rid, {}).get("kandidaten", []):
            cands.append({
                "stem": c["stem"], "voice_id": c["voice_id"],
                "bron": "voice_library",
                "bestand": f"progress_media/casting/{c['bestand']}",
                "eigenschappen": c["eigenschappen"],
                "licentie": c["licentie"],
            })
        out[rid] = {"label": role["label"], "prio": role["prio"],
                    "tier": role["tier"], "kandidaten": cands}
    return out


def fingerprint(cands):
    joined = "|".join(c["voice_id"] for c in cands)
    return hashlib.sha256(joined.encode()).hexdigest()[:16]


def main() -> int:
    if not CHOICE.is_file():
        print(f"No picks yet: {CHOICE} does not exist.")
        return 0
    picks = json.loads(CHOICE.read_text("utf-8"))
    catalog = ordered_candidates()

    # DRIFT GUARD. A pick is an index into a shortlist. If the shortlist is
    # rebuilt in a different order -- or replaced outright -- the same index
    # silently means a different actor, and casting is permanent. So compare
    # against the fingerprint recorded when the pick was first resolved and
    # refuse rather than re-resolve. This is not hypothetical: on 31-07 ten
    # roles were swapped to Voice Library voices while the owner was still
    # looking at the page built from the previous list.
    previous = {}
    if OUT.is_file():
        previous = json.loads(OUT.read_text("utf-8")).get("rollen", {})
    # What matters is not that the list changed -- appending new candidates is
    # harmless and must not raise an alarm, or the alarm stops being read. What
    # matters is whether a pick now points at a DIFFERENT VOICE than when it was
    # recorded. So compare per pick, by voice id.
    drifted = []
    for rid in picks:
        if rid not in catalog or rid not in previous:
            continue
        cur = catalog[rid]["kandidaten"]
        moved = []
        for f in previous[rid].get("finalisten", []):
            i = f["keuze_index"]
            now_id = cur[i - 1]["voice_id"] if i <= len(cur) else None
            now_name = cur[i - 1]["stem"] if i <= len(cur) else "(out of range)"
            if now_id != f["voice_id"]:
                moved.append((i, f["stem"], now_name))
        if moved:
            drifted.append((rid, previous[rid], moved))
    if drifted:
        print("REFUSING TO RE-RESOLVE - a pick now points at a different voice.\n")
        for rid, prev, moved in drifted:
            print(f"  {prev['label']}")
            for i, was_name, now_name in moved:
                print(f"    pick #{i}: was '{was_name}' -> would now become '{now_name}'")
        print("\nThe recorded casting in CASTING_RESOLVED.json is left untouched.")
        print("Decide deliberately: either restore the shortlist the owner actually")
        print("listened to, or have him re-pick against the new one. Do not let an")
        print("index quietly change who plays the part.")
        return 1

    resolved, problems = {}, []
    for rid, idxs in picks.items():
        role = catalog.get(rid)
        if role is None:
            problems.append(f"'{rid}' is not a role on the casting page")
            continue
        cands = role["kandidaten"]
        finalists = []
        for rank, idx in enumerate(idxs, 1):
            if not isinstance(idx, int) or not (1 <= idx <= len(cands)):
                problems.append(
                    f"{rid}: pick {idx} is out of range (1..{len(cands)})")
                continue
            c = dict(cands[idx - 1])
            # You cannot pick a voice you could not hear. If a candidate has no
            # audio file, the shortlist was rebuilt without rebuilding the page,
            # so this index refers to something the owner never listened to.
            f = c.get("bestand")
            if not f or not (ROOT / f).is_file():
                problems.append(
                    f"{rid}: pick #{idx} ('{c['stem']}') has no preview audio "
                    f"({f or 'no file recorded'}). The shortlist was changed "
                    f"without rebuilding CASTING.html - this pick cannot be trusted.")
                continue
            c["keuze_index"] = idx
            c["rang"] = rank            # 1 = eerste keus, 2 = reserve
            finalists.append(c)
        resolved[rid] = {
            "label": role["label"], "prio": role["prio"], "tier": role["tier"],
            "aantal_kandidaten": len(cands),
            "ordering_fingerprint": fingerprint(cands),
            "finalisten": finalists,
        }

    lic = [(rid, f["stem"]) for rid, r in resolved.items()
           for f in r["finalisten"] if f["bron"] == "voice_library"]

    # Casting conflicts. A voice can only be cast once: two characters sharing a
    # voice sound like the same person, which is worst exactly where it matters
    # (a named companion vs. a generic fighter). First choices are what bite;
    # a clash between reserves only bites if a first choice fails stage 2.
    def clashes(rank_filter):
        by_voice = {}
        for rid, r in resolved.items():
            for f in r["finalisten"]:
                if rank_filter(f["rang"]):
                    by_voice.setdefault(f["voice_id"], []).append(
                        (r["label"], f["stem"], f["rang"]))
        return {v: rs for v, rs in by_voice.items() if len(rs) > 1}

    first_clash = clashes(lambda r: r == 1)
    any_clash = clashes(lambda r: True)

    OUT.write_text(json.dumps({
        "bron": "phase0/CASTING_KEUZE.json",
        "toelichting": "Index = positie op progress_media/casting/CASTING.html "
                       "(premade eerst, daarna Voice Library). Controleer "
                       "ordering_fingerprint als de shortlist opnieuw is gebouwd.",
        "rollen_gekozen": len(resolved),
        "finalisten_totaal": sum(len(r["finalisten"]) for r in resolved.values()),
        "library_finalisten_zonder_licentiecheck": lic,
        "conflicten_eerste_keuze": {v: r for v, r in first_clash.items()},
        "conflicten_inclusief_reserves": {v: r for v, r in any_clash.items()},
        "rollen": resolved,
    }, indent=1, ensure_ascii=False) + "\n", encoding="utf-8")

    for rid, r in sorted(resolved.items(), key=lambda x: x[1]["prio"]):
        print(f"{r['label']}  [prio {r['prio']}]  fp={r['ordering_fingerprint']}")
        for f in r["finalisten"]:
            tag = "LIBRARY - licentie onbekend" if f["bron"] == "voice_library" else "premade"
            print(f"   {f['rang']}. #{f['keuze_index']:<2} {f['stem']:<28} "
                  f"{f['voice_id']}  ({tag})")

    print(f"\n{len(resolved)} rollen gekozen, "
          f"{sum(len(r['finalisten']) for r in resolved.values())} finalisten.")
    if lic:
        print(f"LICENTIECONTROLE NODIG voor {len(lic)}: {lic}")
    else:
        print("Alle finalisten zijn premade-stemmen van ElevenLabs zelf — "
              "geen externe licentievoorwaarden, dus geen kaartcontrole nodig.")
    if first_clash:
        print("\nBOTSING OP EERSTE KEUS — twee personages zouden identiek klinken:")
        for v, rs in first_clash.items():
            print(f"  {rs[0][1]} ({v}) is eerste keus voor: "
                  + ", ".join(r[0] for r in rs))
    else:
        print("\nEerste keuzes: geen botsingen, elke rol een eigen stem.")
    reserve_only = {v: rs for v, rs in any_clash.items() if v not in first_clash}
    if reserve_only:
        print("Overlap in de RESERVES (bijt pas als een eerste keus in stage 2 afvalt):")
        for v, rs in reserve_only.items():
            print(f"  {rs[0][1]}: " + ", ".join(f"{r[0]} (keus {r[2]})" for r in rs))

    for p in problems:
        print("PROBLEEM:", p)
    print(f"\ngeschreven: {OUT.relative_to(ROOT)}")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
